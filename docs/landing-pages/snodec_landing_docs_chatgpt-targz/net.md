# Network module (`src/net/*` and `src/net/config/*`)

This document covers:

- **Transport address families** and helper types under `src/net/*`
- The **configuration framework** for clients and servers under `src/net/config/*`

The `net` module is where SNode.C ties together:

- a concrete address family (IPv4, IPv6, Unix domain sockets, Bluetooth RFCOMM/L2CAP)
- a physical socket implementation
- a configuration object that can be populated from:
  - command line,
  - config files,
  - or in-code API calls.

---

## 1. Overview

SNode.C uses templates to keep protocol modules generic over transports.
Instead of hardcoding “TCP/IPv4 only”, most server/client types are defined as:

- `web::http::legacy::in::Server` (plain HTTP over IPv4)
- `web::http::tls::in6::Server` (HTTPS over IPv6)
- `web::http::legacy::un::Server` (HTTP over Unix domain sockets)
- `express::tls::in::WebApp` (Express-like app on TLS/IPv4)
- etc.

Those convenience typedefs exist because the underlying machinery is generic.

The `net` module supplies:

1. **Address types** (`SocketAddress`) for each family.
2. A consistent “socket address” API used by `core/socket`.
3. A rich **configuration layer** that standardizes how instances are created and configured.

---

## 2. Address families (`src/net/*`)

The `src/net` directory defines concrete socket address types and resolution helpers.

### 2.1 Common base: `net::SocketAddress`

- `src/net/SocketAddress.h` / `SocketAddress.hpp`

This is a thin generic wrapper that allows the rest of the system to treat addresses uniformly, while still using family-specific data internally.

### 2.2 IPv4 (`net/in/*`) and IPv6 (`net/in6/*`)

- `src/net/in/SocketAddress.*`
- `src/net/in/SocketAddrInfo.*` — DNS/service resolution helpers
- `src/net/in6/SocketAddress.*`
- `src/net/in6/SocketAddrInfo.*`

These types encapsulate:

- IP + port
- parsing and formatting
- integration with `getaddrinfo` (via core/system wrappers)

### 2.3 Unix domain sockets (`net/un/*`)

- `src/net/un/SocketAddress.*`

Encapsulates filesystem-based socket paths and their POSIX semantics.

### 2.4 Bluetooth RFCOMM (`net/rc/*`) and L2CAP (`net/l2/*`)

- `src/net/rc/SocketAddress.*`
- `src/net/l2/SocketAddress.*`

These types allow SNode.C to reuse the same connection model for Bluetooth transports.

> Practical note: availability depends on platform headers and build options. This is precisely why SNode.C keeps the transport layer modular.

### 2.5 Physical sockets (`net/phy/*`)

`src/net/phy/PhysicalSocket.h/.hpp` and socket option helpers abstract:

- OS socket creation
- family-specific sockopts
- bind/connect primitives
- timeout and non-blocking setup

`core/socket/stream::SocketConnectionT` composes a `PhysicalSocket` with reader/writer strategies.

---

## 3. Configuration framework (`src/net/config/*`)

The configuration subsystem is one of SNode.C’s most distinctive pieces:
it makes “instances” a first-class concept.

### 3.1 Why “instances”?

Most network services have multiple listeners and/or clients:

- multiple HTTP listeners (IPv4 + IPv6; plain + TLS)
- multiple MQTT clients (to different brokers)
- multiple DB connections (different users/databases)
- multiple vhosts (domain-specific configs)

SNode.C models each as an **instance**, configurable via CLI subcommands *and* config files.

### 3.2 `net::config::ConfigInstance`

`ConfigInstance` (`src/net/config/ConfigInstance.h/.cpp`) represents one configured unit of work.

Key ideas:

- Each instance registers itself as a **CLI11 subcommand** (“INSTANCE”).
- Instances are typed by role:
  - `Role::SERVER`
  - `Role::CLIENT`
- Instances can be **disabled** via a persistent option:
  - `--disabled{true}` (see `disableOpt` in `ConfigInstance.cpp`)
- Instances can contain **sections** (subcommands of the instance), e.g.:
  - address section
  - TLS section
  - socket options section
  - protocol-specific section

The class provides:
- `addSection(...)` to create sections with consistent flags/help
- `required(section, bool)` to enforce required sections/options
- `getSection(name, onlyGot, recursive)` to locate sections

This gives SNode.C a uniform way to express “for this instance to be valid, these options must exist”.

### 3.3 `net::config::ConfigSection`

`ConfigSection` (`src/net/config/ConfigSection.h/.cpp`) wraps a section subcommand:

- options are grouped into “Persistent Options” vs “Nonpersistent Options”
- options can be marked required and that requirement percolates:
  - option required → section required → instance required

This is more than UI nicety: it shapes config file stability and helps users discover the correct configuration through `--help`.

### 3.4 The `utils::Config` bridge

Under the hood, instance/section options are implemented on top of `utils::Config` (see `src/utils/Config.h/.cpp`):

- config file support via `-c,--config-file` (defaulting into `~/.config/snode.c/<app>.conf`)
- global standard flags (log levels, verbosity, daemonization, etc.)
- custom help formatting to reflect SNode.C’s “instances” terminology

In other words:

- `utils::Config` is the global CLI/config runtime,
- `net::config::*` are structured building blocks for network endpoints.

---

## 4. Address configuration objects

The `net/config` subtree contains templates and helpers that connect:

- an instance + section
- to a concrete `SocketAddressT`
- with explicit “renew” semantics (re-parse from CLI/config)

The central template is:

- `ConfigAddress<SocketAddressT>` (`src/net/config/ConfigAddress.h`)

It provides:

- `getSocketAddress()` to obtain the concrete address
- `renew()` to rebuild address state after config parsing changes
- a virtual `init()` which family-specific subclasses implement

There are specializations/helpers for common needs:

- local vs remote addresses
- address reverse lookups (`ConfigAddressReverse`)
- etc.

---

## 5. Connection and socket option configuration

Beyond addresses, an endpoint needs:

- backlog / reuseaddr / keepalive / nodelay / buffer sizes
- timeouts
- TLS configuration (for TLS endpoints)

The following config objects appear repeatedly in server/client types:

- `ConfigConnection` (`ConfigConnection.h/.cpp`) — connection-level settings (timeouts, naming, …)
- `ConfigPhysicalSocket*` (`ConfigPhysicalSocket*.h/.cpp`) — socket options
- `ConfigTls` (`ConfigTls.h/.cpp`) — shared TLS client/server options
- `ConfigTlsServer` (`ConfigTlsServer.h/.cpp`) — server-specific TLS (SNI certs, force-SNI)

### 5.1 TLS server configuration and SNI

`ConfigTlsServer` exposes:

- `setForceSni(bool)` — require a valid SNI from clients
- `addSniCerts(...)` / `addSniCert(...)` — inject per-domain TLS material
- `getSniCerts()` — obtain configured map

The SNI cert map is compatible with the `ssl_utils` mapping logic in `core/socket/stream/tls`.

You can see this pattern in the HTTP example app:

- `src/apps/http/httpserver.cpp` sets up an SNI map in C++ and passes it into the server config.

---

## 6. How configuration flows through a typical application

A typical SNode.C service sets things up like this:

1. Call `core::SNodeC::init(argc, argv)` (which initializes `utils::Config` early).
2. Construct server/client instances.
3. Each instance registers its config sections/options with `utils::Config` and/or `net::config::ConfigInstance`.
4. The application triggers parsing (directly or indirectly via bootstrap).
5. Instances inspect:
   - which subcommands were provided,
   - which config sections are present,
   - whether the instance is disabled,
   - then bind/listen/connect accordingly.

### Practical outcome

- You can run the same binary with multiple instances configured in one config file.
- You can disable instances without removing their configs.
- Modules can add options without breaking existing config files by keeping “persistent option” semantics stable.

---

## 7. In-code API vs CLI/config file

SNode.C supports all three configuration styles:

### 7.1 Command line (one-off)
Good for quick testing:

- `myapp instanceA address --bind 0.0.0.0 --port 8080 ...`

### 7.2 Config file (persistent)
Good for services:

- `~/.config/snode.c/myapp.conf`
- `/etc/snode.c/myapp.conf` when running as root (see `utils::Config.cpp`)

### 7.3 In-code configuration (programmatic defaults/overrides)
Used by sample apps and by “library” scenarios:

- constructing a config object with defaults
- injecting SNI cert maps
- overriding options based on runtime discovery

A robust pattern is:

- **provide safe defaults in code**,
- allow users to override via config file / CLI.

---

## 8. Pros / cons

### Pros
- “Instances” make complex services manageable (multiple listeners/clients in one binary).
- Uniform configuration structure across modules.
- Config file and CLI semantics are consistent and can be documented once for all services.

### Cons / tradeoffs
- Template configuration objects can be intimidating at first glance.
- The CLI model (instances + sections as subcommands) is powerful but different from many typical Unix daemons.

### Gotchas
- A section can exist but be *disabled* if an instance name is empty or the section name is empty (`ConfigInstance::addSection`).
- Required option propagation means: marking many things “required” can make startup strict; be deliberate about which options are required vs having good defaults.

---

## 9. Next steps

- [`web_http.md`](web_http.md) shows how HTTP binds to `net` + `core/socket` configs.
- [`express.md`](express.md) shows the same for Express-like apps (with additional app-level configuration).
