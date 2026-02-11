# Socket layer (`src/core/socket/*`) — stream, contexts, and TLS

This document covers the **socket layer** of SNode.C:

- `src/core/socket/*` (including `stream/*` and `dgram/*`)
- The “stream” subsystem is fully implemented and used by HTTP, WebSocket, MQTT, and database modules.
- The “dgram” subtree currently contains placeholders (README files only in the provided code base).

TLS/SSL support is integrated as a first-class “stream type” (`stream/tls/*`) and is covered in detail below.

---

## 1. Overview

The socket layer is responsible for:

- turning **OS-level sockets** (file descriptors) into event-loop driven objects
- managing **connection lifecycle** and connection-specific statistics
- providing a **SocketContext** abstraction that higher layers (HTTP, MQTT, …) build upon
- optional **TLS encryption** without changing the user-facing application model

SNode.C uses a consistent pattern across transports:

1. A **logical socket** object holds configuration (`core::socket::Socket<ConfigT>`).
2. An **acceptor** or **connector** creates a `SocketConnection`.
3. The `SocketConnection` installs a **SocketContext** created by a context factory.
4. Event receivers (read/write/accept) drive the connection and call into protocol code.

---

## 2. Public API entry points

### Base socket abstractions
- `#include "core/socket/Socket.h"`
- `#include "core/socket/SocketContext.h"`
- `#include "core/socket/SocketAddress.h"`
- `#include "core/socket/State.h"` — connection/listen state reporting (`OK`, `DISABLED`, `ERROR`, `FATAL`, …)

### Stream sockets (non-TLS)
- `#include "core/socket/stream/SocketConnection.h"`
- `#include "core/socket/stream/SocketAcceptor.h"` / `SocketAcceptor.hpp`
- `#include "core/socket/stream/SocketConnector.h"` / `SocketConnector.hpp`
- `#include "core/socket/stream/SocketContext.h"`
- `#include "core/socket/stream/SocketContextFactory.h"`

### Stream sockets (TLS)
- `#include "core/socket/stream/tls/SocketConnection.h"`
- `#include "core/socket/stream/tls/SocketAcceptor.h"` / `SocketAcceptor.hpp`
- `#include "core/socket/stream/tls/SocketConnector.h"` / `SocketConnector.hpp`
- `#include "core/socket/stream/tls/TLSHandshake.h"`
- `#include "core/socket/stream/tls/TLSShutdown.h"`
- `#include "core/socket/stream/tls/ssl_utils.h"`

---

## 3. Key concepts

### 3.1 `core::socket::Socket<ConfigT>` — the logical socket

`Socket<ConfigT>` (`src/core/socket/Socket.h`) is a small base class that primarily:

- holds a `std::shared_ptr<ConfigT>` (the configuration object)
- provides `getConfig()` access
- gives each socket an instance name (via the configuration)

It is deliberately minimal because most behavior lives in acceptors/connectors and connections.

### 3.2 `core::socket::SocketAddress` — an abstract address

`core::socket::SocketAddress` (`src/core/socket/SocketAddress.h/.cpp`) provides an abstract “address” view.
Concrete address types are defined in `src/net/*` (IPv4/IPv6/Unix/Bluetooth) and used via templates throughout the stack.

The important idea: higher modules depend on “socket address” as a concept, not on `sockaddr_in` directly.

### 3.3 `core::socket::SocketContext` — the protocol attachment point

`SocketContext` (`src/core/socket/SocketContext.h`) is the key abstraction that lets you implement protocols independent of the concrete socket type.

A `SocketContext`:

- can `sendToPeer(...)` and `readFromPeer(...)`
- can `setTimeout(...)` and `close()`
- exposes accounting: totals sent/queued/read/processed
- exposes connection age (`getOnlineSince()`, `getOnlineDuration()`)
- receives callbacks that drive protocol logic:
  - `onReceivedFromPeer()`
  - `onSignal(int)`
  - `onWriteError(int)`, `onReadError(int)`

Higher-level modules provide concrete `SocketContext` implementations:
HTTP server socket contexts, MQTT socket contexts, database socket contexts, etc.

---

## 4. Stream sockets (`src/core/socket/stream/*`)

### 4.1 `SocketConnection`: connection lifecycle and metrics

`core::socket::stream::SocketConnection` (`src/core/socket/stream/SocketConnection.h`) is an abstract base that defines the connection contract:

- `sendToPeer` / `readFromPeer`
- graceful shutdown: `shutdownRead()` / `shutdownWrite()`
- address introspection: `getBindAddress()` / `getLocalAddress()` / `getRemoteAddress()`
- `close()`
- timeouts: `setTimeout`, `setReadTimeout`, `setWriteTimeout`
- accounting (total sent / queued / read / processed)
- connection identity:
  - `getInstanceName()` (server instance)
  - `getConnectionName()` (unique per connection)
  - online since / duration formatting helpers

This is the object the event loop ultimately drives via event receivers.

### 4.2 `SocketConnectionT`: composing physical sockets + reader/writer strategies

`SocketConnectionT<PhysicalSocketT, SocketReaderT, SocketWriterT, ConfigT>` is the concrete template that composes:

- a `PhysicalSocketT` (from `src/net/phy/*`)
- a **reader** strategy (`SocketReaderT`)
- a **writer** strategy (`SocketWriterT`)
- a configuration (`ConfigT`)

This composition allows SNode.C to reuse the same “connection skeleton” for:

- legacy (plain) streams
- TLS streams (different reader/writer strategies)
- potentially other stream variants

### 4.3 Acceptors and connectors

The “server vs client” sides are separated into acceptors and connectors.

- `SocketAcceptor` accepts incoming connections, creates a `SocketConnection`, installs a `SocketContext` via a context factory.
- `SocketConnector` initiates outgoing connections, then follows the same “connection + context” model.

This separation keeps protocol code independent of whether it is used in a client or server role.

### 4.4 SocketContext factories (why they matter)

Instead of hardcoding “every accepted connection is HTTP” (for example), SNode.C uses **factories**:

- a factory creates the right context for each connection
- factories can implement:
  - protocol detection (“sniffing”)
  - multi-protocol servers
  - connection metadata injection

You see this pattern in HTTP and WebSocket modules: HTTP can upgrade to WebSocket by swapping or layering contexts.

---

## 5. TLS/SSL integration (`src/core/socket/stream/tls/*`)

TLS is implemented as a *stream variant* using the same connection/context architecture.
From the perspective of higher layers:

- the API stays the same (`sendToPeer`, `readFromPeer`, `setTimeout`, …)
- connection creation differs (TLS acceptor/connector)
- the connection performs a handshake and then exposes encrypted I/O transparently

### 5.1 `tls::SocketConnection` and SSL object ownership

`core::socket::stream::tls::SocketConnection` (`src/core/socket/stream/tls/SocketConnection.h`) wraps the standard stream connection but adds:

- an `SSL* ssl` handle (OpenSSL)
- start/stop logic for the SSL object:
  - `startSSL(int fd, SSL_CTX* ctx)`
  - `stopSSL()`
- handshake and shutdown logic:
  - `doSSLHandshake(...)`
  - `doSSLShutdown()`
- a `getSSL()` accessor (useful for SNI inspection, peer cert queries, etc.)

The handshake and shutdown are integrated into the event-driven reader/writer logic so that TLS operations do not block.

### 5.2 Handshake (`TLSHandshake`)

`TLSHandshake` (`src/core/socket/stream/tls/TLSHandshake.h/.cpp`) models the handshake as a small state machine:

- It owns a reference to the `SSL*` and triggers `SSL_do_handshake()` steps.
- It cooperates with the event loop:
  - if OpenSSL signals “want read” / “want write”, the socket stays observed for the relevant direction
  - timeouts are enforced (see the presence of `sslInitTimeout` in the TLS connection)

This is the correct approach for non-blocking TLS: the handshake becomes just another event-driven sequence.

### 5.3 Shutdown (`TLSShutdown`)

`TLSShutdown` (`src/core/socket/stream/tls/TLSShutdown.h/.cpp`) performs graceful TLS close:

- drives `SSL_shutdown()` potentially in multiple steps
- deals with “close notify” semantics (see `closeNotifyIsEOF` in `tls::SocketConnection`)
- integrates timeouts (`sslShutdownTimeout`)

Correct TLS shutdown is notoriously tricky; having a dedicated helper makes correctness more achievable and keeps the core connection logic readable.

### 5.4 Certificate handling, SNI and SAN mapping (`ssl_utils`)

`ssl_utils.h/.cpp` provides higher-level certificate utilities.
In the current code base you can observe:

- a `Map` type that maps:
  - **SNI names** → a map of SSL options (cert path, key path, verification flags, …)
  - and additionally supports *Subject Alternative Names* (SAN) extraction

This is used in application code too:
see `src/apps/http/httpserver.cpp` where SNI certificates are configured via a map and injected into config.

**Why this is valuable**

- It enables **multi-domain TLS** on the same listener (virtual hosting at the TLS layer).
- It supports both strict and permissive verification modes per SNI entry.
- It keeps OpenSSL detail out of higher-level modules.

### 5.5 TLS configuration from the outside

The socket layer itself doesn’t parse CLI flags directly; it consumes configuration objects (see `net/config`, documented in `net.md`).

You typically configure TLS via:

- programmatic API calls (e.g. add SNI cert maps)
- config file options
- command line options

and then create a TLS `WebApp` or TLS server instance which binds those options into a concrete `SSL_CTX` setup.

---

## 6. Datagram sockets (`src/core/socket/dgram/*`)

In the provided source tree, `src/core/socket/dgram` contains README placeholders but no C++ implementation files.

This suggests one of the following:

- datagram support is planned but not yet implemented in this code snapshot, or
- it is implemented elsewhere and this directory is a stub.

If/when implemented, it would likely follow the same pattern:

- a `SocketConnection`-like object for datagrams
- a `SocketContext` for protocol attachment
- integration with the multiplexer via read/write event receivers

---

## 7. Pros / cons and design tradeoffs

### Pros
- Clear separation of concerns:
  - physical socket vs connection vs context vs protocol
- TLS is integrated without changing the external protocol APIs.
- Non-blocking handshake/shutdown are handled explicitly and safely.

### Cons / tradeoffs
- Template-heavy design can increase compile times and error verbosity.
- Correctness depends on strict lifetime management:
  - connection owns context pointers, context can reference connection
  - factories must ensure contexts outlive their callbacks

### Gotchas
- Don’t block inside `onReceivedFromPeer()`; doing so stalls the entire event loop.
- When implementing a context, treat `readFromPeer` as “read what is available”, not “read until complete”.
- TLS handshakes can require multiple read/write cycles; ensure your application’s “connected” state is only signaled after handshake completion.

---

## 8. Where it is used

Understanding the socket layer is essential before reading:

- `web/http` server socket contexts (HTTP parsing & response sending)
- `web/websocket` (upgrade + frame I/O)
- `iot/mqtt` (packet parsing, keepalive, broker integration)
- `database` (MariaDB async commands driven by FD events)

Continue with:

- [`net.md`](net.md) — address families and configuration model
