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

## D-002 — RP2350 → CH569 data link uses CH569 HSPI in 16-bit mode (status: proposed)
Context: Need high-throughput RP2350 → CH569 → USB3 path.
Choice: RP2350 PIO emulates an HSPI transmitter (16 data + clock + handshake lines).
Alternatives: SerDes (not reachable from RP2350), SPI (too slow).
Open: exact HSPI handshake, max clock achievable with RP2350 PIO + board routing.
