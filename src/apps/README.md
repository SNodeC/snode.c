# Example applications

The programs in this directory are runnable examples of SNode.C composition and
configuration. The worked example below is intentionally presented as a small
**external application project** built against an installed SNode.C package. That
is the normal shape of an application developer's code: SNode.C is discovered
with CMake, framework headers are included from the installed include tree, and
the application links an exported `snodec::` target.

The first path uses only plain IPv4 streams. It is small enough to trace from
source code through CMake, build, configuration, connection establishment,
semantic application logging, and payload exchange before introducing TLS,
HTTP, WebSocket, MQTT, or another protocol layer.

[Project README](../../README.md) ·
[API documentation](https://snodec.github.io/snode.c-doc/html/index.html) ·
[Network component tests](../../tests/component/net/README.md)

## Worked path: external plain-IPv4 echo pair

The standalone project builds two executables:

| Item | Selection |
| --- | --- |
| Address family | IPv4 · `in` |
| Stream mode | plain/non-TLS · `legacy` |
| Server executable | `echoserver` |
| Client executable | `echoclient` |
| Server instance | `echoserver` |
| Client instance | `echoclient` |
| Example address | `127.0.0.1:18001` |
| SNode.C CMake component | `net-in-stream-legacy` |
| Imported target | `snodec::net-in-stream-legacy` |

The SNode.C package exports `net-in-stream-legacy` as a supported component.
That component brings in the generic IPv4 stream layer and the plain stream
connection implementation through its dependency closure.

A compact project layout is enough:

```text
snodec-echo/
├── CMakeLists.txt
├── EchoSocketContext.h
├── EchoSocketContext.cpp
├── echoserver.cpp
└── echoclient.cpp
```

## 1. Define the connection-local behavior

`EchoSocketContext` is the application object attached to one established
stream connection. The server and client use the same context implementation;
the `Role` value only decides whether the context sends the initial greeting.

### `EchoSocketContext.h`

```cpp
#pragma once

#include <core/socket/stream/SocketContext.h>
#include <core/socket/stream/SocketContextFactory.h>

#include <cstddef>

namespace core::socket::stream {
    class SocketConnection;
}

namespace echo {

    class EchoSocketContext : public core::socket::stream::SocketContext {
    public:
        enum class Role { SERVER, CLIENT };

        EchoSocketContext(
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

} // namespace echo
```

All SNode.C headers use `<...>` because they come from the installed package.
The project-local header is included with quotes from its own `.cpp` files.

### `EchoSocketContext.cpp`

```cpp
#include "EchoSocketContext.h"

#include <string>

namespace echo {

    EchoSocketContext::EchoSocketContext(
        core::socket::stream::SocketConnection* socketConnection,
        Role role)
        : core::socket::stream::SocketContext(socketConnection)
        , role(role) {
    }

    void EchoSocketContext::onConnected() {
        const char* roleName = role == Role::CLIENT ? "client" : "server";
        const auto contextLog = log();

        contextLog.info("Echo {} context attached", roleName);

        if (role == Role::CLIENT) {
            contextLog.info(
                "Echo client: sending initial greeting: '{}'",
                "Hello peer! Nice to see you!!!");
            sendToPeer("Hello peer! Nice to see you!!!");
        }
    }

    void EchoSocketContext::onDisconnected() {
        const char* roleName = role == Role::CLIENT ? "client" : "server";

        log().info(
            "Echo {} context detached: {}",
            roleName,
            getDetachReason() == DetachReason::ContextSwitch
                ? "context switch"
                : "connection close");
    }

    bool EchoSocketContext::onSignal([[maybe_unused]] int signum) {
        return true;
    }

    std::size_t EchoSocketContext::onReceivedFromPeer() {
        char chunk[4096];
        const std::size_t chunklen = readFromPeer(chunk, sizeof(chunk));

        if (chunklen > 0) {
            const char* roleName = role == Role::CLIENT ? "client" : "server";

            log().info(
                "Echo {}: data to reflect: {}",
                roleName,
                std::string(chunk, chunklen));

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

} // namespace echo
```

The essential receive path is deliberately direct:

```text
readFromPeer()
      │
      ├─ semantic application log
      │
      └─ sendToPeer()
```

There is no extra observation state in the context. Every received chunk is
reported and immediately queued back to the peer. That makes the behavior easy
to see while learning the framework and keeps the example source aligned with
the operation it demonstrates.

`SocketContext::log()` is the semantic application logger for the current
context. Its scope carries the context boundary together with instance and
connection identity, so the example does not need legacy `LOG`/`VLOG` macros or
its own logging infrastructure.

Because both executables below use the same reflecting context, the client
initiates a continuing echo exchange. The information log therefore continues
until one side is stopped; that is intentional for this small teaching example.

## 2. Compose the IPv4/plain endpoint types

The installed wrapper headers select the concrete IPv4/plain stream endpoint
families:

```cpp
#include <net/in/stream/legacy/SocketClient.h>
#include <net/in/stream/legacy/SocketServer.h>
```

For this application the important aliases are:

```cpp
using EchoSocketServer =
    net::in::stream::legacy::SocketServer<
        echo::EchoServerSocketContextFactory>;

using EchoSocketClient =
    net::in::stream::legacy::SocketClient<
        echo::EchoClientSocketContextFactory>;
```

The factory parameter is the application/framework boundary: once a transport
connection exists, SNode.C asks the retained factory to create the
connection-local `EchoSocketContext`.

## 3. Server application

### `echoserver.cpp`

```cpp
#include "EchoSocketContext.h"

#include <core/SNodeC.h>
#include <core/socket/State.h>
#include <net/in/stream/legacy/SocketServer.h>

using EchoSocketServer =
    net::in::stream::legacy::SocketServer<
        echo::EchoServerSocketContextFactory>;

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    const EchoSocketServer server("echoserver");

    server.listen(
        [log = server.log()](
            const EchoSocketServer::SocketAddress& socketAddress,
            core::socket::State state) {
            if (state == core::socket::State::OK) {
                log.info("Listening on {}", socketAddress.toString());
            }
        });

    return core::SNodeC::start();
}
```

The executable creates one named server instance, starts its listener, and then
enters the shared caller-thread event loop. The instance name `echoserver`
becomes part of the generated configuration hierarchy used at runtime.

## 4. Client application

### `echoclient.cpp`

```cpp
#include "EchoSocketContext.h"

#include <core/SNodeC.h>
#include <core/socket/State.h>
#include <net/in/stream/legacy/SocketClient.h>

using EchoSocketClient =
    net::in::stream::legacy::SocketClient<
        echo::EchoClientSocketContextFactory>;

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    const EchoSocketClient client("echoclient");

    client.connect(
        [log = client.log()](
            const EchoSocketClient::SocketAddress& socketAddress,
            core::socket::State state) {
            if (state == core::socket::State::OK) {
                log.info("Connected to {}", socketAddress.toString());
            }
        });

    return core::SNodeC::start();
}
```

The client has the same lifecycle shape as the server. Once the connection is
established, its factory creates a client-role `EchoSocketContext`, whose
`onConnected()` callback sends the initial greeting.

## Essential types and headers

| Type/header | Role in the application |
| --- | --- |
| `<core/SNodeC.h>` | Initializes SNode.C and starts the shared event loop. |
| `<core/socket/stream/SocketContext.h>` | Defines connection-local lifecycle, receive/send operations, and semantic context logging. |
| `<core/socket/stream/SocketContextFactory.h>` | Defines the factory extension point used when a connection gets its application context. |
| `<net/in/stream/legacy/SocketServer.h>` | Provides the IPv4 plain-stream server wrapper and its configuration type. |
| `<net/in/stream/legacy/SocketClient.h>` | Provides the IPv4 plain-stream client wrapper and its configuration type. |
| `core::socket::stream::SocketConnection` | Owns one established stream connection and its active context. |
| `core::socket::State` | Reports endpoint setup state to the listen/connect callback. |
| `EchoSocketContext::Role` | Lets one small context implementation serve both sides; the client role sends the greeting. |

The application does not include internal build-tree paths, define SNode.C
network-selection macros, or reproduce SNode.C dependency wiring itself. The
installed component target owns that information.

## 5. CMake: consume the installed package

### `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.18)

project(snodec-echo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(
    snodec REQUIRED
    COMPONENTS net-in-stream-legacy
)

add_library(
    echo-context STATIC
    EchoSocketContext.cpp
    EchoSocketContext.h
)

target_include_directories(
    echo-context PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    echo-context PUBLIC
    snodec::net-in-stream-legacy
)

add_executable(echoserver echoserver.cpp)
target_link_libraries(echoserver PRIVATE echo-context)

add_executable(echoclient echoclient.cpp)
target_link_libraries(echoclient PRIVATE echo-context)
```

`find_package(snodec ...)` loads the installed SNode.C package configuration and
requests exactly the component this application uses. The imported
`snodec::net-in-stream-legacy` target contributes the installed include path and
its transitive SNode.C link dependencies. The application therefore names the
framework dependency once instead of reconstructing the internal library graph.

`echo-context` is application code. Making its SNode.C dependency `PUBLIC`
means the two executables inherit the framework include/link requirements when
they link `echo-context`.

## 6. Install SNode.C and build the external project

Install SNode.C into a normal prefix first. A user-local prefix keeps the
example self-contained:

```sh
cmake -S /path/to/snode.c -B /tmp/snodec-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DSNODEC_BUILD_APPS=OFF \
  -DSNODEC_BUILD_TESTS=OFF \
  -DCMAKE_INSTALL_PREFIX="$HOME/.local"

cmake --build /tmp/snodec-build --parallel 8
cmake --install /tmp/snodec-build
```

Then configure and build the standalone echo project from its own directory:

```sh
cd /path/to/snodec-echo

cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$HOME/.local"

cmake --build build --parallel 8
mkdir -p build/echo-config
```

`CMAKE_PREFIX_PATH` is only needed when the SNode.C installation prefix is not
already in CMake's normal package search path.

The resulting application artifacts are simply:

```text
build/echoserver
build/echoclient
```

## 7. Run the pair

Use an isolated configuration root so unrelated user configuration cannot alter
the example.

Start the server:

```sh
XDG_CONFIG_HOME="$PWD/build/echo-config" \
  ./build/echoserver \
  --monochrom=true \
  echoserver local --host 127.0.0.1 --port 18001
```

The command hierarchy reads directly from left to right:

```text
echoserver            executable
    echoserver         named SocketServer instance
         local         listener configuration section
  --host / --port      IPv4 listener values
```

Then start the client in a second terminal:

```sh
XDG_CONFIG_HOME="$PWD/build/echo-config" \
  ./build/echoclient \
  --monochrom=true \
  echoclient remote --host 127.0.0.1 --port 18001
```

Here `remote` is the client destination section.

Ignoring timestamps and other semantic-log metadata, the useful output includes
messages of this form:

```text
Listening on 127.0.0.1:18001
Echo server context attached
Echo server: data to reflect: Hello peer! Nice to see you!!!
```

and on the client:

```text
Connected to 127.0.0.1:18001
Echo client context attached
Echo client: sending initial greeting: 'Hello peer! Nice to see you!!!'
Echo client: data to reflect: Hello peer! Nice to see you!!!
```

The client sends the greeting once from `onConnected()`. The server reads and
reflects it; the client does the same when the reflected data arrives. Since both
contexts are echo contexts, this repeats until one process is stopped with
<kbd>Ctrl</kbd>+<kbd>C</kbd>.

The runtime object path is:

```text
SocketServer.listen()                 SocketClient.connect()
        │                                      │
        └────────── IPv4 plain stream ─────────┘
                           │
                           ▼
                    SocketConnection
                           │
                    retained factory
                           │ create(...)
                           ▼
                    EchoSocketContext
                           │
              onConnected / onReceivedFromPeer
                           │
                    read → log → send
```

## 8. Verify the application

SNode.C already has component tests for IPv4/plain composition, payload
exchange, framed and large payloads, multiple messages, multiple clients,
disconnect lifecycle, controlled close, connection failure, and effective
listener addresses under [`tests/component/net`](../../tests/component/net/).
The following checks target something different: the **standalone application
artifacts** shown on this page.

Set a few paths once:

```sh
SERVER=./build/echoserver
CLIENT=./build/echoclient
CFG="$PWD/build/echo-config"
mkdir -p "$CFG"
```

### Test 1 — generated configuration surface

Verify that the documented instance and endpoint sections are present in the
actual executables:

```sh
XDG_CONFIG_HOME="$CFG" "$SERVER" --monochrom=true --help=expanded \
  > /tmp/snodec-echo-server-help.txt
XDG_CONFIG_HOME="$CFG" "$CLIENT" --monochrom=true --help=expanded \
  > /tmp/snodec-echo-client-help.txt

grep -F 'echoserver' /tmp/snodec-echo-server-help.txt
grep -F 'local'      /tmp/snodec-echo-server-help.txt
grep -F 'echoclient' /tmp/snodec-echo-client-help.txt
grep -F 'remote'     /tmp/snodec-echo-client-help.txt
```

A missing entry makes `grep` return a non-zero status. This verifies the
application's generated configuration contract, not just the lower-level socket
implementation.

### Test 2 — real server with a deterministic peer

Start the real external server:

```sh
SERVER_LOG=/tmp/snodec-echo-server.log

XDG_CONFIG_HOME="$CFG" "$SERVER" --monochrom=true \
  echoserver local --host 127.0.0.1 --port 18001 \
  >"$SERVER_LOG" 2>&1 &
SERVER_PID=$!

trap 'kill "$SERVER_PID" 2>/dev/null || true' EXIT
sleep 0.3
```

Use a small Python peer to verify byte-for-byte reflection:

```sh
python3 - <<'PY'
import socket

payload = b"hello from deterministic peer\n"

with socket.create_connection(("127.0.0.1", 18001), timeout=2) as sock:
    sock.sendall(payload)
    received = b""
    while len(received) < len(payload):
        part = sock.recv(len(payload) - len(received))
        if not part:
            raise SystemExit("server closed before echo was complete")
        received += part

assert received == payload, (received, payload)
print("server echo: OK")
PY

grep -F 'data to reflect: hello from deterministic peer' "$SERVER_LOG"
kill "$SERVER_PID"
wait "$SERVER_PID" 2>/dev/null || true
trap - EXIT
```

This exercises the real executable through listener creation, connection
factory, `EchoSocketContext`, `readFromPeer()`, and `sendToPeer()`.

### Test 3 — real client with a deterministic peer

A deterministic Python server can validate the client-specific behavior: the
client must send the exact greeting when its context attaches, then reflect the
returned bytes.

Start the peer:

```sh
python3 - <<'PY' &
import socket

expected = b"Hello peer! Nice to see you!!!"

with socket.socket() as server:
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(("127.0.0.1", 18002))
    server.listen(1)
    conn, _ = server.accept()
    with conn:
        greeting = conn.recv(4096)
        assert greeting == expected, (greeting, expected)
        conn.sendall(greeting)
        reflected = conn.recv(4096)
        assert reflected == expected, (reflected, expected)

print("client greeting/reflection: OK")
PY
PEER_PID=$!

sleep 0.2
```

Run the real client for a bounded interval:

```sh
CLIENT_LOG=/tmp/snodec-echo-client.log

timeout 3s env XDG_CONFIG_HOME="$CFG" \
  "$CLIENT" --monochrom=true \
  echoclient remote --host 127.0.0.1 --port 18002 \
  >"$CLIENT_LOG" 2>&1 || test $? -eq 124

wait "$PEER_PID"

grep -F "sending initial greeting: 'Hello peer! Nice to see you!!!'" \
  "$CLIENT_LOG"
grep -F 'data to reflect: Hello peer! Nice to see you!!!' "$CLIENT_LOG"
```

The timeout bounds the executable even if its configured reconnect policy keeps
it alive after the deterministic peer closes.

### Test 4 — real pair smoke test

Finally run the two external executables together and verify the composition as
a pair. Since both sides continuously reflect, keep the observation interval
short.

```sh
SERVER_LOG=/tmp/snodec-echo-pair-server.log
CLIENT_LOG=/tmp/snodec-echo-pair-client.log

XDG_CONFIG_HOME="$CFG" "$SERVER" --monochrom=true \
  echoserver local --host 127.0.0.1 --port 18003 \
  >"$SERVER_LOG" 2>&1 &
SERVER_PID=$!

sleep 0.3

XDG_CONFIG_HOME="$CFG" "$CLIENT" --monochrom=true \
  echoclient remote --host 127.0.0.1 --port 18003 \
  >"$CLIENT_LOG" 2>&1 &
CLIENT_PID=$!

sleep 0.5
kill "$CLIENT_PID" "$SERVER_PID" 2>/dev/null || true
wait "$CLIENT_PID" 2>/dev/null || true
wait "$SERVER_PID" 2>/dev/null || true

grep -F 'Echo server context attached' "$SERVER_LOG"
grep -F 'Echo client context attached' "$CLIENT_LOG"
grep -F 'data to reflect: Hello peer! Nice to see you!!!' "$SERVER_LOG"
grep -F 'data to reflect: Hello peer! Nice to see you!!!' "$CLIENT_LOG"
```

This final check is deliberately a smoke test. The deterministic-peer tests are
better for precise assertions; the real-pair test proves that the two documented
application artifacts compose as expected.

## What this example establishes

The example demonstrates the complete application-facing path:

1. install SNode.C as a CMake package;
2. discover the required component with `find_package(snodec ...)`;
3. include installed SNode.C headers with `<...>`;
4. implement one connection-local `SocketContext`;
5. provide factories that create that context for established connections;
6. compose concrete IPv4/plain `SocketServer` and `SocketClient` types;
7. link the exported `snodec::net-in-stream-legacy` target;
8. configure the named endpoint instances through SNode.C's generated hierarchy;
9. enter the shared event loop and observe the application through semantic
   context logging;
10. verify the real executables with deterministic peers and a bounded pair
    smoke test.

The page intentionally stays within one plain-IPv4 path. IPv6, Unix-domain
sockets, TLS, HTTP/Express, SSE/EventSource, WebSocket, MQTT, database
integration, and other examples add their own transport or protocol semantics
above the same event-driven connection/context foundation.
