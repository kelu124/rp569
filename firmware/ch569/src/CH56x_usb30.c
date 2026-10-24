/********************************************************************************
 * CH56x_usb30.c — unified composite USB3 device layer: MSC (SD card) +
 * CDC-ACM (FPGA UART bridge).
 *
 * Hand merge of the WCH MSC_U-Disk and SimulateCDC examples' USB30 layers
 * (the two files define identical symbols and cannot co-link). MSC keeps its
 * native EP2-IN/EP3-OUT so SW_UDISK.c runs unchanged; CDC data moves from
 * EP2 to EP4; EP1-IN remains the (idle) CDC notification endpoint.
 *
 * Class requests are routed by request type + bRequest and validated against
 * the target interface in wIndex (MSC = IF0, CDC = IF1), plus two vendor
 * requests for out-of-band SD-ownership control (see bridge_config.h).
 *
 * Link management (LINK_IRQHandler, TMR0 USB3->USB2 fallback) is byte-for-byte
 * the MSC example's. On the USB2 fallback the device is MSC-only (the vendor
 * USB2 stack has no CDC function) — documented limitation.
 *******************************************************************************/
#include "CH56x_common.h"
#include "CH56xusb30_LIB.h"
#include "CH56x_usb20.h"
#include "CH56x_usb30.h"
#include "SW_UDISK.h"
#include "console.h"
#include "cdc_uart.h"
#include "bridge_config.h"
#include "ownership.h"
#if DEBUG_OVER_USB
#include "debug_cdc.h"
#endif

/* Global Variable */
UINT8V        Tx_Lmp_Port = 0;
UINT8V        Link_Sta = 0;
static UINT32 SetupLen = 0;
static UINT8  SetupReqCode = 0;
static UINT8  SetupReqIface = 0;     /* target interface of a pending class req */
static PUINT8 pDescr;

__attribute__((aligned(16))) UINT8 endp0RTbuff[512] __attribute__((section(".DMADATA")));
__attribute__((aligned(16))) UINT8 UDisk_In_Buf[UDISKSIZE] __attribute__((section(".DMADATA")));
__attribute__((aligned(16))) UINT8 UDisk_Out_Buf[UDISKSIZE] __attribute__((section(".DMADATA")));
__attribute__((aligned(16))) static UINT8 endp1NotifyBuff[64] __attribute__((section(".DMADATA")));

/* SuperSpeed device descriptor: Miscellaneous/IAD so the host loads its
 * composite parent driver and binds MSC + CDC separately. */
const UINT8 SS_DeviceDescriptor[] =
{
    0x12,                  // bLength
    0x01,                  // DEVICE descriptor type
    0x00, 0x03,            // bcdUSB 3.00
    0xEF,                  // bDeviceClass: Miscellaneous
    0x02,                  // bDeviceSubClass: Common Class
    0x01,                  // bDeviceProtocol: Interface Association Descriptor
    0x09,                  // bMaxPacketSize0: 2^9 = 512B
    BRIDGE_USB_VID_L, BRIDGE_USB_VID_H,
    BRIDGE_USB_PID_L, BRIDGE_USB_PID_H,
    0x01, 0x00,            // bcdDevice 0.01
    0x01,                  // iManufacturer
    0x02,                  // iProduct
    0x03,                  // iSerialNumber
    0x01                   // bNumConfigurations
};

/* SuperSpeed configuration: IF0 = MSC, IAD( IF1 = CDC comm, IF2 = CDC data ).
 * Every endpoint descriptor is followed by its SS endpoint companion; the
 * bMaxBurst bytes must match the runtime USB30_IN_Set/OUT_Set burst args. */
const UINT8 SS_ConfigDescriptor[] =
{
    /* Configuration Descriptor */
    0x09,                  // bLength
    0x02,                  // bDescriptorType (Configuration)
    SIZE_CONFIG_DESC & 0xFF, (SIZE_CONFIG_DESC >> 8) & 0xFF,   // wTotalLength
    BRIDGE_NUM_INTERFACES, // bNumInterfaces (3, or 5 with the debug CDC)
    0x01,                  // bConfigurationValue
    0x00,                  // iConfiguration
    0x80,                  // bmAttributes: bus powered
    0x64,                  // bMaxPower 800mA (8mA units at SS)

    /* ---------------- Interface 0: Mass Storage (SD card) --------------- */
    0x09,                  // bLength
    0x04,                  // bDescriptorType (Interface)
    0x00,                  // bInterfaceNumber 0
    0x00,                  // bAlternateSetting
    0x02,                  // bNumEndpoints 2
    0x08,                  // bInterfaceClass: Mass Storage
    0x06,                  // bInterfaceSubClass: SCSI transparent
    0x50,                  // bInterfaceProtocol: Bulk-Only Transport
    0x00,                  // iInterface

    /* EP2 IN (MSC data in) */
    0x07, 0x05, 0x82, 0x02, 0x00, 0x04, 0x00,   // bulk, 1024B
    0x06, 0x30, 0x00, 0x00, 0x00, 0x00,         // companion: burst 0(=1 pkt)

    /* EP3 OUT (MSC data out) */
    0x07, 0x05, 0x03, 0x02, 0x00, 0x04, 0x00,   // bulk, 1024B
    0x06, 0x30, 0x00, 0x00, 0x00, 0x00,         // companion: burst 0 (=1 pkt), MUST match DEF_ENDP3_OUT_BURST_LEVEL

    /* ------------- Interface Association: CDC (IF1 + IF2) --------------- */
    0x08,                  // bLength
    0x0B,                  // bDescriptorType (IAD)
    0x01,                  // bFirstInterface 1
    0x02,                  // bInterfaceCount 2
    0x02,                  // bFunctionClass: CDC
    0x02,                  // bFunctionSubClass: ACM
    0x01,                  // bFunctionProtocol: AT commands
    0x00,                  // iFunction

    /* ------------- Interface 1: CDC Communications (control) ------------ */
    0x09,                  // bLength
    0x04,                  // bDescriptorType (Interface)
    0x01,                  // bInterfaceNumber 1
    0x00,                  // bAlternateSetting
    0x01,                  // bNumEndpoints 1 (notify)
    0x02,                  // bInterfaceClass: CDC
    0x02,                  // bInterfaceSubClass: ACM
    0x01,                  // bInterfaceProtocol: AT commands
    0x00,                  // iInterface

    /* CDC functional descriptors */
    0x05, 0x24, 0x00, 0x10, 0x01,               // Header, bcdCDC 1.10
    0x05, 0x24, 0x01, 0x00, 0x02,               // Call Mgmt, data IF 2
    0x04, 0x24, 0x02, 0x02,                     // ACM, line coding + serial state
    0x05, 0x24, 0x06, 0x01, 0x02,               // Union, master IF1, slave IF2

    /* EP1 IN (CDC notification) */
    0x07, 0x05, 0x81, 0x03, 0x00, 0x04, 0x08,   // interrupt, 1024B, interval 8
    0x06, 0x30, 0x00, 0x00, 0x00, 0x04,         // companion: wBytesPerInterval 1024

    /* ------------------ Interface 2: CDC Data --------------------------- */
    0x09,                  // bLength
    0x04,                  // bDescriptorType (Interface)
    0x02,                  // bInterfaceNumber 2
    0x00,                  // bAlternateSetting
    0x02,                  // bNumEndpoints 2
    0x0A,                  // bInterfaceClass: CDC Data
    0x00, 0x00,            // bInterfaceSubClass/Protocol
    0x00,                  // iInterface

    /* EP4 IN (CDC data in) */
    0x07, 0x05, 0x84, 0x02, 0x00, 0x04, 0x00,   // bulk, 1024B
    0x06, 0x30, 0x00, 0x00, 0x00, 0x00,         // companion: burst 0

    /* EP4 OUT (CDC data out) */
    0x07, 0x05, 0x04, 0x02, 0x00, 0x04, 0x00,   // bulk, 1024B
    0x06, 0x30, 0x00, 0x00, 0x00, 0x00,         // companion: burst 0

#if DEBUG_OVER_USB
    /* ===== Second CDC function: read-only debug log (EP5 notify, EP6 data) ==== */

    /* Interface Association: CDC debug (IF3 + IF4) */
    0x08, 0x0B, 0x03, 0x02, 0x02, 0x02, 0x01, 0x00,

    /* Interface 3: CDC Communications (debug control) */
    0x09, 0x04, 0x03, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,
    0x05, 0x24, 0x00, 0x10, 0x01,               // Header
    0x05, 0x24, 0x01, 0x00, 0x04,               // Call Mgmt, data IF 4
    0x04, 0x24, 0x02, 0x02,                     // ACM
    0x05, 0x24, 0x06, 0x03, 0x04,               // Union, master IF3, slave IF4

    /* EP5 IN (debug CDC notification) */
    0x07, 0x05, 0x85, 0x03, 0x00, 0x04, 0x08,   // interrupt, 1024B, interval 8
    0x06, 0x30, 0x00, 0x00, 0x00, 0x04,         // companion

    /* Interface 4: CDC Data (debug) */
    0x09, 0x04, 0x04, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,

    /* EP6 IN (debug data in — carries the log) */
    0x07, 0x05, 0x86, 0x02, 0x00, 0x04, 0x00,   // bulk, 1024B
    0x06, 0x30, 0x00, 0x00, 0x00, 0x00,

    /* EP6 OUT (debug data out — present for host driver binding; ignored) */
    0x07, 0x05, 0x06, 0x02, 0x00, 0x04, 0x00,   // bulk, 1024B
    0x06, 0x30, 0x00, 0x00, 0x00, 0x00
#endif
};

/*String Descriptor Lang ID*/
const UINT8 StringLangID[] =
{
    0x04, 0x03, 0x09, 0x04
};

/*String Descriptor Vendor*/
const UINT8 StringVendor[] =
{
    0x08, 0x03,
    'W', 0x00, 'C', 0x00, 'H', 0x00
};

/*String Descriptor Product: "rp569 CH569" (11 chars). The host library
 * (python/rp569) finds the ports by VID/PID 1a86:fe2c + this product string. */
const UINT8 StringProduct[] =
{
    24, 0x03,
    'r', 0x00, 'p', 0x00, '5', 0x00, '6', 0x00, '9', 0x00, ' ', 0x00,
    'C', 0x00, 'H', 0x00, '5', 0x00, '6', 0x00, '9', 0x00
};

/*String Descriptor Serial: the chip unique ID as 16 hex digits, filled in by
 * USB30_SetSerialFromUID() before USB init (same UID as the 'version' command). */
UINT8 StringSerial[] =
{
    0x22, 0x03,
    '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00,
    '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00, '0', 0x00
};

void USB30_SetSerial(const char *hex16)
{
    UINT8 i;
    for(i = 0; i < 16 && hex16[i]; i++)
        StringSerial[2 + 2 * i] = (UINT8)hex16[i];
}

const UINT8 OSStringDescriptor[] =
{
    0x12, 0x03,
    'M', 0x00, 'S', 0x00, 'F', 0x00, 'T', 0x00,
    '1', 0x00, '0', 0x00, '0', 0x00,
    0x01, 0x00
};

const UINT8 BOSDescriptor[] =
{
    0x05,                  // bLength
    0x0f,                  // BOS descriptor type
    0x16, 0x00,            // wTotalLength 22
    0x02,                  // bNumDeviceCaps

    /* USB2.0 extension */
    0x07, 0x10, 0x02, 0x06, 0x00, 0x00, 0x00,

    /* SuperSpeed device capability */
    0x0a, 0x10, 0x03, 0x00,
    0x0e, 0x00,            // ss/hs/fs supported
    0x01,                  // lowest speed full-speed
    0x00,                  // bU1DevExitLat = 0: U1 not supported
    0x00, 0x00             // wU2DevExitLat = 0: U2 not supported
    /* U1/U2 disabled on purpose: with them advertised, Linux xHCI repeatedly
     * fails to set the link PM state on this device ("Set PM failed", "Disable
     * of device-initiated U1/U2 failed") and resets it in a loop. Declaring no
     * U1/U2 support keeps the SuperSpeed link stable. */
};

/*******************************************************************************
 * @fn      USB30D_init
 *
 * @brief   USB3.0 device init: composite endpoint set
 *          EP0 ctrl / EP1-IN notify / EP2-IN+EP3-OUT MSC / EP4-IN+OUT CDC.
 ******************************************************************************/
void USB30D_init(FunctionalState sta)
{
    if(sta)
    {
        USB30_Device_Init();

        USBSS->UEP_CFG = EP0_R_EN | EP0_T_EN | EP1_T_EN | EP2_T_EN | EP3_R_EN |
                         EP4_T_EN | EP4_R_EN
#if DEBUG_OVER_USB
                         | EP5_T_EN | EP6_T_EN | EP6_R_EN
#endif
                         ;

        USBSS->UEP0_DMA    = (UINT32)(UINT8 *)endp0RTbuff;
        USBSS->UEP1_TX_DMA = (UINT32)(UINT8 *)endp1NotifyBuff;
        USBSS->UEP2_TX_DMA = (UINT32)(UINT8 *)UDisk_In_Buf;
        USBSS->UEP3_RX_DMA = (UINT32)(UINT8 *)UDisk_Out_Buf;
        USBSS->UEP4_TX_DMA = (UINT32)(UINT8 *)endp4Txbuff;
        USBSS->UEP4_RX_DMA = (UINT32)(UINT8 *)endp4Rxbuff;
#if DEBUG_OVER_USB
        USBSS->UEP5_TX_DMA = (UINT32)(UINT8 *)endp1NotifyBuff;  /* debug notify: idle, shares dummy buf */
        USBSS->UEP6_TX_DMA = (UINT32)(UINT8 *)endp6Txbuff;
        USBSS->UEP6_RX_DMA = (UINT32)(UINT8 *)endp6Rxbuff;
#endif

        /* MSC: ready for a CBW on EP3, EP2 idle until there is data to send */
        USB30_OUT_Set(ENDP_3, ACK, DEF_ENDP3_OUT_BURST_LEVEL);
        USB30_IN_Set(ENDP_2, ENABLE, NRDY, DEF_ENDP2_IN_BURST_LEVEL, 0);

        /* CDC: notification idle; OUT armed for host data */
        USB30_IN_Set(ENDP_1, DISABLE, NRDY, 0, 0);
        USB30_IN_Set(ENDP_4, ENABLE, NRDY, DEF_ENDP4_BURST_LEVEL, 0);
        USB30_OUT_Set(ENDP_4, ACK, DEF_ENDP4_BURST_LEVEL);
        DownloadPoint4_Busy = 1;

#if DEBUG_OVER_USB
        /* Debug CDC: notify idle; IN idle (filled by poll); OUT armed + ignored */
        USB30_IN_Set(ENDP_5, DISABLE, NRDY, 0, 0);
        USB30_IN_Set(ENDP_6, ENABLE, NRDY, DEF_ENDP6_BURST_LEVEL, 0);
        USB30_OUT_Set(ENDP_6, ACK, DEF_ENDP6_BURST_LEVEL);
        debug_cdc_clear();
#endif
    }
    else
    {
        USB30_Switch_Powermode(POWER_MODE_2);
        USBSS->LINK_CFG = PIPE_RESET | LFPS_RX_PD;
        USBSS->LINK_CTRL = GO_DISABLED | POWER_MODE_3;
        USBSS->LINK_INT_CTRL = 0;
        USBSS->USB_CONTROL = USB_FORCE_RST | USB_ALL_CLR;
    }
}

/*******************************************************************************
 * @fn      USB30_NonStandardReq
 *
 * @brief   Class + vendor requests, routed by request type and validated
 *          against the target interface (wIndex): MSC = IF0, CDC = IF1.
 ******************************************************************************/
UINT16 USB30_NonStandardReq()
{
    UINT8  req_typ;
    UINT16 len = 0;

    SetupReqCode = UsbSetupBuf->bRequest;
    SetupLen = UsbSetupBuf->wLength;
    req_typ = UsbSetupBuf->bRequestType;

    if((req_typ & USB_REQ_TYP_MASK) == USB_REQ_TYP_VENDOR)
    {
        /* Out-of-band SD ownership control (see bridge_config.h) */
        switch(SetupReqCode)
        {
            case BRIDGE_REQ_SD_OWNER:                       /* host -> device */
                switch(UsbSetupBuf->wValueL)
                {
                    case 0:  ownership_request(OWN_REQ_RELEASE); break;
                    case 1:  ownership_request(OWN_REQ_CLAIM);   break;
                    default: ownership_request(OWN_REQ_TOGGLE);  break;
                }
                len = 0;
                break;

            case BRIDGE_REQ_SD_STATUS:                      /* device -> host */
                endp0RTbuff[0] = (own_ch569_owns ? 0x01 : 0x00) |
                                 (own_card_ready ? 0x02 : 0x00);
                len = 1;
                break;

            default:
                SetupReqCode = INVALID_REQ_CODE;
                return USB_DESCR_UNSUPPORTED;
        }
        return len;
    }

    /* Class requests */
    switch(SetupReqCode)
    {
        /* ---- MSC (interface 0) ---- */
        case CMD_UDISK_GET_MAX_LUN:
            if(UsbSetupBuf->wIndexL != 0)
                goto stall;
            endp0RTbuff[0] = 0;                             /* single LUN */
            len = 1;
            break;

        case CMD_UDISK_RESET:
            if(UsbSetupBuf->wIndexL != 0)
                goto stall;
            Udisk_Sense_Key = 0x00;
            Udisk_Sense_ASC = 0x00;
            Udisk_CSW_Status = 0x00;
            Udisk_Transfer_Status = 0x00;
            UDISK_Transfer_DataLen = 0x00;
            len = 0;
            break;

        /* ---- CDC-ACM ---- IF1 = FPGA-UART bridge, IF3 = debug log (accepted
         * but ignored: the debug port is read-only, no real UART behind it). */
        case CDC_SET_LINE_CODING:                           /* data on EP0 OUT */
            if(UsbSetupBuf->wIndexL != 1 && UsbSetupBuf->wIndexL != 3)
                goto stall;
            SetupReqIface = UsbSetupBuf->wIndexL;           /* 1=bridge, 3=debug */
            USB30_OUT_Set(ENDP_0, ACK, 1);
            len = 0;
            break;

        case CDC_GET_LINE_CODING:
            if(UsbSetupBuf->wIndexL != 1 && UsbSetupBuf->wIndexL != 3)
                goto stall;
            memcpy(endp0RTbuff, &cdc_line_coding, 7);
            len = 7;
            break;

        case CDC_SET_CONTROL_LINE_STATE:                    /* DTR/RTS: accept */
            if(UsbSetupBuf->wIndexL != 1 && UsbSetupBuf->wIndexL != 3)
                goto stall;
            len = 0;
            break;

        default:
        stall:
            SetupReqCode = INVALID_REQ_CODE;
            return USB_DESCR_UNSUPPORTED;
    }
    return len;
}

/*******************************************************************************
 * @fn      USB30_StandardReq
 *
 * @brief   USB3.0 standard requests (descriptor GETs, address, features).
 *          CLEAR_FEATURE(ENDPOINT_HALT) covers every composite data endpoint
 *          — 0x82/0x03 (MSC) and 0x81/0x84/0x04 (CDC) — so both functions'
 *          stall recovery works.
 ******************************************************************************/
UINT16 USB30_StandardReq()
{
    SetupReqCode = UsbSetupBuf->bRequest;
    SetupLen = UsbSetupBuf->wLength;
    UINT16 len = 0;

    if((UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD)
    {
        len = USB30_NonStandardReq();
    }
    else
    {
        switch(SetupReqCode)
        {
            case USB_GET_DESCRIPTOR:
                switch(UsbSetupBuf->wValueH)
                {
                    case USB_DESCR_TYP_DEVICE:
                        if(SetupLen > SIZE_DEVICE_DESC)
                            SetupLen = SIZE_DEVICE_DESC;
                        pDescr = (PUINT8)SS_DeviceDescriptor;
                        break;
                    case USB_DESCR_TYP_CONFIG:
                        if(SetupLen > SIZE_CONFIG_DESC)
                            SetupLen = SIZE_CONFIG_DESC;
                        pDescr = (PUINT8)SS_ConfigDescriptor;
                        break;
                    case USB_DESCR_TYP_BOS:
                        if(SetupLen > SIZE_BOS_DESC)
                            SetupLen = SIZE_BOS_DESC;
                        pDescr = (PUINT8)BOSDescriptor;
                        break;
                    case USB_DESCR_TYP_STRING:
                        switch(UsbSetupBuf->wValueL)
                        {
                            case USB_DESCR_LANGID_STRING:
                                if(SetupLen > SIZE_STRING_LANGID)
                                    SetupLen = SIZE_STRING_LANGID;
                                pDescr = (PUINT8)StringLangID;
                                break;
                            case USB_DESCR_VENDOR_STRING:
                                if(SetupLen > SIZE_STRING_VENDOR)
                                    SetupLen = SIZE_STRING_VENDOR;
                                pDescr = (PUINT8)StringVendor;
                                break;
                            case USB_DESCR_PRODUCT_STRING:
                                if(SetupLen > SIZE_STRING_PRODUCT)
                                    SetupLen = SIZE_STRING_PRODUCT;
                                pDescr = (PUINT8)StringProduct;
                                break;
                            case USB_DESCR_SERIAL_STRING:
                                if(SetupLen > SIZE_STRING_SERIAL)
                                    SetupLen = SIZE_STRING_SERIAL;
                                pDescr = (PUINT8)StringSerial;
                                break;
                            case USB_DESCR_OS_STRING:
                                if(SetupLen > SIZE_STRING_OS)
                                    SetupLen = SIZE_STRING_OS;
                                pDescr = (PUINT8)OSStringDescriptor;
                                break;
                            default:
                                len = USB_DESCR_UNSUPPORTED;
                                SetupReqCode = INVALID_REQ_CODE;
                                break;
                        }
                        break;
                    default:
                        len = USB_DESCR_UNSUPPORTED;
                        SetupReqCode = INVALID_REQ_CODE;
                        break;
                }
                if(len != USB_DESCR_UNSUPPORTED)
                {
                    len = SetupLen >= ENDP0_MAXPACK ? ENDP0_MAXPACK : SetupLen;
                    memcpy(endp0RTbuff, pDescr, len);
                    SetupLen -= len;
                    pDescr += len;
                }
                break;

            case USB_SET_ADDRESS:
                SetupLen = UsbSetupBuf->wValueL;
                break;

            case 0x31:                       /* SET_ISOCH_DELAY */
                SetupLen = UsbSetupBuf->wValueL;
                break;

            case 0x30:                       /* SET_SEL */
                break;

            case USB_SET_CONFIGURATION:
                /* (Re)arm all data endpoints — also recovers after hot reset */
                USB30_OUT_Set(ENDP_3, ACK, DEF_ENDP3_OUT_BURST_LEVEL);
                USB30_IN_Set(ENDP_2, ENABLE, NRDY, DEF_ENDP2_IN_BURST_LEVEL, 0);
                USB30_IN_Set(ENDP_1, DISABLE, NRDY, 0, 0);
                USB30_IN_Set(ENDP_4, ENABLE, NRDY, DEF_ENDP4_BURST_LEVEL, 0);
                USBSS->UEP4_RX_DMA = (UINT32)(UINT8 *)endp4Rxbuff;
                USB30_OUT_Set(ENDP_4, ACK, DEF_ENDP4_BURST_LEVEL);
                CDC_Variable_Clear();
                DownloadPoint4_Busy = 1;
#if DEBUG_OVER_USB
                USB30_IN_Set(ENDP_5, DISABLE, NRDY, 0, 0);
                USB30_IN_Set(ENDP_6, ENABLE, NRDY, DEF_ENDP6_BURST_LEVEL, 0);
                USBSS->UEP6_RX_DMA = (UINT32)(UINT8 *)endp6Rxbuff;
                USB30_OUT_Set(ENDP_6, ACK, DEF_ENDP6_BURST_LEVEL);
                debug_cdc_clear();
#endif
                break;

            case USB_GET_STATUS:
                len = 2;
                endp0RTbuff[0] = 0x01;
                endp0RTbuff[1] = 0x00;
                SetupLen = 0;
                break;

            case USB_CLEAR_FEATURE:
                switch(UsbSetupBuf->wIndexL)
                {
                    case 0x82:               /* MSC IN  */
                        USB30_IN_ClearIT(ENDP_2);
                        /* If a command failed with a stalled data-IN phase (e.g.
                         * READ CAPACITY while the card is owned by the FPGA), the
                         * host clears the stall and then reads the CSW from this
                         * endpoint. Deliver that pending CSW now — otherwise the
                         * endpoint sits NRDY, usb-storage times out (DID_ERROR)
                         * and resets the device in a loop. */
                        if(Udisk_Transfer_Status & DEF_UDISK_CSW_UP_FLAG)
                            UDISK_Up_CSW();
                        else
                            USB30_IN_Set(ENDP_2, ENABLE, NRDY, DEF_ENDP2_IN_BURST_LEVEL, 0);
                        break;
                    case 0x03:               /* MSC OUT: re-arm for next CBW  */
                        USB30_OUT_ClearIT(ENDP_3);
                        USBSS->UEP3_RX_DMA = (UINT32)(UINT8 *)UDisk_Out_Buf;
                        USB30_OUT_Set(ENDP_3, ACK, DEF_ENDP3_OUT_BURST_LEVEL);
                        USB30_Send_ERDY(ENDP_3 | OUT, DEF_ENDP3_OUT_BURST_LEVEL);
                        break;
                    case 0x81:               /* CDC notify */
                        USB30_IN_ClearIT(ENDP_1);
                        USB30_IN_Set(ENDP_1, DISABLE, NRDY, 0, 0);
                        break;
                    case 0x84:               /* CDC IN  */
                        USB30_IN_ClearIT(ENDP_4);
                        USB30_IN_Set(ENDP_4, ENABLE, NRDY, DEF_ENDP4_BURST_LEVEL, 0);
                        UploadPoint4_Busy = 0;
                        break;
                    case 0x04:               /* CDC OUT */
                        USB30_OUT_ClearIT(ENDP_4);
                        USBSS->UEP4_RX_DMA = (UINT32)(UINT8 *)endp4Rxbuff;
                        USB30_OUT_Set(ENDP_4, ACK, DEF_ENDP4_BURST_LEVEL);
                        USB30_Send_ERDY(ENDP_4 | OUT, DEF_ENDP4_BURST_LEVEL);
                        USBByteCount = 0;
                        USBBufOutPoint = 0;
                        DownloadPoint4_Busy = 1;
                        break;
#if DEBUG_OVER_USB
                    case 0x85:               /* debug notify */
                        USB30_IN_ClearIT(ENDP_5);
                        USB30_IN_Set(ENDP_5, DISABLE, NRDY, 0, 0);
                        break;
                    case 0x86:               /* debug IN  */
                        USB30_IN_ClearIT(ENDP_6);
                        USB30_IN_Set(ENDP_6, ENABLE, NRDY, DEF_ENDP6_BURST_LEVEL, 0);
                        DebugUpload_Busy = 0;
                        break;
                    case 0x06:               /* debug OUT */
                        USB30_OUT_ClearIT(ENDP_6);
                        USBSS->UEP6_RX_DMA = (UINT32)(UINT8 *)endp6Rxbuff;
                        USB30_OUT_Set(ENDP_6, ACK, DEF_ENDP6_BURST_LEVEL);
                        USB30_Send_ERDY(ENDP_6 | OUT, DEF_ENDP6_BURST_LEVEL);
                        break;
#endif
                    default:
                        SetupLen = USB_DESCR_UNSUPPORTED;
                        break;
                }
                break;

            case USB_SET_FEATURE:
                break;

            case USB_SET_INTERFACE:
                /* All interfaces have a single alt-setting: accept silently. */
                break;

            default:
                len = USB_DESCR_UNSUPPORTED;
                SetupReqCode = INVALID_REQ_CODE;
                break;
        }
    }
    return len;
}

/*******************************************************************************
 * @fn      EP0_IN_Callback
 ******************************************************************************/
UINT16 EP0_IN_Callback(void)
{
    UINT16 len = 0;
    switch(SetupReqCode)
    {
        case USB_GET_DESCRIPTOR:
            len = SetupLen >= ENDP0_MAXPACK ? ENDP0_MAXPACK : SetupLen;
            memcpy(endp0RTbuff, pDescr, len);
            SetupLen -= len;
            pDescr += len;
            break;
    }
    return len;
}

/*******************************************************************************
 * @fn      EP0_OUT_Callback
 *
 * @brief   EP0 OUT data: only CDC SET_LINE_CODING carries one (7 bytes).
 ******************************************************************************/
UINT16 EP0_OUT_Callback(void)
{
    if(SetupReqCode == CDC_SET_LINE_CODING && SetupReqIface == 1)
    {
        /* Bridge port only: apply the host's baud to the real FPGA UART.
         * The debug port (IF3) accepts line coding but drives no UART. */
        UINT32 baudrate;

        baudrate = endp0RTbuff[0];
        baudrate += ((UINT32)endp0RTbuff[1] << 8);
        baudrate += ((UINT32)endp0RTbuff[2] << 16);
        baudrate += ((UINT32)endp0RTbuff[3] << 24);

        cdc_line_coding.dwDTERate   = baudrate;
        cdc_line_coding.bCharFormat = endp0RTbuff[4];
        cdc_line_coding.bParityType = endp0RTbuff[5];
        cdc_line_coding.bDataBits   = endp0RTbuff[6];

        CDC_Uart_Init(baudrate);
        PRINT("CDC: line coding %ld baud\n", baudrate);
        while(!(R8_UART1_LSR & RB_LSR_TX_ALL_EMP));
    }
    return 0;
}

/*******************************************************************************
 * @fn      USB30_Setup_Status
 ******************************************************************************/
void USB30_Setup_Status(void)
{
    switch(SetupReqCode)
    {
        case USB_SET_ADDRESS:
            USB30_Device_Setaddress(SetupLen);
            break;
        case 0x31:
            break;
    }
}

/*******************************************************************************
 * @fn      USBSS_IRQHandler
 ******************************************************************************/
void USBSS_IRQHandler(void)
{
    USB30_IRQHandler();
}

/*******************************************************************************
 * @fn      TMR0_IRQHandler
 *
 * @brief   USB3 link-training timeout -> fall back to the USB2 device stack.
 *          NOTE: the USB2 fallback exposes the MSC function only (no CDC).
 ******************************************************************************/
void TMR0_IRQHandler()
{
    R8_TMR0_INT_FLAG = RB_TMR_IF_CYC_END;

    if(Link_Sta == LINK_STA_1)
    {
        Link_Sta = 0;
        PFIC_DisableIRQ(USBSS_IRQn);
        PFIC_DisableIRQ(LINK_IRQn);
        USB30D_init(DISABLE);
        PRINT("USB3.0 disable\n");
        return;
    }
    if(Link_Sta != LINK_STA_3)
    {
        PFIC_DisableIRQ(USBSS_IRQn);
        PFIC_DisableIRQ(LINK_IRQn);
        USB30D_init(DISABLE);
        R32_USB_CONTROL = 0;
        PFIC_EnableIRQ(USBHS_IRQn);
        USB20_Device_Init(ENABLE);
        PRINT("USB2.0 fallback (MSC only)\n");
    }
    Link_Sta = LINK_STA_1;
    R8_TMR0_INTER_EN = 0;
    PFIC_DisableIRQ(TMR0_IRQn);
    R8_TMR0_CTRL_MOD = RB_TMR_ALL_CLEAR;
}

/*******************************************************************************
 * @fn      LINK_IRQHandler
 *
 * @brief   USB3.0 link state machine — byte-for-byte the MSC example's.
 ******************************************************************************/
void LINK_IRQHandler()
{
    if(USBSS->LINK_INT_FLAG & LINK_Ux_EXIT_FLAG)
    {
        USBSS->LINK_CFG = CFG_EQ_EN | TX_SWING | DEEMPH_CFG | TERM_EN;
        USB30_Switch_Powermode(POWER_MODE_0);
        USBSS->LINK_INT_FLAG = LINK_Ux_EXIT_FLAG;
    }
    if(USBSS->LINK_INT_FLAG & LINK_RDY_FLAG)
    {
        USBSS->LINK_INT_FLAG = LINK_RDY_FLAG;
        if(Tx_Lmp_Port)
        {
            USBSS->LMP_TX_DATA0 = LINK_SPEED | PORT_CAP | LMP_HP;
            USBSS->LMP_TX_DATA1 = UP_STREAM | NUM_HP_BUF;
            USBSS->LMP_TX_DATA2 = 0x0;
            Tx_Lmp_Port = 0;
        }
        /* USB3.0 link established */
        Link_Sta = LINK_STA_3;
        PFIC_DisableIRQ(TMR0_IRQn);
        R8_TMR0_CTRL_MOD = RB_TMR_ALL_CLEAR;
        R8_TMR0_INTER_EN = 0;
        PFIC_DisableIRQ(USBHS_IRQn);
        USB20_Device_Init(DISABLE);
    }

    if(USBSS->LINK_INT_FLAG & LINK_INACT_FLAG)
    {
        Link_Sta = 0;
        PFIC_EnableIRQ(USBSS_IRQn);
        PFIC_EnableIRQ(LINK_IRQn);
        PFIC_EnableIRQ(TMR0_IRQn);
        R8_TMR0_INTER_EN = RB_TMR_IE_CYC_END;
        TMR0_TimerInit(67000000);
        USB30D_init(ENABLE);
        USBSS->LINK_INT_FLAG = LINK_INACT_FLAG;
        USB30_Switch_Powermode(POWER_MODE_2);
    }
    if(USBSS->LINK_INT_FLAG & LINK_DISABLE_FLAG)
    {
        USBSS->LINK_INT_FLAG = LINK_DISABLE_FLAG;
        Link_Sta = LINK_STA_1;
        USB30D_init(DISABLE);
        PFIC_DisableIRQ(USBSS_IRQn);
        R8_TMR0_CTRL_MOD = RB_TMR_ALL_CLEAR;
        R8_TMR0_INTER_EN = 0;
        PFIC_DisableIRQ(TMR0_IRQn);
        PFIC_EnableIRQ(USBHS_IRQn);
        USB20_Device_Init(ENABLE);
    }
    if(USBSS->LINK_INT_FLAG & LINK_RX_DET_FLAG)
    {
        USBSS->LINK_INT_FLAG = LINK_RX_DET_FLAG;
        USB30_Switch_Powermode(POWER_MODE_2);
    }
    if(USBSS->LINK_INT_FLAG & TERM_PRESENT_FLAG)
    {
        USBSS->LINK_INT_FLAG = TERM_PRESENT_FLAG;
        if(USBSS->LINK_STATUS & LINK_PRESENT)
        {
            USB30_Switch_Powermode(POWER_MODE_2);
            USBSS->LINK_CTRL |= POLLING_EN;
        }
        else
        {
            USBSS->LINK_INT_CTRL = 0;
            mDelayuS(2);
        }
    }
    if(USBSS->LINK_INT_FLAG & LINK_TXEQ_FLAG)
    {
        Tx_Lmp_Port = 1;
        USBSS->LINK_INT_FLAG = LINK_TXEQ_FLAG;
        USB30_Switch_Powermode(POWER_MODE_0);
    }
    if(USBSS->LINK_INT_FLAG & WARM_RESET_FLAG)
    {
        USBSS->LINK_INT_FLAG = WARM_RESET_FLAG;
        USB30_Switch_Powermode(POWER_MODE_2);
        USBSS->LINK_CTRL |= TX_WARM_RESET;
        while(USBSS->LINK_STATUS & RX_WARM_RESET)
            ;
        USBSS->LINK_CTRL &= ~TX_WARM_RESET;
        mDelayuS(2);
        USB30_Device_Setaddress(0);
    }
    if(USBSS->LINK_INT_FLAG & HOT_RESET_FLAG)
    {
        USBSS->USB_CONTROL |= 1 << 31;
        USBSS->LINK_INT_FLAG = HOT_RESET_FLAG;
        USBSS->UEP0_TX_CTRL = 0;
        USB30_IN_Set(ENDP_1, DISABLE, NRDY, 0, 0);
        USB30_IN_Set(ENDP_2, DISABLE, NRDY, 0, 0);
        USB30_IN_Set(ENDP_3, DISABLE, NRDY, 0, 0);
        USB30_IN_Set(ENDP_4, DISABLE, NRDY, 0, 0);
        USB30_IN_Set(ENDP_5, DISABLE, NRDY, 0, 0);
        USB30_IN_Set(ENDP_6, DISABLE, NRDY, 0, 0);
        USB30_IN_Set(ENDP_7, DISABLE, NRDY, 0, 0);
        USB30_OUT_Set(ENDP_1, NRDY, 0);
        USB30_OUT_Set(ENDP_2, NRDY, 0);
        USB30_OUT_Set(ENDP_3, NRDY, 0);
        USB30_OUT_Set(ENDP_4, NRDY, 0);
        USB30_OUT_Set(ENDP_5, NRDY, 0);
        USB30_OUT_Set(ENDP_6, NRDY, 0);
        USB30_OUT_Set(ENDP_7, NRDY, 0);

        USB30_Device_Setaddress(0);
        USBSS->LINK_CTRL &= ~TX_HOT_RESET;
    }
    if(USBSS->LINK_INT_FLAG & LINK_GO_U1_FLAG)
    {
        USB30_Switch_Powermode(POWER_MODE_1);
        USBSS->LINK_INT_FLAG = LINK_GO_U1_FLAG;
    }
    if(USBSS->LINK_INT_FLAG & LINK_GO_U2_FLAG)
    {
        USB30_Switch_Powermode(POWER_MODE_2);
        USBSS->LINK_INT_FLAG = LINK_GO_U2_FLAG;
    }
    if(USBSS->LINK_INT_FLAG & LINK_GO_U3_FLAG)
    {
        USB30_Switch_Powermode(POWER_MODE_2);
        USBSS->LINK_INT_FLAG = LINK_GO_U3_FLAG;
    }
}

/***************Endpoint IN Transaction Processing*******************/

/* EP1 IN: CDC notification — declared but never used (no serial-state
 * events are generated; hosts tolerate an idle notify endpoint). */
void EP1_IN_Callback(void)
{
}

/* EP2 IN: MSC data to host (unchanged MSC example path). */
void EP2_IN_Callback(void)
{
    USB30_IN_ClearIT(ENDP_2);
    USB30_IN_Set(ENDP_2, ENABLE, NRDY, 0, 0);
    UDISK_In_EP_Deal();
}

void EP3_IN_Callback(void)
{
}

/* EP4 IN: CDC data to host sent — free the bridge for the next chunk. */
void EP4_IN_Callback(void)
{
    USB30_IN_ClearIT(ENDP_4);
    USB30_IN_Set(ENDP_4, ENABLE, NRDY, 0, 0);
    UploadPoint4_Busy = 0;
}

void EP5_IN_Callback(void)
{
}

/* EP6 IN: a debug-log chunk finished sending — free it for the next poll. */
void EP6_IN_Callback(void)
{
    USB30_IN_ClearIT(ENDP_6);
    USB30_IN_Set(ENDP_6, ENABLE, NRDY, 0, 0);
#if DEBUG_OVER_USB
    DebugUpload_Busy = 0;
#endif
}

void EP7_IN_Callback(void)
{
}

/***************Endpoint OUT Transaction Processing*******************/

void EP1_OUT_Callback(void)
{
}
void EP2_OUT_Callback(void)
{
}

/* EP3 OUT: MSC CBW / data from host (unchanged MSC example path). */
void EP3_OUT_Callback(void)
{
    UINT16  len;
    uint8_t nump;

    USB30_OUT_Status(ENDP_3, &nump, &len, NULL);
    USB30_OUT_ClearIT(ENDP_3);

    USBSS->UEP3_RX_DMA = (UINT32)(UINT8 *)UDisk_Out_Buf;
    USB30_OUT_Set(ENDP_3, NRDY, 0);

    UDISK_Out_EP_Deal(UDisk_Out_Buf, len);
    if(UDISK_OutPackflag == 0)
    {
        USB30_OUT_Set(ENDP_3, ACK, DEF_ENDP3_OUT_BURST_LEVEL);
        USB30_Send_ERDY(ENDP_3 | OUT, DEF_ENDP3_OUT_BURST_LEVEL);
    }
}

/* EP4 OUT: CDC data from host — handed to the UART pump in cdc_uart.c. */
void EP4_OUT_Callback(void)
{
    UINT16 rx_len;
    UINT8  nump;
    UINT8  status;

    USB30_OUT_Status(ENDP_4, &nump, &rx_len, &status);
    USBByteCount = rx_len;
    USB30_OUT_ClearIT(ENDP_4);
    USB30_OUT_Set(ENDP_4, NRDY, 0);

    DownloadPoint4_Busy = 0;
}

void EP5_OUT_Callback(void)
{
}

/* EP6 OUT: bytes typed on the debug port go to the console (console.c), re-arm. */
void EP6_OUT_Callback(void)
{
#if DEBUG_OVER_USB
    UINT16 rx_len;
    UINT8  nump, status;
    USB30_OUT_Status(ENDP_6, &nump, &rx_len, &status);
    console_rx(endp6Rxbuff, rx_len);
    USB30_OUT_ClearIT(ENDP_6);
    USBSS->UEP6_RX_DMA = (UINT32)(UINT8 *)endp6Rxbuff;
    USB30_OUT_Set(ENDP_6, ACK, DEF_ENDP6_BURST_LEVEL);
    USB30_Send_ERDY(ENDP_6 | OUT, DEF_ENDP6_BURST_LEVEL);
#endif
}
void EP7_OUT_Callback(void)
{
}

/*******************************************************************************
 * @fn      USB30_ITP_Callback
 ******************************************************************************/
void USB30_ITP_Callback(UINT32 ITPCounter)
{
}
