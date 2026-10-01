# Install SNode.C

[← SNode.C](../../README.md)

## Choose your route

| Goal | Route |
| --- | --- |
| Use the framework without compiling it | [Signed distribution packages](packages.md). |
| Develop against current source or customize a build | [Build from source](#build-from-source). |
| Cross-compile for a router | [OpenWrt packages and SDK route](packages.md#openwrt). |
| Build your first application | [Factory/Context example and its CMake project](../../README.md#your-first-program-a-factory-and-a-context). |

## Build from source

SNode.C requires a C++20 compiler and CMake 3.18 or newer. Its configure checks require GCC 12.2+ or Clang 13+. Optional development libraries enable Bluetooth, file-type detection, MariaDB and the configuration tool’s terminal UI.

On Debian/Ubuntu, install the build tools and development libraries:

```sh
sudo apt-get update
sudo apt-get install git cmake ninja-build g++ pkg-config \
  libssl-dev nlohmann-json3-dev libbluetooth-dev libmagic-dev \
  libmariadb-dev libncurses-dev
```

The JSON-dependent modules require `nlohmann_json` 3.11 or newer. Package names and available versions differ on other distributions; use their development equivalents or choose the [binary package route](packages.md).

```sh
git clone https://github.com/SNodeC/snode.c.git
cd snode.c
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build --parallel
sudo cmake --install build
sudo ldconfig
```

These commands build the checked-out source, not a pinned release. Record `git rev-parse HEAD` when you need to reproduce a deployment. For a packaged release, use a coherent package set rather than mixing source-installed and distribution-installed libraries with the same names.

To install under a non-system prefix, change `CMAKE_INSTALL_PREFIX`, omit `sudo` when that prefix is writable, and provide it to consumer projects through `CMAKE_PREFIX_PATH`. Configure the runtime library search path for that prefix as well; finding a library at build time does not make it discoverable at runtime.

### Select build features

| CMake option | Purpose |
| --- | --- |
| `SNODEC_BUILD_APPS=OFF` | Omit demonstration applications. |
| `SNODEC_BUILD_TESTS=ON` | Build the framework’s test suite. |
| `SNODEC_ENABLE_ASAN=ON` | Enable AddressSanitizer for a development build. |
| `SNODEC_CONTROL_BUILD_TUI=OFF` | Build `snodec-control` without its optional Curses UI. |

The OpenWrt feed additionally offers package-specific component selections. Do not assume feed configuration symbols are upstream CMake options.

## Development checks

```sh
cmake -S . -B build-check -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DSNODEC_BUILD_TESTS=ON
cmake --build build-check --parallel
ctest --test-dir build-check --output-on-failure --parallel
```

Hardware-specific capabilities still need an appropriate device and environment. See the framework’s [network test notes](https://github.com/SNodeC/snode.c/blob/master/tests/component/net/README.md).

## Link an application

Use the installed package’s component targets instead of manually assembling library filenames:

```cmake
find_package(snodec REQUIRED COMPONENTS net-in-stream-legacy)
target_link_libraries(my-service PRIVATE snodec::net-in-stream-legacy)
```

[Complete runnable example →](examples.md)
