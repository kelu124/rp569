/********************************************************************************
 * console.c — small command line on the debug port (CDC #2, EP6 OUT) and on
 * UART0 RX (CH UART header). Replies go to the debug log (both ports).
 *
 * Commands live in the table between COMMANDS-BEGIN / COMMANDS-END: the host
 * library (python/rp569/ch569.py) must have one @command(...) method per
 * entry; tools/check_commands.py (pre-commit hook) enforces it.
 *******************************************************************************/
#include <string.h>
#include "CH56x_common.h"
#include "CH56x_usb30.h"
#include "console.h"
#include "chip_uid.h"
#include "cdc_uart.h"
#include "ownership.h"
#include "bridge_config.h"
#include "mode.h"
#include "fw_version.h"           /* FW_VERSION / FW_GIT, generated from version.yaml */
#include "SW_UDISK.h"
#include "sd_storage.h"

#define CON_RING   128            /* power of two */
#define CON_LINE   64

static volatile UINT8  con_ring[CON_RING];
static volatile UINT16 con_head = 0;  /* producer: EP6 OUT callback / main loop (UART0) */
static volatile UINT16 con_tail = 0;  /* consumer: console_poll */
static char   con_line[CON_LINE];
static UINT8  con_len = 0;

void console_rx(const UINT8 *buf, UINT16 len)
{
    UINT16 i;
    for(i = 0; i < len; i++)
    {
        UINT16 next = (con_head + 1) & (CON_RING - 1);
        if(next == con_tail)
            break;                /* full: drop the rest */
        con_ring[con_head] = buf[i];
        con_head = next;
    }
}

static void cmd_help(void);

/* One line, parsed by the host library: "rp569 CH569W firmware <x.y.z> git <hash> built <date> uid <hex>" */
static void cmd_version(void)
{
    char uid[2 * CHIP_UID_LEN + 1];
    chip_uid_hex(uid);
    PRINT("rp569 CH569W firmware " FW_VERSION " git " FW_GIT " built " __DATE__ " uid %s\n", uid);
}

/* "usb3 irqs N rx_detect N term N present N txeq N ready N inactive N ux_exit N disabled N timeouts N flags X status X" */
static void cmd_usb3(void)
{
    PRINT("usb3 irqs %lu rx_detect %lu term %lu present %lu txeq %lu ready %lu inactive %lu ux_exit %lu disabled %lu "
          "timeouts %lu flags %08lx status %08lx\n",
          usb3_diag.irqs, usb3_diag.rx_detect, usb3_diag.term, usb3_diag.term_present, usb3_diag.txeq,
          usb3_diag.ready, usb3_diag.inactive, usb3_diag.ux_exit, usb3_diag.disabled, usb3_diag.timeouts,
          usb3_diag.last_flags, usb3_diag.last_status);
}

/* One line, parsed by the host library (python decodes the CID fields):
 * "sd cid <32 hex, bits 127..0> size <MB> MB clock <MHz> MHz <mounted|released>", or "sd none" before any mount. */
static void cmd_sd(void)
{
    if(!TF_EMMCParam.EMMCSecNum)
    {
        PRINT("sd none det %s\n", sd_card_detect() ? "in" : "out");
        return;
    }
    PRINT("sd cid %08lx%08lx%08lx%08lx size %ld MB clock %d MHz %s det %s\n",
          (unsigned long)TF_EMMCParam.EMMC_CID[3], (unsigned long)TF_EMMCParam.EMMC_CID[2],
          (unsigned long)TF_EMMCParam.EMMC_CID[1], (unsigned long)TF_EMMCParam.EMMC_CID[0],
          (long)(TF_EMMCParam.EMMCSecNum / 2048), sd_high_speed ? 48 : 24, own_card_ready ? "mounted" : "released",
          sd_card_detect() ? "in" : "out");
}

/* Serial mode -> drive mode: the serial ports disconnect, the card is mounted and the drive appears (mode.h). */
static void cmd_mount(void)
{
    if(bridge_mode == MODE_DRIVE)
    {
        PRINT("already in drive mode\n");
        return;
    }
    PRINT("mounting the SD card: switching to drive mode, this port disconnects\n");
    mode_request(MODE_DRIVE, 0);
}

static void cmd_status(void)
{
    PRINT("usb %s, sd %s (%ld MB), link UART1 %ld baud, lines from RP2350 %d, echo %s, sd clock %d MHz, mode %s\n",
          Link_Sta == LINK_STA_3 ? "usb3" : (Link_Sta == LINK_STA_1 ? "usb2" : "down"),
          own_card_ready ? "ready" : "none", own_card_ready ? (long)(TF_EMMCParam.EMMCSecNum / 2048) : 0L,
          (long)cdc_line_coding.dwDTERate, link_lines_rx, LINK_ECHO ? "on" : "off",
          own_card_ready ? (sd_high_speed ? 48 : 24) : 0, bridge_mode == MODE_DRIVE ? "drive" : "serial");
}

/* Crystal check (test 4): the RP2350 drives a 1 kHz square wave derived from its 12 MHz crystal on GPIO21
 * (`clkout`, HSPI_TREQ net) = PA9 = TMR0 capture input (CH569DS1 8.3.3, pin table PA9/TMR0). TMR0 captures
 * rising-to-rising periods in Fsys ticks; consecutive periods tile the time, so the sum of N periods is exact to
 * +-1 tick (0.0125 ppm over 1 s at 80 MHz). Fsys = 30 MHz HSE x PLL, so the result is the CH569 crystal error
 * relative to the RP2350 crystal (both sides' errors combined). TMR0 is otherwise the USB3 retry timer
 * (CH56x_usb30.c): refused while it is armed, left cleared and with its interrupt off afterwards. */
#define XTAL_REF_HZ   1000
#define XTAL_PERIODS  1000        /* 1 s */

static void cmd_xtal(void)
{
    UINT32 sum = 0, n = 0, timeouts = 0, v;
    UINT32 expect = FREQ_SYS / XTAL_REF_HZ;
    UINT8  first = 1;
    if(R8_TMR0_CTRL_MOD & RB_TMR_COUNT_EN)
    {
        PRINT("xtal busy (TMR0 = USB3 retry timer armed), try again\n");
        return;
    }
    PFIC_DisableIRQ(TMR0_IRQn);
    R8_TMR0_INTER_EN = 0;
    R32_PA_DIR &= ~(1UL << 9);               /* PA9 input, no pull */
    R32_PA_PU  &= ~(1UL << 9);
    R32_PA_PD  &= ~(1UL << 9);
    R32_TMR0_CNT_END = expect * 4;           /* capture timeout: 4 periods without an edge */
    R8_TMR0_CTRL_MOD = RB_TMR_ALL_CLEAR;
    R8_TMR0_CTRL_MOD = RB_TMR_MODE_IN | (3 << 6) | RB_TMR_COUNT_EN;   /* capture, rising to rising */
    while(n < XTAL_PERIODS && timeouts < 4)
    {
        if(!R8_TMR0_FIFO_COUNT)
            continue;
        v = R32_TMR0_FIFO;
        if(v & (1UL << 25))
        {
            timeouts++;                      /* no edge within CNT_END */
            continue;
        }
        if(first)
        {
            first = 0;                       /* from the enable to the first edge: partial */
            continue;
        }
        sum += v & 0x01FFFFFF;
        n++;
    }
    R8_TMR0_CTRL_MOD = RB_TMR_ALL_CLEAR;
    if(n < XTAL_PERIODS)
    {
        PRINT("xtal no signal on PA9 (RP2350 `clkout` first)\n");
        return;
    }
    /* ppm x100 = (sum - n*expect) / (n*expect) * 1e8; positive = the CH569 clock runs fast vs the RP2350 one */
    {
        INT32 diff = (INT32)(sum - n * expect);
        INT32 ppm100 = (INT32)((long long)diff * 100000000LL / (long long)(n * expect));
        INT32 a = ppm100 < 0 ? -ppm100 : ppm100;
        PRINT("xtal periods %lu ticks %lu expected %lu ppm %s%ld.%02ld\n", (unsigned long)n, (unsigned long)sum,
              (unsigned long)(n * expect), ppm100 < 0 ? "-" : "+", (long)(a / 100), (long)(a % 100));
    }
}

/* Reset source of the last reset (R8_RST_BOOT_STAT RB_RESET_FLAG, CH569DS1 3.2 / reset status register). */
static void cmd_lastreset(void)
{
    static const char *const names[4] = {"software", "power-on", "watchdog", "manual"};
    UINT8 f = R8_RST_BOOT_STAT & RB_RESET_FLAG;
    PRINT("lastreset %s flag %d boot_stat %02x wdog ctrl %02x count %d\n", names[f], f, R8_RST_BOOT_STAT,
          R8_RST_WDOG_CTRL, R8_WDOG_COUNT);
}

/* Watchdog test (test 9): arm the watchdog reset and never feed it; the main loop keeps running. The counter
 * (8 bits, Fsys/524288 = 6.55 ms per tick at 80 MHz, CH569DS1 3.2.4) runs from boot whether armed or not, so it
 * is cleared first: the reset comes ~1.68 s later, the firmware restarts (power-up path: drive mode) and
 * `lastreset` then says "watchdog" (found on board #1, 2026-11-20). The protected write uses the SDK helper:
 * NO __disable_irq() around it, the firmware runs in user mode where mstatus traps (hardware notes). */
static void cmd_wdtest(void)
{
    UINT8 c0, c1;
    R8_WDOG_COUNT = 0;
    WWDG_ResetCfg(ENABLE);
    c0 = R8_WDOG_COUNT;
    mDelaymS(20);
    c1 = R8_WDOG_COUNT;
    PRINT("wdtest armed ctrl %02x count %d -> %d in 20 ms, reset in ~%lu ms\n", R8_RST_WDOG_CTRL, c0, c1,
          (unsigned long)(256ULL * 524288 * 1000 / FREQ_SYS));
}

typedef struct
{
    const char *name;
    const char *help;
    void (*fn)(void);
} command_t;

/* COMMANDS-BEGIN (keep in sync with python/rp569/ch569.py: tools/check_commands.py) */
static const command_t commands[] =
{
    {"help",    "this list",                                     cmd_help},
    {"version", "firmware version, git hash and chip unique ID", cmd_version},
    {"status",  "USB link, SD card, RP2350 link counters",       cmd_status},
    {"mount",   "mount the SD card and switch to drive mode",    cmd_mount},
    {"sd",      "SD card CID (128 bits), size, bus clock",       cmd_sd},
    {"usb3",    "SuperSpeed link diagnostics (link interrupts)",  cmd_usb3},
    {"xtal",    "crystal error vs the RP2350 (needs RP2350 clkout)", cmd_xtal},
    {"lastreset", "source of the last reset",                    cmd_lastreset},
    {"wdtest",  "arm the watchdog reset, never fed (~1.7 s)",      cmd_wdtest},
};
/* COMMANDS-END */
#define N_COMMANDS (sizeof(commands) / sizeof(commands[0]))

static void cmd_help(void)
{
    UINT8 i;
    PRINT("commands:\n");
    for(i = 0; i < N_COMMANDS; i++)
        PRINT("  %-10s %s\n", commands[i].name, commands[i].help);
}

static void run(char *cmd)
{
    UINT8 i;
    while(*cmd == ' ')
        cmd++;
    for(i = 0; cmd[0] && i < N_COMMANDS; i++)
    {
        if(!strcmp(cmd, commands[i].name))
        {
            commands[i].fn();
            return;
        }
    }
    if(cmd[0])
        PRINT("unknown command '%s' (try 'help')\n", cmd);
}

void console_poll(void)
{
    /* UART0 RX (CH UART header) feeds the same ring */
    while(R8_UART0_RFC)
    {
        UINT8 b = R8_UART0_RBR;
        console_rx(&b, 1);
    }
    while(con_tail != con_head)
    {
        char c = con_ring[con_tail];
        con_tail = (con_tail + 1) & (CON_RING - 1);
        if(c == '\r' || c == '\n')
        {
            if(c == '\n' && con_len == 0)
                continue;         /* second half of a CRLF */
            con_line[con_len] = 0;
            PRINT("\n");
            run(con_line);
            con_len = 0;
            PRINT("ch569> ");
        }
        else if((c == '\b' || c == 0x7F) && con_len)
        {
            con_len--;
            PRINT("\b \b");
        }
        else if(c >= ' ' && c < 0x7F && con_len < CON_LINE - 1)
        {
            con_line[con_len++] = c;
            PRINT("%c", c);
        }
    }
}
