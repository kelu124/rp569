// rp569 RP2350B firmware: USB CDC command line for the board (LED, buttons, ADC, DFU) and CH569 control.
//
// The CLI runs on the USB serial port (and mirrored on the UART0 debug pins of the RP EXT header).
// Text received from the CH569 on the inter-chip UART is printed as "CH> ..." lines.
// Pins: boards/rp569.h. CH569 power/boot sequence: specification.md D-004.
//
// Commands live in the table between COMMANDS-BEGIN / COMMANDS-END: the host library (python/rp569/rp2350.py)
// must have one @command(...) method per entry; tools/check_commands.py (pre-commit hook) enforces it.

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "hardware/watchdog.h"
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "pico/unique_id.h"

#include "fw_version.h"           // FW_VERSION / FW_GIT, generated from version.yaml

#define LINK_BAUD        115200
#define CH_POWER_ON_BOOT 1        // power the CH569 (normal boot) when the RP2350 starts
#define LINE_MAX         128

static uart_inst_t *const link_uart = UART_INSTANCE(RP569_LINK_UART);
static bool ch_powered;
static bool ch_fault_seen;
static const uint led_pins[3] = {RP569_LED_R_PIN, RP569_LED_G_PIN, RP569_LED_B_PIN};
static bool led_on[3];

static char line[LINE_MAX];       // command being typed
static size_t line_len;
static char ch_line[LINE_MAX];    // text arriving from the CH569
static size_t ch_len;

// ---------------------------------------------------------------- RGB LED (common anode: low = on)

static void led_set(int i, bool on) {
    led_on[i] = on;
    gpio_put(led_pins[i], !on);
}

static void led_init(void) {
    for (int i = 0; i < 3; i++) {
        gpio_init(led_pins[i]);
        gpio_set_dir(led_pins[i], GPIO_OUT);
        led_set(i, false);
    }
}

// ---------------------------------------------------------------- ADC monitors

static float adc_volts(uint gpio, float divider) {
    adc_select_input(gpio - 40);          // RP2350B: GPIO40-47 = ADC0-7
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++)
        sum += adc_read();
    return sum / 16.0f * 3.3f / 4096.0f * divider;
}

static float vbus3_volts(void) { return adc_volts(RP569_VBUS3_SENSE_PIN, 147.0f / 47.0f); }  // 100k / 47k
static float ch3v3_volts(void) { return adc_volts(RP569_CH3V3_SENSE_PIN, 2.0f); }            // 100k / 100k

// ---------------------------------------------------------------- CH569 power, boot strap, reset, link UART

// While the CH569 is off every line to it is an input without pull-up, so nothing back-powers it.
static void ch_lines_release(void) {
    gpio_init(RP569_LINK_TX_PIN);
    gpio_init(RP569_LINK_RX_PIN);
    gpio_init(RP569_HSPI_D0_PIN);
    gpio_disable_pulls(RP569_LINK_TX_PIN);
    gpio_disable_pulls(RP569_HSPI_D0_PIN);
}

static void ch_power_off(void) {
    gpio_put(RP569_CH_PWR_EN_PIN, 0);
    ch_lines_release();
    ch_powered = false;
    absolute_time_t until = make_timeout_time_ms(500);
    while (ch3v3_volts() > 0.5f && !time_reached(until))
        sleep_ms(5);
    printf("CH569 off (3V3_CH %.2f V)\n", ch3v3_volts());
}

// bootloader = true: PA5 (HSPI_D0) held low at power-up -> WCH USB bootloader on the USB3 connector, ~10 s.
// Normal boot: HSPI_D0 only gets the weak pull-up, so the CH BOOT jumper/button can still force the bootloader.
static void ch_power_on(bool bootloader) {
    if (ch_powered)
        ch_power_off();
    gpio_init(RP569_HSPI_D0_PIN);
    if (bootloader) {
        gpio_put(RP569_HSPI_D0_PIN, 0);
        gpio_set_dir(RP569_HSPI_D0_PIN, GPIO_OUT);
    } else {
        gpio_pull_up(RP569_HSPI_D0_PIN);
    }
    gpio_put(RP569_CH_PWR_EN_PIN, 1);
    sleep_ms(50);                         // AP2171W soft start + both bucks
    gpio_set_function(RP569_LINK_TX_PIN, UART_FUNCSEL_NUM(link_uart, RP569_LINK_TX_PIN));
    gpio_set_function(RP569_LINK_RX_PIN, UART_FUNCSEL_NUM(link_uart, RP569_LINK_RX_PIN));
    gpio_pull_up(RP569_LINK_RX_PIN);
    ch_powered = true;
    if (bootloader) {
        sleep_ms(200);                    // PA5 is sampled at power-up
        gpio_set_dir(RP569_HSPI_D0_PIN, GPIO_IN);
    }
    printf("CH569 on%s (3V3_CH %.2f V, VBUS3 %.2f V)\n", bootloader ? ", USB bootloader for ~10 s" : "",
           ch3v3_volts(), vbus3_volts());
    if (vbus3_volts() < 4.0f)
        printf("  warning: no USB3 VBUS, the CH569 is fed from VBUS_3 only (unless the DNP 0R is fitted)\n");
}

// RST# only acts when the CH569 config bit RESET_EN is set (off on a new chip): power-cycle otherwise.
static void ch_reset_pulse(void) {
    gpio_put(RP569_CH_RST_N_PIN, 0);
    gpio_set_dir(RP569_CH_RST_N_PIN, GPIO_OUT);
    sleep_ms(10);
    gpio_set_dir(RP569_CH_RST_N_PIN, GPIO_IN);
    printf("RST# pulsed (needs RESET_EN in the CH569 config; else use 'ch cycle')\n");
}

static void ch_init(void) {
    gpio_init(RP569_CH_PWR_EN_PIN);
    gpio_put(RP569_CH_PWR_EN_PIN, 0);
    gpio_set_dir(RP569_CH_PWR_EN_PIN, GPIO_OUT);
    gpio_init(RP569_CH_RST_N_PIN);        // input: the 10k pull-up holds RST# high
    gpio_disable_pulls(RP569_CH_RST_N_PIN);
    gpio_init(RP569_CH_FAULT_N_PIN);
    uart_init(link_uart, LINK_BAUD);
    ch_lines_release();
}

static void ch_status(void) {
    printf("CH569 power %s, FLG# %s, 3V3_CH %.2f V, VBUS3 %.2f V, link UART%d %d baud\n",
           ch_powered ? "on" : "off", gpio_get(RP569_CH_FAULT_N_PIN) ? "ok" : "FAULT", ch3v3_volts(), vbus3_volts(),
           RP569_LINK_UART, LINK_BAUD);
}

static void ch_send(const char *text) {
    if (!ch_powered) {
        printf("CH569 is off ('ch on' first)\n");
        return;
    }
    uart_puts(link_uart, text);
    uart_puts(link_uart, "\r\n");
}

// ---------------------------------------------------------------- command line

static void prompt(void) {
    printf("rp569> %.*s", (int)line_len, line);
}

static int led_index(const char *s) {
    switch (toupper((unsigned char)s[0])) {
    case 'R': return 0;
    case 'G': return 1;
    case 'B': return 2;
    }
    return -1;
}

// Command handlers: argc/argv are the words after the command name, rest = the raw text after it.
static void cmd_help(int argc, char **argv, char *rest);

static void cmd_set(int argc, char **argv, char *rest) {
    if (argc == 2 && !strcasecmp(argv[0], "rgb") && strlen(argv[1]) == 3) {
        for (int i = 0; i < 3; i++)
            led_set(i, argv[1][i] == '1');
        printf("R %d G %d B %d\n", led_on[0], led_on[1], led_on[2]);
    } else if (argc == 2 && strlen(argv[0]) == 1 && led_index(argv[0]) >= 0 &&
               (!strcmp(argv[1], "0") || !strcmp(argv[1], "1"))) {
        led_set(led_index(argv[0]), argv[1][0] == '1');
        printf("%c %s\n", toupper((unsigned char)argv[0][0]), argv[1][0] == '1' ? "on" : "off");
    } else {
        printf("usage: set <R|G|B> <0|1> | set rgb <RGB bits, e.g. 101>\n");
    }
}

static void cmd_led(int argc, char **argv, char *rest) {
    printf("R %d G %d B %d\n", led_on[0], led_on[1], led_on[2]);
}

static void cmd_btn(int argc, char **argv, char *rest) {
    printf("BTN1 %s, BTN2 %s\n", gpio_get(RP569_BTN1_PIN) ? "up" : "down", gpio_get(RP569_BTN2_PIN) ? "up" : "down");
}

static void cmd_adc(int argc, char **argv, char *rest) {
    printf("VBUS3 %.2f V, 3V3_CH %.2f V\n", vbus3_volts(), ch3v3_volts());
}

static void cmd_dfu(int argc, char **argv, char *rest) {
    printf("rebooting into the USB bootloader (RP2350 drive / picotool)\n");
    stdio_flush();
    sleep_ms(100);
    reset_usb_boot(0, 0);
}

static void cmd_reboot(int argc, char **argv, char *rest) {
    printf("rebooting\n");
    stdio_flush();
    sleep_ms(100);
    watchdog_reboot(0, 0, 0);
}

// One line, parsed by the host library: "rp569 RP2350B firmware <x.y.z> git <hash> built <date> uid <hex>"
static void cmd_version(int argc, char **argv, char *rest) {
    char uid[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
    pico_get_unique_board_id_string(uid, sizeof(uid));
    printf("rp569 RP2350B firmware " FW_VERSION " git " FW_GIT " built " __DATE__ " uid %s\n", uid);
}

static void cmd_ch_on(int argc, char **argv, char *rest) { ch_power_on(false); }
static void cmd_ch_off(int argc, char **argv, char *rest) { ch_power_off(); }
static void cmd_ch_cycle(int argc, char **argv, char *rest) { ch_power_off(); ch_power_on(false); }
static void cmd_ch_boot(int argc, char **argv, char *rest) { ch_power_on(true); }
static void cmd_ch_reset(int argc, char **argv, char *rest) { ch_reset_pulse(); }
static void cmd_ch_status(int argc, char **argv, char *rest) { ch_status(); }

static void cmd_ch_send(int argc, char **argv, char *rest) {
    if (rest && *rest)
        ch_send(rest);
    else
        printf("usage: ch send <text>\n");
}

typedef struct {
    const char *name;             // one or two words
    const char *args;
    const char *help;
    void (*fn)(int argc, char **argv, char *rest);
} command_t;

// COMMANDS-BEGIN (keep in sync with python/rp569/rp2350.py: tools/check_commands.py)
static const command_t commands[] = {
    {"help",      "",                "this list",                                                       cmd_help},
    {"version",   "",                "firmware version, git hash and chip unique ID",                   cmd_version},
    {"set",       "<R|G|B> <0|1>",   "RGB LED colour on/off (also 'set rgb 101')",                      cmd_set},
    {"led",       "",                "LED state",                                                       cmd_led},
    {"btn",       "",                "user buttons",                                                    cmd_btn},
    {"adc",       "",                "VBUS3 and 3V3_CH voltages",                                       cmd_adc},
    {"dfu",       "",                "reboot the RP2350 into its USB bootloader (BOOTSEL / UF2)",       cmd_dfu},
    {"reboot",    "",                "reboot the RP2350",                                               cmd_reboot},
    {"ch on",     "",                "CH569 power on (normal boot)",                                    cmd_ch_on},
    {"ch off",    "",                "CH569 power off",                                                 cmd_ch_off},
    {"ch cycle",  "",                "CH569 power off, then normal boot",                               cmd_ch_cycle},
    {"ch boot",   "",                "power-cycle the CH569 into its USB bootloader (~10 s)",           cmd_ch_boot},
    {"ch reset",  "",                "pulse RST# (only with RESET_EN set in the CH569)",                cmd_ch_reset},
    {"ch status", "",                "CH569 power, fault flag, rails",                                  cmd_ch_status},
    {"ch send",   "<text>",          "send a line to the CH569 over the link UART (it echoes it back)", cmd_ch_send},
};
// COMMANDS-END
#define N_COMMANDS (sizeof(commands) / sizeof(commands[0]))

static void cmd_help(int argc, char **argv, char *rest) {
    printf("commands:\n");
    for (size_t i = 0; i < N_COMMANDS; i++) {
        char left[32];
        snprintf(left, sizeof(left), "%s %s", commands[i].name, commands[i].args);
        printf("  %-24s %s\n", left, commands[i].help);
    }
}

// Longest match on the first one or two words; the handler gets the remaining words and the raw remainder.
static void run(char *cmd) {
    while (*cmd == ' ' || *cmd == '\t')
        cmd++;
    if (!*cmd)
        return;
    const command_t *best = NULL;
    size_t best_len = 0;
    for (size_t i = 0; i < N_COMMANDS; i++) {
        size_t n = strlen(commands[i].name);
        if (n > best_len && !strncmp(cmd, commands[i].name, n) && (cmd[n] == 0 || cmd[n] == ' ' || cmd[n] == '\t')) {
            best = &commands[i];
            best_len = n;
        }
    }
    if (!best) {
        printf("unknown command '%s' (try 'help')\n", cmd);
        return;
    }
    char *rest = cmd + best_len;
    while (*rest == ' ' || *rest == '\t')
        rest++;
    char raw[LINE_MAX];
    strncpy(raw, rest, sizeof(raw) - 1);
    raw[sizeof(raw) - 1] = 0;
    char *argv[8];
    int argc = 0;
    for (char *tok = strtok(rest, " \t"); tok && argc < 8; tok = strtok(NULL, " \t"))
        argv[argc++] = tok;
    best->fn(argc, argv, raw);
}

static void cli_poll(void) {
    int c;
    while ((c = getchar_timeout_us(0)) != PICO_ERROR_TIMEOUT) {
        if (c == '\r' || c == '\n') {
            if (c == '\n' && line_len == 0)
                continue;                 // second half of a CRLF
            printf("\n");
            line[line_len] = 0;
            run(line);
            line_len = 0;
            prompt();
        } else if ((c == '\b' || c == 0x7f) && line_len) {
            line_len--;
            printf("\b \b");
        } else if (isprint(c) && line_len < LINE_MAX - 1) {
            line[line_len++] = (char)c;
            putchar(c);
        }
    }
}

// Lines from the CH569: printed above the command being typed.
static void link_poll(void) {
    while (ch_powered && uart_is_readable(link_uart)) {
        char c = uart_getc(link_uart);
        if (c == '\r')
            continue;
        if (c != '\n' && ch_len < LINE_MAX - 1) {
            ch_line[ch_len++] = c;
            continue;
        }
        printf("\r\x1b[2KCH> %.*s\n", (int)ch_len, ch_line);
        ch_len = 0;
        prompt();
    }
}

static void fault_poll(void) {
    bool fault = ch_powered && !gpio_get(RP569_CH_FAULT_N_PIN);
    if (fault && !ch_fault_seen) {
        printf("\r\x1b[2KCH569 power switch fault (FLG# low: over-current or thermal)\n");
        prompt();
    }
    ch_fault_seen = fault;
}

int main(void) {
    stdio_init_all();
    led_init();
    adc_init();
    adc_gpio_init(RP569_VBUS3_SENSE_PIN);
    adc_gpio_init(RP569_CH3V3_SENSE_PIN);
    gpio_init(RP569_BTN1_PIN);            // external 10k pull-ups
    gpio_init(RP569_BTN2_PIN);
    ch_init();
    if (CH_POWER_ON_BOOT)
        ch_power_on(false);

    bool connected = false;
    while (true) {
        bool now = stdio_usb_connected();
        if (now && !connected) {          // terminal opened: banner + prompt
            printf("\nrp569 RP2350B firmware " FW_VERSION " - type 'help'\n");
            prompt();
        }
        connected = now;
        cli_poll();
        link_poll();
        fault_poll();
        sleep_us(200);
    }
}
