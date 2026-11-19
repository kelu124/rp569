/********************************************************************************
 * mode.c — drive / serial USB personality and the switch between them.
 * See mode.h.
 *
 * The switch is done in place, without a chip reset: a software reset does not
 * reload the program into SRAM (CH569DS1 §3.2, only a power-on reset does) and
 * left board #1 dead (2026-11-19). Instead USB is stopped long enough for the
 * host to see a disconnect, the function state is cleared, and the USB stack is
 * started again exactly as at boot, with the other mode's descriptors.
 *******************************************************************************/
#include "mode.h"
#include "CH56x_usb20.h"
#include "CH56x_usb30.h"
#include "SW_UDISK.h"
#include "cdc_uart.h"
#include "debug_cdc.h"
#include "ownership.h"
#include "bridge_config.h"

UINT8 bridge_mode = MODE_DRIVE;

static volatile UINT8 req_pending = 0;
static volatile UINT8 req_mode = MODE_DRIVE;
static volatile UINT8 req_announce = 0;

void mode_request(UINT8 mode, UINT8 announce)
{
    req_mode = mode;
    req_announce = announce;
    req_pending = 1;
}

/* USB start, SuperSpeed first, TMR0 times out into the USB2 fallback (CH56x_usb30.c TMR0_IRQHandler). */
void usb_start(void)
{
    Link_Sta = 0;
    g_devInfo.dev_enum_status = 0;
    g_devInfo.dev_config_value = 0;
    R32_USB_CONTROL = 0;
    PFIC_EnableIRQ(USBSS_IRQn);
    PFIC_EnableIRQ(LINK_IRQn);
    PFIC_EnableIRQ(TMR0_IRQn);
    R8_TMR0_INTER_EN = RB_TMR_IE_CYC_END;
    TMR0_TimerInit(67000000);
    USB30D_init(ENABLE);
}

/* Drive mode needs the card: mount it, else stay in (or fall back to) serial mode. */
UINT8 mode_enter_drive(void)
{
    ownership_request(OWN_REQ_CLAIM);
    ownership_poll();
    return own_card_ready ? MODE_DRIVE : MODE_SERIAL;
}

/* Let queued bytes reach the RP2350 (echo ring, then the UART1 FIFO), 50 ms max. */
static void link_drain(void)
{
    UINT16 ms;
    for(ms = 0; ms < 50; ms++)
    {
        Link_Echo_Deal();
        if(!Link_Tx_Pending() && (R8_UART1_LSR & RB_LSR_TX_ALL_EMP))
            break;
        mDelaymS(1);
    }
}

void mode_poll(void)
{
    if(!req_pending)
        return;
    if(req_mode == bridge_mode)
    {
        req_pending = 0;
        return;
    }
    /* Never in the middle of an MSC command or before its CSW is queued. */
    if(Udisk_Transfer_Status || UDISK_OutPackflag || UDISK_InPackflag || ownership_pending())
        return;
    req_pending = 0;

    PRINT("MODE: switching to %s mode\n", req_mode == MODE_SERIAL ? "serial" : "drive");
    mDelaymS(50);                          /* the last CSW / reply on its way to the host */

    /* Off USB first, so no host command can start in between (the host sends more after an eject), long enough
     * for the host to see a disconnect. Stop only the stack in use: on the USB2 fallback the SuperSpeed side is
     * already off, and USB30D_init(DISABLE) again never returned (board #1, 2026-11-19). */
    PFIC_DisableIRQ(USBHS_IRQn);
    PFIC_DisableIRQ(USBSS_IRQn);
    PFIC_DisableIRQ(LINK_IRQn);
    PFIC_DisableIRQ(TMR0_IRQn);
    R8_TMR0_INTER_EN = 0;
    if(Link_Sta == LINK_STA_3)
        USB30D_init(DISABLE);
    else
        USB20_Device_Init(DISABLE);        /* pull-up off */
    R32_USB_CONTROL = 0;

    /* Fresh function state for the new personality (drops any half-received MSC command) */
    Udisk_Transfer_Status = 0;
    UDISK_OutPackflag = 0;
    UDISK_InPackflag = 0;
    CDC_Variable_Clear();
#if DEBUG_OVER_USB
    debug_cdc_clear();
#endif

    if(bridge_mode == MODE_DRIVE)
    {
        /* Leaving drive mode: card flushed and idle, clock stopped (it stays powered with the CH569). */
        ownership_request(OWN_REQ_RELEASE);
        ownership_poll();
        if(req_announce)
            Link_Ctrl_Send("SDOFF");       /* the RP2350 may now cut the power */
        link_drain();
    }
    mDelaymS(300);

    bridge_mode = (req_mode == MODE_DRIVE) ? mode_enter_drive() : MODE_SERIAL;
    PRINT("MODE: %s\n", bridge_mode == MODE_DRIVE ? "drive (SD card on USB)" : "serial (card released)");
    if(req_mode == MODE_DRIVE && req_announce)  /* the RP2350 asked for this mount (ch mount) */
        Link_Ctrl_Send(bridge_mode == MODE_DRIVE ? "SDON" : "SDERR");

    usb_start();
}
