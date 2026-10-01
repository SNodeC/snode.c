# Deploy an SNode.C application

[← SNode.C](../../README.md)

SNode.C supplies libraries, runtime facilities and configuration. **Your executable is the service**: choose the interfaces it exposes, the state it persists and the operating-system account that runs it.

## Start with one explicit configuration

Use the application’s own configuration interface rather than guessing settings from another program:

```sh
./my-service --help=expanded
./my-service --show-config
snodec-control --target ./my-service --ui
```

The terminal UI requires a build with Curses support. CLI inspection and file generation do not. Protect exported configuration files: they can contain credentials and private-key paths.

Persist the selected configuration with `--write-config path/to/service.conf`. This writes and exits; it is not a live reload. Start the service with `--config-file path/to/service.conf`. Applications that construct additional instances dynamically can use the framework’s reconfiguration API, but that is distinct from promising arbitrary hot reload of application state.

## Before exposing a listener

- Bind only the required interfaces. Use loopback or a Unix-domain socket for local services.
- Configure TLS certificates, private-key permissions, peer verification and trust roots for encrypted connections. Never deploy the repository’s demonstration keys as production credentials.
- Choose authentication and authorization appropriate to the application. TLS encryption and HTTP Basic middleware do not by themselves define access policy.
- Set connection, body/message and write-queue limits for the expected workload. Review [resource-policy and streaming guidance](https://github.com/SNodeC/snode.c/blob/master/docs/resource-policy-and-streaming.md).
- Keep callbacks non-blocking. Size retry/backoff and reconnect settings to avoid overwhelming a recovering peer.
- Create writable state directories for the service account. Keep binaries and configuration read-only where practical.
- Select log levels deliberately. Trace output and payload dumps can be expensive and can expose application data; use scoped filters and rotation.

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

```sh
sudo systemctl daemon-reload
sudo systemctl enable --now my-service
journalctl -u my-service -f
```

For containers, package your executable with its runtime libraries, mount configuration/state explicitly and bind the intended network interface. Run it in the foreground under the container supervisor.

## OpenWrt and embedded deployments

Install only the required framework/application packages from the [matching feed](packages.md#openwrt). Use a matching SDK for custom builds. Ready-made application packages may provide init scripts; a custom executable needs its own suitable service definition. Check RAM, storage, TLS trust material and persistent-data locations on the actual device.

## Upgrade deliberately

Keep executables, shared libraries and dynamically loaded protocol plugins compatible. Back up configuration and application state; verify the new build on a separate instance before replacing a running service. Major framework releases can change ABI even when application source needs little adjustment. When upgrading from SNode.C 1.x to 2.0, rebuild consuming applications, libraries and plugins against the updated headers and libraries; do not mix 1.x and 2.0 C++ binaries in one process.
