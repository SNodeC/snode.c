<a id="banner"></a>

<!-- snodec:begin page-header -->
<a id="banner"></a><a id="page-overview"></a>
<p><picture><source media="(max-width: 640px)" srcset="../media/nav/banner-guides-compact.svg"><img src="../media/nav/banner-guides.svg" alt="SNode.C — Guides" width="100%"></picture></p>
<p><a href="README.md#banner" title="Up to Guides"><picture><source media="(prefers-color-scheme: dark)" srcset="../media/nav/up-guides-dark.svg"><img src="../media/nav/up-guides-light.svg" alt="Up to Guides" title="Up to Guides" width="160" height="28"></picture></a><a href="https://github.com/VolkerChristian#banner" title="Volker Christian account"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../media/nav/account-icon-dark.svg"><source media="(max-width: 640px)" srcset="../media/nav/account-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../media/nav/account-dark.svg"><img align="right" src="../media/nav/account-light.svg" alt="Volker Christian account" title="Volker Christian account"></picture></a><a href="https://github.com/SNodeC#banner" title="SNode.C organization"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../media/nav/organization-icon-dark.svg"><source media="(max-width: 640px)" srcset="../media/nav/organization-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../media/nav/organization-dark.svg"><img align="right" src="../media/nav/organization-light.svg" alt="SNode.C organization" title="SNode.C organization"></picture></a><a href="../../README.md#banner" title="SNode.C repository"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../media/nav/repository-icon-dark.svg"><source media="(max-width: 640px)" srcset="../media/nav/repository-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../media/nav/repository-dark.svg"><img align="right" src="../media/nav/repository-light.svg" alt="SNode.C repository" title="SNode.C repository"></picture></a><br clear="all"></p>
<!-- snodec:end page-header -->

# Deploy an SNode.C application

SNode.C supplies libraries, runtime facilities and configuration. **Your executable is the service**: choose the interfaces it exposes, the state it persists and the operating-system account that runs it.

## Start with one explicit configuration

Use the application’s own configuration interface rather than guessing settings from another program:

```text
./my-service --help=expanded
./my-service --show-config
snodec-control --target ./my-service --ui
```

The [configuration-tool guide](../../src/tools/snodec-control/README.md#banner) covers inspection, editing and saving. The terminal UI requires a build with Curses support. CLI inspection and file generation do not. Protect exported configuration files: they can contain credentials and private-key paths.

Persist the selected configuration with `--write-config path/to/service.conf`. This writes and exits; it is not a live reload. Start the service with `--config-file path/to/service.conf`. Applications that construct additional instances dynamically can use the framework’s reconfiguration API, but that is distinct from promising arbitrary hot reload of application state.

## Before exposing a listener

- Bind only the required interfaces. Use loopback or a Unix-domain socket for local services.
- Configure TLS certificates, private-key permissions, peer verification and trust roots for encrypted connections. Never deploy the repository’s demonstration keys as production credentials.
- Choose authentication and authorization appropriate to the application. TLS encryption and HTTP Basic middleware do not by themselves define access policy.
- Set connection, body/message and write-queue limits for the expected workload. Review [resource-policy and streaming guidance](../design/resource-policies-and-streaming.md#banner).
- Keep callbacks non-blocking. Size retry/backoff and reconnect settings to avoid overwhelming a recovering peer.
- Create writable state directories for the service account. Keep binaries and configuration read-only where practical.
- Select log levels deliberately. Trace output and payload dumps can be expensive and can expose application data; use scoped filters and rotation.

## Logging and process options

Application-level options precede instance subcommands. These names and levels are defined by `src/utils/Config.cpp`; inspect the installed executable's `--help=expanded` if its version differs.

| Option | Meaning |
| --- | --- |
| `--log-level 0..6` | 0 off, 1 critical, 2 error, 3 warn, 4 info (default), 5 debug, 6 trace |
| `--log-format text` / `json` | Semantic log serialization; text is the default |
| `--log-origin-level application=5` | Override framework/application origin levels |
| `--log-boundary-level connection=5` | Override a semantic boundary level |
| `--log-component-level COMPONENT=5` | Override a named component; use identifiers from actual records |
| `--log-instance-level INSTANCE=5` | Override a named instance |
| `--verbose-level 0..10` | Legacy verbosity only; does not change semantic log levels |
| `--log-file PATH` | Select the file used for file logging |
| `--enforce-log-file=true` | Enable file logging for a foreground process |
| `--daemonize=false` | Stay in the foreground (default); retain this under systemd/container supervision |

For example, replace the executable with your service:

```sh
./my-service --config-file /etc/my-service/service.conf --log-level 4 --log-format json --daemonize=false
```

Start with supervisor-captured foreground logs (`journalctl` for the unit below). When opting into file logging, provision a writable, permission-restricted log destination and an external rotation/retention policy appropriate to your supervisor; no rotation flag is promised here. Debug/trace records can expose payloads and credentials. For application code rather than operator options, see the [public logging API](../logging/public-api.md#banner).

## Configure a TLS instance

Choose the TLS transport/component when building the application; passing TLS options does not turn a legacy TCP instance into TLS. Replace the executable and `tls-listener` below with the names exposed by your application's `--help=expanded`, then supply your own PEM certificate chain and private key:

```text
./my-service tls-listener --help=expanded
./my-service tls-listener tls \
  --cert /path/to/server-chain.pem --cert-key /path/to/server-key.pem
```

A TLS client's corresponding `tls` section accepts `--ca-cert /path/to/trusted-ca.pem` or a configured CA directory. Inspect the selected instance's help for SNI and peer-verification settings. Restrict private-key permissions and test both accepted and rejected peers for the actual DNS/IP identity. `--ca-cert-accept-unknown` is not a production fix for trust failures. Certificate trust does not replace your application's authentication/authorization policy.

## Supervise the foreground process

On a systemd system, a small unit can run an installed application in the foreground. This is an **example unit**, not a service installed by the framework:

```ini
[Unit]
Description=My SNode.C service
After=network.target

[Service]
Type=simple
User=my-service
Group=my-service
ExecStart=/usr/local/bin/my-service --config-file /etc/my-service/service.conf
Restart=on-failure
RestartSec=3
NoNewPrivileges=true
PrivateTmp=true

[Install]
WantedBy=multi-user.target
```

Create the service account and configuration first; adapt paths and sandbox policy to the application. Keep daemonization disabled under `Type=simple`. If a Unix-domain socket must be shared with another process, choose an explicit accessible runtime directory rather than relying on a private `/tmp`.

```text
sudo systemctl daemon-reload
sudo systemctl enable --now my-service
journalctl -u my-service -f
```

For containers, package your executable with its runtime libraries, mount configuration/state explicitly and bind the intended network interface. Run it in the foreground under the container supervisor.

## OpenWrt and embedded deployments

Install only the required framework/application packages from the [OpenWrt feed](https://github.com/SNodeC/Packages/blob/main/docs/installation/openwrt.md#banner). Ready-made application packages may provide init scripts; a custom executable needs its own suitable service definition. Check RAM, storage, TLS trust material and persistent-data locations on the actual device.

## Upgrade deliberately

Keep executables, shared libraries and dynamically loaded protocol plugins compatible. Back up configuration and application state; verify the new build on a separate instance before replacing a running service. Major framework releases can change ABI even when application source needs little adjustment. When upgrading from SNode.C 1.x to 2.0, rebuild consuming applications, libraries and plugins against the updated headers and libraries; do not mix 1.x and 2.0 C++ binaries in one process.

## Troubleshoot a first program

- **CMake cannot find `snodec`:** install the development components used by the example, or point `CMAKE_PREFIX_PATH` to that complete installation; do not combine unrelated headers and libraries.
- **Listener startup fails:** read the reported socket error, check the bind address/port or Unix path, and choose an available endpoint. Update both sides of the example together.
- **Runtime library/plugin lookup fails:** check the installation prefix and runtime search path. The WebSocket example needs its named protocol plugin as well as the HTTP transport libraries.
- **A stream seems truncated or combined:** TCP read boundaries are not message boundaries; use the [line-framing example](examples.md#a-line-oriented-protocol) or your protocol's own framing.
- **A service hangs:** callbacks must stay non-blocking; inspect connection/write-queue limits and logs before increasing buffers or timeouts.
