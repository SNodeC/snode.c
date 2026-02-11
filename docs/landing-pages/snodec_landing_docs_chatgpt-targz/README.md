# SNode.C — module overview and landing-page documentation

This folder contains **landing-page style documentation** for SNode.C, split by module, plus an overall `README.md` that connects everything.

> Paths below are relative to the repository root (the code base you provided in `src/…`).
> The documentation is intentionally *source-oriented*: it references concrete types, files, and responsibilities so readers can jump directly from docs to code.

## What is SNode.C?

SNode.C is a slim, modular C++ toolkit for building networked systems (servers, clients, gateways) on top of a **single event-loop abstraction** and a consistent **socket/transport model**.

It is organized into modules, each of which can be used independently but is designed to compose:

- `core` — runtime: event loop, multiplexer, timers, descriptor event receivers, dynamic loader, file utilities.
- `core/socket` — socket layer: stream & datagram sockets + TLS/SSL support, socket contexts, connection lifecycle.
- `net` — transport address families and reusable *configuration* objects for socket endpoints (CLI + config file + programmatic).
- `web/http` — HTTP client & server stacks + reusable parser/decoder infrastructure; includes a **client-side EventSource** helper.
- `express` — Express.js-inspired routing/middleware for `web/http` servers, including static files, JSON middleware, vhost, auth.
- `web/websocket` — WebSocket upgrade and framing, client and server subprotocol infrastructure, grouping helpers.
- `iot/mqtt` — MQTT protocol implementation (client & server/broker) built on SNode.C sockets.
- `database` — MariaDB client integration (sync + async) driven by the same event loop.

## How to read this documentation

Each module document follows the same structure:

- **Overview & Design Goals**
- **Key Concepts and Architecture**
- **Public API entry points** (headers most users include)
- **Configuration & lifecycle**
- **Typical flows** (how objects cooperate at runtime)
- **Examples**
- **Pros / Cons / Gotchas**

## Module map

Start here:

1. **Core runtime (core + utils + log)**  
   → [`core.md`](core.md)

2. **Socket layer (core/socket) incl. TLS**  
   → [`core_socket.md`](core_socket.md)

3. **Network abstraction and configuration (net + net/config)**  
   → [`net.md`](net.md)

4. **HTTP (web/http) incl. EventSource (SSE client)**  
   → [`web_http.md`](web_http.md)

5. **Express-like web framework (express)**  
   → [`express.md`](express.md)

6. **WebSocket (web/websocket)**  
   → [`websocket.md`](websocket.md)

7. **MQTT (iot/mqtt)**  
   → [`iot_mqtt.md`](iot_mqtt.md)

8. **Database (database; MariaDB integration)**  
   → [`database.md`](database.md)

## Quick mental model

The following layering shows the intended dependency direction:

```
            +------------------------------+
            |          application         |
            | (apps/*, your service code)  |
            +---------------+--------------+
                            |
                            v
  +-------------------+  +------------------+  +--------------------+
  |      express      |  |   web/websocket  |  |      iot/mqtt       |
  +---------+---------+  +---------+--------+  +----------+---------+
            |                      |                      |
            v                      v                      v
        +---+----------------------+----------------------+-+
        |                web/http (client/server)           |
        +---------------------------+-----------------------+
                                    |
                                    v
                         +----------+----------+
                         |  net (addresses +   |
                         |   config objects)   |
                         +----------+----------+
                                    |
                                    v
                         +----------+----------+
                         | core/socket (stream |
                         |  + dgram + TLS)     |
                         +----------+----------+
                                    |
                                    v
                         +----------+----------+
                         | core (event loop,   |
                         | multiplexer, timers |
                         +---------------------+
```

## Build & run (high-level)

SNode.C is CMake-based and typically builds shared libraries plus example apps. The exact build flags and targets vary by platform and enabled modules; see the repository’s top-level build instructions.

A **typical runtime lifecycle** looks like:

1. `core::SNodeC::init(argc, argv);`  
2. Create and configure instances (e.g. HTTP server / WebApp / MQTT broker / DB client).  
3. Call `.listen()` / `.connect()` as needed.  
4. Enter the loop via `core::SNodeC::start()` (or `tick()` for embedding).  
5. On shutdown: `core::SNodeC::stop();` and `core::SNodeC::free();`

Express’ `express::WebApp` is essentially a thin facade over `core::SNodeC` (see `src/express/WebApp.cpp`).

## Where to look for real code examples

The `src/apps/…` directory contains example applications that exercise most modules, e.g.:

- HTTP server: `src/apps/http/httpserver.cpp`
- WebSocket echo server: `src/apps/websocket/echoserver.cpp`
- MariaDB test app: `src/apps/database/testmariadb.cpp`
- Express compat server: `src/apps/express_compat_server.cpp`

The docs in this folder often reference these examples.

---

If you want this documentation to become the “front door” of the GitHub repository, the usual pattern is:

- keep the repository root `README.md` short and inviting,
- put deeper docs under `docs/` (or `doc/`) and link them from the root.

This folder is already structured that way: you can copy it verbatim to `docs/` or merge it into your existing documentation layout.
