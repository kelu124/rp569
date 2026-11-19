---
title: Toolchain and machine setup
---

# Toolchain and machine setup

What has to be installed on a Linux host (Ubuntu 22.04 here) to build, flash and test the rp569 firmware, and
how to update each piece. Versions are the ones in use on 2026-11-19.

## Summary

| Tool | Version | Installed from | Used for |
|---|---|---|---|
| Pico SDK | 2.2.0 | VS Code *Raspberry Pi Pico* extension → `~/.pico-sdk/sdk/2.2.0` | RP2350 firmware (`firmware/rp2350/build.sh`) |
| arm-none-eabi-gcc | 10.3.1 | apt `gcc-arm-none-eabi` (`/usr/bin`) | RP2350 compiler (also `~/.pico-sdk/toolchain/14_2_Rel1`, unused) |
| cmake / ninja | 3.25.1 / 1.10.1 | apt `cmake` / `ninja-build` | RP2350 build |
| picotool | 2.3.0 | built from source → `/usr/local/bin/picotool` | RP2350 flash / info |
| riscv-none-elf-gcc | 15.2.0 (xPack) | `xpm` → `~/.local/xPacks/@xpack-dev-tools/riscv-none-elf-gcc/` | CH569 firmware (`firmware/ch569/build.sh`) |
| wchisp | 0.3.0 | `cargo install wchisp --locked` → `~/.cargo/bin` | `wchisp info` only: cannot flash CH569 bootloader 2.70 (below) |
| wch-ch56x-isp | 0.3.1 | source, github.com/hydrausb3/wch-ch56x-isp (`git clone --recursive`, `make`, needs `libusb-1.0-0-dev`) | **the CH569 flasher** |
| Rust (rustc / cargo) | 1.75.0 | apt `rustc` / `cargo` | building wchisp |
| Python 3 + pyserial | 3.5 | pip | host library `python/rp569`, tests |
| KiCad | 10.0.8 | KiCad PPA (`kicad-cli`, `pcbnew` in `/usr/bin/python3`) | hardware |

The shell sets `PICO_SDK_PATH=~/projets/pico-sdk` (SDK 2.1.0). `firmware/rp2350/build.sh` ignores it and uses
2.2.0 (override: `RP569_PICO_SDK=/path/to/sdk`). `~/pico/pico-sdk` is SDK 1.5, which has no RP2350 support: do not use it.

## USB permissions (udev)

Without these rules, picotool and the CH569 flashers can only open the boards as root ("appears to be in BOOTSEL
mode, but picotool was unable to connect"). File `/etc/udev/rules.d/60-rp569.rules`:

```
SUBSYSTEM=="usb", ATTRS{idVendor}=="4348", ATTRS{idProduct}=="55e0", MODE="0666"
SUBSYSTEM=="usb", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="55e0", MODE="0666"
SUBSYSTEM=="usb", ATTRS{idVendor}=="2e8a", MODE="0666"
```

| ID | Device |
|---|---|
| `4348:55e0` / `1a86:55e0` | CH569 USB bootloader (WCH uses both vendor IDs) |
| `2e8a:000f` | RP2350 BOOTSEL (ROM bootloader, UF2 drive `RP2350`) |
| `2e8a:0009` | rp569 RP2350B firmware (CDC, product "rp569 RP2350B") |
| `1a86:fe2c` | rp569 CH569 firmware (drive + 2 serial ports at USB3 or USB2, product "rp569 CH569") |

Install it in one go:

```sh
printf 'SUBSYSTEM=="usb", ATTRS{idVendor}=="4348", ATTRS{idProduct}=="55e0", MODE="0666"\nSUBSYSTEM=="usb", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="55e0", MODE="0666"\nSUBSYSTEM=="usb", ATTRS{idVendor}=="2e8a", MODE="0666"\n' \
  | sudo tee /etc/udev/rules.d/60-rp569.rules
sudo udevadm control --reload-rules && sudo udevadm trigger
```

ModemManager probes every new serial port by sending `AT` commands. On the CH569 link port those lines go straight
to the RP2350, so keep it off the rp569 ports with two more lines in the same file:

```
SUBSYSTEM=="usb", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="fe2c", ENV{ID_MM_DEVICE_IGNORE}="1"
SUBSYSTEM=="usb", ATTRS{idVendor}=="2e8a", ATTRS{idProduct}=="0009", ENV{ID_MM_DEVICE_IGNORE}="1"
```

The CDC serial ports (`/dev/ttyACM*`) need the user to be in the `dialout` group (`sudo usermod -aG dialout $USER`,
then log in again); this user already is.

## Flashing

### RP2350B

1. Hold **RPDFU** (SW1), press **RUN**: the RP2350 appears as `2e8a:000f` and mounts a `RP2350` drive.
2. `cp firmware/rp2350/build/rp569_rp2350.uf2 /media/$USER/RP2350/`, or
   `picotool load -x firmware/rp2350/build/rp569_rp2350.uf2`.
3. Running firmware: the `dfu` CLI command reboots into BOOTSEL, or use `picotool load -f -x <uf2>`.

Check: `PYTHONPATH=python python3 -m rp569 versions`.

### CH569W

The RP2350 handles the boot strap: the CLI command `ch boot` power-cycles the CH569 with PA5 held low, and its
bootloader (`4348:55e0`) stays on USB for about 10 s. Run the flasher within that window:

```sh
wch-ch56x-isp -vr -f=firmware/ch569/build/rp569_ch569.bin
```

Unmount the SD drive first (`udisksctl unmount -b /dev/sdX1`): `ch boot` cuts the CH569's power, and a mounted FAT
volume is then left dirty ("Volume was not properly unmounted").

**A new CH569 needs two one-time config changes** (both done on board #1, UID 8A-82-48-26-3B-38-0D-E1, 2026-11-19).
Each step needs its own `ch boot` window:

```sh
wch-ch56x-isp -v -c -d=off   # DEBUG_EN 1 -> 0: with debug on, the bootloader refuses to program ("Fail to program chunk @ 0 error: e0 00")
wch-ch56x-isp -v -c -b=off   # download pin PA13 -> PA5 (ours, held low by the RP2350); invalidates the current image
wch-ch56x-isp -vr -f=firmware/ch569/build/rp569_ch569.bin   # erase, program, verify, reset
```

As delivered, the chip had `DEBUG_EN 1` and `alt boot pin: PA13` (nv `0xCFFFF265`). With PA13 as the download pin,
`ch boot` only works while the code flash is empty: once an image is flashed, only the CH569 button on PA13
(held at power-up) gets back into the bootloader. `-c` prints the config (`DEBUG_EN`, `alt boot pin`, `USER_MEM`) without changing it.
wch-ch56x-isp also sets `USER_MEM` from the image size at every flash: < 32 KB → ROM 32 KB + RAMX 96 KB,
< 64 KB → ROM 64 + RAMX 64, < 96 KB → ROM 96 + RAMX 32. `sdk/Ld/Link.ld` always declares RAMX = 96 KB, so the
linker cannot see the cut. The `.DMADATA` buffers already take 86 KB of RAMX, so an image over 32 KB would overflow RAMX at run
time, with no error at build or flash time. `firmware/ch569/tools/check_mem.py` (run by `make`/`build.sh` and
`build.sh check`) fails the build when RAMX use exceeds what the image size leaves, and prints the margin
(2026-11-19: image 22.1 KB, RAMX 86.1 KB used, 9.9 KB of code left before the cut). wch-ch56x-isp 0.3.1 also refuses
images ≥ 64 KB, because `devices.h` defines `SZ_96K` as `64 * SZ_1K`.

**wchisp 0.3.0 does not work with CH569 bootloader 2.70.** It fails with `isp_key checksum failed`: this bootloader
always replies `00` to the key command. With that check skipped, the program step "succeeds", but wchisp ignores
the bootloader's refusal (the DEBUG_EN problem above), and verify then fails with `Verify failed, mismatch`. Use wch-ch56x-isp.
The bootloader is in ROM, so a failed flash only leaves the code flash empty: run `ch boot` and flash again.

## Updating the tools

| Tool | How to update |
|---|---|
| Rust | apt's 1.75 cannot build current crates (`feature edition2024 is required`); `--locked` works around it. For a recent Rust, install rustup (`curl https://sh.rustup.rs -sSf \| sh`, then `rustup update`). It installs into `~/.cargo/bin`, which comes before `/usr/bin` once `~/.cargo/env` is sourced. Remove the apt one with `sudo apt remove rustc cargo` if they clash. |
| wchisp | `cargo install wchisp --locked --force` (git version: `cargo install --git https://github.com/ch32-rs/wchisp --locked --force`) |
| wch-ch56x-isp | `git -C <clone> pull && git -C <clone> submodule update --init && make -C <clone>`, then copy the binary to `~/.local/bin` |
| riscv-none-elf-gcc | `xpm install --global @xpack-dev-tools/riscv-none-elf-gcc@latest`. It installs next to the old version under `~/.local/xPacks/...`. `firmware/ch569/build.sh` picks the highest version (`sort -V`) unless a `riscv-none-elf-gcc` is already on `PATH`. Rebuild, then run `build.sh check`. |
| Pico SDK | VS Code Pico extension (it downloads SDKs into `~/.pico-sdk/sdk/<ver>`), or `git clone -b <ver> https://github.com/raspberrypi/pico-sdk && git submodule update --init`. Then point `RP569_PICO_SDK` at it or change the default in `firmware/rp2350/build.sh`. |
| picotool | `git clone https://github.com/raspberrypi/picotool && cmake -S picotool -B picotool/build -DPICO_SDK_PATH=~/.pico-sdk/sdk/2.2.0 && cmake --build picotool/build && sudo cmake --install picotool/build`. Its repo ships `udev/60-picotool.rules`, which does the same as the `2e8a` line above. |
| arm-none-eabi-gcc | `sudo apt upgrade gcc-arm-none-eabi`, or use the Arm toolchain the Pico extension installs (`~/.pico-sdk/toolchain/<ver>/bin` on `PATH`) |
| KiCad | `sudo apt update && sudo apt upgrade kicad` (PPA `kicad/kicad-10.0-releases`) |
| pyserial | `pip install -U pyserial` |

After any toolchain update: run both `build.sh` (they must finish without compiler warnings),
`firmware/ch569/build.sh check`, and `cd python && python3 -m unittest discover -s tests`.

## Docker alternative (CH569 build)

The sister project `ultrasounds/we_fpga_hardware/ch569/sd` builds CH569 firmware in Docker: `debian:bookworm-slim`
with the xPack riscv-none-elf-gcc tarball (12.2.0-3) unpacked to `/opt/xpack`, and the source tree mounted at `/work`
(`docker compose run --rm build`). The same pattern would work here, with `XPACK_VERSION=15.2.0-1` and the command
`firmware/ch569/build.sh`. It covers only the build: flashing needs USB access from the host.
