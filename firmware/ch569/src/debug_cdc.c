/********************************************************************************
 * debug_cdc.c — read-only debug-log stream over the second USB CDC port (EP6).
 * See debug_cdc.h.
 *******************************************************************************/
#include "CH56x_common.h"
#include "CH56xusb30_LIB.h"
#include "CH56x_usb30.h"
#include "debug_cdc.h"
#include "bridge_config.h"

#if DEBUG_OVER_USB

#define DBG_RING_SZ   4096          /* power of two */
#define DBG_RING_MASK (DBG_RING_SZ - 1)

__attribute__((aligned(16))) UINT8 endp6Txbuff[1024] __attribute__((section(".DMADATA")));
__attribute__((aligned(16))) UINT8 endp6Rxbuff[1024] __attribute__((section(".DMADATA")));

volatile UINT8 DebugUpload_Busy = 0;

static volatile UINT8  dbg_ring[DBG_RING_SZ];
static volatile UINT16 dbg_head = 0;        /* producer (_write, may be IRQ) */
static volatile UINT16 dbg_tail = 0;        /* consumer (main loop)          */

/*******************************************************************************
 * debug_mirror — weak hook called from _write() (CH56x_sys.c) for every byte
 * of printf output. Pushes into the ring; drops oldest-unsent on overflow (it
 * is only a log). Single-producer / single-consumer, lock-free.
 ******************************************************************************/
void debug_mirror(const char *buf, int size)
{
    for(int i = 0; i < size; i++)
    {
        UINT16 next = (dbg_head + 1) & DBG_RING_MASK;
        if(next == dbg_tail)            /* full: drop one to make room */
            dbg_tail = (dbg_tail + 1) & DBG_RING_MASK;
        dbg_ring[dbg_head] = (UINT8)buf[i];
        dbg_head = next;
    }
}

void debug_cdc_clear(void)
{
    DebugUpload_Busy = 0;
    dbg_tail = dbg_head;
}

/*******************************************************************************
 * debug_cdc_poll — main loop: send any buffered log bytes out of EP6 IN.
 * Only runs on the USB3 link (the USB2 fallback is MSC-only).
 ******************************************************************************/
void debug_cdc_poll(void)
{
    UINT16 n;

    if(Link_Sta == LINK_STA_1)          /* USB2 fallback: no debug CDC */
        return;
    if(DebugUpload_Busy)
        return;
    if(dbg_tail == dbg_head)
        return;

    n = 0;
    while(dbg_tail != dbg_head && n < 1024)
    {
        endp6Txbuff[n++] = dbg_ring[dbg_tail];
        dbg_tail = (dbg_tail + 1) & DBG_RING_MASK;
    }

    DebugUpload_Busy = 1;
    USB30_IN_ClearIT(ENDP_6);
    USB30_IN_Set(ENDP_6, ENABLE, ACK, 1, n);
    USB30_Send_ERDY(ENDP_6 | IN, 1);
}

#endif /* DEBUG_OVER_USB */
