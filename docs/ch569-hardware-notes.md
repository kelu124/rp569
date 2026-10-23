# CH569W hardware notes

Taken from `datasheet/CH569DS1.pdf` and from phantomdrive's KiCad design
(`project/phantomdrive/ee/ovrdrive/`, netlist exported with `kicad-cli sch export netlist`).

## Power (phantomdrive pattern)

| Rail | Source in phantomdrive | CH569 pins |
|---|---|---|
| +3V3 | TLV62569DBV buck from VBUS, 2.2 µH | VDDIO 16/42/61, V33USB 3, V33LDO 62, V33GX 64, SD card VDD |
| +1V2 | TLV62569DBV buck from VBUS, 2.2 µH | V12CORE 9 and 63, V12USB 6 |

- The datasheet describes pin 63 as the *output* of the internal 1.2 V LDO (3.3 µF). phantomdrive
  overrides it with an external 1.2 V buck, probably to keep the USB3 PHY current off the internal LDO.
  We follow phantomdrive (proven design). Check the datasheet's electrical section for the CH569's current draw.
- Decoupling: 100 nF per supply pin, plus 22 µF bulk per rail (phantomdrive uses 0402 22 µF parts).

## Clock

- 30 MHz crystal is required (datasheet §3, HSE feeds the USB PHY). phantomdrive: 3225 4-pin, 15 pF load caps.

## Boot, reset, flashing

- **Boot pin = PA5 (pin 15)**, active low at power-up (phantomdrive: push-button to GND). Can be
  reassigned to PA13 with `wch-ch56x-isp -b on`.
- USB bootloader enumerates as `4348:55E0` and times out after ~10 s.
- **RST# on PB15 is disabled by default** (config bit RESET_EN = 0). phantomdrive still routes RST# to a
  test point with a pull-up.
- If config `DEBUG_EN = 1`, the USB bootloader refuses to flash; the 2-wire debug (PA10 TCK, PA11 TIO)
  is then needed.

## USB3

- phantomdrive: USB3 Type-A plug (Würth 692112030100), TPD2EUSB30 ESD on D±, SSRX± and SSTX±,
  100 nF series caps on SSTX between the chip and the connector.

## SD card

- 4-bit on the eMMC controller: CLK PB14, CMD PB16, D0–D3 PB17–PB20. 47k pull-ups on CMD/D0–D3
  (and CLK via the resistor array). Card detect on PA15. Hirose DM3AT micro-SD socket.
