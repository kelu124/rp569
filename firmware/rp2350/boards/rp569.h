// Board header for the rp569 board (RP2350B side). Pin numbers from hardware/gen/board.py and docs/pinmap.md.
// THIS HEADER IS ALSO INCLUDED BY ASSEMBLER: preprocessor directives only.

#ifndef _BOARDS_RP569_H
#define _BOARDS_RP569_H

pico_board_cmake_set(PICO_PLATFORM, rp2350)

#define RP569_BOARD

// --- RP2350 VARIANT: RP2350B (QFN-80, 48 GPIO) ---
#define PICO_RP2350A 0

// --- UART: UART0 debug on the RP EXT header (pins 2/3) ---
#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 0
#endif
#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 32
#endif
#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 33
#endif

// --- I2C: I2C0 breakout on the RP I2C header (4.7k pull-ups on board) ---
#ifndef PICO_DEFAULT_I2C
#define PICO_DEFAULT_I2C 0
#endif
#ifndef PICO_DEFAULT_I2C_SDA_PIN
#define PICO_DEFAULT_I2C_SDA_PIN 36
#endif
#ifndef PICO_DEFAULT_I2C_SCL_PIN
#define PICO_DEFAULT_I2C_SCL_PIN 37
#endif

// --- FLASH: W25Q128JVS, 16 MB ---
#define PICO_BOOT_STAGE2_CHOOSE_W25Q080 1
#ifndef PICO_FLASH_SPI_CLKDIV
#define PICO_FLASH_SPI_CLKDIV 2
#endif
pico_board_cmake_set_default(PICO_FLASH_SIZE_BYTES, (16 * 1024 * 1024))
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (16 * 1024 * 1024)
#endif

// 12 MHz crystal (Y1), as on the Pico 2: the SDK defaults apply.

pico_board_cmake_set_default(PICO_RP2350_A2_SUPPORTED, 1)
#ifndef PICO_RP2350_A2_SUPPORTED
#define PICO_RP2350_A2_SUPPORTED 1
#endif

// --- rp569 pins (not SDK defaults) ---
#define RP569_LA0_PIN           0   // GPIO0-7: logic analyser inputs (LA IN header, 100R series)
#define RP569_HSPI_D0_PIN       8   // GPIO8-15 HSPI_D0-7, 16-23 HSPI control; HSPI_D0 = CH569 PA5 boot strap
#define RP569_LINK_UART         1   // UART1 to the CH569 UART1 (PA7 RX / PA8 TX)
#define RP569_LINK_TX_PIN       24
#define RP569_LINK_RX_PIN       25
#define RP569_LINK_SDA_PIN      26  // I2C1 shared with the CH569 (bit-banged there)
#define RP569_LINK_SCL_PIN      27
#define RP569_CH_RST_N_PIN      28  // open-drain to CH569 RST# (10k pull-up to 3V3_CH)
#define RP569_CH_PWR_EN_PIN     29  // AP2171W EN, 100k pull-down: CH569 off until driven high
#define RP569_CH_FAULT_N_PIN    30  // AP2171W FLG#, 10k pull-up, low = over-current
#define RP569_BTN1_PIN          31  // active low, 10k pull-up
#define RP569_BTN2_PIN          39
#define RP569_LED_R_PIN         34  // RGB LED, common anode on +3V3: pin low = on
#define RP569_LED_G_PIN         35
#define RP569_LED_B_PIN         38
#define RP569_VBUS3_SENSE_PIN   46  // ADC6: USB3 VBUS through a divider
#define RP569_CH3V3_SENSE_PIN   47  // ADC7: CH569 3V3 through a divider

#endif
