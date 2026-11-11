#!/bin/sh
# Source-owned build policy; dependency installation belongs to trusted preparation.
set -eu
: "${SOURCE_DIR:?}" "${TARGET_JSON:?}" "${OUTPUT_DIR:?}"
format=$(python3 -c 'import json,os; print(json.load(open(os.environ["TARGET_JSON"]))["build"]["format"])')
if [ "$format" = deb ]; then
    libdir="lib/$(dpkg-architecture -qDEB_HOST_MULTIARCH)"
else
    libdir=lib64
    if [ -f /opt/rh/gcc-toolset-14/enable ]; then
        . /opt/rh/gcc-toolset-14/enable
    fi
fi
build=/work/build-product
runtime=
if python3 -c 'import json,os,sys; t=json.load(open(os.environ["TARGET_JSON"])); sys.exit(not (t.get("distribution")=="fedora" and t.get("arch")=="aarch64"))'; then
    runtime=/work/mariadb-runtime
    python3 "$CI_DIR/ci/package/mariadb_runtime.py" "$runtime"
    export LD_LIBRARY_PATH="$runtime/usr/lib64"
fi
cmake -S "$SOURCE_DIR" -B "$build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_INSTALL_LIBDIR="$libdir" -DCMAKE_INSTALL_SYSCONFDIR=/etc \
    -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSPDLOG_SYSTEM_INCLUDES=ON \
    -DSNODEC_MARIADB_RUNTIME="$runtime" \
    -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES="$CI_DIR/ci/package/mariadb-runtime.cmake"
cmake --build "$build" --parallel "$(nproc)"
ctest --test-dir "$build" --output-on-failure --output-junit core-ci-tests.xml
python3 /tool/native_container.py --package "$build"
