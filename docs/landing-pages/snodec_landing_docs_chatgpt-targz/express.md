# Express module (`src/express/*`) — Express.js-inspired routing and middleware

This document covers the `src/express/*` module in detail.

SNode.C’s Express module provides an **Express.js v4-like programming model** on top of the SNode.C HTTP server:

- `express::WebApp` wraps an HTTP server instance and exposes the familiar `app.use(...)`, `app.get(...)`, … API.
- `express::Router` builds routing trees (routers mounted into routers).
- Request/response wrappers (`express::Request`, `express::Response`) provide an Express-like surface.
- A dispatcher subsystem implements matching and “next()” behavior.
- A small collection of useful middleware is included (static files, JSON, basic auth, vhost, verbose logging).
- A compatibility test suite exists under `src/express/express-compat-suite/*`.

---

## 1. Design goals

The Express module aims to:

- provide a **productive web API** in idiomatic C++ while staying close to Express.js semantics,
- remain transport-agnostic (HTTP over IPv4/IPv6/unix/bluetooth; plain or TLS),
- keep request dispatching predictable and observable (routes listable, consistent matching rules),
- integrate seamlessly with the SNode.C event loop and backpressure model.

Unlike “full-stack” C++ web frameworks, this module intentionally stays thin:
it delegates I/O, connection management, and HTTP parsing to the lower layers.

---

## 2. Public API entry points

### Core Express API
- `#include "express/WebApp.h"` — application facade (templated + concrete bindings)
- `#include "express/Router.h"` — router trees and method chaining
- `#include "express/Request.h"` / `express/Response.h`
- `#include "express/Next.h"` — `next()` continuation object

### Middleware
- `#include "express/middleware/StaticMiddleware.h"`
- `#include "express/middleware/JsonMiddleware.h"`
- `#include "express/middleware/BasicAuthentication.h"`
- `#include "express/middleware/VHost.h"`
- `#include "express/middleware/VerboseRequest.h"`

### Transport bindings
Express provides “pre-bound” WebApp types under:

- `src/express/legacy/*` (plain HTTP)
- `src/express/tls/*` (HTTPS)

with address family subfolders:

- `in` (IPv4), `in6` (IPv6), `un` (Unix), `rc` (Bluetooth RFCOMM)

Example include:
- `#include "express/legacy/in/WebApp.h"`

---

## 3. High-level architecture

At runtime, an Express request flows like this:

1. An HTTP connection receives bytes.
2. `web::http::server::SocketContext` parses the request.
3. Express wraps HTTP request/response into `express::Request` and `express::Response`.
4. A `express::Controller` is created (holds request/response + dispatch state).
5. The controller is dispatched through the **route tree** (`RootRoute`).
6. Dispatchers match mount points, update `req.baseUrl`, `req.url`, `req.path`, and `req.params`.
7. The chosen middleware/handler is invoked.
8. The handler either completes the response (`res.send(...); res.end();`) or calls `next()` to continue.

---

## 4. `WebApp`: the application facade

### 4.1 `express::WebAppT` and concrete aliases

`express::WebAppT<Server>` (declared in `express/WebApp.h` and implemented in `WebApp.hpp/.cpp`) is a thin wrapper around:

- a specific HTTP server type
- and a `express::Router` (via the `RootRoute`)

It exposes:

- `listen()`, `stop()`
- access to the underlying server config (`getConfig()`)
- `init(...)` and `start(...)` which forward to `core::SNodeC` (see `WebApp.cpp`)

This makes simple programs feel like Node’s Express:

```cpp
express::WebApp app;
app.get("/hello", APPLICATION(req, res) {
  res->send("hi");
  res->end();
});
app.listen();
app.start();
```

### 4.2 Transport bindings

The module offers convenience typedefs like:

- `express::legacy::in::WebApp`
- `express::tls::in6::WebApp`

These choose a concrete HTTP server and configuration object, so user code does not need to spell out long template types.

---

## 5. Router trees and routes

### 5.1 `express::Router`

`express::Router` (`src/express/Router.h/.hpp`) is the main user-facing builder.

It supports:

- `.use(...)` for middleware, routers, or direct handlers
- HTTP verb methods:
  - `.get`, `.post`, `.put`, `.del`, `.patch`, …
- router settings:
  - `setStrictRouting(bool)`
  - `setCaseInsensitiveRouting(bool)`
  - `setMergeParams(bool)`
- route introspection:
  - `getRoutes()`

The `DECLARE_ROUTER_REQUESTMETHOD` macros generate overloads for:

- mounting routers (`router.use("/x", childRouter)`)
- application-style handlers (no `next`)
- middleware-style handlers (with `next`)
- multiple lambdas chained

### 5.2 `RootRoute` and `Route`

Internally:

- `RootRoute` is the top-level route list, owned by a router.
- Each `Route` contains:
  - a `MountPoint` (`method`, `relativeMountPath`)
  - a `Dispatcher` implementation (application/middleware/router dispatcher)
  - a `nextRoute` pointer (forming a chain)

This is what enables Express-like “stack ordering” and predictable dispatching.

---

## 6. Dispatchers: implementing “next()” and matching

The dispatcher subsystem lives under `src/express/dispatcher/*` and is the core of Express semantics.

### 6.1 `express::Dispatcher`

A dispatcher:

- implements `dispatch(controller, mountPoint, strict, caseInsensitive, mergeParams)`
- can chain to the next route via `dispatchNext(...)`
- can provide route listing (`getRoutes(...)`)

`Route` objects own a dispatcher and use it during dispatch.

### 6.2 Application vs middleware dispatchers

There are multiple concrete dispatchers:

- **ApplicationDispatcher**: handlers without `next`
  - signature: `(req, res)`
  - if a handler returns, the route is considered handled unless the dispatcher explicitly continues

- **MiddlewareDispatcher**: handlers with `next`
  - signature: `(req, res, next)`
  - handler must call `next()` to continue down the chain

- **RouterDispatcher**: composite dispatcher for nested routers
  - owns a list of child routes
  - implements the composite pattern (explicitly mentioned in `RouterDispatcher.h`)

### 6.3 Controller: dispatch state machine

`express::Controller` keeps:

- `std::shared_ptr<Request>` and `std::shared_ptr<Response>`
- pointers to root/current routes
- flags controlling next behavior:
  - `NEXT`, `NEXT_ROUTE`, `NEXT_ROUTER`

`next("…")` encodes the control flow, similar to Express.js:

- `next()` → go to next matching route/middleware
- `next("route")` → skip remaining handlers in this route
- `next("router")` → exit the current router and continue at the parent level

Dispatchers interpret these flags via `controller.dispatchNext(...)`.

### 6.4 Mount-point matching and parameter extraction (`regex_utils`)

`src/express/dispatcher/regex_utils.h/.cpp` implements Express-like route matching.

Key responsibilities:

- **Method matching** (`methodMatches`)
  - `"use"` and `"all"` semantics (wildcard methods)
  - case sensitivity behaviors

- **Path joining** (`joinMountPath`)
  - safe concatenation of parent + relative mount paths without `//`

- **Match + consume semantics**
  - on a match, the matcher computes:
    - whether the match was prefix-only
    - how many characters of the URL path were consumed
    - extracted params (`std::map<string,string>`)
    - query pairs (to avoid treating query strings as part of the path)

- **Scoped URL stripping and restoration**
  - `ScopedPathStrip` temporarily adjusts:
    - `req.url`
    - `req.baseUrl`
    - `req.path`
    - `req.file` (legacy field)
  - This models Express’ “router mounts change the effective URL”.

- **Scoped params snapshot/restore**
  - `ScopedParams` snapshots `req.params` and restores on scope exit.
  - Supports `mergeParams`:
    - if enabled, child params are merged into parent params
    - otherwise params are layer-scoped (Express behavior)

The net effect is: nested routers behave like in Express: they see a “stripped” URL and have their own params scope.

### 6.5 Supported path patterns (Express v4-like)

From the comments and implementation approach, the matcher supports:

- named parameters: `:id`
- optional/quantifier modifiers for params: `:id?`, `:id+`, `:id*`
- custom regex for params: `:id(<re>)`
- wildcards and operators: `*`, `?`, `+`, and grouping `(...)`

This is intended to approximate Express’ `path-to-regexp` behavior for *string routes*.

---

## 7. Request and response objects

### 7.1 `express::Request`

`express::Request` wraps `web::http::server::Request` and exposes an Express-like surface:

- `method`, `url`, `httpVersion`, `httpMajor`, `httpMinor`
- `headers`, `cookies`, `queries`
- `body` (request body as string)
- Express-derived fields computed in `extend()` (`Request.cpp`):
  - `originalUrl` (full raw url)
  - `path` (pathname only; no query string)
  - `originalPath` (decoded pathname)
  - `baseUrl` (built from mount stack)
  - `params` (route params; filled by dispatcher)
- helpers:
  - `req.get(headerName, i)`
  - `req.cookie(name)`, `req.query(name)`
  - `req.param(id, fallback)` (param with fallback)

**Note**: `extend()` makes a clear distinction between raw path and decoded path,
which is essential for correct filesystem mapping and security-sensitive routing.

### 7.2 `express::Response`

`express::Response` wraps `web::http::server::Response` and provides common Express-style helpers:

- `status(code)`
- `set(field, value)`, `append(field, value)`, `type(mime)`
- `cookie(name, value, options)`, `clearCookie(...)`
- `send(string|buffer)`, `end()`
- `json(nlohmann::json)`
- `sendFile(path)`, `download(path, filename)`
- redirects:
  - `redirect(location)`, `redirect(code, location)`
  - `location(loc)`
- streaming:
  - `sendHeader()`
  - `sendFragment(...)`
- protocol upgrade:
  - `upgrade(req, statusCallback)` (used for WebSocket)

Because it wraps the HTTP response, it inherits the same backpressure and connection semantics.

---

## 8. Built-in middleware

SNode.C ships with a small set of middleware that covers common needs without external dependencies.

### 8.1 Static file middleware

- `express/middleware/StaticMiddleware.*`

Responsibilities typically include:

- mapping a request path to a filesystem root
- safe path normalization (avoid `..` traversal)
- setting correct content types (via `web/http/MimeTypes`)
- optionally enabling directory indexes / default files

### 8.2 JSON middleware

- `express/middleware/JsonMiddleware.*`

Parses JSON bodies (likely using `nlohmann::json`) and sets response/request helpers.

### 8.3 Basic authentication

- `express/middleware/BasicAuthentication.*`

Implements HTTP Basic Auth via header inspection.

### 8.4 Virtual host middleware

- `express/middleware/VHost.*`

Routes based on `Host` header to different routers/apps (useful with TLS SNI + vhosts).

### 8.5 Verbose request logging

- `express/middleware/VerboseRequest.*`

Convenience middleware to log requests and possibly latency/headers.

---

## 9. Example: Express app with a router and WebSocket upgrade

The WebSocket echo server in `src/apps/websocket/echoserver.cpp` demonstrates a classic pattern:

- Express handles HTTP routing
- For `/ws` it upgrades to WebSocket and installs a WebSocket subprotocol
- For normal routes it serves plain HTTP

The relevant part is:

```cpp
app.get("/ws", [&](auto req, auto res) {
  res->upgrade(req, [&](const std::string status) {
    // WebSocket handshake response status callback
  });
});
```

After upgrade, the WebSocket module takes over frame parsing and transmission.

---

## 10. Express compatibility suite

The folder:

- `src/express/express-compat-suite/*`

contains a Node.js reference implementation and an SNode.C implementation used to compare semantics across a large set of tests.

This is important because:
- route matching semantics are subtle (especially with nested routers and `mergeParams`)
- query string handling must match Express (queries are not part of path matching)
- wildcard captures and parameter decoding must match expected behavior

If you contribute to routing behavior, running this suite is strongly recommended.

---

## 11. Pros / cons

### Pros
- Express-like ergonomics in C++ with minimal overhead.
- Router trees, middleware chains, and `next()` semantics are implemented explicitly (and testable).
- Transport bindings make it easy to deploy the same app on different transports or with TLS.

### Cons / tradeoffs
- Express’ matching semantics are complex; parity is an ongoing effort.
- Template-heavy transport bindings can make type errors verbose.
- Middleware lifecycle differs from Node: everything runs in one event loop thread by default; long tasks must be offloaded explicitly.

### Gotchas
- Don’t do CPU-heavy work inside a handler; it blocks the entire loop. Use a worker thread and respond asynchronously.
- Understand `mergeParams` and param scope:
  - if you expect parent params to be visible in child routers, enable `setMergeParams(true)` on the child router (matching Express).
- Use `req.originalPath` (decoded) carefully for filesystem operations; don’t trust raw `req.url` blindly.

---

## 12. Next steps

- [`websocket.md`](websocket.md) — the WebSocket layer that Express upgrades into.
- [`web_http.md`](web_http.md) — the underlying HTTP server and upgrade facilities.
