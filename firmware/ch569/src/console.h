/********************************************************************************
 * console.h — command line on the debug port (CDC #2) and UART0, see console.c.
 *******************************************************************************/
#ifndef CONSOLE_H_
#define CONSOLE_H_

#include "CH56x_common.h"

void console_rx(const UINT8 *buf, UINT16 len);   /* bytes from the host (IRQ safe) */
void console_poll(void);                         /* main loop: parse and run lines */

#endif /* CONSOLE_H_ */
