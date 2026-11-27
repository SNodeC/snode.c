#!/bin/sh
set -eu
recipe="$SDK_DIR/feeds/base/snode.c"
rm -rf "$recipe"
mkdir -p "$recipe" "$SDK_DIR/dl" "$OUTPUT_DIR"
ln -s "$recipe" "$SDK_DIR/package/snode.c"
cp -R "$CI_DIR/openwrt/." "$recipe/"
{
    printf 'SNODEC_PACKAGE_VERSION:=%s\n' "$PACKAGE_VERSION"
    printf 'SNODEC_PACKAGE_RELEASE:=%s\n' "$PACKAGE_RELEASE"
    printf 'SNODEC_SOURCE_DIR:=%s\n' "$SOURCE_DIR"
    cat "$CI_DIR/openwrt/Makefile"
} > "$recipe/Makefile"
cd "$SDK_DIR"
printf 'CONFIG_PACKAGE_snodec=y\n' >> .config
make defconfig
make package/snode.c/compile CMAKE_BUILD_TYPE=Release V=s -j"$(nproc)"
find bin/packages -type f \( -name 'snodec*.ipk' -o -name 'snodec*.apk' \) \
    -exec cp '{}' "$OUTPUT_DIR/" \;
