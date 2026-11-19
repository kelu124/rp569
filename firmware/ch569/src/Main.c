/********************************************************************************
 * Main.c — rp569 CH569 firmware: USB3 composite (SD card as a drive, RP2350
 * link as a serial port, debug log as a second serial port).
 *
 *   - The RP2350 link (UART1, PA7/PA8) is a CDC-ACM port: what the RP2350
 *     sends appears on the host, what the host types goes to the RP2350, and
 *     every byte from the RP2350 is echoed back to it (LINK_ECHO).
 *   - The microSD card (wired straight to the CH569 EMMC pins) is a removable
 *     drive, served as raw sectors over MSC/BOT, mounted at power-up.
 *   - The debug log goes to UART0 (PB6 TX, CH UART header) and to the second
 *     CDC port (DEBUG_OVER_USB). Both also take console commands (console.c:
 *     help, version, status); the USB serial number is the chip unique ID.
 *
 * On a USB2-only link the device falls back to MSC-only (no COM port) — the
 * vendor USB2 stack has no CDC function. The link echo runs either way.
 *
 * Forked from CH569_Composite (github.com/kelu124/CH569_Composite), itself
 * derived from the WCH CH569 EVT examples (MSC_U-Disk, SimulateCDC, SD).
 *******************************************************************************/
#include <stdio.h>
#include "CH56x_common.h"
#include "CH56x_usb30.h"
#include "CH56x_usb20.h"
#include "SW_UDISK.h"
#include "cdc_uart.h"
#include "ownership.h"
#include "bridge_config.h"
#include "console.h"
#include "chip_uid.h"
#include "fw_version.h"
#if DEBUG_OVER_USB
#include "debug_cdc.h"
#include "mode.h"
#endif

#if STATUS_LEDS
/* Drive a status LED to reflect a boolean state, honouring the board's
 * active level (bridge_config.h). */
static void led_set(UINT32 pin, UINT8 on)
{
#if STATUS_LED_ACTIVE_HIGH
    if(on) GPIOB_SetBits(pin); else GPIOB_ResetBits(pin);
#else
    if(on) GPIOB_ResetBits(pin); else GPIOB_SetBits(pin);
#endif
}

static void status_leds_init(void)
{
    GPIOB_ModeCfg(LED_READY_PIN | LED_LINK_PIN | LED_RX_PIN, GPIO_Slowascent_PP_8mA);
    led_set(LED_READY_PIN, 0);
    led_set(LED_LINK_PIN, 0);
    led_set(LED_RX_PIN, 0);
}

static void status_leds_update(void)
{
    led_set(LED_READY_PIN, own_card_ready);
    led_set(LED_LINK_PIN,  Link_Sta == LINK_STA_3);   /* USB3 SuperSpeed up */
    led_set(LED_RX_PIN,    link_lines_rx & 1);         /* toggles per RP2350 line */
}
#endif

/*******************************************************************************
 * @fn        DebugInit
 *
 * @brief     UART0 (PB5=RX / PB6=TX, CH UART header) debug log output.
 *            UART0 stays on PB5/PB6: its alternate pins PA5/PA6 are HSPI/boot.
 ******************************************************************************/
void DebugInit(UINT32 baudrate)
{
    UINT32 x;
    UINT32 t = FREQ_SYS;

    x = 10 * t * 2 / 16 / baudrate;
    x = (x + 5) / 10;
    R8_PIN_ALTERNATE &= ~RB_PIN_UART0;
    R8_UART0_DIV = 1;
    R16_UART0_DL = x;
    R8_UART0_FCR = RB_FCR_FIFO_TRIG | RB_FCR_TX_FIFO_CLR | RB_FCR_RX_FIFO_CLR | RB_FCR_FIFO_EN;
    R8_UART0_LCR = RB_LCR_WORD_SZ;
    R8_UART0_IER = RB_IER_TXD_EN;
    GPIOB_SetBits(GPIO_Pin_6);
    GPIOB_ModeCfg(GPIO_Pin_5, GPIO_ModeIN_PU_NSMT);
    GPIOB_ModeCfg(GPIO_Pin_6, GPIO_Slowascent_PP_8mA);
}

/*********************************************************************
 * @fn      main
 *********************************************************************/
int main(void)
{
    SystemInit(FREQ_SYS);
    Delay_Init(FREQ_SYS);

    DebugInit(DEBUG_UART_BAUD);
    setvbuf(stdout, NULL, _IONBF, 0);   /* no stdio buffering: the console prompt has no '\n' */
    {
        char uid[2 * CHIP_UID_LEN + 1];
        chip_uid_hex(uid);
        USB30_SetSerial(uid);
        PRINT("\nrp569 CH569W firmware " FW_VERSION " git " FW_GIT " uid %s (%d MHz)\n", uid, FREQ_SYS / 1000000);
    }

    /* SD card state (no ownership line on rp569, see bridge_config.h). */
    ownership_init();

#if STATUS_LEDS
    status_leds_init();         /* RGB LED: card ready / USB3 link / RP2350 RX */
#endif

    /* RP2350 link UART1 (CDC bridge + echo) + RX flush timer */
    CDC_Uart_Init(LINK_UART_DEFAULT_BAUD);
    TMR2_TimerInit1();

    /* The disk starts "not present": no EMMC access happens until a claim. */
    Udisk_Status &= ~DEF_UDISK_EN_FLAG;

    /* Mode (mode.h), decided before USB comes up: drive mode needs the card mounted, else serial mode. */
    bridge_mode = mode_enter_drive();
    PRINT("MODE: %s\n", bridge_mode == MODE_DRIVE ? "drive (SD card on USB)" : "serial (card released)");

    /* USB: try SuperSpeed first; TMR0 times out into the USB2 fallback. */
    usb_start();
    mode_announce();
    PRINT("USB init done\n");

    while(1)
    {
        if(bridge_mode == MODE_DRIVE)
        {
            UDISK_onePack_Deal();   /* MSC multi-sector transfers (EMMC <-> USB) */
            ownership_poll();       /* executes pending SD mount/unmount         */
        }
        else
        {
            CDC_Uart_Deal();        /* CDC <-> UART1 (RP2350) pump               */
#if DEBUG_OVER_USB
            debug_cdc_poll();       /* stream debug log out the console port     */
#endif
        }
        Link_Echo_Deal();       /* echo RP2350 bytes back on UART1           */
        Link_Ctrl_Deal();       /* RP2350 control lines (SD release / mount) */
        sd_det_poll();          /* card detect: swap handling + link signal   */
        console_poll();         /* commands from the console port / UART0    */
        mode_poll();            /* drive <-> serial switch (re-enumerates)   */
#if STATUS_LEDS
        status_leds_update();   /* refresh the RGB status LED                */
#endif
    }
}
