#!/bin/sh
set -eu
sdk_stage=$1
package_stage=$2
found=$(PKG_CONFIG_PATH= PKG_CONFIG_LIBDIR="$sdk_stage/usr/lib/pkgconfig:$sdk_stage/usr/share/pkgconfig" \
    pkg-config --modversion nlohmann_json) || found=missing
if [ "$found" != 3.11.2 ]; then
    printf 'nlohmann_json %s ≠ 3.11.2 expected by ci/\n' "$found" >&2
    exit 1
fi

# Keep the upstream install layout and resolve its headers against the SDK.
config="$package_stage/share/cmake/nlohmann_json"
mkdir -p "$config"
ln -sfn "$sdk_stage/usr/include" "$package_stage/include"
cp "$(dirname "$0")"/nlohmann_json/*.cmake "$config/"
