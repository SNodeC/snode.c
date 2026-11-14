#!/bin/sh
set -eu
family=$(printf '%s' "$TARGET_JSON" | jq -r .family)
build=/tmp/codex/build/snodec/wave
mkdir -p "$build" "$OUTPUT_DIR"
export TMPDIR="$build" QT_QPA_PLATFORM=offscreen
cmake -S "$SOURCE_DIR" -B "$build" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr -DSNODEC_BUILD_TESTS=ON
cmake --build "$build" --parallel "$(nproc)"
ctest --test-dir "$build" -LE mariadb --output-on-failure
case "$family" in
    deb)
        cpack --config "$build/CPackConfig.cmake" -G DEB -B "$OUTPUT_DIR" \
            -D "CPACK_PACKAGE_VERSION=$PACKAGE_VERSION-$PACKAGE_RELEASE"
        ;;
    rpm)
        cpack --config "$build/CPackConfig.cmake" -G RPM -B "$OUTPUT_DIR" \
            -D "CPACK_PACKAGE_VERSION=$PACKAGE_VERSION" \
            -D "CPACK_RPM_PACKAGE_RELEASE=$PACKAGE_RELEASE"
        ;;
esac
