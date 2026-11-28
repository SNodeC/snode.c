#!/bin/sh
set -eu
mkdir -p "$OUTPUT_DIR"
cd "$SDK_DIR"
printf 'CONFIG_PACKAGE_snodec=y\n' >> .config
make defconfig
make package/snode.c/compile CMAKE_BUILD_TYPE=Release V=s -j"$(nproc)"
find bin/packages -type f \( -name 'snodec*.ipk' -o -name 'snodec*.apk' \) \
    -exec cp '{}' "$OUTPUT_DIR/" \;
