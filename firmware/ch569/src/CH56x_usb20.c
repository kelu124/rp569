/********************************** (C) COPYRIGHT *******************************
* File Name          : CH56x_usb20.c
* Author             : WCH
* Version            : V1.1
* Date               : 2023/02/16
*
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#include "CH56x_common.h"
#include "CH56x_usb20.h"
#include "CH56x_usb30.h"
#include "CH56xusb30_LIB.h"   /* exact case: Linux/GCC builds are case-sensitive */
#include "SW_UDISK.h"
#include "bridge_config.h"
#include "cdc_uart.h"
#include "console.h"
#include "debug_cdc.h"
#include "ownership.h"

/* Global Variable */
UINT16V  U20_EndpnMaxSize = 512;
UINT16V  SetupReqLen=0;            //Host request data length
UINT16V  SetupLen = 0;             //Data length actually sent or received in data phase
UINT32V seq_num = 0;
DevInfo_Typedef  g_devInfo;
static UINT8V SetupReqType = 0;    //Host request descriptor type
static UINT8V SetupReq = 0;        //Host request descriptor type
static UINT8V SetupIface = 0;      //Interface of the pending CDC SET_LINE_CODING
static PUINT8 pDescr;

__attribute__ ((aligned(16))) UINT8 vendor_buff[16]  __attribute__((section(".DMADATA")));
/* CDC notification endpoints (EP1, EP5): never sent, NAKed; DMA needs a buffer. */
__attribute__ ((aligned(16))) static UINT8 u20_notify_buff[16]  __attribute__((section(".DMADATA")));
/* Function declaration.
 * Plain interrupt attribute: upstream GCC ignores "WCH-Interrupt-fast". */
void USBHS_IRQHandler(void) __attribute__((interrupt));

/* High-speed composite, same layout as the SuperSpeed one (CH56x_usb30.c) so the host library finds the same
 * interfaces at either speed: IF0 = MSC (SD card), IAD(IF1 + IF2) = CDC bridged to the RP2350 link UART,
 * IAD(IF3 + IF4) = CDC debug console (DEBUG_OVER_USB). Same VID/PID, product string and UID serial. */
const UINT8 hs_device_descriptor[] =
{
    0x12,   // bLength
    0x01,   // DEVICE descriptor type
    0x00,   // bcdUSB 2.00
    0x02,
    0xEF,   // bDeviceClass: Miscellaneous
    0x02,   // bDeviceSubClass: Common Class
    0x01,   // bDeviceProtocol: Interface Association Descriptor
    0x40,   // max packet size 64B
    BRIDGE_USB_VID_L, BRIDGE_USB_VID_H,
    BRIDGE_USB_PID_L, BRIDGE_USB_PID_H,
    0x00,   //bcdDevice 0x0111
    0x11,
    0x01,   // manufacturer index string
    0x02,   // product index string
    0x03,   // serial number index string (chip UID)
    0x01    // number of configurations
};

const UINT8 hs_config_descriptor[] =
{
    /* Configuration Descriptor */
    0x09,                  // bLength
    0x02,                  // bDescriptorType (Configuration)
    HS_SIZE_CONFIG_DESC & 0xFF, (HS_SIZE_CONFIG_DESC >> 8) & 0xFF,   // wTotalLength
    BRIDGE_NUM_INTERFACES, // bNumInterfaces (3, or 5 with the debug CDC)
    0x01,                  // bConfigurationValue
    0x00,                  // iConfiguration (String Index)
    0xC0,                  // bmAttributes Self Powered
    0x32,                  // bMaxPower 100mA

    /* ---------------- Interface 0: Mass Storage (SD card) --------------- */
    0x09, 0x04, 0x00, 0x00, 0x02, 0x08, 0x06, 0x50, 0x00,
    0x07, 0x05, 0x82, 0x02, 0x00, 0x02, 0x00,   // EP2 IN  bulk 512
    0x07, 0x05, 0x03, 0x02, 0x00, 0x02, 0x00,   // EP3 OUT bulk 512

    /* ------------- Interface Association: CDC (IF1 + IF2) --------------- */
    0x08, 0x0B, 0x01, 0x02, 0x02, 0x02, 0x01, 0x00,
    /* Interface 1: CDC Communications (control) */
    0x09, 0x04, 0x01, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,
    0x05, 0x24, 0x00, 0x10, 0x01,               // Header, bcdCDC 1.10
    0x05, 0x24, 0x01, 0x00, 0x02,               // Call Mgmt, data IF 2
    0x04, 0x24, 0x02, 0x02,                     // ACM, line coding + serial state
    0x05, 0x24, 0x06, 0x01, 0x02,               // Union, master IF1, slave IF2
    0x07, 0x05, 0x81, 0x03, 0x10, 0x00, 0x08,   // EP1 IN interrupt 16B, 16 ms
    /* Interface 2: CDC Data */
    0x09, 0x04, 0x02, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,
    0x07, 0x05, 0x84, 0x02, 0x00, 0x02, 0x00,   // EP4 IN  bulk 512
    0x07, 0x05, 0x04, 0x02, 0x00, 0x02, 0x00,   // EP4 OUT bulk 512

#if DEBUG_OVER_USB
    /* ------------- Interface Association: CDC debug console (IF3 + IF4) - */
    0x08, 0x0B, 0x03, 0x02, 0x02, 0x02, 0x01, 0x00,
    /* Interface 3: CDC Communications (debug control) */
    0x09, 0x04, 0x03, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,
    0x05, 0x24, 0x00, 0x10, 0x01,               // Header
    0x05, 0x24, 0x01, 0x00, 0x04,               // Call Mgmt, data IF 4
    0x04, 0x24, 0x02, 0x02,                     // ACM
    0x05, 0x24, 0x06, 0x03, 0x04,               // Union, master IF3, slave IF4
    0x07, 0x05, 0x85, 0x03, 0x10, 0x00, 0x08,   // EP5 IN interrupt 16B, 16 ms
    /* Interface 4: CDC Data (debug) */
    0x09, 0x04, 0x04, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,
    0x07, 0x05, 0x86, 0x02, 0x00, 0x02, 0x00,   // EP6 IN  bulk 512 (log + console replies)
    0x07, 0x05, 0x06, 0x02, 0x00, 0x02, 0x00    // EP6 OUT bulk 512 (console input)
#endif
};

/* Language Descriptor */
const UINT8 hs_string_descriptor0[] =
{
    0x04,   // this descriptor length
    0x03,   // descriptor type
    0x09,   // Language ID 0 low byte
    0x04    // Language ID 0 high byte
};

/* Manufacturer Descriptor */
const UINT8 hs_string_descriptor1[] =
{
    0x08,   // length of this descriptor
    0x03,
    'W',
    0x00,
    'C',
    0x00,
    'H',
    0x00
};

const UINT8 hs_bos_descriptor[] =
{
    0x05,   // length of this descriptor
    0x0f,   // CONFIGURATION (2)
    0x16,   // total length includes endpoint descriptors (should be 1 more than last address)
    0x00,   // total length high byte
    0x02,       // number of device cap

    0x07,
    0x10,   // DEVICE CAPABILITY type
    0x02,   // USB2.0 EXTENSION
    0x02,
    0x00,
    0x00,
    0x00,

    0x0a,   // length of this descriptor
    0x10,   // DEVICE CAPABILITY type
    0x03,   // superspeed usb device capability
    0x00,   //
    0x0e,   // ss/hs/fs
    0x00,
    0x01,   // the lowest speed is full speed
    0x0a,   // u1 exit latency is 10us
    0xff,   // u1 exit latency is 8us
    0x07
};

/* CDC interfaces that take line-coding requests: IF1 (RP2350 link), IF3 (debug console). */
static UINT8 u20_cdc_iface(UINT8 iface)
{
    return iface == 1 || (DEBUG_OVER_USB && iface == 3);
}

/* Endpoint control registers by number (only the endpoints this device uses). */
static volatile UINT8 *u20_tx_ctrl(UINT8 n)
{
    switch(n)
    {
        case 1: return &R8_UEP1_TX_CTRL;
        case 2: return &R8_UEP2_TX_CTRL;
        case 4: return &R8_UEP4_TX_CTRL;
#if DEBUG_OVER_USB
        case 5: return &R8_UEP5_TX_CTRL;
        case 6: return &R8_UEP6_TX_CTRL;
#endif
    }
    return 0;
}

static volatile UINT8 *u20_rx_ctrl(UINT8 n)
{
    switch(n)
    {
        case 3: return &R8_UEP3_RX_CTRL;
        case 4: return &R8_UEP4_RX_CTRL;
#if DEBUG_OVER_USB
        case 6: return &R8_UEP6_RX_CTRL;
#endif
    }
    return 0;
}

/* CLEAR_FEATURE(ENDPOINT_HALT): reset the toggle to DATA0, keep auto-toggle, and restart the function's state on
 * that endpoint (MSC: send a pending CSW, the host expects it after clearing a bulk-IN stall). 0 = ok. */
static UINT8 u20_clear_halt(UINT8 ep)
{
    UINT8 n = ep & 0x0F;
    volatile UINT8 *ctrl = (ep & 0x80) ? u20_tx_ctrl(n) : u20_rx_ctrl(n);
    if(!ctrl)
        return 1;
    if(ep & 0x80)
    {
        *ctrl = UEP_T_RES_NAK | RB_UEP_T_TOG_0 | RB_UEP_T_AUTOTOG;
        if(n == 2)
        {
            R16_UEP2_T_LEN = 0;
            if(Udisk_Transfer_Status & DEF_UDISK_CSW_UP_FLAG)
                UDISK_Up_CSW();
        }
        else if(n == 4)
            UploadPoint4_Busy = 0;
#if DEBUG_OVER_USB
        else if(n == 6)
            DebugUpload_Busy = 0;
#endif
    }
    else
    {
        if(n == 3)
            R32_UEP3_RX_DMA = (UINT32)(UINT8 *)UDisk_Out_Buf;
        else if(n == 4)
        {
            R32_UEP4_RX_DMA = (UINT32)(UINT8 *)endp4Rxbuff;
            USBByteCount = 0;
            USBBufOutPoint = 0;
            DownloadPoint4_Busy = 1;
        }
#if DEBUG_OVER_USB
        else if(n == 6)
            R32_UEP6_RX_DMA = (UINT32)(UINT8 *)endp6Rxbuff;
#endif
        *ctrl = UEP_R_RES_ACK | RB_UEP_R_TOG_0 | RB_UEP_R_AUTOTOG;
    }
    return 0;
}

static UINT8 u20_set_halt(UINT8 ep)
{
    UINT8 n = ep & 0x0F;
    volatile UINT8 *ctrl = (ep & 0x80) ? u20_tx_ctrl(n) : u20_rx_ctrl(n);
    if(!ctrl)
        return 1;
    if(ep & 0x80)
        *ctrl = (*ctrl & ~RB_UEP_TRES_MASK) | UEP_T_RES_STALL;
    else
        *ctrl = (*ctrl & ~RB_UEP_RRES_MASK) | UEP_R_RES_STALL;
    return 0;
}

/* 1 = halted, 0 = not, 0xFF = no such endpoint */
static UINT8 u20_halted(UINT8 ep)
{
    UINT8 n = ep & 0x0F;
    if(n == 0)
        return 0;
    volatile UINT8 *ctrl = (ep & 0x80) ? u20_tx_ctrl(n) : u20_rx_ctrl(n);
    if(!ctrl)
        return 0xFF;
    if(ep & 0x80)
        return (*ctrl & RB_UEP_TRES_MASK) == UEP_T_RES_STALL;
    return (*ctrl & RB_UEP_RRES_MASK) == UEP_R_RES_STALL;
}

/*******************************************************************************
 * @fn     USB20_Endp_Init
 *
 * @brief  USB2.0 Endpoint initialization
 *
 * @return  None
 */
void USB20_Endp_Init ( )	// USBHS device endpoint initial
{
    R8_UEP4_1_MOD = RB_UEP1_TX_EN | RB_UEP4_TX_EN | RB_UEP4_RX_EN;
    R8_UEP2_3_MOD = RB_UEP2_TX_EN | RB_UEP3_RX_EN;
#if DEBUG_OVER_USB
    R8_UEP5_6_MOD = RB_UEP5_TX_EN | RB_UEP6_TX_EN | RB_UEP6_RX_EN;
#endif

    R16_UEP0_MAX_LEN = 64;
    R16_UEP1_MAX_LEN = 16;
    R16_UEP2_MAX_LEN = 512;
    R16_UEP3_MAX_LEN = 512;
    R16_UEP4_MAX_LEN = 512;

    R32_UEP0_RT_DMA = (UINT32)(UINT8 *)endp0RTbuff;

	R32_UEP2_TX_DMA = (UINT32)(UINT8 *)UDisk_In_Buf;
	R32_UEP3_RX_DMA = (UINT32)(UINT8 *)UDisk_Out_Buf;

	R16_UEP0_T_LEN = 0;
	R8_UEP0_TX_CTRL = UEP_T_RES_NAK;
	R8_UEP0_RX_CTRL = 0;

    R16_UEP2_T_LEN = U20_MAXPACKET_LEN;
    R8_UEP2_TX_CTRL = UEP_T_RES_NAK | RB_UEP_T_AUTOTOG;
    R8_UEP3_RX_CTRL = UEP_R_RES_ACK | RB_UEP_R_AUTOTOG;

    /* CDC #1 (RP2350 link): EP1 notify never sends, EP4 IN idle, EP4 OUT armed */
    CDC_Variable_Clear();
    R32_UEP1_TX_DMA = (UINT32)(UINT8 *)u20_notify_buff;
    R16_UEP1_T_LEN = 0;
    R8_UEP1_TX_CTRL = UEP_T_RES_NAK | RB_UEP_T_AUTOTOG;
    R32_UEP4_TX_DMA = (UINT32)(UINT8 *)endp4Txbuff;
    R32_UEP4_RX_DMA = (UINT32)(UINT8 *)endp4Rxbuff;
    R16_UEP4_T_LEN = 0;
    R8_UEP4_TX_CTRL = UEP_T_RES_NAK | RB_UEP_T_AUTOTOG;
    R8_UEP4_RX_CTRL = UEP_R_RES_ACK | RB_UEP_R_AUTOTOG;
    DownloadPoint4_Busy = 1;

#if DEBUG_OVER_USB
    /* CDC #2 (debug console): EP5 notify never sends, EP6 IN idle, EP6 OUT armed */
    R16_UEP5_MAX_LEN = 16;
    R16_UEP6_MAX_LEN = 512;
    R32_UEP5_TX_DMA = (UINT32)(UINT8 *)u20_notify_buff;
    R16_UEP5_T_LEN = 0;
    R8_UEP5_TX_CTRL = UEP_T_RES_NAK | RB_UEP_T_AUTOTOG;
    R32_UEP6_TX_DMA = (UINT32)(UINT8 *)endp6Txbuff;
    R32_UEP6_RX_DMA = (UINT32)(UINT8 *)endp6Rxbuff;
    R16_UEP6_T_LEN = 0;
    R8_UEP6_TX_CTRL = UEP_T_RES_NAK | RB_UEP_T_AUTOTOG;
    R8_UEP6_RX_CTRL = UEP_R_RES_ACK | RB_UEP_R_AUTOTOG;
    debug_cdc_clear();
#endif
}

/*******************************************************************************
 * @fn      USB20_Device_Init
 *
 * @brief   USB2.0 Device initialization
 *
 * @param   sta - ENABLE / DISABLE
 *
 * @return   None
 */
void USB20_Device_Init ( FunctionalState sta )  // USBHS device initial
{
    UINT16 i;
    UINT32 *p;
    if(sta)
    {
        R8_USB_CTRL = 0;
        R8_USB_CTRL =  UCST_HS | RB_DEV_PU_EN | RB_USB_INT_BUSY |RB_USB_DMA_EN;
        R8_USB_INT_EN = RB_USB_IE_SETUPACT | RB_USB_IE_TRANS | RB_USB_IE_SUSPEND  |RB_USB_IE_BUSRST ;
        USB20_Endp_Init();
    }
    else
    {
        R8_USB_CTRL = RB_USB_CLR_ALL | RB_USB_RESET_SIE;
    }
}

/*******************************************************************************
 * @fn      USB20_Device_setaddress
 *
 * @brief   USB2.0 Set device address
 *
 * @param   address
 *
 * @return  None
 **/
void USB20_Device_Setaddress( UINT32 address )
{
    R8_USB_DEV_AD = address; // SET ADDRESS
}

/*******************************************************************************
 * @fn       U20_NonStandard_Request_Deal
 *
 * @brief   USB2.0 Interrupt Handler.
 *
 * @return   None
 */
UINT16 U20_NonStandard_Request_Deal()
{
  UINT16 len = 0;
  UINT8 iface = UsbSetupBuf->wIndexL;

  SetupLen = 0;
  if((SetupReqType & USB_REQ_TYP_MASK) == USB_REQ_TYP_VENDOR)
  {
      /* Out-of-band SD ownership control, as on USB3 (bridge_config.h) */
      switch(SetupReq)
      {
        case BRIDGE_REQ_SD_OWNER:
            switch(UsbSetupBuf->wValueL)
            {
                case 0:  ownership_request(OWN_REQ_RELEASE); break;
                case 1:  ownership_request(OWN_REQ_CLAIM);   break;
                default: ownership_request(OWN_REQ_TOGGLE);  break;
            }
            return 0;
        case BRIDGE_REQ_SD_STATUS:
            endp0RTbuff[0] = (own_ch569_owns ? 0x01 : 0x00) | (own_card_ready ? 0x02 : 0x00);
            return 1;
        default:
            return USB_DESCR_UNSUPPORTED;
      }
  }

  /* Class requests: MSC on IF0, CDC-ACM on IF1 (link) and IF3 (console) */
  switch( SetupReq )
  {
    case CMD_UDISK_GET_MAX_LUN:
        if(iface != 0)
            return USB_DESCR_UNSUPPORTED;
        endp0RTbuff[ 0 ] = 0;                     /* single LUN */
        len = 1;
        break;
    case CMD_UDISK_RESET:
        if(iface != 0)
            return USB_DESCR_UNSUPPORTED;
        Udisk_Sense_Key = 0x00;
        Udisk_Sense_ASC = 0x00;
        Udisk_CSW_Status = 0x00;
        Udisk_Transfer_Status = 0x00;
        UDISK_Transfer_DataLen = 0x00;
        break;
    case CDC_SET_LINE_CODING:                     /* 7 bytes follow on EP0 OUT */
        if(!u20_cdc_iface(iface))
            return USB_DESCR_UNSUPPORTED;
        SetupIface = iface;
        SetupLen = SetupReqLen;
        break;
    case CDC_GET_LINE_CODING:
        if(!u20_cdc_iface(iface))
            return USB_DESCR_UNSUPPORTED;
        memcpy(endp0RTbuff, &cdc_line_coding, 7);
        len = SetupReqLen < 7 ? SetupReqLen : 7;
        break;
    case CDC_SET_CONTROL_LINE_STATE:              /* DTR/RTS: accept */
        if(!u20_cdc_iface(iface))
            return USB_DESCR_UNSUPPORTED;
        break;
    default:
        len = USB_DESCR_UNSUPPORTED;              /* stall unknown requests */
        break;
  }
  return len;
}

/* EP0 OUT data stage complete: CDC line coding. Only the link port (IF1) has a real UART behind it, and only
 * LINK_FOLLOW_HOST_BAUD lets the host change its rate (bridge_config.h). */
static void u20_ep0_out_done(void)
{
    if(SetupReq == CDC_SET_LINE_CODING && SetupIface == 1)
    {
        UINT32 baudrate = endp0RTbuff[0] | ((UINT32)endp0RTbuff[1] << 8) |
                          ((UINT32)endp0RTbuff[2] << 16) | ((UINT32)endp0RTbuff[3] << 24);
#if LINK_FOLLOW_HOST_BAUD
        cdc_line_coding.bCharFormat = endp0RTbuff[4];
        cdc_line_coding.bParityType = endp0RTbuff[5];
        cdc_line_coding.bDataBits   = endp0RTbuff[6];
        if(baudrate)
            CDC_Uart_Init(baudrate);              /* sets cdc_line_coding.dwDTERate */
#else
        (void)baudrate;                           /* UART1 stays at LINK_UART_DEFAULT_BAUD */
#endif
    }
}

/*******************************************************************************
 * @fn       U20_Standard_Request_Deal
 *
 * @brief    USB2.0 standard request deal
 *
 * @return   None
 */
UINT16 U20_Standard_Request_Deal()
{
  UINT16 len = 0;
  UINT8 endp_dir;
  SetupLen = 0;
  endp_dir = UsbSetupBuf->bRequestType & 0x80;
  switch( SetupReq )
  {
    case USB_GET_DESCRIPTOR:
    {
        switch( UsbSetupBuf->wValueH )
        {
            case USB_DESCR_TYP_DEVICE:
                pDescr = (UINT8 *)hs_device_descriptor;
                SetupLen = ( SetupReqLen > sizeof(hs_device_descriptor) )? sizeof(hs_device_descriptor):SetupReqLen;
                break;
            case USB_DESCR_TYP_CONFIG:
                pDescr = (UINT8 *)hs_config_descriptor;
                SetupLen = ( SetupReqLen > sizeof(hs_config_descriptor) )? sizeof(hs_config_descriptor):SetupReqLen;
                break;
            case USB_DESCR_TYP_STRING:
                switch( UsbSetupBuf->wValueL )
                {
                    case USB_DESCR_LANGID_STRING:

                        pDescr = (UINT8 *)hs_string_descriptor0;
                        SetupLen = ( SetupReqLen > sizeof(hs_string_descriptor0) )? sizeof(hs_string_descriptor0):SetupReqLen;
                        break;
                    case USB_DESCR_VENDOR_STRING:
                        pDescr = (UINT8 *)hs_string_descriptor1;
                        SetupLen = ( SetupReqLen > sizeof(hs_string_descriptor1) )? sizeof(hs_string_descriptor1):SetupReqLen;
                        break;
                    case USB_DESCR_PRODUCT_STRING:      /* "rp569 CH569", shared with USB3 */
                        pDescr = (UINT8 *)StringProduct;
                        SetupLen = ( SetupReqLen > StringProduct[0] )? StringProduct[0]:SetupReqLen;
                        break;
                    case USB_DESCR_SERIAL_STRING:       /* chip UID, shared with USB3 */
                        pDescr = (UINT8 *)StringSerial;
                        SetupLen = ( SetupReqLen > StringSerial[0] )? StringSerial[0]:SetupReqLen;
                        break;
                    default:
                        SetupLen = USB_DESCR_UNSUPPORTED;
                        break;
                }
                break;
            case USB_DESCR_TYP_BOS:
                 pDescr =(UINT8 *) hs_bos_descriptor;
                 SetupLen = ( SetupReqLen > sizeof(hs_bos_descriptor) )? sizeof(hs_bos_descriptor):SetupReqLen;
                 break;
            default :
                SetupLen = USB_DESCR_UNSUPPORTED;
                break;
        }
    }
        break;
    case USB_SET_ADDRESS:
        g_devInfo.dev_addr = UsbSetupBuf->wValueL;
        break;
    case USB_GET_CONFIGURATION:
        endp0RTbuff[ 0 ] = g_devInfo.dev_config_value;
        SetupLen = 1;
        break;

    case USB_SET_CONFIGURATION:
        if( (R8_USB_SPD_TYPE & RB_USBSPEED_MASK)  == UST_FS )
        {
            U20_EndpnMaxSize = 64;
        }
        else if( (R8_USB_SPD_TYPE & RB_USBSPEED_MASK) == UST_LS )
        {
            U20_EndpnMaxSize = 8;
        }
        g_devInfo.dev_config_value = UsbSetupBuf->wValueL;
        g_devInfo.dev_enum_status = 0x01;
        break;
    case USB_CLEAR_FEATURE:
        if( ( UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP )
        {
            if( u20_clear_halt( UsbSetupBuf->wIndexL ) )
                SetupLen = USB_DESCR_UNSUPPORTED;
        }
        else if( ( UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_DEVICE )
        {
            if( (  UsbSetupBuf->wValueL ) == 1 )
            {
                g_devInfo.dev_sleep_status &= ~0x01;
            }
        }
        else
        {
            SetupLen = USB_DESCR_UNSUPPORTED;
        }
        break;
    case USB_SET_FEATURE:
        if( ( UsbSetupBuf->bRequestType & 0x1F ) == 0x00 )
        {
            if( UsbSetupBuf->wValueL == 0x01 && ( hs_config_descriptor[ 7 ] & 0x20 ) )
                g_devInfo.dev_sleep_status = 0x01;
            else
                SetupLen = USB_DESCR_UNSUPPORTED;
        }
        else if( ( UsbSetupBuf->bRequestType & 0x1F ) == 0x02 && UsbSetupBuf->wValueL == 0x00 )
        {
            if( u20_set_halt( UsbSetupBuf->wIndexL ) )
                SetupLen = USB_DESCR_UNSUPPORTED;
        }
        else
        {
            SetupLen = USB_DESCR_UNSUPPORTED;
        }
        break;
    case USB_GET_INTERFACE:
        break;
    case USB_SET_INTERFACE:
        break;
    case USB_GET_STATUS:
        endp0RTbuff[ 0 ] = 0x00;
        endp0RTbuff[ 1 ] = 0x00;
        SetupLen = 2;
        if( ( UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP )
        {
            UINT8 h = u20_halted( UsbSetupBuf->wIndexL );
            if( h == 0xFF )
                SetupLen = USB_DESCR_UNSUPPORTED;
            else
                endp0RTbuff[ 0 ] = h;
        }
        break;
    default:
        SetupLen = USB_DESCR_UNSUPPORTED;
        break;
   }

  if( (SetupLen != USB_DESCR_UNSUPPORTED) && (SetupLen != 0))
  {
      len = ( SetupLen >= U20_UEP0_MAXSIZE ) ? U20_UEP0_MAXSIZE : SetupLen;
      if(endp_dir)
      {
          memcpy( endp0RTbuff, pDescr, len );
          pDescr += len;
      }
      SetupLen -= len;
  }
   return len;
}

/*******************************************************************************
 * @fn       USBHSD_IRQHandler
 *
 * @brief    USB2.0 Interrupt Handler.
 *
 * @return   None
 */
void USBHS_IRQHandler(void)			                                //USBHS interrupt service
{
	UINT32 end_num;
	UINT32 rx_token;
	UINT16 ret_len,i;
	UINT16 rxlen;
	UINT8  *p8;
	UINT8 int_flg;
	int_flg = R8_USB_INT_FG;
	if( int_flg & RB_USB_IF_SETUOACT )                   //SETUP interrupt
	{
#if 0
		 printf("SETUP :");
		 p8 = (UINT8 *)endp0RTbuff;
		 for(i=0; i<8; i++)  { printf("%02x ", *p8++); }
		 printf("\n");
#endif
		 SetupReqType = UsbSetupBuf->bRequestType;
		 SetupReq = UsbSetupBuf->bRequest;
		 SetupReqLen = UsbSetupBuf->wLength;            //Data length

		 /*Analyze host requests*/
		 if((UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD)
         {
             ret_len =  U20_NonStandard_Request_Deal();
         }
         else
         {
             ret_len = U20_Standard_Request_Deal();
         }
		 if(ret_len == 0xFFFF)
         {
              R16_UEP0_T_LEN = 0;
              R8_UEP0_TX_CTRL = UEP_T_RES_STALL ;
              R8_UEP0_RX_CTRL = UEP_R_RES_STALL ;
         }
         else
         {
              R16_UEP0_T_LEN = ret_len;
              R8_UEP0_TX_CTRL = UEP_T_RES_ACK | RB_UEP_T_TOG_1;
              R8_UEP0_RX_CTRL = UEP_R_RES_ACK | RB_UEP_R_TOG_1;
         }
		R8_USB_INT_FG = RB_USB_IF_SETUOACT;              // clear int flag
	}
	/*Transaction transfer complete interrupt*/
	else if( int_flg & RB_USB_IF_TRANSFER )
	{
		end_num =   R8_USB_INT_ST & 0xf;
		rx_token = ( (R8_USB_INT_ST )>>4) & 0x3;
#if 0
		if( !(R8_USB_INT_ST & RB_USB_ST_TOGOK) )
		{
			printf(" TOG MATCH FAIL : ENDP %x token %x \n", end_num, rx_token);
		}
#endif
		switch( end_num )
		{
		   case 0:
                if( rx_token == PID_IN )
                {
                    ret_len = U20_Endp0_IN_Callback();
                    if(ret_len == 0)
                    {
                        R8_UEP0_RX_CTRL = UEP_R_RES_ACK | RB_UEP_R_TOG_1;
                        R16_UEP0_T_LEN = 0;
                        R8_UEP0_TX_CTRL = 0;
                    }
                    else
                    {
                        R16_UEP0_T_LEN = ret_len;
                        R8_UEP0_TX_CTRL ^= RB_UEP_T_TOG_1;
                        R8_UEP0_TX_CTRL = ( R8_UEP0_TX_CTRL &~RB_UEP_TRES_MASK )| UEP_T_RES_ACK ;
                    }
                }
                else if( rx_token == PID_OUT )
                {
                    SetupLen -= SetupLen > R16_USB_RX_LEN ? R16_USB_RX_LEN :SetupLen;
                    if( SetupLen > 0 )
                    {
                       R8_UEP0_RX_CTRL ^=RB_UEP_R_TOG_1;
                       R8_UEP0_RX_CTRL = ( R8_UEP0_RX_CTRL &~RB_UEP_RRES_MASK) | UEP_R_RES_ACK;

                    }
                    else
                    {
                        u20_ep0_out_done();
                        R16_UEP0_T_LEN = 0;
                        R8_UEP0_TX_CTRL = UEP_T_RES_ACK | RB_UEP_T_TOG_1;
                        R8_UEP0_RX_CTRL = 0 ;
                    }
                }
                break;
		   case 1:
		       break;
		   case 2:
               if( rx_token == PID_IN )
               {
                   R32_UEP2_TX_DMA = (UINT32)(UINT8 *)UDisk_In_Buf;
                   R8_UEP2_TX_CTRL = (R8_UEP2_TX_CTRL & ~RB_UEP_TRES_MASK) | UEP_T_RES_NAK;
                   UDISK_In_EP_Deal();
               }
		       break;
		   case 3:
		       if( rx_token == PID_OUT )
               {
		           R32_UEP3_RX_DMA = (UINT32)(UINT8 *)UDisk_Out_Buf;
                   R8_UEP3_RX_CTRL = (R8_UEP3_RX_CTRL & ~RB_UEP_TRES_MASK) | UEP_R_RES_NAK;

                   rxlen = R16_USB_RX_LEN;

                   UDISK_Out_EP_Deal(UDisk_Out_Buf,rxlen);
                   if( UDISK_OutPackflag == 0 )
                   {
                       R8_UEP3_RX_CTRL = (R8_UEP3_RX_CTRL & ~RB_UEP_RRES_MASK) | UEP_R_RES_ACK;
                   }
               }
		       break;
           case 4:                                       /* CDC #1: RP2350 link */
                if( rx_token == PID_IN )
                {
                    R8_UEP4_TX_CTRL = (R8_UEP4_TX_CTRL & ~RB_UEP_TRES_MASK) | UEP_T_RES_NAK;
                    UploadPoint4_Busy = 0;
                }
                else if( rx_token == PID_OUT )
                {
                    if( R8_USB_INT_ST & RB_USB_ST_TOGOK )   /* else a resent packet: drop it, stay armed */
                    {
                        R8_UEP4_RX_CTRL = (R8_UEP4_RX_CTRL & ~RB_UEP_RRES_MASK) | UEP_R_RES_NAK;
                        USBByteCount = R16_USB_RX_LEN;
                        USBBufOutPoint = 0;
                        DownloadPoint4_Busy = 0;         /* cdc_uart.c drains it, then re-arms */
                    }
                }
                break;
           case 5:
                break;
           case 6:                                       /* CDC #2: debug console */
#if DEBUG_OVER_USB
                if( rx_token == PID_IN )
                {
                    R8_UEP6_TX_CTRL = (R8_UEP6_TX_CTRL & ~RB_UEP_TRES_MASK) | UEP_T_RES_NAK;
                    DebugUpload_Busy = 0;
                }
                else if( rx_token == PID_OUT )
                {
                    if( R8_USB_INT_ST & RB_USB_ST_TOGOK )
                        console_rx( endp6Rxbuff, R16_USB_RX_LEN );
                    R32_UEP6_RX_DMA = (UINT32)(UINT8 *)endp6Rxbuff;
                    R8_UEP6_RX_CTRL = (R8_UEP6_RX_CTRL & ~RB_UEP_RRES_MASK) | UEP_R_RES_ACK;
                }
#endif
                break;
           case 7:
                break;
           default:
                break;

		}
		R8_USB_INT_FG = RB_USB_IF_TRANSFER;
	}
	else if( int_flg & RB_USB_IF_BUSRST )
	{
		USB20_Endp_Init();
		USB20_Device_Setaddress( 0 );
		g_devInfo.dev_enum_status = 0;           /* CDC pumps wait for SET_CONFIGURATION */
		g_devInfo.dev_config_value = 0;
		R8_USB_INT_FG = RB_USB_IF_BUSRST;
        if( Link_Sta == LINK_STA_1 )
        {
            PFIC_EnableIRQ(USBSS_IRQn);
            PFIC_EnableIRQ(LINK_IRQn);
            PFIC_EnableIRQ(TMR0_IRQn);
            R8_TMR0_INTER_EN = 1;
            TMR0_TimerInit( 67000000 );
            USB30D_init(ENABLE);
       }
	}
	else if( int_flg & RB_USB_IF_SUSPEND )
	{
		R8_USB_INT_FG = RB_USB_IF_SUSPEND;
	}
}

/*******************************************************************************
 * @fn       U20_Endp0_IN_Callback
 *
 * @brief    U20_Endp0_IN_Callback Handler.
 *
 * @return   None
 */
UINT16 U20_Endp0_IN_Callback(void)
{
    UINT16 len = 0;
    switch(SetupReq)
    {
      case USB_GET_DESCRIPTOR:
          len = SetupLen >= U20_UEP0_MAXSIZE ? U20_UEP0_MAXSIZE : SetupLen;
          memcpy(endp0RTbuff, pDescr, len);
          SetupLen -= len;
          pDescr += len;
          break;
      case USB_SET_ADDRESS:
          USB20_Device_Setaddress(g_devInfo.dev_addr);
          break;
      default:
          break;
    }
    return len;
}


