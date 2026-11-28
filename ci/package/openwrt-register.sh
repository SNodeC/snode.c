#!/bin/sh
set -eu
recipe="$SDK_DIR/package/snode.c"
mkdir "$recipe"
cp -R "$CI_DIR/openwrt/." "$recipe/"
{
    printf 'SNODEC_PACKAGE_VERSION:=%s\n' "$PACKAGE_VERSION"
    printf 'SNODEC_PACKAGE_RELEASE:=%s\n' "$PACKAGE_RELEASE"
    printf 'SNODEC_SOURCE_DIR:=%s\n' "$SOURCE_DIR"
    cat "$CI_DIR/openwrt/Makefile"
} > "$recipe/Makefile"
