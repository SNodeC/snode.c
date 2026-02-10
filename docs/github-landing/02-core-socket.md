# Core Socket Module (`src/core/socket/*`)

## What this module is

This layer turns raw descriptors into connection-oriented and datagram-capable transport primitives used by all higher modules (`net`, `http`, `websocket`, `mqtt`).

Key strata:

- `core/socket/*`: generic socket state, context, addresses
- `core/socket/stream/*`: connection-oriented accept/connect/read/write/context abstractions
- `core/socket/stream/legacy/*`: unencrypted stream implementation
- `core/socket/stream/tls/*`: TLS-secured stream implementation
- `core/socket/dgram/*`: datagram scaffolding (with legacy/tls placeholders/readmes)

## Core concepts

- **SocketAddress**: normalized endpoint representation.
- **SocketContext**: per-connection protocol logic attachment point.
- **SocketConnection**: transport implementation owning read/write behavior.
- **SocketAcceptor / SocketConnector**: server/client connection establishment.
- **SocketContextFactory**: factory creating protocol context per accepted/connected socket.
- **AutoConnectControl**: retry/reconnect policy plumbing.

## Stream stack architecture

### Common stream base (`core/socket/stream`)

Provides the reusable machinery:

- generic acceptor/connector templates,
- context/context-factory contracts,
- reader/writer interfaces,
- server/client wrappers.

### Legacy stream (`core/socket/stream/legacy`)

- Plain read/write over stream sockets.
- Minimal overhead and operationally simple.
- Best for trusted networks, local sockets, or termination behind external TLS proxy.

### TLS stream (`core/socket/stream/tls`)

- TLS-aware `SocketConnection`, `SocketReader`, `SocketWriter`, `SocketAcceptor`, `SocketConnector`.
- Explicit handshake and shutdown state handlers (`TLSHandshake`, `TLSShutdown`).
- OpenSSL adapters in `system/ssl.*` normalize errno handling around SSL read/write.

## SSL/TLS implementation details

### Handshake behavior

- Non-blocking handshake progression is integrated with read/write event receivers.
- `SSL_ERROR_WANT_READ` / `SSL_ERROR_WANT_WRITE` transitions decide which receiver is active.
- Success path enables normal data flow; error path disables receivers and reports status.

### Shutdown behavior

- TLS close-notify sequence is handled by `TLSShutdown` with timeout protection.
- Handles asymmetry where peer shutdown may require additional read/write cycles.
- On timeout, both directions are disabled and timeout callback is triggered.

### Utility wrappers

- `core::ssl::SSL_read` and `core::ssl::SSL_write` clear `errno` before calling OpenSSL functions.
- This reduces stale errno confusion when diagnosing TLS I/O edges.

## Functionality summary

- Unified stream socket life cycle (server + client)
- Context-based protocol injection point
- Legacy plaintext transport and TLS transport under one model
- Event-driven TLS handshake/shutdown integration

## Pros

1. **Clean separation** between transport mechanics and protocol logic.
2. **Same app-level patterns** across legacy and TLS.
3. **Good non-blocking TLS design** tied directly to event receivers.
4. **Template aliases and helper APIs** reduce boilerplate for consumers.

## Cons / tradeoffs

1. **Template-heavy layering** can produce long compiler diagnostics.
2. **TLS internals are intricate**; correctness relies on carefully handled edge states.
3. **Datagram branch appears less feature-complete** in current tree compared to stream branch.

## Practical guidance

- For public or cross-host deployments, prefer TLS connectors/acceptors.
- Keep legacy mode for local IPC, testing, or TLS-offloaded topologies.
- Treat handshake/shutdown timeout configuration as production-critical reliability knobs.
