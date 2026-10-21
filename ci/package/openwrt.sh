#!/usr/bin/env bash
cp -a source/supplement/openwrt/. recipes/snode.c/
export SNODEC_PACKAGE_RELEASE="$PACKAGE_RELEASE"
export SNODEC_SOURCE_TAG=$(jq -r '.source_tags["snode.c"]' "$bundle/context.json")
export SNODEC_PACKAGE_VERSION=$(jq -r '.versions["snode.c"]' "$bundle/context.json")
export SNODEC_SOURCE_HASH=$(sha256sum "$bundle/snode.c-"*.tar.gz | cut -d' ' -f1)
compile() {
    make -j"$(nproc)" package/snode.c/compile V=s BUILD_LOG=1
    python3 "$sdk/source/ci/package/openwrt-tests.py" "$sdk"
    local target
    target=$(echo staging_dir/target-*)
    python3 -m ci.repository sdk-export "$sdk" "$bundle" recipes/snode.c \
        "$target/usr/include" "$target/usr/lib" "$target"/pkginfo/*.provides
    return 0
}
