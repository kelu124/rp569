/********************************************************************************
 * msc_fast.h — full-speed READ10 / WRITE10 data phases in drive mode on the USB2
 * high-speed link (msc_fast.c, specification D-029).
 *******************************************************************************/
#ifndef MSC_FAST_H_
#define MSC_FAST_H_

#include "CH56x_common.h"

UINT8 msc_fast_active(void);   /* drive mode on the USB2 link: the fast paths apply */
void  msc_fast_read(void);     /* READ10 data phase + CSW (after CMD_RD_WR_Deal_Pre) */
void  msc_fast_write(void);    /* WRITE10 data phase + CSW, first packet already in UDisk_Out_Buf */

#endif /* MSC_FAST_H_ */
