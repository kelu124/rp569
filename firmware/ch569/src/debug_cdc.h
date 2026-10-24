/********************************************************************************
 * debug_cdc.h — read-only debug-log stream over a second USB CDC port.
 *
 * printf()/PRINT() output is mirrored (via the _write hook in CH56x_sys.c)
 * into a ring buffer here, and drained to the debug CDC IN endpoint (EP6) from
 * the main loop. The port is read-only from the host's side: any bytes the host
 * sends are discarded. Independent of the FPGA-UART bridge CDC port (EP1/EP4).
 *
 * Enabled by DEBUG_OVER_USB in bridge_config.h.
 *******************************************************************************/
#ifndef DEBUG_CDC_H_
#define DEBUG_CDC_H_

#include "CH56x_common.h"

/* EP6 DMA buffers (single 1024-byte SS bulk packet each). */
extern __attribute__((aligned(16))) UINT8 endp6Txbuff[1024] __attribute__((section(".DMADATA")));
extern __attribute__((aligned(16))) UINT8 endp6Rxbuff[1024] __attribute__((section(".DMADATA")));

extern volatile UINT8 DebugUpload_Busy;     /* EP6 IN transfer in flight */

void debug_cdc_poll(void);                  /* main-loop: drain ring -> EP6 IN */
void debug_cdc_clear(void);                 /* reset ring + endpoint state     */

#endif /* DEBUG_CDC_H_ */
