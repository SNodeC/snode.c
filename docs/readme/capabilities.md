# SNode.C capability map

[← SNode.C](../../README.md)

This inventory describes the public framework’s major capability families. Optional dependencies and target-specific package selections determine which components are installed. The [API reference](https://snodec.github.io/snode.c-doc/html/index.html) supplies the class-level detail.

## Runtime and composition

| Capability | Scope |
| --- | --- |
| Event-driven execution | A shared event loop dispatches readiness, timers and lifecycle callbacks. Applications can combine multiple clients and servers. |
| Multiplexers | `select`, `poll` and Linux `epoll` implementations. |
| Timers and deferred work | One-shot/repeating timer and event facilities integrate application work into the runtime. |
| Client/server lifecycle | Connect and listen operations, state callbacks, retries and client reconnection. |
| Flow control | Each connect/listen activation has its own control handle. Keep the handle if you need to control that activation; discarding it does not stop the operation. |
| Shutdown | Signal-aware termination and connection shutdown support. |
| Streams and piping | Buffered reads/writes, pipe/source/sink facilities and configurable write-queue limits and watermarks. |
| Protocol extension | A `SocketContextFactory` creates each connection’s `SocketContext`; a context implements application-protocol behavior. |

SNode.C does not make a blocking callback asynchronous. Keep CPU-intensive or blocking application work away from the event-loop path and respect object lifetimes when integrating external workers.

## Connection variants

The stream families provide client and server templates. `legacy` means **plain/unencrypted**; `tls` selects the TLS connection layer.

| Family | C++ namespace | Endpoint | Plain / TLS |
| --- | --- | --- | --- |
| IPv4 | `net::in::stream` | Host/address and port | Both. |
| IPv6 | `net::in6::stream` | Host/address and port | Both. |
| Unix domain | `net::un::stream` | Filesystem socket path | Both. |
| Bluetooth RFCOMM | `net::rc::stream` | Bluetooth address and channel | Both; BlueZ-dependent. |
| Bluetooth L2CAP | `net::l2::stream` | Bluetooth address and PSM | Both; BlueZ-dependent. |

For example, a custom context factory can be used with `net::in::stream::legacy::SocketServer<MyFactory>` or `net::in::stream::tls::SocketServer<MyFactory>`. Changing the connection type does not replace the protocol implementation; certificates, trust and peer verification must still be configured for TLS.

`net::un::dgram::Socket` is a separate lower-level facility. This inventory covers the stream framework, not UDP, DTLS, QUIC or HTTP/3.

## HTTP and Express-style applications

- HTTP client and server APIs, including request and response headers, bodies and streaming.
- HTTP upgrades, used by WebSockets and custom upgrade implementations.
- Express-style route registration, composed routers and route parameters/regular-expression matching.
- Middleware for static resources, JSON bodies, HTTP Basic authentication, virtual hosts and request logging.
- Plain and TLS convenience wrappers; IP, Unix and RFCOMM wrappers are present where defined by the relevant module.
- File/body delivery and application-level control over buffering and resource limits.

HTTP/1.x is the intended scope of these APIs. Express-style naming does not imply drop-in compatibility with JavaScript Express or the Node.js package ecosystem.

## WebSockets

Client and server implementations handle HTTP upgrade, WebSocket framing and application subprotocols. Applications implement the subprotocol behavior separately from socket transport. The source includes echo subprotocol examples and MQTT-over-WebSocket adapters.

Plain WebSockets and TLS-protected WebSockets use the corresponding HTTP connection. A WebSocket connection is bidirectional; SSE is not a substitute when both sides must send application messages over the same stream.

The [complete WebSocket example](examples.md#a-websocket-echo-page) includes the HTTP upgrade route, browser client, build file and run instructions.

## Server-Sent Events

On the server, stream `text/event-stream` responses with SSE fields and message boundaries. On the client, `EventSource` provides open/error callbacks, ordinary message callbacks, named event listeners and reconnect behavior. Event IDs and the `Last-Event-ID` mechanism support resumption, but **the application must implement any event retention and replay policy**; reconnecting does not create a durable event log.

Use SSE for telemetry, progress and live status flowing from server to client. The [inline server and native client](../../README.md#receive-a-server-sent-event) demonstrate named-event delivery.

## MQTT

The framework contains **MQTT 3.1.1** packet handling, client/server APIs, broker facilities and MQTT-over-WebSocket adapters. MQTT QoS, sessions, subscriptions, retained messages and will messages belong to this protocol layer. Applications choose their authentication, authorization, persistence and resource policies.

SNode.C is the library foundation; [MQTTSuite](https://github.com/SNodeC/mqttsuite) supplies the broker, bridge, integrator, CLI and database-store executables. Neither this inventory nor MQTT 3.1.1 support implies MQTT 5 support.

## Data, configuration and operations

| Facility | What to use it for |
| --- | --- |
| MariaDB connector | Event-driven database access through the optional `db-mariadb` component. |
| C++ / CLI / configuration files | Configure application sections and named instances through the same configuration model. |
| `snodec-control` | Discover an application’s configuration, inspect/edit values, generate configuration files and optionally use a Curses terminal UI. |
| Retry and reconnect policy | Retry limits, delays, backoff and jitter; reconnect after connection loss. |
| Timeouts and limits | Connection timeouts, write queues and protocol/body limits; see the [resource-policy reference](https://github.com/SNodeC/snode.c/blob/master/docs/resource-policy-and-streaming.md). |
| Semantic logging | Text/JSON output; application/framework, component, boundary and instance filtering; structured context and hex dumps. |
| CMake components | Imported `snodec::…` targets for external applications. |
| Packaging | Component packages, source builds and OpenWrt SDK/feed builds. |

## Demonstrations and deployment

The source contains HTTP, echo, WebSocket, database and OAuth2 demonstrations. Production services still need explicit access controls, TLS configuration and qualification on their target hardware; the OAuth2 sample is a learning application.

[Install](install.md) · [Run examples](examples.md) · [Deploy](deployment.md)
