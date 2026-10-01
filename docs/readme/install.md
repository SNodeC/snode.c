# Install SNode.C

[← SNode.C](../../README.md)

## Choose your route

| Goal | Route |
| --- | --- |
| Binary packages | [Signed distribution packages](packages.md). |
| OpenWrt | [Packages and SDK route](packages.md#openwrt). |
| Build from source | [Develop against current source or customize a build](#build-from-source). |

## Install binary packages

Follow the [package guide](packages.md) to prepare the signed feed for your distribution, release and architecture. On Debian/Ubuntu and Raspberry Pi OS, install the framework with:

```sh
sudo apt-get install snodec
```

The package guide also covers RPM-based systems, OpenWrt and component selections.

## Build from source

### Requirements

SNode.C requires a C++20 compiler and CMake 3.18 or newer. Its configure checks require GCC 12.2+ or Clang 13+. Optional development libraries enable Bluetooth, file-type detection, MariaDB and the configuration tool’s terminal UI.

On Debian/Ubuntu, install the build tools and development libraries:

```sh
sudo apt-get update
sudo apt-get install git cmake ninja-build g++ pkg-config \
  libssl-dev nlohmann-json3-dev libbluetooth-dev libmagic-dev \
  libmariadb-dev libncurses-dev
```

The JSON-dependent modules require `nlohmann_json` 3.11 or newer. Package names and available versions differ on other distributions; use their development equivalents or choose the [binary package route](packages.md).

### Build and install

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

For a development build with tests enabled:

```sh
cmake -S . -B build-check -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DSNODEC_BUILD_TESTS=ON
cmake --build build-check --parallel
ctest --test-dir build-check --output-on-failure --parallel
```

Hardware-specific capabilities still need an appropriate device and environment. See the framework’s [network test notes](https://github.com/SNodeC/snode.c/blob/master/tests/component/net/README.md).

## Verify the installation

For a development installation with the plain IPv4 stream component, save this as `CMakeLists.txt` in an empty directory:

```cmake
cmake_minimum_required(VERSION 3.18)
project(check_snodec LANGUAGES CXX)
find_package(snodec REQUIRED COMPONENTS net-in-stream-legacy)
```

```sh
cmake -S . -B build
```

A successful configure confirms that CMake can find that installed development component. A runtime-only package selection does not include a consumer development environment; verify it through the application that uses it instead.

## Next step

Use the installed package’s component targets instead of manually assembling library filenames:

```cmake
find_package(snodec REQUIRED COMPONENTS net-in-stream-legacy)
target_link_libraries(my-service PRIVATE snodec::net-in-stream-legacy)
```

[Build the native client and server →](../../README.md#your-first-program-a-factory-and-a-context)
