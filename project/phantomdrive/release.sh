#!/usr/bin/env bash

set -euo pipefail

version=$(git describe --tags --exact-match 2>/dev/null || git rev-parse --short HEAD)
[ -z "$(git status --porcelain -- src/)" ] || version="$(git rev-parse --short HEAD)-dirty"
printf '%s\n' "$version"

make clean
make AES_MODE=CTR KDF_ROUNDS=100000
make AES_MODE=CTR KDF_ROUNDS=600000
make AES_MODE=XTS KDF_ROUNDS=100000
make AES_MODE=XTS KDF_ROUNDS=600000

cp build/CTR_100K/Phantomdrive_MSC.bin build/phantomdrive_ctr_100k_$version.bin
cp build/CTR_600K/Phantomdrive_MSC.bin build/phantomdrive_ctr_600k_$version.bin
cp build/XTS_100K/Phantomdrive_MSC.bin build/phantomdrive_xts_100k_$version.bin
cp build/XTS_600K/Phantomdrive_MSC.bin build/phantomdrive_xts_600k_$version.bin
