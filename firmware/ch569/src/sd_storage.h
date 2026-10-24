/********************************************************************************
 * sd_storage.h — SD-card (not eMMC) bring-up on the CH569 EMMC controller.
 *
 * The WCH MSC example initialises an eMMC via EMMCCardConfig() (CMD1 flow);
 * the target card here is a microSDXC, which needs the SD flow instead:
 * CMD0 -> CMD8(0x1AA) -> ACMD41 -> CMD2 -> CMD3 -> CMD9 -> CMD7 -> ACMD6
 * (4-bit) -> SCR read. Ported from the WCH "SD" EVT example (SD/User/Main.c)
 * on top of the SD.c helpers, with the 8-step IO-timing adaptive loop kept.
 *
 * Card parameters land in TF_EMMCParam (defined in SW_UDISK.c); the MSC layer
 * serves sectors directly via the EMMC controller's CMD18/CMD25 engine.
 *******************************************************************************/
#ifndef SD_STORAGE_H_
#define SD_STORAGE_H_

#include "CH56x_common.h"

/* Full mount: adaptive IO init + SD identification. OP_SUCCESS / OP_FAILED. */
UINT8 sd_storage_mount(void);

/* Quiesce the controller before the card is handed back to the FPGA. */
void sd_storage_unmount(void);

#endif /* SD_STORAGE_H_ */
