/********************************************************************************
 * msc_fast.c — drive-mode READ10 / WRITE10 data phases at full speed on the USB2
 * high-speed link (specification D-029).
 *
 * Drive mode has nothing else on USB (mode.h), so one MSC command may own the
 * bus: the USB interrupt is off for the data phase and both ends are polled.
 *   READ10:  one CMD18 for the whole command (AUTOGAPSTOP: the card pauses at a
 *            block gap when the 80-sector ring UDisk_In_Buf is full); USB sends
 *            each sector one behind the card.
 *   WRITE10: up to 80 sectors (40 KB) received by polling, written with one
 *            CMD25; the next chunk is received into the other buffer while the
 *            card still programs (the next CMD25 waits for DAT0).
 * Ported from phantomdrive (github.com/o7-machinehum/phantomdrive, MIT):
 * src/msc_read.c read_stream_usb2, src/msc_write.c write_stream_usb2,
 * src/emmc_ops.h — on EP2 IN / EP3 OUT, with no-progress timeouts.
 *******************************************************************************/
#include "msc_fast.h"
#include "CH56x_usb20.h"
#include "CH56x_usb30.h"
#include "SW_UDISK.h"
#include "SD.h"
#include "mode.h"
#include "bridge_config.h"
#include <string.h>

#define SEC             DEF_UDISK_SECTOR_SIZE
#define RING_SECTORS    (UDISKSIZE / SEC)            /* 80 */
#define STALL_US        2000000UL                    /* no progress for 2 s: host gone, abort */

static UINT8 *ring_slot(UINT16 slot) { return UDisk_In_Buf + (UINT32)slot * SEC; }

static UINT16 next_slot(UINT16 slot) { return (slot + 1 == RING_SECTORS) ? 0 : slot + 1; }

/* SysTick-free microsecond progress counter: one mDelayuS(1) per idle poll. */
#define IDLE_TICK(idle)  do { if(++(idle) > STALL_US) goto timeout; mDelayuS(1); } while(0)

UINT8 msc_fast_active(void)
{
    return bridge_mode == MODE_DRIVE && Link_Sta == LINK_STA_1;
}

/* EP0 is NAKed and the USB interrupt masked for the data phase; restored after. */
static UINT8 ep0_tx_save, ep0_rx_save;

static void usb_quiet_begin(void)
{
    ep0_tx_save = R8_UEP0_TX_CTRL;
    ep0_rx_save = R8_UEP0_RX_CTRL;
    PFIC_DisableIRQ(USBHS_IRQn);
    R8_UEP0_TX_CTRL = UEP_T_RES_NAK;
    R8_UEP0_RX_CTRL = UEP_R_RES_NAK;
}

static void usb_quiet_end(void)
{
    R8_USB_INT_FG = RB_USB_IF_TRANSFER | RB_USB_IF_SETUOACT;
    R8_UEP0_TX_CTRL = ep0_tx_save;
    R8_UEP0_RX_CTRL = ep0_rx_save;
    PFIC_EnableIRQ(USBHS_IRQn);
}

/* A SETUP arriving meanwhile is NAKed (EP0) and its flag dropped: the host retries after the transfer. */
static void drop_setup(void)
{
    if(R8_USB_INT_FG & RB_USB_IF_SETUOACT)
        R8_USB_INT_FG = RB_USB_IF_SETUOACT;
}

/*******************************************************************************
 * READ10
 ******************************************************************************/
static void queue_in(UINT16 slot)
{
    R32_UEP2_TX_DMA = (UINT32)ring_slot(slot);
    R16_UEP2_T_LEN = SEC;
    __asm__ volatile("fence" ::: "memory");
    R8_UEP2_TX_CTRL = (R8_UEP2_TX_CTRL & ~RB_UEP_TRES_MASK) | UEP_T_RES_ACK;
}

void msc_fast_read(void)
{
    UINT32 lba = UDISK_Cur_Sec_Lba;
    UINT16 total = (UINT16)(UDISK_Transfer_DataLen / SEC);
    UINT16 emmc_done = 0, usb_done = 0, emmc_slot = 0, usb_slot = 0;
    UINT8  paused = 0, first = 1;
    UINT32 idle = 0;

    if(total == 0 || lba + total > TF_EMMCParam.EMMCSecNum)
    {
        UDISK_CMD_Deal_Status(0x05, 0x21, 0x01);      /* ILLEGAL REQUEST, LBA out of range */
        UDISK_CMD_Deal_Fail();
        return;
    }
    {   /* card ready (a previous write may still be programming) */
        UINT32 us;
        for(us = 0; us < 500000 && !(R32_EMMC_STATUS & (1<<17)); us += 10)
            mDelayuS(10);
    }

    usb_quiet_begin();
    R8_UEP2_TX_CTRL = (R8_UEP2_TX_CTRL & ~RB_UEP_TRES_MASK) | UEP_T_RES_NAK;
    R8_USB_INT_FG = RB_USB_IF_TRANSFER;

    /* CMD18 for the whole command, pausing at every block gap (AUTOGAPSTOP). Our EMMC_IRQHandler clears every
     * flag, BKGAP included: keep it off for the transfer. */
    PFIC_DisableIRQ(EMMC_IRQn);
    TF_EMMCParam.EMMCOpErr = 0;
    R16_EMMC_INT_FG = 0xffff;
    R32_EMMC_DMA_BEG1 = (UINT32)ring_slot(0);
    R32_EMMC_TRAN_MODE = RB_EMMC_AUTOGAPSTOP | RB_EMMC_GAP_STOP;
    R32_EMMC_BLOCK_CFG = (UINT32)SEC << 16 | total;
    EMMCSendCmd(lba, RB_EMMC_CKIDX | RB_EMMC_CKCRC | RESP_TYPE_48 | EMMC_CMD18);

    while(1)
    {
        UINT8 progress = 0;
        UINT8 tx_done = (R8_USB_INT_FG & RB_USB_IF_TRANSFER) != 0;

        if(tx_done)
            R8_UEP2_TX_CTRL = (R8_UEP2_TX_CTRL & ~RB_UEP_TRES_MASK) | UEP_T_RES_NAK;
        /* USB one sector behind the card, so it never sends the sector being filled */
        if(emmc_done > 1 && usb_done < emmc_done - 1 && (tx_done || first))
        {
            R8_USB_INT_FG = RB_USB_IF_TRANSFER;
            first = 0;
            queue_in(usb_slot);
            usb_done++;
            usb_slot = next_slot(usb_slot);
            progress = 1;
            if(paused)
            {
                paused = 0;
                __asm__ volatile("fence" ::: "memory");
                R32_EMMC_TRAN_MODE = RB_EMMC_AUTOGAPSTOP;      /* release the gap stop */
            }
        }
        drop_setup();

        if(R16_EMMC_INT_FG & RB_EMMC_IF_BKGAP)
        {
            R16_EMMC_INT_FG = RB_EMMC_IF_BKGAP;
            emmc_done++;
            emmc_slot = next_slot(emmc_slot);
            R32_EMMC_DMA_BEG1 = (UINT32)ring_slot(emmc_slot);
            if(emmc_done - usb_done < RING_SECTORS - 2)
            {
                __asm__ volatile("fence" ::: "memory");
                R32_EMMC_TRAN_MODE = RB_EMMC_AUTOGAPSTOP;
            }
            else
                paused = 1;
            progress = 1;
        }
        else if(R16_EMMC_INT_FG & RB_EMMC_IF_TRANDONE)
        {
            R16_EMMC_INT_FG = RB_EMMC_IF_TRANDONE | RB_EMMC_IF_CMDDONE;
            emmc_done++;
            break;
        }
        if(R16_EMMC_INT_FG & (RB_EMMC_IF_TRANERR | RB_EMMC_IF_DATTMO | RB_EMMC_IF_FIFO_OV))
            goto sd_error;
        if(progress)
            idle = 0;
        else
            IDLE_TICK(idle);
    }

    /* Drain: the sectors the card has finished and USB has not sent yet */
    idle = 0;
    while(usb_done < emmc_done)
    {
        UINT8 tx_done = (R8_USB_INT_FG & RB_USB_IF_TRANSFER) != 0;
        if(tx_done)
            R8_UEP2_TX_CTRL = (R8_UEP2_TX_CTRL & ~RB_UEP_TRES_MASK) | UEP_T_RES_NAK;
        if(tx_done || first)
        {
            R8_USB_INT_FG = RB_USB_IF_TRANSFER;
            first = 0;
            queue_in(usb_slot);
            usb_done++;
            usb_slot = next_slot(usb_slot);
            idle = 0;
        }
        else
            IDLE_TICK(idle);
        drop_setup();
    }
    idle = 0;
    while(!(R8_USB_INT_FG & RB_USB_IF_TRANSFER))
    {
        IDLE_TICK(idle);
        drop_setup();
    }
    R8_USB_INT_FG = RB_USB_IF_TRANSFER;
    R8_UEP2_TX_CTRL = (R8_UEP2_TX_CTRL & ~RB_UEP_TRES_MASK) | UEP_T_RES_NAK;

    R32_EMMC_TRAN_MODE = 0;
    EMMCSendCmd(0, RB_EMMC_CKIDX | RB_EMMC_CKCRC | RESP_TYPE_R1b | EMMC_CMD12);
    {
        UINT32 g = MSC_EMMC_GUARD;
        while(CheckCMDComp(&TF_EMMCParam) == CMD_NULL && --g)
            ;
    }
    R16_EMMC_INT_FG = 0xffff;
    TF_EMMCParam.EMMCOpErr = 0;
    PFIC_EnableIRQ(EMMC_IRQn);
    usb_quiet_end();

    UDISK_Cur_Sec_Lba += total;
    UDISK_Transfer_DataLen = 0;
    Udisk_Transfer_Status &= ~DEF_UDISK_BLUCK_UP_FLAG;
    /* The CSW goes out from the next EP2 IN via the normal path: queue it now (EP2 is idle). */
    UDISK_Up_CSW();
    return;

sd_error:
timeout:
    PRINT("MSC: fast read %lu+%u stopped at %u/%u (flags %04x)\n", (unsigned long)lba, total, usb_done, emmc_done,
          R16_EMMC_INT_FG);
    R32_EMMC_TRAN_MODE = 0;
    EMMCSendCmd(0, RB_EMMC_CKIDX | RB_EMMC_CKCRC | RESP_TYPE_R1b | EMMC_CMD12);
    mDelaymS(2);
    R16_EMMC_INT_FG = 0xffff;
    TF_EMMCParam.EMMCOpErr = 0;
    PFIC_EnableIRQ(EMMC_IRQn);
    usb_quiet_end();
    UDISK_CMD_Deal_Status(0x04, 0x11, 0x01);          /* MEDIUM ERROR, unrecovered read */
    UDISK_CMD_Deal_Fail();                            /* STALL EP2, CSW after the host clears it */
}

/*******************************************************************************
 * WRITE10
 ******************************************************************************/
/* Receive n sectors into buf by polling EP3 (the first `have` are already there). 0 = ok. */
static UINT8 receive_chunk(UINT8 *buf, UINT16 have, UINT16 n)
{
    UINT16 i;
    for(i = have; i < n; )
    {
        UINT32 idle = 0;
        UINT8  tog_ok;
        R32_UEP3_RX_DMA = (UINT32)(buf + (UINT32)i * SEC);
        __asm__ volatile("fence" ::: "memory");
        R8_UEP3_RX_CTRL = (R8_UEP3_RX_CTRL & ~RB_UEP_RRES_MASK) | UEP_R_RES_ACK;
        while(!(R8_USB_INT_FG & RB_USB_IF_TRANSFER))
        {
            drop_setup();
            if(++idle > STALL_US)
                return 1;
            mDelayuS(1);
        }
        /* NAK first, then clear the flag: the controller only auto-NAKs while the flag is set (RB_USB_INT_BUSY),
         * so clearing it first let the host's next packet land on this same slot (board #1, 2026-11-19). */
        R8_UEP3_RX_CTRL = (R8_UEP3_RX_CTRL & ~RB_UEP_RRES_MASK) | UEP_R_RES_NAK;
        tog_ok = (R8_USB_INT_ST & RB_USB_ST_TOGOK) != 0;
        R8_USB_INT_FG = RB_USB_IF_TRANSFER;
        if(tog_ok)
            i++;                                   /* else a resent packet: receive this slot again */
    }
    return 0;
}

void msc_fast_write(void)
{
    UINT32 lba = UDISK_Cur_Sec_Lba;
    UINT16 left = (UINT16)(UDISK_Transfer_DataLen / SEC);
    UINT8 *buf = UDisk_Out_Buf;
    UINT16 have = 0, chunk;

    /* The first data packet came in through the USB interrupt (UDISK_Out_EP_Deal), at UDisk_Out_Buf. */
    if(UDISK_Out_Pack_Len >= SEC)
        have = 1;

    usb_quiet_begin();
    R8_USB_INT_FG = RB_USB_IF_TRANSFER;

    while(left)
    {
        chunk = (left < RING_SECTORS) ? left : RING_SECTORS;
        if(receive_chunk(buf, have, chunk))
        {
            PRINT("MSC: fast write %lu: host stopped sending\n", (unsigned long)lba);
            goto fail;
        }
        have = 0;
        if(SDCardWriteMulSec(&TF_EMMCParam, buf, lba, chunk) != OP_SUCCESS)   /* waits for DAT0 first */
            goto fail;
        lba  += chunk;
        left -= chunk;
        buf = (buf == UDisk_Out_Buf) ? UDisk_In_Buf : UDisk_Out_Buf;      /* next chunk while the card programs */
    }
    {   /* last chunk programmed before the CSW reports success */
        UINT32 us;
        for(us = 0; us < 500000 && !(R32_EMMC_STATUS & (1<<17)); us += 10)
            mDelayuS(10);
    }

    R32_UEP3_RX_DMA = (UINT32)UDisk_Out_Buf;          /* next CBW */
    R8_UEP3_RX_CTRL = (R8_UEP3_RX_CTRL & ~RB_UEP_RRES_MASK) | UEP_R_RES_ACK;
    usb_quiet_end();

    UDISK_Cur_Sec_Lba = lba;
    UDISK_Transfer_DataLen = 0;
    Udisk_Transfer_Status &= ~DEF_UDISK_BLUCK_DOWN_FLAG;
    UDISK_Up_CSW();
    return;

fail:
    R32_UEP3_RX_DMA = (UINT32)UDisk_Out_Buf;
    usb_quiet_end();
    UDISK_CMD_Deal_Status(0x03, 0x0C, 0x00);          /* WRITE ERROR */
    Udisk_Transfer_Status |= DEF_UDISK_BLUCK_DOWN_FLAG;
    UDISK_CMD_Deal_Fail();                            /* STALL EP3, CSW queued */
}
