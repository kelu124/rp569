// stream.c — RP2350 firmware variant 1 ("stream", D-031): counter test stream to the CH569 over the 8-bit HSPI bus.
//
// Core 1 builds HSPI packets (4-byte header, 4096-byte payload of 32-bit little-endian counter words that continue
// across packets, 2-byte CRC field) and paces them to the target rate; one DMA channel feeds them to the PIO
// transmitter (hspi_tx.pio). The CH569 stream firmware (firmware/ch569 VARIANT=stream) receives and sends the
// payload to the host on its bulk endpoint, where python/rp569/stream.py checks every word.
// CRC16 (polynomial 0x8005, CH569DS1 10.2.2): the datasheet does not give init / bit order / coverage, so the variant
// is selectable (crc_mode below). A sweep on board #1 found it: mode 8 = CRC-16/USB (reflected 0xA001, init and
// final XOR 0xFFFF, over header + payload, low byte first), computed with slicing-by-4 tables. The CH569 counts CRC
// errors but keeps the data either way.
#include "stream.h"

#include <stdio.h>
#include <string.h>

#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"

#include "hspi_tx.pio.h"

#define PKT_PAYLOAD   4096                          // bytes, the CH569 receive slot size
#define PKT_WORDS     (1 + 1 + PKT_PAYLOAD / 4 + 1)  // count, header, payload, CRC word
#define PKT_BYTES     (4 + PKT_PAYLOAD + 2)          // header + payload + CRC16
#define NBUF          3
#define PIN_D0        8                              // GPIO8-15 HD0-7
#define PIN_CLK       16                             // HRCLK
#define PIN_ACT       17                             // HRACT (+ GPIO18 HRVLD)
#define PIN_ACK       19                             // HTACK (input)

static uint32_t bufs[NBUF][PKT_WORDS];
static PIO pio = pio0;
static int sm = -1, dma_ch = -1;
static uint offset;
static volatile bool running, core1_done = true;
static volatile uint32_t packets;
static volatile uint64_t start_us, stop_us;
static float target_mbs, bus_mhz;
static uint32_t counter;                           // next payload word value
static uint32_t tsqn;                              // HSPI sequence number: continues across stream starts, reset
                                                   // only when the CH569 (re)starts (its RX sequence starts at 0)

void stream_seq_reset(void) { tsqn = 0; }
// crc_mode 0 = no CRC; 1..32 = 1 + bit field: bit0 reflected (poly 0xA001), bit1 init 0xFFFF, bit2 final XOR 0xFFFF,
// bit3 payload only (else header + payload), bit4 CRC high byte first. Sweep with `stream start <MB/s> <MHz> <mode>`.
static int crc_mode;
static uint16_t crc_tab[256];
static uint16_t crc_slice[4][256];                 // CRC-16/USB slicing-by-4 (mode 8, the CH569's CRC)

static void crc_usb_tables(void) {
    for (int i = 0; i < 256; i++) {
        uint16_t c = i;
        for (int b = 0; b < 8; b++)
            c = (c & 1) ? (c >> 1) ^ 0xA001 : c >> 1;
        crc_slice[0][i] = c;
    }
    for (int k = 1; k < 4; k++)
        for (int i = 0; i < 256; i++)
            crc_slice[k][i] = (crc_slice[k - 1][i] >> 8) ^ crc_slice[0][crc_slice[k - 1][i] & 0xFF];
}

// CRC-16/USB (poly 0x8005 reflected, init and final XOR 0xFFFF) over n 32-bit little-endian words.
static uint32_t crc_usb_words(const uint32_t *w, int n) {
    uint32_t c = 0xFFFF;
    while (n--) {
        uint32_t x = *w++ ^ c;
        c = crc_slice[3][x & 0xFF] ^ crc_slice[2][(x >> 8) & 0xFF] ^ crc_slice[1][(x >> 16) & 0xFF] ^
            crc_slice[0][x >> 24];
    }
    return c ^ 0xFFFF;
}

static void crc_table(bool reflected) {
    for (int i = 0; i < 256; i++) {
        uint16_t c;
        if (reflected) {
            c = i;
            for (int b = 0; b < 8; b++)
                c = (c & 1) ? (c >> 1) ^ 0xA001 : c >> 1;
        } else {
            c = i << 8;
            for (int b = 0; b < 8; b++)
                c = (c & 0x8000) ? (c << 1) ^ 0x8005 : c << 1;
        }
        crc_tab[i] = c;
    }
}

static uint32_t crc_word(const uint32_t *hdr) {
    int f = crc_mode - 1;
    const uint8_t *p = (const uint8_t *)hdr + ((f & 8) ? 4 : 0);
    int n = (f & 8) ? PKT_PAYLOAD : 4 + PKT_PAYLOAD;
    uint16_t c = (f & 2) ? 0xFFFF : 0;
    if (f & 1)
        while (n--)
            c = (c >> 8) ^ crc_tab[(c ^ *p++) & 0xFF];
    else
        while (n--)
            c = (c << 8) ^ crc_tab[((c >> 8) ^ *p++) & 0xFF];
    if (f & 4)
        c ^= 0xFFFF;
    return (f & 16) ? (uint32_t)((c >> 8) | ((c & 0xFF) << 8)) : c;
}

static void core1_loop(void) {
    uint64_t period_ns = target_mbs > 0 ? (uint64_t)(PKT_PAYLOAD * 1000.0 / target_mbs) : 0;
    uint64_t next_ns = time_us_64() * 1000;
    int i = 0;
    while (running) {
        uint32_t *b = bufs[i];
        b[0] = PKT_BYTES - 1;
        // header, transmitted LSB first: TLL2B [1:0] = 0, TSQN [5:2], USDF [31:6] = packet number (CH569DS1 10.2.2;
        // the opposite reading, TSQN in [29:26], gave a sequence error on every packet on board #1)
        b[1] = ((tsqn & 0x03FFFFFF) << 6) | ((tsqn & 0xF) << 2);   // USDF = full sequence count (debug)
        tsqn++;
        for (int j = 0; j < PKT_PAYLOAD / 4; j++)
            b[2 + j] = counter++;
        // CRC16 field (2 bytes on the bus): mode 8 = CRC-16/USB, the one the CH569 checks (board #1 sweep)
        b[PKT_WORDS - 1] = crc_mode == 8 ? crc_usb_words(&b[1], 1 + PKT_PAYLOAD / 4) : crc_mode ? crc_word(&b[1]) : 0;
        if (period_ns) {
            while (time_us_64() * 1000 < next_ns)
                tight_loop_contents();
            next_ns += period_ns;
        }
        dma_channel_wait_for_finish_blocking(dma_ch);
        dma_channel_transfer_from_buffer_now(dma_ch, b, PKT_WORDS);
        packets++;
        i = (i + 1) % NBUF;
    }
    dma_channel_wait_for_finish_blocking(dma_ch);
    core1_done = true;
}

static void pins_to_pio(void) {
    for (uint p = PIN_D0; p <= PIN_ACT + 1; p++)
        pio_gpio_init(pio, p);
    gpio_init(PIN_ACK);
    gpio_set_dir(PIN_ACK, GPIO_IN);
    gpio_disable_pulls(PIN_ACK);
    pio_sm_set_pins_with_mask(pio, sm, 0, 0x7FFu << PIN_D0);
    pio_sm_set_consecutive_pindirs(pio, sm, PIN_D0, PIN_ACT + 2 - PIN_D0, true);   // GPIO8..18 outputs
}

// Back to the main firmware's state: inputs without pulls, HSPI_D0 (CH569 boot strap PA5) with its pull-up.
static void pins_release(void) {
    for (uint p = PIN_D0; p <= PIN_ACK; p++) {
        gpio_init(p);
        gpio_disable_pulls(p);
    }
    gpio_pull_up(PIN_D0);
}

bool stream_start(float mbs, float clk_mhz, int crc) {
    if (running)
        return false;
    crc_mode = (crc >= 0 && crc <= 32) ? crc : 0;
    if (crc_mode == 8)
        crc_usb_tables();
    else if (crc_mode)
        crc_table((crc_mode - 1) & 1);
    if (sm < 0) {
        sm = pio_claim_unused_sm(pio, true);
        offset = pio_add_program(pio, &hspi_tx_program);
        dma_ch = dma_claim_unused_channel(true);
    }
    // 2 instructions per clock period: clock = sys / (2 * div)
    float div = clock_get_hz(clk_sys) / (2.0f * clk_mhz * 1e6f);
    if (div < 1.0f)
        div = 1.0f;
    bus_mhz = clock_get_hz(clk_sys) / (2.0f * div) / 1e6f;
    target_mbs = mbs;

    pio_sm_set_enabled(pio, sm, false);
    pio_sm_config c = hspi_tx_program_get_default_config(offset);
    sm_config_set_out_pins(&c, PIN_D0, 8);
    sm_config_set_sideset_pins(&c, PIN_CLK);
    sm_config_set_set_pins(&c, PIN_ACT, 2);
    sm_config_set_jmp_pin(&c, PIN_ACK);
    sm_config_set_out_shift(&c, true, true, 32);                 // LSB byte first, autopull
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    sm_config_set_mov_status(&c, STATUS_TX_LESSTHAN, 1);
    sm_config_set_clkdiv(&c, div);
    pio_sm_init(pio, sm, offset, &c);
    pins_to_pio();
    pio_sm_clear_fifos(pio, sm);
    pio_sm_set_enabled(pio, sm, true);

    dma_channel_config d = dma_channel_get_default_config(dma_ch);
    channel_config_set_transfer_data_size(&d, DMA_SIZE_32);
    channel_config_set_read_increment(&d, true);
    channel_config_set_write_increment(&d, false);
    channel_config_set_dreq(&d, pio_get_dreq(pio, sm, true));
    dma_channel_configure(dma_ch, &d, &pio->txf[sm], bufs[0], 0, false);

    packets = 0;
    counter = 0;
    running = true;
    core1_done = false;
    start_us = time_us_64();
    multicore_reset_core1();
    multicore_launch_core1(core1_loop);
    return true;
}

void stream_stop(void) {
    if (!running && core1_done)
        return;
    running = false;
    absolute_time_t until = make_timeout_time_ms(500);
    while (!core1_done && !time_reached(until))
        tight_loop_contents();
    // let the PIO finish the last packet (or give up if the CH569 never acknowledged it)
    until = make_timeout_time_ms(50);
    while (!pio_sm_is_tx_fifo_empty(pio, sm) && !time_reached(until))
        tight_loop_contents();
    sleep_us(500);
    stop_us = time_us_64();
    pio_sm_set_enabled(pio, sm, false);
    dma_channel_abort(dma_ch);
    pins_release();
}

void stream_print_status(void) {
    uint64_t end = running ? time_us_64() : stop_us;
    double secs = (end > start_us) ? (end - start_us) / 1e6 : 0;
    double bytes = (double)packets * PKT_PAYLOAD;
    printf("stream %s, packets %lu, payload %.0f bytes, %.2f s, %.2f MB/s, target %.1f MB/s, bus %.1f MHz, crc %d\n",
           running ? "on" : "off", (unsigned long)packets, bytes, secs, secs > 0 ? bytes / secs / 1e6 : 0.0,
           target_mbs, bus_mhz, crc_mode);
}
