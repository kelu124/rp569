#!/bin/sh
# Build the RP2350B firmware -> build/rp569_rp2350.uf2 (Pico SDK 2.2, arm-none-eabi-gcc, cmake, ninja).
set -e
cd "$(dirname "$0")"
export PICO_SDK_PATH="${RP569_PICO_SDK:-$HOME/.pico-sdk/sdk/2.2.0}"   # not the shell's PICO_SDK_PATH (2.1)
cmake -S . -B build -G Ninja >/dev/null
ninja -C build
ls -l build/rp569_rp2350.uf2
