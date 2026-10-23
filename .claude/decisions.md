# Design decisions

<!-- Format:
## D-NNN — title (status: proposed | accepted | superseded)
Context / Choice / Alternatives / Rationale / Source (datasheet section)
-->

## D-001 — Separate crystals per chip (status: proposed)
Context: User wants a clock shared between RP2350B and CH569.
Choice: Each chip keeps its own reference crystal (CH569 30 MHz, RP2350 12 MHz). The shared
clock is the parallel-bus data clock, driven by RP2350 PIO toward CH569 HSPI.
Rationale: CH569 USB3 PLL has tight jitter requirements; a GPIO-generated clock is a risk.

## D-003 — Parallel bus width vs SD card pin conflict (status: OPEN, needs user choice)
Context: CH569DS1 pin table: HSPI HD4..HD7 = PA2,PA1,PA0,PB21 (= eMMC MD7..MD4),
HD8..HD11 = PB20..PB17 (= MD3..MD0), HD13 = PB16 (= MCMD). A 4-bit SD card needs MD0..MD3 + MCMD + clock.
Consequence: 16-bit HSPI and an SD card on the eMMC controller cannot coexist. 8-bit HSPI (HD0..HD7) + 4-bit SD can.
Options: (a) 8-bit HSPI + 4-bit SD on eMMC controller (fits the 8-channel logic analyser naturally);
(b) 16-bit HSPI + SD in SPI mode on CH569 SPI0/SPI1 (slow, ~few MB/s);
(c) 16-bit HSPI, no SD card.
Source: datasheet/CH569DS1.pdf, pin description table (pins 15–29, QFN68). Verify in the PDF.

## D-002 — RP2350 → CH569 data link uses CH569 HSPI in 16-bit mode (status: proposed)
Context: Need high-throughput RP2350 → CH569 → USB3 path.
Choice: RP2350 PIO emulates an HSPI transmitter (16 data + clock + handshake lines).
Alternatives: SerDes (not reachable from RP2350), SPI (too slow).
Open: exact HSPI handshake, max clock achievable with RP2350 PIO + board routing.
