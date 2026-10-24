# Pin map — RP2350B ↔ CH569W

Status: **v0.2 (2026-10-24, pins reassigned by chip side, D-018)**. Sources: `datasheet/CH569DS1.pdf` Table 1-2 (CH569 QFN-68 pin
definitions) and Table 10-1 (HSPI interface); `datasheet/rp2350-datasheet.pdf` GPIO function table
(§9.4, F1–F11). Pin numbers checked against the PDFs, not the Markdown conversions.

## 1. Inter-chip HSPI link (8-bit, D-003)

CH569 is the HSPI **receiver** for the main data path (RP2350 → CH569 → USB3). Its HSPI transmitter
control lines are also wired to the RP2350 so the reverse direction is possible later (HSPI data lines are
bidirectional; half-duplex).

| RP2350B GPIO | Net | Dir (from RP2350) | CH569 pin | CH569 function | Notes |
|---|---|---|---|---|---|
| GPIO8  | HSPI_D0 | out | 15 PA5  | HD0 | **Also the CH569 boot pin** (low at power-up → USB bootloader) |
| GPIO9  | HSPI_D1 | out | 17 PA4  | HD1 | |
| GPIO10 | HSPI_D2 | out | 53 PA22 | HD2 | |
| GPIO11 | HSPI_D3 | out | 19 PA3  | HD3 | |
| GPIO12 | HSPI_D4 | out | 20 PA2  | HD4 | (= eMMC MD7, unused in 4-bit SD) |
| GPIO13 | HSPI_D5 | out | 21 PA1  | HD5 | (= eMMC MD6) |
| GPIO14 | HSPI_D6 | out | 22 PA0  | HD6 | (= eMMC MD5) |
| GPIO15 | HSPI_D7 | out | 23 PB21 | HD7 | (= eMMC MD4) |
| GPIO16 | HSPI_RCLK | out | 10 PA19 | HRCLK | Sampling clock for the CH569 receiver (PIO side-set) |
| GPIO17 | HSPI_RACT | out | 11 PA18 | HRACT | Transmit request (RP2350 HTREQ role) |
| GPIO18 | HSPI_RVLD | out | 14 PA6  | HRVLD | Data valid (RP2350 HTVLD role) |
| GPIO19 | HSPI_RACK | in  | 55 PA10 | HTACK | CH569 ready-to-receive. **Shares pin with TCK (2-wire debug)** |
| GPIO20 | HSPI_TCLK | in  | 56 PA11 | HTCLK | CH569 → RP2350 direction (future). **Shares pin with TIO (debug)** |
| GPIO21 | HSPI_TREQ | in  | 54 PA9  | HTREQ | CH569 → RP2350 (future) |
| GPIO22 | HSPI_TVLD | in  | 52 PA21 | HTVLD | CH569 → RP2350 (future) |
| GPIO23 | HSPI_TRDY | out | 18 PA23 | HTRDY | CH569 → RP2350 (future) |

GPIO0–23 sit in one 32-GPIO PIO window (GPIOBASE = 0), so a single PIO block can sample the logic
analyser inputs and drive the bus.

## 2. Logic analyser inputs (RP2350B)

| RP2350B GPIO | Net | Notes |
|---|---|---|
| GPIO0–GPIO7 | LA0–LA7 | Consecutive for PIO `in pins, 8`. 3.3 V logic; series resistor (e.g. 33–100 Ω) + header with GND pins |

## 3. Other inter-chip signals

| RP2350B GPIO | Function | Net | CH569 pin | CH569 function | Notes |
|---|---|---|---|---|---|
| GPIO24 | UART1 TX | RP_TX_CH_RX | 13 PA7 | RXD1 | Inter-chip UART (same UART phantomdrive uses for debug) |
| GPIO25 | UART1 RX | CH_TX_RP_RX | 12 PA8 | TXD1 | |
| GPIO26 | I2C1 SDA | I2C_LINK_SDA | 59 PA14 | GPIO (bit-bang) | Shared inter-chip I2C, pull-ups 4.7k to 3V3 |
| GPIO27 | I2C1 SCL | I2C_LINK_SCL | 60 PA15 | GPIO (bit-bang) | |
| GPIO29 | GPIO out | CH569_PWR_EN | — | — | Load-switch EN for 5V_CH (CH569 power from its USB3 VBUS), pull-down. See `docs/power.md` (D-004, D-007) |
| GPIO30 | GPIO in | CH569_PWR_FAULT# | — | — | Load-switch fault (open-drain) |
| GPIO46 | ADC6 | VBUS3_SENSE | — | — | USB3 VBUS present/level (divider) |
| GPIO47 | ADC7 | CH569_3V3_SENSE | — | — | CH569 3V3 rail monitor (divider) |
| GPIO28 | GPIO out (open-drain) | CH569_RST# | 30 PB15 | RST# | Only works if config bit RESET_EN = 1 (default 0). 10k pull-up |

## 4. RP2350B local I/O

| RP2350B GPIO | Function | Net | Notes |
|---|---|---|---|
| GPIO36 | I2C0 SDA | RP_I2C_SDA | Breakout header (3V3, SDA, SCL, GND), 4.7k pull-ups |
| GPIO37 | I2C0 SCL | RP_I2C_SCL | |
| GPIO32 | UART0 TX | RP_DBG_TX | Debug UART, on the RP EXT header (pins 2/3) |
| GPIO33 | UART0 RX | RP_DBG_RX | |
| GPIO34 | PWM9 A | RP_LED_R | Discrete RGB LED (D-005), active low via series R |
| GPIO35 | PWM9 B | RP_LED_G | |
| GPIO38 | PWM11 A | RP_LED_B | |
| GPIO40–45 | ADC0–5 / GPIO | EXT0–5 | RP EXT header 1x10: 3V3, DBG_TX, DBG_RX, ADC0–5, GND |
| GPIO31, GPIO39 | GPIO | RP_BTN1, RP_BTN2 | User buttons to GND, 10k pull-ups to +3V3 |
| — | USB DP/DM | USB-C | Dedicated pins |
| — | QSPI | Flash + BOOTSEL | Dedicated pins |
| — | SWCLK/SWDIO | SWD header | Dedicated pins |

## 5. CH569W local I/O

| CH569 pin | Name | Net | Notes |
|---|---|---|---|
| 1, 2 | UD+/UD- | USB_D± | USB2 lines of the USB3 connector (also used by the USB bootloader) |
| 4, 5 / 7, 8 | SSTX± / SSRX± | USB_SS* | USB3 SuperSpeed, 100 nF series caps on TX, ESD (TPD2EUSB30 in phantomdrive) |
| 31 PB14 | MSDCK | SD_CLK | SD clock |
| 29 PB16 | MCMD | SD_CMD | 47k pull-up |
| 27–24 PB17–PB20 | MD0–MD3 | SD_D0–D3 | 47k pull-ups |
| 35 PA20 | GPIO | SD_DET | Card detect, next to the socket (D-018) |
| 32 PB0 | TMR1_ (PWM) | CH_LED_R | Discrete RGB LED (D-005) |
| 33 PB1 | PWM2 | CH_LED_G | |
| 34 PB2 | PWM3 | CH_LED_B | |
| 50 PB12 | GPIO (bit-bang) | CH_I2C_SDA | Breakout header, 4.7k pull-ups |
| 51 PB13 | GPIO (bit-bang) | CH_I2C_SCL | |
| 39 PB6 / 38 PB5 | TXD0 / RXD0 | CH_DBG_TX/RX | CH569 debug UART header |
| 57 PA12 / 58 PA13 | GPIO | CH_BTN1, CH_BTN2 | User buttons to GND, 10k pull-ups to 3V3_CH |
| 15 PA5 | HD0 / boot | CH_BOOT_SW (via 1k) | BOOT button + 2-pin jumper `CH BOOT JP` to GND: fitted = bootloader at every power-up |
| 67, 68 | XO/XI | — | 30 MHz crystal, 15 pF |
| 65, 66 | GXM/GXP | — | SerDes, not connected |
| 28 PA17, 36 PB3, 37 PB4, 40 PA16, 41 PB7, 43 PB8, 44 PB9, 45 PB22, 46 PB10, 47 PB23, 48 PB11, 49 PB24 | — | CH_EXTx | Spare header 2x07 (12 GPIO in this order + 3V3_CH + GND) |

## 6. Constraints to respect in firmware / schematic

- **Back-powering**: when CH569 is powered off, the RP2350 must tri-state every line to the CH569 except
  HSPI_D0 (held low to select the bootloader, or released). The HSPI lines have no series resistors (D-018):
  set the RP2350 pads to 4 mA drive and slow slew if edges ring.
- **Bootloader entry**: RP2350 drives HSPI_D0 (PA5) low → asserts CH569_PWR_EN → waits → releases.
  CH569 enumerates as `4348:55E0` on the USB3 connector's USB2 lines; host runs `wch-ch56x-isp`.
  The bootloader times out after ~10 s without activity.
- **2-wire debug** (PA10 TCK / PA11 TIO) is shared with HTACK/HTCLK, so it is unavailable while the
  HSPI link is in use. Flashing uses USB ISP instead.
