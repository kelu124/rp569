/********************************************************************************
 * usb_stream.c — USB2 high-speed vendor device for the stream variant (stream.h).
 * Polled (no USB interrupt): EP0 control + bulk IN 0x82. Register use follows
 * firmware/ch569/src/CH56x_usb20.c (vendor USBHS stack, rp569 changes).
 *******************************************************************************/
#include "stream.h"
#include "chip_uid.h"
#include <string.h>

typedef struct __attribute__((packed))
{
    UINT8  bRequestType;
    UINT8  bRequest;
    UINT8  wValueL;
    UINT8  wValueH;
    UINT8  wIndexL;
    UINT8  wIndexH;
    UINT16 wLength;
} setup_t;

#define EP0_SIZE      64
#define BULK_SIZE     512

__attribute__((aligned(16))) static UINT8 ep0_buf[EP0_SIZE] __attribute__((section(".DMADATA")));
#define SETUP ((setup_t *)ep0_buf)

static const UINT8 dev_desc[] =
{
    0x12, 0x01, 0x00, 0x02,      /* USB 2.00 */
    0x00, 0x00, 0x00, EP0_SIZE,  /* class per interface */
    0x86, 0x1A, 0x2C, 0xFE,      /* 1a86:fe2c */
    0x00, 0x01,                  /* bcdDevice 1.00 (variant 1) */
    0x01, 0x02, 0x03, 0x01
};

static const UINT8 cfg_desc[] =
{
    0x09, 0x02, 25, 0x00, 0x01, 0x01, 0x00, 0x80, 0x32,
    /* Interface 0: vendor, bulk IN stream */
    0x09, 0x04, 0x00, 0x00, 0x01, 0xFF, 0x00, 0x00, 0x00,
    0x07, 0x05, 0x82, 0x02, BULK_SIZE & 0xFF, BULK_SIZE >> 8, 0x00
};

static const UINT8 str_lang[] = { 0x04, 0x03, 0x09, 0x04 };
static const UINT8 str_vendor[] = { 0x08, 0x03, 'W', 0, 'C', 0, 'H', 0 };
/* "rp569 CH569 stream": the host library finds the CH569 by "rp569 CH569" and reads the mode from the rest. */
static const UINT8 str_product[] =
{
    38, 0x03, 'r', 0, 'p', 0, '5', 0, '6', 0, '9', 0, ' ', 0, 'C', 0, 'H', 0, '5', 0, '6', 0, '9', 0, ' ', 0,
    's', 0, 't', 0, 'r', 0, 'e', 0, 'a', 0, 'm', 0
};
static UINT8 str_serial[34] = { 34, 0x03 };      /* chip UID, 16 hex digits */

static const UINT8 *ep0_ptr;
static UINT16 ep0_left;
static UINT8 setup_req, dev_addr, configured;
/* Bulk IN 0x82: double-buffered (CH569DS1 table 12-4, TX_EN + BUF_MOD: DATA0 sent from UEP2_TX_DMA, DATA1 from
 * UEP2_RX_DMA), see usb_stream_fill(). */
static UINT8  inflight;                /* packets armed and not yet sent (0..2) */
static UINT8  sw_tog;                  /* toggle of the oldest armed packet (= buffer the controller sends next) */
static UINT16 inflight_len[2];         /* their lengths, oldest first */

/* ------------------------------------------------------------------ init */
void usb_stream_init(void)
{
    char uid[2 * CHIP_UID_LEN + 1];
    UINT8 i;
    chip_uid_hex(uid);
    for(i = 0; i < 16; i++)
    {
        str_serial[2 + 2 * i] = (UINT8)uid[i];
        str_serial[3 + 2 * i] = 0;
    }

    R8_USB_CTRL = RB_USB_CLR_ALL | RB_USB_RESET_SIE;
    mDelaymS(10);
    R8_USB_CTRL = 0;
    R8_USB_CTRL = UCST_HS | RB_DEV_PU_EN | RB_USB_INT_BUSY | RB_USB_DMA_EN;
    R8_USB_INT_EN = 0;                          /* polled */
    R8_USB_DEV_AD = 0;

    R8_UEP2_3_MOD = RB_UEP2_TX_EN | RB_UEP2_BUF_MOD;
    R16_UEP0_MAX_LEN = EP0_SIZE;
    R16_UEP2_MAX_LEN = BULK_SIZE;
    R32_UEP0_RT_DMA = (UINT32)ep0_buf;
    R16_UEP0_T_LEN = 0;
    R8_UEP0_TX_CTRL = UEP_T_RES_NAK;
    R8_UEP0_RX_CTRL = UEP_R_RES_NAK;
    R16_UEP2_T_LEN = 0;
    R8_UEP2_TX_CTRL = UEP_T_RES_NAK | RB_UEP_T_AUTOTOG;
    configured = 0;
    inflight = 0;
    sw_tog = 0;
}

UINT8 usb_stream_configured(void) { return configured; }

/* Bulk IN endpoint back to DATA0 with nothing armed (configuration, clear halt, bus reset). */
static void ep2_restart(void)
{
    R8_UEP2_TX_CTRL = UEP_T_RES_NAK | RB_UEP_T_TOG_0 | RB_UEP_T_AUTOTOG;
    R8_USB_CTRL |= RB_USB_INT_BUSY;
    inflight = 0;
    sw_tog = 0;
    stream_chunk_rewind();
}

static inline UINT8 hw_tog(void) { return (R8_UEP2_TX_CTRL & RB_UEP_T_TOG_MASK) ? 1 : 0; }

static inline void ep2_res(UINT8 res)     /* only while no IN can complete (endpoint NAKing or frozen) */
{
    R8_UEP2_TX_CTRL = (R8_UEP2_TX_CTRL & ~RB_UEP_TRES_MASK) | res;
}

/* Arm the free buffer(s) from the chunk cursor. Writing the DMA address of the buffer the controller is not using is
 * safe at any time. All chunks are 512 bytes, so the shared T_LEN fits both. Returns the number armed. */
static UINT8 ep2_arm(void)
{
    UINT8 *p, k = 0;
    UINT16 n;
    while(inflight < 2 && (p = stream_chunk_take(&n)) != 0)
    {
        if(sw_tog ^ inflight)
            R32_UEP2_RX_DMA = (UINT32)p;
        else
            R32_UEP2_TX_DMA = (UINT32)p;
        R16_UEP2_T_LEN = n;
        inflight_len[inflight++] = n;
        k++;
    }
    return k;
}

/* Bulk IN service, every main-loop pass (board #1 findings, docs/ch569-hardware-notes.md):
 * - RB_USB_INT_BUSY (auto-NAK while the transfer flag is set) halves the rate (22.8 MB/s: the host gets a NAK on the
 *   IN right after each packet and backs off), so it is off while both buffers are armed: 44.8 MB/s.
 * - Without it the controller goes straight on to the other buffer after a packet, so with a single packet armed it
 *   would then send a stale buffer: INT_BUSY is on whenever fewer than two are armed, which freezes the endpoint
 *   after that packet until the firmware has set NAK or re-armed.
 * - Completions are counted from the data toggle, not the shared transfer flag (two may merge into one flag): the
 *   loop runs far faster than one 512-byte packet (~9 us), so the toggle moves at most once per pass.
 * - R8_UEP2_TX_CTRL is only written while no IN can complete (endpoint NAKing or frozen): a read-modify-write racing
 *   a completion undoes its auto-toggle, and the host drops the next packet as a retransmission. */
void usb_stream_fill(void)
{
    if(!configured)
        return;
    if(inflight && hw_tog() != sw_tog)
    {
        UINT16 n = inflight_len[0];
        inflight_len[0] = inflight_len[1];
        inflight--;
        sw_tog ^= 1;
        stream_chunk_done(n);
        if(!inflight)
        {
            /* the lone packet went out: INT_BUSY was on, the endpoint is frozen until the flag is cleared */
            if(!ep2_arm())
                ep2_res(UEP_T_RES_NAK);
            else if(inflight == 2)
                R8_USB_CTRL &= ~RB_USB_INT_BUSY;
            R8_USB_INT_FG = RB_USB_IF_TRANSFER;
            return;
        }
        /* one still armed, maybe on the wire: refill the buffer just sent, or freeze after the last one */
        if(!ep2_arm())
        {
            R8_USB_CTRL |= RB_USB_INT_BUSY;
            R8_USB_INT_FG = RB_USB_IF_TRANSFER;       /* the flag of the packet just counted: let the last one go */
            if(hw_tog() != sw_tog)
            {
                /* it went out before INT_BUSY was on (its flag merged with the previous one): nothing armed, and
                 * the controller now points at a sent buffer: NAK at once (narrow race, see above) */
                ep2_res(UEP_T_RES_NAK);
                n = inflight_len[0];
                inflight = 0;
                sw_tog ^= 1;
                stream_chunk_done(n);
            }
        }
        else if((R8_USB_INT_ST & 0x0F) == 2)
            R8_USB_INT_FG = RB_USB_IF_TRANSFER;
        return;
    }
    if(!inflight)
    {
        if(ep2_arm())
        {
            if(inflight == 2)
                R8_USB_CTRL &= ~RB_USB_INT_BUSY;
            else
                R8_USB_CTRL |= RB_USB_INT_BUSY;
            ep2_res(UEP_T_RES_ACK);                    /* endpoint was NAKing: safe */
        }
    }
    else if(inflight == 1 && ep2_arm())
        R8_USB_CTRL &= ~RB_USB_INT_BUSY;               /* both armed again: no more auto-NAK */
}

/* ------------------------------------------------------------------ EP0 */
static UINT16 ep0_chunk(void)
{
    UINT16 n = ep0_left > EP0_SIZE ? EP0_SIZE : ep0_left;
    memcpy(ep0_buf, ep0_ptr, n);
    ep0_ptr += n;
    ep0_left -= n;
    return n;
}

static void ep0_reply(const void *data, UINT16 len)
{
    if(len > SETUP->wLength)
        len = SETUP->wLength;
    ep0_ptr = (const UINT8 *)data;
    ep0_left = len;
    R16_UEP0_T_LEN = ep0_chunk();
    R8_UEP0_TX_CTRL = UEP_T_RES_ACK | RB_UEP_T_TOG_1;
    R8_UEP0_RX_CTRL = UEP_R_RES_ACK | RB_UEP_R_TOG_1;
}

static void ep0_stall(void)
{
    R16_UEP0_T_LEN = 0;
    R8_UEP0_TX_CTRL = UEP_T_RES_STALL;
    R8_UEP0_RX_CTRL = UEP_R_RES_STALL;
}

static void ep0_status_in(void)   /* zero-length status for OUT / no-data requests */
{
    ep0_left = 0;
    R16_UEP0_T_LEN = 0;
    R8_UEP0_TX_CTRL = UEP_T_RES_ACK | RB_UEP_T_TOG_1;
    R8_UEP0_RX_CTRL = UEP_R_RES_ACK | RB_UEP_R_TOG_1;
}

static void handle_setup(void)
{
    static UINT8 status[2];
    setup_req = SETUP->bRequest;

    if((SETUP->bRequestType & 0x60) == USB_REQ_TYP_VENDOR)
    {
        switch(setup_req)
        {
            case STREAM_REQ_STATS: ep0_reply((const void *)&stream_stats, sizeof(stream_stats)); return;
            case STREAM_REQ_RESET: stream_reset(); ep0_status_in(); return;
            case STREAM_REQ_EDGE:  hspi_rx_set_edge(SETUP->wValueL & 1); ep0_status_in(); return;
            default: ep0_stall(); return;
        }
    }
    switch(setup_req)
    {
        case USB_GET_DESCRIPTOR:
            switch(SETUP->wValueH)
            {
                case USB_DESCR_TYP_DEVICE: ep0_reply(dev_desc, sizeof(dev_desc)); return;
                case USB_DESCR_TYP_CONFIG: ep0_reply(cfg_desc, sizeof(cfg_desc)); return;
                case USB_DESCR_TYP_STRING:
                    switch(SETUP->wValueL)
                    {
                        case 0: ep0_reply(str_lang, sizeof(str_lang)); return;
                        case 1: ep0_reply(str_vendor, sizeof(str_vendor)); return;
                        case 2: ep0_reply(str_product, sizeof(str_product)); return;
                        case 3: ep0_reply(str_serial, sizeof(str_serial)); return;
                    }
                    break;
            }
            ep0_stall();
            return;
        case USB_SET_ADDRESS:
            dev_addr = SETUP->wValueL;            /* applied after the status stage */
            ep0_status_in();
            return;
        case USB_SET_CONFIGURATION:
            configured = SETUP->wValueL;
            ep2_restart();
            ep0_status_in();
            return;
        case USB_GET_CONFIGURATION:
            status[0] = configured;
            ep0_reply(status, 1);
            return;
        case USB_GET_STATUS:
            status[0] = status[1] = 0;
            ep0_reply(status, 2);
            return;
        case USB_CLEAR_FEATURE:                   /* endpoint halt on 0x82: back to DATA0 */
            if((SETUP->bRequestType & 0x1F) == USB_REQ_RECIP_ENDP && SETUP->wIndexL == 0x82)
                ep2_restart();
            ep0_status_in();
            return;
        case USB_SET_INTERFACE:
        case USB_SET_FEATURE:
            ep0_status_in();
            return;
        default:
            ep0_stall();
            return;
    }
}

void usb_stream_poll(void)
{
    UINT8 fg = R8_USB_INT_FG;

    if(fg & RB_USB_IF_SETUOACT)
    {
        handle_setup();
        R8_USB_INT_FG = RB_USB_IF_SETUOACT;
    }
    else if(fg & RB_USB_IF_TRANSFER)
    {
        UINT8 st = R8_USB_INT_ST;
        UINT8 ep = st & 0x0F;
        UINT8 token = (st >> 4) & 0x03;            /* 0 OUT, 2 IN */
        if(ep == 0 && token == 2)
        {
            if(setup_req == USB_SET_ADDRESS)
            {
                R8_USB_DEV_AD = dev_addr;
                setup_req = 0xFF;
                R8_UEP0_TX_CTRL = UEP_T_RES_NAK;
            }
            else if(ep0_left)
            {
                R16_UEP0_T_LEN = ep0_chunk();
                R8_UEP0_TX_CTRL ^= RB_UEP_T_TOG_1;
                R8_UEP0_TX_CTRL = (R8_UEP0_TX_CTRL & ~RB_UEP_TRES_MASK) | UEP_T_RES_ACK;
            }
            else
            {
                /* data stage (or no-data status) done: NAK further INs, accept the status OUT. EP0 must never be
                 * left in ACK when idle: with the controller's auto-NAK off (two bulk IN packets armed) the host's IN right
                 * after the next SETUP would get a stale buffer. */
                R16_UEP0_T_LEN = 0;
                R8_UEP0_TX_CTRL = UEP_T_RES_NAK;
                R8_UEP0_RX_CTRL = UEP_R_RES_ACK | RB_UEP_R_TOG_1;
            }
        }
        else if(ep == 0)                            /* OUT: status stage of an IN request: EP0 idle */
        {
            R16_UEP0_T_LEN = 0;
            R8_UEP0_TX_CTRL = UEP_T_RES_NAK;
            R8_UEP0_RX_CTRL = UEP_R_RES_NAK;
        }
        else if(ep == 2)
            return;                                 /* bulk IN: usb_stream_fill() (data toggle) owns this flag */
        R8_USB_INT_FG = RB_USB_IF_TRANSFER;
    }
    else if(fg & RB_USB_IF_BUSRST)
    {
        R8_USB_DEV_AD = 0;
        configured = 0;
        R16_UEP0_T_LEN = 0;
        R8_UEP0_TX_CTRL = UEP_T_RES_NAK;
        R8_UEP0_RX_CTRL = UEP_R_RES_NAK;
        ep2_restart();
        R8_USB_INT_FG = RB_USB_IF_BUSRST;
    }
    else if(fg & RB_USB_IF_SUSPEND)
        R8_USB_INT_FG = RB_USB_IF_SUSPEND;
}
