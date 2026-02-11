# HTTP module (`src/web/http/*`) — client, server, decoder, and EventSource

This document covers:

- `src/web/http/*`
- HTTP **server** stack (`src/web/http/server/*`)
- HTTP **client** stack (`src/web/http/client/*`)
- the shared parsing/decoding infrastructure (`Parser`, `decoder/*`, headers, status, mime types)
- the **EventSource** helper (Server-Sent Events client) under `client/tools/EventSource`

---

## 1. Overview

The HTTP module provides a full HTTP/1.x stack that is:

- **event-driven** (built on `core/socket` stream contexts)
- **transport-flexible** (IPv4/IPv6/Unix/Bluetooth; plain or TLS)
- designed to support higher-level frameworks (most notably `express`)
- explicitly supports **protocol upgrades** (WebSocket is implemented as an upgrade)

You can use the HTTP server directly (as in `src/apps/http/httpserver.cpp`) or indirectly through the Express module (which uses the HTTP server internally).

---

## 2. Public API entry points

### Shared HTTP utilities
- `#include "web/http/Parser.h"`
- `#include "web/http/CiStringMap.h"` — case-insensitive header map
- `#include "web/http/StatusCodes.h"`
- `#include "web/http/MimeTypes.h"`
- `#include "web/http/http_utils.h"`
- `#include "web/http/ContentDecoder.h"`
- `#include "web/http/SocketContextUpgrade.h"` / `SocketContextUpgradeFactory.h`

### Server
- `#include "web/http/server/Server.h"`
- `#include "web/http/server/Request.h"`
- `#include "web/http/server/Response.h"`
- `#include "web/http/server/SocketContext.h"`
- transport bindings:
  - `web/http/legacy/in/*`, `in6/*`, `un/*`, `rc/*`
  - `web/http/tls/in/*`, `in6/*`, `un/*`, `rc/*`

### Client
- `#include "web/http/client/Client.h"`
- `#include "web/http/client/MasterRequest.h"`
- `#include "web/http/client/Request.h"`
- `#include "web/http/client/Response.h"`
- request commands:
  - `web/http/client/commands/*` (GET, POST, SSE, …)
- tools:
  - `web/http/client/tools/EventSource.h` (SSE client)

---

## 3. Transport variants: legacy vs TLS

The module provides “pre-bound” convenience type families:

- **legacy** — plain TCP-like streams:
  - `src/web/http/legacy/in/*` (IPv4)
  - `src/web/http/legacy/in6/*` (IPv6)
  - `src/web/http/legacy/un/*` (Unix domain sockets)
  - `src/web/http/legacy/rc/*` (Bluetooth RFCOMM)

- **tls** — TLS-wrapped streams:
  - same address families under `src/web/http/tls/*`

Internally, these are typedefs that pick:

- a `core/socket/stream` (legacy or TLS)
- a physical socket type (IPv4/IPv6/unix/rc)
- a server/client config type (from `net/config`)

This is why the HTTP module stays transport-agnostic.

---

## 4. Server stack (`src/web/http/server/*`)

### 4.1 The server template

The main server type is:

- `web::http::server::Server<SocketServerT>` (`src/web/http/server/Server.h`)

It is a thin adapter that “turns a socket server into an HTTP server” by selecting:

- `web::http::server::SocketContextFactory`
- and an `onRequestReady(req, res)` callback signature

This design pushes HTTP parsing into the socket context.

### 4.2 Server socket contexts: parsing + response coupling

The server `SocketContext` (`src/web/http/server/SocketContext.h/.cpp`) is where:

- bytes are read from the connection
- HTTP parsing happens (via `web::http::Parser`)
- request objects are built
- response objects are associated with the request
- and the `onRequestReady` callback is invoked when a request is complete

### 4.3 Request and response objects

- `web::http::server::Request` represents the parsed request state:
  - method, target URL, query, headers, body, cookies, etc.
- `web::http::server::Response` is the sending facade:
  - set status and headers
  - send body in one chunk or as fragments
  - send files (via core/file utilities)
  - upgrade the underlying socket context (e.g. to WebSocket)

Express wraps these types and offers an Express-like facade (`express::Request`, `express::Response`).

### 4.4 Backpressure and “fragment” sending

SNode.C supports incremental output through `sendFragment(...)` APIs (see Express response facade too).

This matters for:

- Server-Sent Events (SSE)
- chunked responses
- large file streaming

Because the runtime is event-driven, correct backpressure handling is crucial:
sending huge data without considering write readiness would stall or grow queues.

---

## 5. Client stack (`src/web/http/client/*`)

### 5.1 Client template and MasterRequest

The main client type:

- `web::http::client::Client<SocketClientT>` (`src/web/http/client/Client.h`)

It is parametrized by a socket-client template and binds:

- `web::http::client::SocketContextFactory`
- on-connect and on-disconnect callbacks for HTTP-level lifecycle
- an accessor for the underlying `net::config::ConfigInstance` (so the factory can read config)

The `MasterRequest` concept groups a connection with a sequence of requests and their state.
This is what enables:

- connection reuse
- pipelining (optional; see `setPipelinedRequests(bool)`)

### 5.2 Request commands

Under `src/web/http/client/commands/*`, requests are represented as commands that execute against a `MasterRequest`.

Examples include:

- basic request/response commands
- `SseCommand` (`client/commands/SseCommand.*`) for SSE parsing

This command-based design makes it easier to add new client behaviors (auth, retries, streaming) without turning the client into a monolith.

---

## 6. Parsing and decoding infrastructure

### 6.1 `web::http::Parser`

`Parser` (`src/web/http/Parser.h/.cpp`) is used by both client and server socket contexts.

It is responsible for:

- parsing request/response start lines
- parsing headers into `CiStringMap`
- tracking state transitions (`ConnectionState` enum, see `ConnectionState.h`)
- determining body handling:
  - content-length
  - chunked transfer encoding
  - connection-close semantics

### 6.2 Decoders (`src/web/http/decoder/*`)

Decoders implement body decoding strategies:

- `Identity` (raw body)
- `Chunked` (HTTP chunked transfer decoding)
- `HTTP10Response` (HTTP/1.0 response specifics)

The separation allows:

- strict parsing
- reuse in both client and server sides
- incremental feeding (important for non-blocking reads)

### 6.3 Headers and utilities

- `CiStringMap` provides case-insensitive header keys (matching HTTP semantics).
- `ContentType`, `MimeTypes` help with content negotiation and file sending.
- `StatusCodes` centralizes status texts.
- `http_utils` contains common parsing and formatting helpers.

---

## 7. Protocol upgrades: `SocketContextUpgrade*`

SNode.C models upgrades as explicit transitions at the socket-context level:

- `SocketContextUpgrade` and `SocketContextUpgradeFactory` define the interface
- `SocketContextUpgradeFactorySelector` selects an upgrade factory based on request data
- Upgrades are used heavily by WebSocket:
  - HTTP parses the upgrade request
  - the response triggers `upgrade(...)`
  - the socket context is swapped into a WebSocket context

This architecture keeps the HTTP parser small and leaves frame-level behavior to the WebSocket module.

---

## 8. EventSource (`client/tools/EventSource`) — SSE client

`web::http::client::tools::EventSource` (`src/web/http/client/tools/EventSource.h`) implements a client-side **Server-Sent Events** consumer.

### 8.1 API and lifecycle

EventSource provides:

- constructor: `EventSource(url, options)`
- callbacks:
  - `onOpen(handler)`
  - `onError(handler)`
  - `onMessage(handler)`
  - `addEventListener(type, handler)`
- control:
  - `connect()`, `close()`
- state:
  - `readyState()` (`CONNECTING`, `OPEN`, `CLOSED`)
  - `url()`, `withCredentials()`, `retry()`, `lastEventId()`

### 8.2 Parsing model (SSE specifics)

The class implements the SSE line-based parsing rules:

- lines are collected into fields:
  - `data: …`
  - `event: …`
  - `id: …`
  - `retry: …`
- an empty line dispatches the accumulated event
- data lines are concatenated with `\n` semantics

You can see internal limits intended to prevent runaway input:

- `maxlineLength` (default 1024)
- `maxdataSize` (default 16 MiB)

### 8.3 Reconnect and retry

EventSource maintains:

- `reconnectInterval` (default 3 seconds)
- `retryTime` / `retryTimer`
- automatic reconnect scheduling after disconnect/errors

The `retry:` field can update the reconnect interval (SSE standard behavior).

### 8.4 Headers and credentials

`Options` allow:

- custom headers
- `withCredentials` flag
- initial `method` and body (even though classic EventSource uses GET, this is a pragmatic extension)

The implementation also tracks `lastEventId` and uses it across reconnects.

---

## 9. Example: minimal HTTP server

The simplest shape of an HTTP server is:

```cpp
#include "core/SNodeC.h"
#include "web/http/legacy/in/Server.h"   // choose your transport

int main(int argc, char** argv) {
  core::SNodeC::init(argc, argv);

  web::http::legacy::in::Server server(
    "http",
    [](auto const& req, auto const& res) {
      res->set("content-type", "text/plain");
      res->send("Hello from SNode.C\n");
      res->end();
    }
  );

  server.listen();       // binds based on config
  core::SNodeC::start(); // enter event loop
  core::SNodeC::free();
}
```

For a real example with TLS and SNI, see: `src/apps/http/httpserver.cpp`.

---

## 10. Pros / cons

### Pros
- Unified client/server parsing and decoding.
- Transport-agnostic design: IPv4/IPv6/unix/bluetooth, plain or TLS.
- Explicit protocol upgrade hooks (WebSocket integration is clean).
- EventSource provides a high-level SSE client without external dependencies.

### Cons / tradeoffs
- HTTP/2 is not the focus here; this module is fundamentally HTTP/1.x oriented.
- The template-based transport binding leads to many types (helpful for performance, harder for newcomers).

### Gotchas
- Correctly handle connection keep-alive vs close; the parser will decide based on headers and version.
- When streaming responses, remember to flush headers (`sendHeader`) and use fragments, not `send()` huge buffers.
- EventSource parsing is strict about line breaks and limits; if your server emits unusually long lines, tune options.

---

## 11. Next steps

- [`express.md`](express.md) — Express.js-like routing and middleware on top of this HTTP stack.
- [`websocket.md`](websocket.md) — WebSocket upgrade + frame I/O built on `SocketContextUpgrade`.
