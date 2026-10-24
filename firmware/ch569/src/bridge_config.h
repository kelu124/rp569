/********************************************************************************
 * bridge_config.h — every board/behaviour tunable of the rp569 CH569 firmware.
 *
 * Forked from CH569_Composite (USB3_SSP board). Board facts for rp569, from
 * hardware/gen/board.py and docs/pinmap.md (checked against CH569DS1 pin table):
 *   - SD card    : SDIO 4-bit on the CH569's fixed EMMC pins (CLK=PB14,
 *                  CMD=PB16, DAT0..3=PB17..PB20), wired directly (no MUX):
 *                  the CH569 always owns the card.
 *   - UART1      : PA7 (RXD1, pin 13, from RP2350 GPIO24) / PA8 (TXD1, pin 12,
 *                  to RP2350 GPIO25). The RP2350 link: bytes from the RP2350
 *                  are echoed back and copied to the USB CDC port; bytes typed
 *                  on the CDC port go to the RP2350.
 *   - UART0      : PB5 (RXD0, pin 38) / PB6 (TXD0, pin 39), CH UART header J8:
 *                  debug log. Never remap UART0 to PA5/PA6 (PA5 = HD0 = boot).
 *   - RGB LED    : PB0 / PB1 / PB2 = R / G / B, common anode on 3V3_CH,
 *                  pin low = on.
 *******************************************************************************/
#ifndef BRIDGE_CONFIG_H_
#define BRIDGE_CONFIG_H_

/* USB identity (SuperSpeed composite). 0x1A86 = WCH; PID picked to avoid the
 * stock example PIDs (FE10 MSC / FE0C CDC) so cached host drivers don't clash. */
#define BRIDGE_USB_VID_L            0x86
#define BRIDGE_USB_VID_H            0x1A
#define BRIDGE_USB_PID_L            0x2C
#define BRIDGE_USB_PID_H            0xFE

/* Vendor control requests on EP0 (kept from CH569_Composite):
 *   bmRequestType 0x40, bRequest 0x50, wValue 0=release / 1=claim / 2=toggle
 *   bmRequestType 0xC0, bRequest 0x51 -> 1 status byte:
 *       bit0 = CH569 owns the card, bit1 = card mounted/ready              */
#define BRIDGE_REQ_SD_OWNER         0x50
#define BRIDGE_REQ_SD_STATUS        0x51

/* rp569 has no SD MUX: mount the card at power-up. */
#define CLAIM_ON_BOOT               1

/* SD ownership request line. 0 = none (rp569: card wired straight to the
 * CH569, PB10 is a free pin on the CH EXT header). */
#define SD_OWNERSHIP_PIN            0
#define PB10_SETTLE_MS              0

/* Attempts of the full SD init (each attempt internally tries all 8 EMMC IO
 * timing modes, as in the WCH SD example). */
#define SD_MOUNT_RETRIES            3

/* RP2350 link (UART1) baud rate; must match LINK_BAUD in firmware/rp2350.
 * A CDC SET_LINE_CODING from the host overrides it at run time. 8N1. */
#define LINK_UART_DEFAULT_BAUD      115200

/* 1 = echo every byte received from the RP2350 back to it (link test). */
#define LINK_ECHO                   1

/* Debug log UART0 baud rate (PB6 = TX, PB5 = RX, CH UART header). */
#define DEBUG_UART_BAUD             115200

/* Bounded spin-guard for every EMMC command/data completion wait. Finite so an
 * unanswered / unreachable card fails the mount cleanly instead of hanging the
 * whole device (and starving USB) forever. */
#define MSC_EMMC_GUARD              2000000UL

/* 1 = also expose the debug log as a SECOND, read-only USB CDC port. The
 * first CDC port stays the RP2350-link bridge. */
#define DEBUG_OVER_USB              1

/* Status on the RGB LED (PB0/PB1/PB2, active low):
 *   R = SD card mounted / ready
 *   G = USB3 SuperSpeed link up
 *   B = toggles on every line received from the RP2350 */
#define STATUS_LEDS                 1
#define STATUS_LED_ACTIVE_HIGH      0
#define LED_READY_PIN               GPIO_Pin_0
#define LED_LINK_PIN                GPIO_Pin_1
#define LED_RX_PIN                  GPIO_Pin_2

#endif /* BRIDGE_CONFIG_H_ */
