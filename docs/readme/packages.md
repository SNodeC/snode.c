# SNode.C binary packages

[← Install SNode.C](install.md)

The **SNode.C package feed** supplies signed binaries for the distribution families below. Use the distribution guide and published package status to select the right source for your system.

## Choose distribution, release and architecture

Catalog snapshot: **1 October 2026**. Check the [live per-target status][status] for available versions and publication results.

| Distribution | Releases | Package architectures | Guide |
| --- | --- | --- | --- |
| Debian | Trixie, Forky, Sid | `amd64`, `arm64`, `armhf`, `riscv64` | [Debian][debian] |
| Ubuntu | Noble, Resolute | `amd64`, `arm64` | [Ubuntu][ubuntu] |
| Raspberry Pi OS | Bookworm, Trixie | `arm64`; Pi 3, 4 and 5 | [Raspberry Pi OS][raspberrypi] |
| Rocky Linux | 9, 10 | `x86_64`, `aarch64` | [Rocky Linux][rocky] |
| Fedora | 43, 44 | `x86_64`, `aarch64` | [Fedora][fedora] |
| OpenWrt | 24.10, 25.12 | 25 package architectures per release; ARM, AArch64, x86, MIPS, PowerPC, RISC-V and LoongArch variants | [OpenWrt][openwrt] |

Match the installed distribution and release as well as the architecture. Raspberry Pi OS Bookworm does not imply a Debian Bookworm feed. Debian Sid may need explicit suite selection, and Rocky Linux requires the documented CRB/EPEL prerequisites.

## Prepare the signed feed

First follow the distribution guide above to configure its signed source. The feed installer’s **`--prepare`** mode registers the source and refreshes indexes without installing both complete project sets. Review the installer before running it with administrative privileges, or use the guide’s manual configuration procedure. Keep official distribution repositories enabled for dependencies.

## Install with the system package manager

Install the framework after preparing the feed:

```sh
# Debian, Ubuntu or Raspberry Pi OS
sudo apt-get install snodec
```

```sh
# Fedora or Rocky Linux
sudo dnf install snodec
```

The DEB/RPM `snodec` metapackage includes framework components, headers, examples and tools. For a smaller runtime deployment, select component packages using the [component catalog][linux-components]. Do not disable signature verification to bypass an installation error.

## OpenWrt

Read `/etc/openwrt_release` and use `DISTRIB_ARCH`, not only `uname -m`. Match both release and package architecture. OpenWrt **24.10 uses opkg/IPK**, while **25.12 uses apk/APK**.

After preparing the feed:

```sh
# OpenWrt 24.10, as root
opkg install snode.c-full snode.c-apps snode.c-control
```

```sh
# OpenWrt 25.12, as root
apk add snode.c-full snode.c-apps snode.c-control
```

`snode.c-full` supplies the framework runtime modules; applications and the configuration tool are separate packages. See the [OpenWrt component catalog][openwrt-components] to install a smaller selection. Installing a downstream application package pulls its required framework modules automatically.

For a custom firmware image or an unlisted target, follow the feed’s [OpenWrt SDK build guide][sdk]. An SDK must match the target release and ABI; a prebuilt package for another CPU variant is not a replacement.

## Updates and verification

Use your package manager’s normal update path after reviewing the distribution guide. Keep signing enabled and consult [published package/build metadata][status] before selecting a version. The framework and applications have separate release versions; update related libraries, executables and plugins as a compatible set.

[Feed overview and signing keys][feed] · [Installation entry point][install] · [Troubleshooting][troubleshooting]

<!-- Rename-sensitive external destinations are deliberately centralized here. -->
[feed]: https://github.com/SNodeC/OpenWRT
[install]: https://github.com/SNodeC/OpenWRT/blob/main/docs/install-snodec.md
[status]: https://github.com/SNodeC/OpenWRT/blob/packages/README.md
[debian]: https://github.com/SNodeC/OpenWRT/blob/main/docs/debian.md
[ubuntu]: https://github.com/SNodeC/OpenWRT/blob/main/docs/ubuntu.md
[raspberrypi]: https://github.com/SNodeC/OpenWRT/blob/main/docs/raspberrypi.md
[rocky]: https://github.com/SNodeC/OpenWRT/blob/main/docs/rocky.md
[fedora]: https://github.com/SNodeC/OpenWRT/blob/main/docs/fedora.md
[openwrt]: https://github.com/SNodeC/OpenWRT/blob/main/docs/openwrt.md
[linux-components]: https://github.com/SNodeC/OpenWRT/blob/main/docs/linux.md
[openwrt-components]: https://github.com/SNodeC/OpenWRT/blob/main/docs/snodec-package-options.md
[sdk]: https://github.com/SNodeC/OpenWRT/blob/main/docs/openwrt-build.md
[troubleshooting]: https://github.com/SNodeC/OpenWRT/blob/main/docs/troubleshooting.md
