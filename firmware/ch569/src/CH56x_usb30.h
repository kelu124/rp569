/********************************************************************************
 * CH56x_usb30.h — unified composite USB3 device layer (MSC + CDC-ACM).
 *
 * This file replaces BOTH examples' USB30/CH56x_usb30.c headers: the MSC and
 * CDC EVT projects each define the same symbols and cannot co-link, so the
 * enumeration layer here is a hand merge (see CH56x_usb30.c).
 *
 * Endpoint map (SuperSpeed):
 *   EP0          control      enumeration + class/vendor requests
 *   EP1 IN 0x81  interrupt    CDC notification (declared, idle)
 *   EP2 IN 0x82  bulk         MSC data IN  (unchanged from MSC example)
 *   EP3 OUT 0x03 bulk         MSC data OUT (unchanged from MSC example)
 *   EP4 IN 0x84  bulk         CDC data IN
 *   EP4 OUT 0x04 bulk         CDC data OUT
 *
 * Interfaces: IF0 = MSC (08/06/50), IAD groups IF1 = CDC Communications
 * (02/02/01) + IF2 = CDC Data (0A/00/00). Device class EF/02/01 so Windows
 * binds the composite-device parent driver.
 *******************************************************************************/
#ifndef USB30_CH56X_USB30_H_
#define USB30_CH56X_USB30_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "CH56x_common.h"
#include "bridge_config.h"

/* Burst levels: must stay consistent with the SS endpoint companion
 * descriptors in CH56x_usb30.c and every USB30_IN_Set/OUT_Set call. */
#define DEF_ENDP2_IN_BURST_LEVEL     1
#define DEF_ENDP3_OUT_BURST_LEVEL    1
#define DEF_ENDP4_BURST_LEVEL        1
#define DEF_ENDP6_BURST_LEVEL        1

#define SIZE_DEVICE_DESC             18

/* Composite layout. With DEBUG_OVER_USB the device adds a second CDC function
 * (read-only debug log): 5 interfaces instead of 3, config descriptor 212 vs
 * 128 bytes. Keep these in lock-step with SS_ConfigDescriptor. */
#if DEBUG_OVER_USB
  #define SIZE_CONFIG_DESC           212
  #define BRIDGE_NUM_INTERFACES      0x05
#else
  #define SIZE_CONFIG_DESC           128
  #define BRIDGE_NUM_INTERFACES      0x03
#endif
#define SIZE_STRING_LANGID           4
#define SIZE_STRING_VENDOR           8
#define SIZE_STRING_PRODUCT          42
#define SIZE_STRING_SERIAL           22
#define SIZE_BOS_DESC                22
#define SIZE_STRING_OS               18

#define LINK_STA_1                   (1 << 0)   /* running on USB2 fallback   */
#define LINK_STA_3                   (1 << 2)   /* USB3 SuperSpeed link up    */

/* CDC-ACM class requests (interface 1) */
#define CDC_SET_LINE_CODING          0x20
#define CDC_GET_LINE_CODING          0x21
#define CDC_SET_CONTROL_LINE_STATE   0x22

/* MSC disk DMA ring: 24 sectors x 512B per direction. */
#define UDISKSIZE                    (1024 * 4 * 10)

/* Global Variable */
extern __attribute__((aligned(16))) UINT8 endp0RTbuff[512] __attribute__((section(".DMADATA")));
extern __attribute__((aligned(16))) UINT8 UDisk_In_Buf[UDISKSIZE] __attribute__((section(".DMADATA")));
extern __attribute__((aligned(16))) UINT8 UDisk_Out_Buf[UDISKSIZE] __attribute__((section(".DMADATA")));
extern UINT8V Link_Sta;

/* Function declaration.
 * Plain __attribute__((interrupt)): upstream GCC ignores MounRiver's
 * "WCH-Interrupt-fast" variant, which would leave these as ordinary
 * functions and crash on the first interrupt. */
void USB30D_init(FunctionalState sta);
void TMR0_IRQHandler(void) __attribute__((interrupt));
void LINK_IRQHandler(void) __attribute__((interrupt));
void USBSS_IRQHandler(void) __attribute__((interrupt));

#ifdef __cplusplus
}
#endif

#endif /* USB30_CH56X_USB30_H_ */
