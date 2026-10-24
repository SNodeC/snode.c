#!/usr/bin/env bash
cp -a source/supplement/openwrt/. recipes/snode.c/
export SNODEC_PACKAGE_RELEASE="$PACKAGE_RELEASE"
export SNODEC_SOURCE_TAG=$(jq -r '.source_tags["snode.c"]' "$bundle/context.json")
export SNODEC_PACKAGE_VERSION=$(jq -r '.versions["snode.c"]' "$bundle/context.json")
export SNODEC_SOURCE_HASH=$(sha256sum "$bundle/snode.c-"*.tar.gz | cut -d' ' -f1)
compile() {
    make -j"$(nproc)" package/snode.c/compile V=s BUILD_LOG=1
    python3 "$sdk/source/ci/package/openwrt-tests.py" "$sdk"
    local target file
    local -a files=(recipes/snode.c)
    target=$(echo staging_dir/target-*)
    while IFS= read -r file; do
        if [[ -f "$target/$file" || -L "$target/$file" ]]; then
            files+=("$target/$file")
        fi
    done < "$target/packages/snode.c.list"
    for file in "$target"/pkginfo/snodec*.provides; do
        if [[ -f "$file" ]]; then
            files+=("$file")
        fi
    done
    python3 -m ci.repository sdk-export "$sdk" "$bundle" "${files[@]}"
    return 0
}
