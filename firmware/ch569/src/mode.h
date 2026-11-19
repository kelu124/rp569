/********************************************************************************
 * mode.h — CH569 USB personality: drive mode (SD card as mass storage, nothing
 * else on USB, full speed) or serial mode (card released, RP2350 link CDC +
 * console CDC). Specification D-028.
 *
 * Switching re-enumerates in place: USB off for 300 ms (the host sees a
 * disconnect), function state cleared, USB started again with the other
 * mode's descriptors (no chip reset, see mode.c).
 *   drive -> serial: host eject (SCSI START STOP UNIT, LoEj=1 Start=0), or the
 *                    RP2350 link line "\x01SDREL" (ch eject / before power-off)
 *   serial -> drive: console `mount`, or the RP2350 link line "\x01SDMNT"
 *                    (ch mount); the CH569 answers "\x01SDON" (or "\x01SDERR")
 *                    on the link once the card is mounted again.
 * Power-up starts in drive mode; no card -> serial mode.
 *******************************************************************************/
#ifndef MODE_H_
#define MODE_H_

#include "CH56x_common.h"

#define MODE_DRIVE    0
#define MODE_SERIAL   1

/* Interfaces in serial mode (drive mode has the MSC on interface 0 only) */
#define SERIAL_LINK_IF      0      /* CDC comm, data on IF1: RP2350 link UART */
#define SERIAL_CONSOLE_IF   2      /* CDC comm, data on IF3: debug console     */

extern UINT8 bridge_mode;          /* MODE_DRIVE / MODE_SERIAL, fixed until the next switch */

/* Mount the card for drive mode: MODE_DRIVE, or MODE_SERIAL if there is no usable card. */
UINT8 mode_enter_drive(void);

/* Start the USB stack (SuperSpeed first, USB2 fallback) with bridge_mode's descriptors. */
void usb_start(void);

/* Request a switch; it runs from mode_poll() in the main loop once the MSC engine is idle. */
void mode_request(UINT8 mode, UINT8 announce);
void mode_poll(void);

#endif /* MODE_H_ */
