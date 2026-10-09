# Build and install SNode.C from source

<p>
  <a href="../../README.md"><img src="media/menu/back-snode-c.svg" alt="← SNode.C" width="96" height="24"></a>
</p>

<p>
  <a href="#native-linux-build" title="Native Linux build"><img src="media/install-native-linux.svg" alt="Native Linux build" width="222" height="24"></a>
  <a href="#openwrt-cross-compilation" title="OpenWrt cross-compilation"><img src="media/install-openwrt.svg" alt="OpenWrt cross-compilation" width="222" height="24"></a>
</p>

**Prefer prebuilt binaries?** See [Packages](https://github.com/SNodeC/Packages#readme) for supported distributions and installation instructions.

## Native Linux build

Build and install on the Linux machine that will run the software.

### Requirements

SNode.C requires a C++20 compiler and CMake 3.18 or newer. Its configure checks require GCC 12.2+ or Clang 13+. Optional development libraries enable Bluetooth, file-type detection, MariaDB and the configuration tool’s terminal UI.

On Debian/Ubuntu, install the build tools and development libraries:

```text
sudo apt-get update
sudo apt-get install git cmake ninja-build g++ pkg-config \
  libssl-dev nlohmann-json3-dev libbluetooth-dev libmagic-dev \
  libmariadb-dev libncurses-dev
```

The JSON-dependent modules require `nlohmann_json` 3.11 or newer. Package names and available versions differ on other distributions; use their development equivalents or choose the [prebuilt packages](https://github.com/SNodeC/Packages#readme).

### Build and install

```text
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
| `SNODEC_BUILD_APPS=OFF` | Omit demos |
| `SNODEC_BUILD_TESTS=ON` | Enable tests |
| `SNODEC_ENABLE_ASAN=ON` | Enable ASan |
| `SNODEC_CONTROL_BUILD_TUI=OFF` | CLI only |

`SNODEC_CONTROL_BUILD_TUI=OFF` builds `snodec-control` without its optional Curses terminal UI. AddressSanitizer (ASan) helps detect memory errors during development.

For a development build with tests enabled:

```text
cmake -S . -B build-check -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DSNODEC_BUILD_TESTS=ON
cmake --build build-check --parallel
ctest --test-dir build-check --output-on-failure --parallel
```

Hardware-specific capabilities still need an appropriate device and environment. See the framework’s [network test notes](https://github.com/SNodeC/snode.c/blob/master/tests/component/net/README.md).

### Verify the installation

For a development installation with the plain IPv4 stream component, save this as `CMakeLists.txt` in an empty directory:

```cmake
cmake_minimum_required(VERSION 3.18)
project(check_snodec LANGUAGES CXX)
find_package(snodec REQUIRED COMPONENTS net-in-stream-legacy)
```

```text
cmake -S . -B build
```

A successful configure confirms that CMake can find that installed development component. DEB/RPM library packages include headers and CMake development files. OpenWrt binary packages contain runtime libraries; verify them through the installed application instead.

### Next step

Use the installed package’s component targets instead of manually assembling library filenames:

```cmake
find_package(snodec REQUIRED COMPONENTS net-in-stream-legacy)
target_link_libraries(my-service PRIVATE snodec::net-in-stream-legacy)
```

[Build the native client and server →](../../README.md#your-first-program-a-factory-and-a-context)

## OpenWrt cross-compilation

Build SNode.C packages with an official OpenWrt SDK matching the device's release, target and ABI. Use the [SNode.C recipe](../../supplement/openwrt/Makefile) in this repository’s `supplement/openwrt/` directory. The recipe uses the upstream build system without source patches. OpenWrt package selections are separate from native CMake options.

### Prepare the SDK

The following is a recorded GL-MT3000 example. Choose the corresponding SDK for a different device or release; a binary for another CPU variant is not a replacement.

The build uses the official OpenWrt 25.12.5 `mediatek/filogic` SDK with GCC 14.3.0 and musl, producing `aarch64_cortex-a53` APK packages. The release's `glinet_gl-mt3000` profile identifies this target. These packages require the matching OpenWrt userspace and, for Bluetooth dependencies, matching kernel ABI. They are not a claim of compatibility with GL.iNet's vendor firmware or with an older OpenWrt release using opkg/IPK.

SDK: <https://downloads.openwrt.org/releases/25.12.5/targets/mediatek/filogic/>

Archive: `openwrt-sdk-25.12.5-mediatek-filogic_gcc-14.3.0_musl.Linux-x86_64.tar.zst`

SHA256: `ff4a38a397caa2cfe1c39e18f84ddede14878221b3593c3f2c4cfe24e3ec4c25`

Obtain the SDK archive and verify its SHA256 before extracting it. Keep its default feed revisions. The recipe is included in your SNode.C checkout; if you do not have one yet, clone it:

```text
git clone https://github.com/SNodeC/snode.c.git
```

### Link the recipe into the SDK

Run the following inside the extracted SDK. Replace `/path/to/snode.c` with the absolute path of your SNode.C checkout:

```text
./scripts/feeds update base packages
./scripts/feeds install nlohmannjson libopenssl libmagic bluez-libs libmariadb
mkdir -p package/local
ln -s /path/to/snode.c/supplement/openwrt package/local/snode.c
```

The symlink uses the recipe from your current checkout and reflects recipe changes automatically. Keep the checkout at that location while using the SDK.

The recipe declares the default source release. To select another release, export `SNODEC_SOURCE_TAG=vX.Y.Z` with a real SNode.C release tag before invoking `make`; the recipe derives the package version from that tag. Record the recipe commit and selected source tag to reproduce the build.

### Select and compile SNode.C

In a fresh SDK, initialize the package selection:

```text
cat > .config <<'EOF'
# CONFIG_ALL is not set
# CONFIG_ALL_NONSHARED is not set
# CONFIG_ALL_KMODS is not set
# CONFIG_SIGNED_PACKAGES is not set
# CONFIG_AUTOREMOVE is not set
CONFIG_PACKAGE_snodec=m
EOF
make defconfig
make -j16 package/local/snode.c/compile V=s
```

This example creates unsigned packages for local use. Adjust the parallel job count to the build host. In an existing SDK, retain your `.config` and change selections through `make menuconfig` instead of replacing it.

Use **Network / SNode.C** in `make menuconfig`. OpenWrt generates a `CONFIG_PACKAGE_<package>` tristate: `m` builds an installable package; `y` also selects it for an image build; `n` omits it unless a selected consumer requires it. Required lower layers follow the consumer's selection level. No second set of module booleans overrides these selectors. See the [package catalog](https://github.com/SNodeC/Packages/blob/main/docs/snodec-package-options.md) for package contents.

`snodec` selects all 63 shared runtime modules, demonstrations (`snodec-apps`) and the control tool (`snodec-control`). Static/internal targets have no separate runtime payload. RFCOMM (`net-rc`) and L2CAP (`net-l2`) each have address/configuration, physical socket, physical stream, stream configuration, legacy stream and TLS stream layers. RFCOMM also has legacy/TLS Express packages.

SNode.C compiles a shared set of libraries and packages the selected modules. Demonstrations compile only when selected. The recipe supplies spdlog 1.17.0 through a checked OpenWrt download, so configuration needs no FetchContent network request. Its derived CMake cache is reset during configuration to avoid stale defaults.

<details>
<summary>SNode.C build defaults</summary>

For source builds, package selection uses `CONFIG_PACKAGE_<name>` with ordinary n/m/y semantics and automatic dependency selection. Build defaults do not replace package selectors.

Every symbol below has the `CONFIG_` prefix in `.config`. Defaults shown are menu defaults. A choice uses exactly one of its alternative symbols. Values are passed into the existing upstream CMake settings; they are runtime defaults that the application's existing configuration system can override.

| Config.in symbol | Meaning | Default |
| --- | --- | --- |
| `SNODEC_GROUP_NAME` | Group name of unix group used for config/log/pid file management | `snodec` |
| `SNODEC_EPOLL` | epoll | `selected` |
| `SNODEC_POLL` | poll | `not selected` |
| `SNODEC_SELECT` | select | `not selected` |
| `SNODEC_READ_BLOCKSIZE` | Read block size in bytes | `16384` |
| `SNODEC_WRITE_BLOCKSIZE` | Write block size in bytes | `16384` |
| `SNODEC_READ_TIMEOUT` | Read inactivity timeout in seconds | `60` |
| `SNODEC_WRITE_TIMEOUT` | Write inactivity timeout in seconds | `60` |
| `SNODEC_MAXIMUM_WRITE_QUEUE_BYTES` | Maximum queued write bytes (0 = unlimited) | `0` |
| `SNODEC_WRITE_QUEUE_HIGH_WATERMARK` | Pipe write queue high watermark (0 = automatic) | `0` |
| `SNODEC_WRITE_QUEUE_LOW_WATERMARK` | Pipe write queue low watermark | `0` |
| `SNODEC_BACKLOG` | Listen backlog | `5` |
| `SNODEC_ACCEPTS_PER_TICK` | Accepts per tick | `1` |
| `SNODEC_ACCEPT_TIMEOUT` | Accept inactivity timeout in seconds | `0` |
| `SNODEC_CONNECT_TIMEOUT` | Connect timeout in seconds | `10` |
| `SNODEC_TERMINATE_TIMEOUT` | Shutdown timeout in seconds | `1` |
| `SNODEC_RECONNECT` | Reconnect after disconnect | `n` |
| `SNODEC_RECONNECT_TIME` | Reconnect time in seconds | `1` |
| `SNODEC_RETRY` | Retry listen and connect | `n` |
| `SNODEC_RETRY_ON_FATAL` | Retry also on fatal error | `n` |
| `SNODEC_RETRY_TIMEOUT` | Retry interval in seconds | `1` |
| `SNODEC_RETRY_TRIES` | Upper limit of retry tries | `0` |
| `SNODEC_RETRY_BASE` | Base of exponential increase | `"1.8"` |
| `SNODEC_RETRY_JITTER` | Jitter of retry timeout in percent | `0` |
| `SNODEC_RETRY_LIMIT` | Upper limit of retry timeout in seconds | `0` |
| `SNODEC_INV4_REUSE_ADDRESS` | Reuse address | `n` |
| `SNODEC_INV4_REUSE_PORT` | Reuse port | `n` |
| `SNODEC_INV4_DISABLE_NAGLE_ALGORITHM_TRUE` | true | `not selected` |
| `SNODEC_INV4_DISABLE_NAGLE_ALGORITHM_FALSE` | false | `not selected` |
| `SNODEC_INV4_DISABLE_NAGLE_ALGORITHM_DEFAULT` | default | `selected` |
| `SNODEC_IPV4_NUMERIC` | Accept numeric IPv4 hostnames only | `n` |
| `SNODEC_IPV4_NUMERIC_REVERSE` | Numeric IPv4 reverse lookup | `n` |
| `SNODEC_IN6_REUSE_ADDRESS` | Reuse address | `n` |
| `SNODEC_IN6_REUSE_PORT` | Reuse port | `n` |
| `SNODEC_INV6_DISABLE_NAGLE_ALGORITHM_TRUE` | true | `not selected` |
| `SNODEC_INV6_DISABLE_NAGLE_ALGORITHM_FALSE` | false | `not selected` |
| `SNODEC_INV6_DISABLE_NAGLE_ALGORITHM_DEFAULT` | default | `selected` |
| `SNODEC_IPV6_ONLY` | IPv6 only | `n` |
| `SNODEC_IPV4_MAPPED` | IPv4-mapped IPv6 addresses | `n` |
| `SNODEC_IPV6_NUMERIC` | Accept numeric IPv6 hostnames only | `n` |
| `SNODEC_IPV6_NUMERIC_REVERSE` | Numeric IPv6 reverse lookup | `n` |
| `SNODEC_TLS_INIT_TIMEOUT` | SSL/TLS initial handshake timeout in seconds | `10` |
| `SNODEC_TLS_SHUTDOWN_TIMEOUT` | SSL/TLS teardown timeout in seconds | `2` |
| `SNODEC_HTTP_REQUEST_PIPELINED` | Pipelined requests | `y` |

Read/write sizes, timeouts, retry/reconnect settings and write-queue limits configure `snodec-net`. IPv4/IPv6 stream flags configure their stream layers; name-resolution flags configure their address layers. TLS defaults are shared by TLS endpoints. HTTP pipelining configures the HTTP client. The I/O choice controls the core's linked default multiplexer and its package dependency; selecting extra multiplexer packages does not change that default.

All 44 SNode.C default/choice symbols and the demo selector participate in recipe reconfiguration. Other module selectors govern package emission and dependency closure; they do not prune the shared library compilation pass.

</details>

### Install and verify on the device

Install the desired packages and their dependencies on matching firmware. For unsigned local APKs, use `apk add --allow-untrusted ./<package>.apk` with the actual package path. Supply the required local dependency packages or a local feed; a single metapackage cannot discover unpublished packages. Check the selected demonstration applications or `snodec-control --help` on the device.

To build MQTTSuite next, retain this SDK and its staged SNode.C development files, then follow [MQTTSuite's OpenWrt instructions](https://github.com/SNodeC/mqttsuite/blob/master/docs/readme/install.md#openwrt-cross-compilation).
