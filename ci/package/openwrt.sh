#!/bin/sh
set -eu
recipe="$SDK_DIR/feeds/base/snode.c"
mkdir -p "$recipe" "$SDK_DIR/dl" "$OUTPUT_DIR"
ln -s "$recipe" "$SDK_DIR/package/snode.c"
cp -R "$SOURCE_DIR/supplement/openwrt/." "$recipe/"
commit=$(git -C "$SOURCE_DIR" rev-parse HEAD)
{
    printf 'SNODEC_PACKAGE_VERSION:=%s\n' "$PACKAGE_VERSION"
    printf 'SNODEC_PACKAGE_RELEASE:=%s\n' "$PACKAGE_RELEASE"
    printf 'SNODEC_SOURCE_TAG:=%s\n' "$commit"
    cat "$SOURCE_DIR/supplement/openwrt/Makefile"
} > "$recipe/Makefile"
git -C "$SOURCE_DIR" archive --format=tar.gz --prefix="snode.c-$PACKAGE_VERSION/" \
    -o "$SDK_DIR/dl/snode.c-$PACKAGE_VERSION.tar.gz" HEAD
cd "$SDK_DIR"
printf 'CONFIG_PACKAGE_snodec=y\n' >> .config
make defconfig
make package/snode.c/compile CMAKE_BUILD_TYPE=Release V=s -j"$(nproc)"
find bin/packages -type f \( -name 'snodec*.ipk' -o -name 'snodec*.apk' \) \
    -exec cp '{}' "$OUTPUT_DIR/" \;
