# Example applications

The programs in this directory are runnable examples of SNode.C composition and
configuration. They are useful as source references, but they are not a support
matrix: a target existing in the tree does not by itself mean that every
transport, connection mode, protocol, and platform combination has equivalent
runtime qualification.

Start with the plain IPv4 echo pair below. It is deliberately small enough to
trace from C++ types through CMake, configuration, connection establishment, and
per-connection callbacks without introducing HTTP, TLS, WebSocket, or another
application protocol first.

[Project README](../../README.md) ·
[API documentation](https://snodec.github.io/snode.c-doc/html/index.html) ·
[Echo sources](echo/)

## Worked path: plain IPv4 echo

This walkthrough follows exactly one build variant:

| Item | Selection |
| --- | --- |
| Address family | IPv4 · `in` |
| Stream mode | plain/non-TLS · `legacy` |
| Server target | `echoserver-legacy-in` |
| Client target | `echoclient-legacy-in` |
| Server instance | `echoserver` |
| Client instance | `echoclient` |
| Example address | `127.0.0.1:18001` |

The echo sources are shared by several generated targets. For this target pair,
CMake defines `NET=in` and `STREAM=legacy`; the preprocessor-selected types
therefore reduce to the concrete IPv4/plain types shown below.

> **Qualification note.** The `echoserver-legacy-in` / `echoclient-legacy-in`
> pair at commit
> [`bf01683`](https://github.com/SNodeC/snode.c/commit/bf01683a53b48220a840522e8ccaf3b48e58c240)
> was configured, built, and run on IPv4 loopback during the August 2026
> publication qualification. The recorded information-level output established
> listener startup and connection formation. Information-level logging does not
> print the reflected payload; the payload path is visible in the source below
> and can be made visible at runtime with application trace logging.

## Files in the path

The complete example contains transport-independent echo behavior plus the
client/server wrappers that select a concrete network target:

| File | Responsibility |
| --- | --- |
| [`echo/model/EchoSocketContext.h`](echo/model/EchoSocketContext.h) | Declares the per-connection echo context and the server/client context factories. |
| [`echo/model/EchoSocketContext.cpp`](echo/model/EchoSocketContext.cpp) | Sends the client greeting and reflects every received chunk. |
| [`echo/model/servers.h`](echo/model/servers.h) | Selects the concrete `SocketServer` type and creates the named server instance. |
| [`echo/model/clients.h`](echo/model/clients.h) | Selects the concrete `SocketClient` type and creates the named client instance. |
| [`echo/echoserver.cpp`](echo/echoserver.cpp) | Initializes SNode.C, starts listening, reports listener state, and enters the event loop. |
| [`echo/echoclient.cpp`](echo/echoclient.cpp) | Initializes SNode.C, starts connecting, reports connection state, and enters the event loop. |
| [`echo/model/CMakeLists.txt`](echo/model/CMakeLists.txt) | Builds the shared example context as the local `echosocketcontext` static library. |
| [`echo/CMakeLists.txt`](echo/CMakeLists.txt) | Generates the client/server executables for the selected network and stream variants. |

The TLS branches and the other address families are intentionally outside this
first path. They use the same application context but add connection- or
address-family-specific behavior.

## 1. Define connection-local behavior

`EchoSocketContext` is the application object attached to one established
`SocketConnection`. The two factories create the same context with different
roles:

```cpp
class EchoSocketContext : public core::socket::stream::SocketContext {
public:
    enum class Role { SERVER, CLIENT };

    explicit EchoSocketContext(
        core::socket::stream::SocketConnection* socketConnection,
        Role role);

private:
    void onConnected() override;
    void onDisconnected() override;
    bool onSignal(int signum) override;
    std::size_t onReceivedFromPeer() override;

    Role role;
};

class EchoServerSocketContextFactory
    : public core::socket::stream::SocketContextFactory {
private:
    core::socket::stream::SocketContext* create(
        core::socket::stream::SocketConnection* socketConnection) override;
};

class EchoClientSocketContextFactory
    : public core::socket::stream::SocketContextFactory {
private:
    core::socket::stream::SocketContext* create(
        core::socket::stream::SocketConnection* socketConnection) override;
};
```

The behavioral core is small. Logging is omitted here; the send/read behavior
matches the current source:

```cpp
void EchoSocketContext::onConnected() {
    if (role == Role::CLIENT) {
        sendToPeer("Hello peer! Nice to see you!!!");
    }
}

std::size_t EchoSocketContext::onReceivedFromPeer() {
    char chunk[4096];
    const std::size_t chunklen = readFromPeer(chunk, 4096);

    if (chunklen > 0) {
        sendToPeer(chunk, chunklen);
    }

    return chunklen;
}

core::socket::stream::SocketContext*
EchoServerSocketContextFactory::create(
    core::socket::stream::SocketConnection* socketConnection) {
    return new EchoSocketContext(
        socketConnection, EchoSocketContext::Role::SERVER);
}

core::socket::stream::SocketContext*
EchoClientSocketContextFactory::create(
    core::socket::stream::SocketConnection* socketConnection) {
    return new EchoSocketContext(
        socketConnection, EchoSocketContext::Role::CLIENT);
}
```

The client sends the initial greeting when its context attaches. Every received
chunk is then read from the connection and sent back unchanged. Because both the
server and the supplied client use the same reflecting context, the greeting is
reflected back and forth until the pair is stopped. That behavior keeps the
example focused on repeated event-driven reads and writes rather than a
single-request protocol.

## 2. Bind the factories to IPv4 plain streams

The real source keeps one `servers.h` and one `clients.h` for all generated
variants. With `NET=in` and `STREAM=legacy`, the important aliases become:

```cpp
using EchoSocketServer =
    net::in::stream::legacy::SocketServer<
        EchoServerSocketContextFactory>;

using EchoSocketClient =
    net::in::stream::legacy::SocketClient<
        EchoClientSocketContextFactory>;
```

The helper functions construct named instances:

```cpp
EchoSocketServer getServer() {
    return EchoSocketServer("echoserver");
}

EchoSocketClient getClient() {
    return EchoSocketClient("echoclient");
}
```

Those names are not cosmetic. They become part of SNode.C's generated
configuration hierarchy, which is why the run commands below contain
`echoserver local ...` and `echoclient remote ...`.

## 3. Start the endpoint flows

The two `main()` functions have the same lifecycle shape:

```text
SNodeC::init()
     │
     ├─ create named SocketServer / SocketClient instance
     │
     ├─ listen() / connect()
     │
     ▼
SNodeC::start()
     │
     ▼
caller-thread event loop
```

For the server, the essential source is:

```cpp
core::SNodeC::init(argc, argv);

using SocketServer = apps::echo::model::legacy::EchoSocketServer;
const SocketServer server = apps::echo::model::legacy::getServer();

server.listen(
    [instanceName = server.getConfig()->getInstanceName()](
        const SocketServer::SocketAddress& socketAddress,
        const core::socket::State& state) {
        // The real source logs OK, DISABLED, ERROR, and FATAL states here.
    });

return core::SNodeC::start();
```

The client mirrors it:

```cpp
core::SNodeC::init(argc, argv);

using SocketClient = apps::echo::model::legacy::EchoSocketClient;
using SocketAddress = SocketClient::SocketAddress;
const SocketClient client = apps::echo::model::legacy::getClient();

client.connect(
    [instanceName = client.getConfig()->getInstanceName()](
        const SocketAddress& socketAddress,
        const core::socket::State& state) {
        // The real source logs OK, DISABLED, ERROR, and FATAL states here.
    });

return core::SNodeC::start();
```

`init()` prepares the framework and parses the application configuration.
`listen()` or `connect()` starts the endpoint operation. `start()` then runs the
framework event loop synchronously on the calling thread.

When an accept or connect succeeds, the endpoint owns an established
`SocketConnection`. Its retained `SocketContextFactory` creates one
`EchoSocketContext` for that connection; attaching the context invokes
`onConnected()`, and incoming data is delivered through
`onReceivedFromPeer()`.

## Essential types

| Type | Role in this example |
| --- | --- |
| `net::in::stream::legacy::SocketServer<Factory>` | IPv4 plain-stream listener parameterized by the context factory used for accepted connections. |
| `net::in::stream::legacy::SocketClient<Factory>` | IPv4 plain-stream connector parameterized by the context factory used after connection. |
| `SocketServer::SocketAddress` / `SocketClient::SocketAddress` | Concrete IPv4 endpoint address type exposed by the selected endpoint. |
| `core::socket::State` | Reports `OK`, `DISABLED`, `ERROR`, or `FATAL` from listen/connect setup. |
| `core::socket::stream::SocketConnection` | Owns the established stream, addresses, queues, timeouts, and active context. |
| `core::socket::stream::SocketContextFactory` | Creates the application context for an established connection. |
| `core::socket::stream::SocketContext` | Base class for connection-local protocol/application callbacks. |
| `EchoSocketContext::Role` | Lets the same context behave as server or client; only the client sends the initial greeting. |

The endpoint's `getConfig()` exposes the concrete configuration assembled for
that selected role and transport. The server command uses its `local` section;
the client command uses its `remote` section.

## Essential includes

The current example uses macro-selected wrapper headers so one source can build
several variants. For the plain IPv4 path, the networking-relevant includes
reduce conceptually to:

```cpp
#include "core/SNodeC.h"
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "net/in/stream/legacy/SocketServer.h"
#include "net/in/stream/legacy/SocketClient.h"
```

Their responsibilities are distinct:

- `core/SNodeC.h` provides framework initialization and the event-loop entry
  point.
- `SocketContext.h` and `SocketContextFactory.h` define the application-facing
  per-connection extension point.
- `net/in/stream/legacy/SocketServer.h` and `SocketClient.h` select IPv4 plus the
  plain stream connection implementation and the corresponding configuration
  types.
- `SemanticLog.h` and `log/Logger.h`, also present in the real example, support
  example logging; they are not what defines the connection/context model.

`net/in/stream/legacy/SocketServer.h` itself composes the generic IPv4 stream
server with the legacy `SocketAcceptor`, legacy `SocketConnection`, and the
IPv4/plain server configuration type. The client header performs the analogous
client composition.

## 4. CMake: from source files to the two executables

The echo model first becomes a local static library:

```cmake
add_library(
    echosocketcontext STATIC
    EchoSocketContext.cpp
    EchoSocketContext.h
)

target_include_directories(
    echosocketcontext PRIVATE ${PROJECT_SOURCE_DIR}
)
```

The real [`echo/CMakeLists.txt`](echo/CMakeLists.txt) loops over address families
and stream modes. For the one path documented here, that loop is equivalent to:

```cmake
add_executable(
    echoserver-legacy-in
    echoserver.cpp
    model/servers.h
)
target_link_libraries(
    echoserver-legacy-in PRIVATE
    net-in-stream-legacy
    echosocketcontext
)
target_compile_definitions(
    echoserver-legacy-in PRIVATE
    NET=in NET_TYPE=1 STREAM=legacy STREAM_TYPE=1
)

add_executable(
    echoclient-legacy-in
    echoclient.cpp
    model/clients.h
)
target_link_libraries(
    echoclient-legacy-in PRIVATE
    net-in-stream-legacy
    echosocketcontext
)
target_compile_definitions(
    echoclient-legacy-in PRIVATE
    NET=in NET_TYPE=1 STREAM=legacy STREAM_TYPE=1
)
```

There are two different kinds of linked target here:

- **`echosocketcontext`** is local example code. It contains the
  `EchoSocketContext` implementation and its factories.
- **`net-in-stream-legacy`** is the SNode.C IPv4/plain-stream component. It is a
  shared library and publicly links `net-in-stream` and
  `core-socket-stream-legacy`, so the executable receives the underlying IPv4
  stream and plain connection machinery through that dependency closure.

The source-tree target is named `net-in-stream-legacy`; the same component also
exports the namespaced alias `snodec::net-in-stream-legacy` for CMake consumers.
No TLS target is linked for this worked variant.

## 5. Configure and build only this pair

The project requires a C++20 toolchain and the dependencies needed by the
configured SNode.C source tree. The root [installation
section](../../README.md#installation) is the canonical dependency reference.
For the Debian/x86-64 qualification path, the base source build used Git, CMake,
Ninja, pkg-config, OpenSSL development files, and nlohmann/json development
files.

From a fresh checkout:

```sh
git clone https://github.com/SNodeC/snode.c.git
cd snode.c

cmake -S . -B cmake-build-release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DSNODEC_BUILD_APPS=ON \
  -DSNODEC_BUILD_TESTS=OFF \
  -DCHECK_INCLUDES=OFF

cmake --build cmake-build-release --parallel \
  --target echoserver-legacy-in echoclient-legacy-in

mkdir -p cmake-build-release/echo-config
```

`SNODEC_BUILD_APPS=ON` is the project default, but it is written explicitly here
so the example does not depend on an implicit build choice. `CHECK_INCLUDES=OFF`
keeps the optional IWYU include-analysis pass out of this first application
build when IWYU is installed.

A fresh configure may need network access for dependencies that the project
retrieves as pinned source. Building the two named targets then builds only
their required dependency closure rather than every application executable.

## 6. Run the server

Use an isolated configuration directory so an existing user configuration
cannot silently change the example:

```sh
XDG_CONFIG_HOME="$PWD/cmake-build-release/echo-config" \
  ./cmake-build-release/src/apps/echo/echoserver-legacy-in \
  --monochrom=true \
  echoserver local --host 127.0.0.1 --port 18001
```

Read the command from left to right:

```text
echoserver-legacy-in     executable
        echoserver       named server instance
             local       local/listen configuration section
     --host / --port     IPv4 listener values
```

Ignoring timestamps and logger prefixes, the qualified run reported a listener
on `127.0.0.1:18001`.

## 7. Run the client

In a second terminal, using the same isolated configuration root:

```sh
XDG_CONFIG_HOME="$PWD/cmake-build-release/echo-config" \
  ./cmake-build-release/src/apps/echo/echoclient-legacy-in \
  --monochrom=true \
  echoclient remote --host 127.0.0.1 --port 18001
```

Here `remote` is the client destination section. The qualified run reported the
client connected to the loopback server and the server accepted the transport
connection.

At that point the complete object path is live:

```text
server.listen()                          client.connect()
      │                                        │
      └──────────── IPv4 plain stream ─────────┘
                       │
                       ▼
                SocketConnection
                       │
                retained factory
                       │ create(...)
                       ▼
                EchoSocketContext
                       │
             lifecycle / data callbacks
```

The client context sends `Hello peer! Nice to see you!!!` from `onConnected()`.
The server's `onReceivedFromPeer()` reads the chunk and sends the same bytes
back. The client has the same reflection behavior, so the payload continues to
travel between the two contexts until you stop the pair with <kbd>Ctrl</kbd>+<kbd>C</kbd>.

### Make the payload visible

The current echo implementation logs each received payload only when
application trace logging is enabled. Add this root option to either run command:

```sh
--log-origin-level application=trace
```

The trace then includes the source message emitted immediately before
`sendToPeer()`:

```text
Data to reflect: Hello peer! Nice to see you!!!
```

Because both supplied peers reflect every received chunk, trace output can be
continuous. Use it briefly to observe the data callback, then stop both
processes.

## What this example establishes

This path demonstrates how a real SNode.C application is assembled:

1. choose a concrete endpoint family and stream mode;
2. implement one connection-local `SocketContext`;
3. provide factories that create that context for established connections;
4. parameterize `SocketServer` and `SocketClient` with those factories;
5. link the matching SNode.C component and the application context code;
6. configure named endpoint instances through the generated hierarchy;
7. enter the shared event loop and handle lifecycle/data callbacks per
   connection.

The recorded IPv4 qualification establishes buildability, listener startup,
and connection formation for this exact pair at the cited revision. The source
establishes the reflection behavior; trace logging is the direct way to make
that payload path visible during a local run. This example does not by itself
qualify IPv6, Unix-domain sockets, TLS, Bluetooth, or higher protocol layers.

## Continue from here

The echo CMake file generates IPv6, Unix-domain, and TLS variants from the same
application-context model, with Bluetooth variants added when BlueZ is
available. Those are useful next steps once the IPv4/plain lifecycle is clear.

For higher application layers, continue through the other source examples in
this directory and the corresponding framework components. HTTP/Express,
SSE/EventSource, WebSocket, MQTT, database integration, and OAuth2 examples add
protocol-specific semantics above the same event-driven connection foundation;
they should be evaluated against the exact transport and configuration path you
intend to deploy.
