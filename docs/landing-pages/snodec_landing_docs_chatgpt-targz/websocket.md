# WebSocket module (`src/web/websocket/*`) — upgrade, framing, subprotocols

This document covers `src/web/websocket/*`.

SNode.C’s WebSocket implementation is designed to be:

- **RFC6455-style framing** (opcode, payload length, masking, continuation frames)
- fully **event-driven**, built on the `web/http` upgrade mechanism
- extensible via **subprotocols** (server and client side)
- convenient for multi-client apps through **groups** and broadcast helpers

---

## 1. Public API entry points

### Core framing
- `#include "web/websocket/Receiver.h"` — frame parsing (state machine)
- `#include "web/websocket/Transmitter.h"` — frame creation + send
- `#include "web/websocket/SubProtocol.h"` / `SubProtocol.hpp` — protocol hooks (generic)

### Upgrade glue (HTTP ↔ WebSocket)
- `#include "web/websocket/SocketContextUpgrade.h"` / `SocketContextUpgrade.hpp`
- Server side:
  - `#include "web/websocket/server/SocketContextUpgradeFactory.h"`
  - `#include "web/websocket/server/SubProtocol.h"`
  - `#include "web/websocket/server/SubProtocolFactorySelector.h"`
  - `#include "web/websocket/server/GroupsManager.h"`
- Client side:
  - `#include "web/websocket/client/SocketContextUpgradeFactory.h"`
  - `#include "web/websocket/client/SubProtocol.h"`
  - `#include "web/websocket/client/SubProtocolFactorySelector.h"`

---

## 2. How WebSocket fits into the stack

WebSocket in SNode.C is implemented as an **HTTP upgrade**:

1. HTTP server parses an incoming request.
2. A handler decides to upgrade the connection (`res->upgrade(req, ...)`).
3. The HTTP socket context is replaced by a `websocket::SocketContextUpgrade`.
4. The WebSocket context:
   - emits the handshake response
   - then switches into a pure frame I/O loop (Receiver/Transmitter)
5. Application code interacts through a **SubProtocol** instance.

This model keeps HTTP parsing and WebSocket framing separate and avoids “if upgrade then …” branches in the HTTP parser.

---

## 3. Frame parsing: `web::websocket::Receiver`

`Receiver` (`src/web/websocket/Receiver.h/.cpp`) is a streaming parser for WebSocket frames.

### 3.1 State machine

The parser is explicit, with states:

- `BEGIN`
- `OPCODE`
- `LENGTH` (payload length base field)
- `ELENGTH` (extended payload length 16/64)
- `MASKINGKEY`
- `PAYLOAD`
- `ERROR`

It maintains:

- `fin`, `continuation`
- `masked` and `maskingExpected`
- opcode and length fields
- byte counters for partial reads (important for non-blocking I/O)

### 3.2 Masking enforcement

The `maskingExpected` constructor argument allows enforcing correct behavior:

- client-to-server frames **must** be masked
- server-to-client frames typically are not masked

SNode.C keeps this explicit so that server and client contexts can be strict where needed.

### 3.3 Message callbacks

`Receiver` is abstract: it requires derived contexts to implement:

- `onMessageStart(opCode)`
- `onMessageData(chunk, len)`
- `onMessageEnd()`
- `onMessageError(errnum)`
- and the low-level `readFrameChunk(...)` that reads bytes from the actual connection

This separates parsing logic from transport I/O.

---

## 4. Frame sending: `web::websocket::Transmitter`

`Transmitter` (`src/web/websocket/Transmitter.h/.cpp`) constructs and sends frames.

### 4.1 Message-level API

It supports:

- `sendMessage(opCode, payload, len)` — one-shot send
- `sendMessageStart(opCode, ...)` / `sendMessageFrame(...)` / `sendMessageEnd(...)` — fragmented messages

Fragmentation is important for:

- streaming large binary payloads
- sending partial updates without buffering everything

### 4.2 Masking for clients

The transmitter can be constructed with `masking=true` so that outgoing frames are masked (client requirement).
It internally uses randomness (`std::random_device` + `uniform_int_distribution`) to generate masking keys.

### 4.3 Close handling

`closeSent` tracks whether a close frame has already been sent, preventing repeated close sequences.

---

## 5. Subprotocols: application logic hooks

SNode.C models application logic as a **SubProtocol** object:

- generic base: `web::websocket::SubProtocol<SocketContextT>`
- server specialization: `web::websocket::server::SubProtocol`
- client specialization: `web::websocket::client::SubProtocol`

A subprotocol typically implements callbacks like:

- `onConnected`
- `onMessage`
- `onPing` / `onPong`
- `onClose`
- etc. (depending on the base class contract)

### 5.1 Server SubProtocol extras: groups and broadcast

Server-side subprotocol (`src/web/websocket/server/SubProtocol.h`) provides:

- `subscribe(group)` — place this connection into a named group
- `sendBroadcast(...)` — send to all connections in the group
- `forEachClient(handler, excludeSelf)` — iterate over group clients
- `excludeSelf` toggles whether the sending client receives its own broadcast

This is backed by `GroupsManager` (`server/GroupsManager.*`) and is extremely useful for:

- chat rooms
- live dashboards
- collaborative control panels

---

## 6. Upgrade contexts

The upgrade context is the bridge between HTTP and WebSocket.

### 6.1 `web::websocket::SocketContextUpgrade` (generic)

`web/websocket/SocketContextUpgrade.h/.hpp` provides the generic upgrade logic:

- emits handshake response headers
- binds a receiver and transmitter to the underlying socket connection
- manages ping/pong and close handshake
- holds a `SubProtocolContext` object (`SubProtocolContext.h`) that provides connection metadata and APIs to the subprotocol

### 6.2 Server upgrade: `web::websocket::server::SocketContextUpgrade`

`server/SocketContextUpgrade.h` specializes the generic upgrade for:

- HTTP server request/response types
- server-side subprotocol selection via `loadSubProtocol(subProtocolNames)`

It stores a pointer to a `SubProtocolFactory<SubProtocol>`.

### 6.3 Subprotocol selection: factory selectors

Selectors choose which subprotocol to instantiate based on:

- the list of `Sec-WebSocket-Protocol` names offered by the peer
- server configuration (which protocols are supported)

This logic is separated into:

- `server/SubProtocolFactorySelector.*`
- `client/SubProtocolFactorySelector.*`

so that selection behavior can evolve without touching frame parsing.

---

## 7. Example: WebSocket echo server

The sample app `src/apps/websocket/echoserver.cpp` demonstrates the typical stack:

1. Express app routes HTTP.
2. On `/ws`, it upgrades and installs a subprotocol.
3. The subprotocol echoes messages and can broadcast to groups.

This example is the recommended starting point when building a custom subprotocol.

---

## 8. Pros / cons

### Pros
- Clean separation: HTTP upgrade vs WebSocket framing vs application logic.
- Strict, incremental frame parsing suitable for non-blocking sockets.
- Built-in helpers for multi-client groups and broadcasts.

### Cons / tradeoffs
- WebSocket is inherently stateful; misuse of blocking operations in callbacks hurts the whole event loop.
- Subprotocol factory selection must be configured correctly; otherwise clients may connect without a protocol match.

### Gotchas
- Remember masking rules: clients must mask outgoing frames; servers must enforce masking on incoming client frames.
- Ping/pong intervals and max flying pings must be tuned for your environment; aggressive pinging can cause unnecessary traffic.
- Large payloads should be sent as fragments to avoid memory spikes and queue growth.

---

## 9. Next steps

- [`iot_mqtt.md`](iot_mqtt.md) — MQTT client/server built on the same event-driven socket layer.
- [`database.md`](database.md) — MariaDB integration, also driven by FD events.
