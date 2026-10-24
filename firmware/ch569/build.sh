#!/bin/sh
# Build the CH569 firmware -> build/rp569_ch569.{bin,hex} (xPack riscv-none-elf-gcc + make).
set -e
cd "$(dirname "$0")"
XPACK="$HOME/.local/xPacks/@xpack-dev-tools/riscv-none-elf-gcc"
command -v riscv-none-elf-gcc >/dev/null || PATH="$(ls -d "$XPACK"/*/.content/bin | sort -V | tail -1):$PATH"
make "$@"
