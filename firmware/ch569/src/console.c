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

static void cmd_status(void)
{
    PRINT("usb %s, sd %s (%ld MB), link UART1 %ld baud, lines from RP2350 %d, echo %s\n",
          Link_Sta == LINK_STA_3 ? "usb3" : (Link_Sta == LINK_STA_1 ? "usb2" : "down"),
          own_card_ready ? "ready" : "none", own_card_ready ? (long)(TF_EMMCParam.EMMCSecNum / 2048) : 0L,
          (long)cdc_line_coding.dwDTERate, link_lines_rx, LINK_ECHO ? "on" : "off");
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
