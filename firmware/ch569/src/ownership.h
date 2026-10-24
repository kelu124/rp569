/********************************************************************************
 * ownership.h — SD-card ownership arbitration between the FPGA and the CH569.
 *
 * rp569: SD_OWNERSHIP_PIN = 0, no MUX and no PB10 line; claim/release only
 * mount/unmount the card (CLAIM_ON_BOOT mounts it at power-up).
 * Original USB3_SSP behaviour:
 * The card sits behind a physical MUX driven by the FPGA. The CH569 requests
 * the card by driving PB10 high (10K pull-down keeps it low / FPGA-owned at
 * reset) and returns it by driving PB10 low. There is no ACK line back:
 * after asserting PB10 the firmware waits PB10_SETTLE_MS and then (re)inits
 * the card ("assert and proceed", agreed with the client).
 *
 * Requests can arrive from interrupt context (EP0 vendor request) or from the
 * CDC byte stream ('t'); the actual claim/release — which includes a slow SD
 * re-init — only ever runs from ownership_poll() in the main loop, so it can
 * never preempt an in-flight MSC transfer.
 *******************************************************************************/
#ifndef OWNERSHIP_H_
#define OWNERSHIP_H_

#include "CH56x_common.h"

#define OWN_REQ_NONE      0
#define OWN_REQ_RELEASE   1
#define OWN_REQ_CLAIM     2
#define OWN_REQ_TOGGLE    3

/* 1 while the CH569 owns the card (PB10 driven high). */
extern volatile UINT8 own_ch569_owns;
/* 1 once the card has been successfully (re)initialised after a claim. */
extern volatile UINT8 own_card_ready;

void ownership_init(void);
void ownership_request(UINT8 req);   /* IRQ-safe: just latches the request   */
void ownership_poll(void);           /* main-loop: executes a pending request */

#endif /* OWNERSHIP_H_ */
