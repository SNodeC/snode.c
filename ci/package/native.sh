#!/usr/bin/env bash
# Product dependencies and build policy are versioned with this source.
if [ "$format" = DEB ]; then
    apt-get install -y build-essential cmake ninja-build pkg-config \
        libbluetooth-dev libmagic-dev libmariadb-dev libssl-dev openssl libncurses-dev nlohmann-json3-dev libsqlite3-dev
else
    dnf install -y gcc gcc-c++ cmake ninja-build make pkgconf-pkg-config rpm-build \
        bluez-libs-devel file-devel mariadb-connector-c-devel openssl-devel openssl ncurses-devel json-devel sqlite-devel
    if [ "$DISTRIBUTION:$SUITE" = rocky:9 ]; then
        dnf install -y gcc-toolset-14-gcc-c++
        source /opt/rh/gcc-toolset-14/enable
    fi
fi
cmake -S sources/snode.c -B build-snodec -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_INSTALL_LIBDIR="$libdir" -DCMAKE_INSTALL_SYSCONFDIR=/etc \
    -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON \
    -DSPDLOG_SYSTEM_INCLUDES=ON
cmake --build build-snodec --parallel 4
package build-snodec
install_packages /work/packages/*."${format,,}"
chown -R package-test:package-test build-snodec
runuser -u package-test -- ctest --test-dir build-snodec --output-on-failure
