/********************************************************************************
 * chip_uid.h — the CH569's factory unique ID (8 bytes), see chip_uid.c.
 *******************************************************************************/
#ifndef CHIP_UID_H_
#define CHIP_UID_H_

#include "CH56x_common.h"

#define CHIP_UID_LEN  8

void chip_uid_read(UINT8 uid[CHIP_UID_LEN]);
void chip_uid_hex(char out[2 * CHIP_UID_LEN + 1]);   /* 16 hex digits + NUL */

#endif /* CHIP_UID_H_ */
