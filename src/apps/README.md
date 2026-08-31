# Example applications

The programs in this directory demonstrate SNode.C composition and configuration.
For a first application, use the complete standalone echo project in
[`examples/echo`](../../examples/echo/). It consumes an **installed** SNode.C
package exactly as an external application does: CMake discovers SNode.C with
`find_package()`, framework headers come from the installed include tree, and the
application links an exported `snodec::` target.

The worked path deliberately uses only plain IPv4 streams. It is small enough to
trace from C++ source through CMake, build, configuration, semantic application
logging, runtime behavior, and CTest verification before introducing TLS, HTTP,
SSE/EventSource, WebSocket, MQTT, or another protocol layer.

[Complete echo project](../../examples/echo/) ·
[Project README](../../README.md) ·
[API documentation](https://snodec.github.io/snode.c-doc/html/index.html) ·
[Network component tests](../../tests/component/net/README.md)

## Worked path: external plain-IPv4 echo pair

The project builds two executables:

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

The complete project is:

```text
examples/echo/
├── CMakeLists.txt
├── README.md
├── EchoSocketContext.h
├── EchoSocketContext.cpp
├── echoserver.cpp
├── echoclient.cpp
└── tests/
    ├── CMakeLists.txt
    └── echo_tests.py
```

The SNode.C package exports `net-in-stream-legacy` as a supported component.
That target contributes the installed public headers and brings in the generic
IPv4 stream and plain stream-connection dependencies transitively.

## 1. Define the connection-local behavior

`EchoSocketContext` is the application object attached to one established stream
connection. The server and client use the same context implementation; `Role`
only decides whether the context sends the initial greeting.

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

All SNode.C headers use `<...>` because they are public headers supplied by the
installed package. Only the project-local `EchoSocketContext.h` is included with
quotes from the application's `.cpp` files.

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

The essential receive path is intentionally direct:

```text
readFromPeer()
      │
      ├─ semantic application log
      │
      └─ sendToPeer()
```

There is no observation state or second logging abstraction in the application
context. `SocketContext::log()` already returns the semantic application logger
for that context, with context, instance, and connection identity in its scope.

The client sends `Hello peer! Nice to see you!!!` once from `onConnected()`.
Every received chunk is logged and queued back unchanged. Since both programs use
the same reflecting context, running them together creates a continuing echo
exchange until one side is stopped.

## 2. Compose the IPv4/plain endpoint types

The installed wrapper headers select the concrete IPv4/plain endpoint families:

```cpp
#include <net/in/stream/legacy/SocketClient.h>
#include <net/in/stream/legacy/SocketServer.h>
```

The application binds its factories to those endpoint types:

```cpp
using EchoSocketServer =
    net::in::stream::legacy::SocketServer<
        echo::EchoServerSocketContextFactory>;

using EchoSocketClient =
    net::in::stream::legacy::SocketClient<
        echo::EchoClientSocketContextFactory>;
```

The factory parameter is the application/framework boundary. Once SNode.C owns
an established connection, the retained factory creates the connection-local
`EchoSocketContext`.

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
enters the caller-thread event loop. The instance name `echoserver` becomes part
of the generated configuration hierarchy.

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

After connection establishment, the client factory creates a client-role
`EchoSocketContext`; its `onConnected()` callback sends the initial greeting.

## Essential types and headers

| Type/header | Role in the application |
| --- | --- |
| `<core/SNodeC.h>` | Initializes SNode.C and starts the shared event loop. |
| `<core/socket/stream/SocketContext.h>` | Defines the connection-local lifecycle, read/write operations, and context-scoped semantic application logger. |
| `<core/socket/stream/SocketContextFactory.h>` | Defines the factory extension point used to create an application context for an established connection. |
| `<net/in/stream/legacy/SocketServer.h>` | Provides the IPv4 plain-stream server wrapper and its configuration type. |
| `<net/in/stream/legacy/SocketClient.h>` | Provides the IPv4 plain-stream client wrapper and its configuration type. |
| `core::socket::stream::SocketConnection` | Owns one established stream connection and its active context. |
| `core::socket::State` | Reports endpoint setup state to the listen/connect callback. |
| `EchoSocketContext::Role` | Lets one context implementation serve both sides; the client role sends the greeting. |

The application does not reproduce SNode.C's internal target graph or define
network-selection build macros. The imported component target owns the public
include path and transitive framework dependencies.

## 5. CMake: consume the installed package

The complete top-level `CMakeLists.txt` is:

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

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
```

`find_package(snodec ...)` requests exactly the installed component needed by
this application. `snodec::net-in-stream-legacy` supplies the installed include
path and its transitive framework link dependencies. `echo-context` is
application code shared by the two executables.

`include(CTest)` provides the standard `BUILD_TESTING` option. With testing
enabled, the standalone project adds its own `tests` directory; it does not
depend on SNode.C's in-tree test harness.

## 6. Install SNode.C and build the project

Install SNode.C into a normal prefix first. A user-local prefix keeps this path
self-contained:

```sh
cmake -S /path/to/snode.c -B /tmp/snodec-build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DSNODEC_BUILD_APPS=OFF \
  -DSNODEC_BUILD_TESTS=OFF \
  -DCMAKE_INSTALL_PREFIX="$HOME/.local"

cmake --build /tmp/snodec-build --parallel 8
cmake --install /tmp/snodec-build
```

Then configure the external project:

```sh
cd /path/to/snode.c/examples/echo

cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$HOME/.local"

cmake --build build --parallel 8
mkdir -p build/echo-config
```

`CMAKE_PREFIX_PATH` is needed only when the SNode.C installation prefix is not
already in CMake's normal package search path. Python 3 is required when
`BUILD_TESTING` is enabled, which is the default provided by `CTest`.

The resulting application artifacts are:

```text
build/echoserver
build/echoclient
```

## 7. Run the pair

Start the server:

```sh
XDG_CONFIG_HOME="$PWD/build/echo-config" \
  ./build/echoserver \
  --monochrom=true \
  echoserver local --host 127.0.0.1 --port 18001
```

The hierarchy reads directly from left to right:

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

Ignoring timestamps and other semantic-log metadata, the useful output includes:

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

## 8. CTest: verify the complete application

The standalone project registers four application-level tests. They complement
SNode.C's lower-level IPv4/plain component tests rather than duplicating them.

`tests/CMakeLists.txt` uses Python only as the deterministic peer/process driver:

```cmake
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(ECHO_TEST_DRIVER "${CMAKE_CURRENT_SOURCE_DIR}/echo_tests.py")

add_test(
    NAME echo.config-surface
    COMMAND
        "${Python3_EXECUTABLE}" "${ECHO_TEST_DRIVER}" config
        --server "$<TARGET_FILE:echoserver>"
        --client "$<TARGET_FILE:echoclient>"
)

add_test(
    NAME echo.server-external-peer
    COMMAND
        "${Python3_EXECUTABLE}" "${ECHO_TEST_DRIVER}" server
        --server "$<TARGET_FILE:echoserver>"
)

add_test(
    NAME echo.client-external-peer
    COMMAND
        "${Python3_EXECUTABLE}" "${ECHO_TEST_DRIVER}" client
        --client "$<TARGET_FILE:echoclient>"
)

add_test(
    NAME echo.pair-smoke
    COMMAND
        "${Python3_EXECUTABLE}" "${ECHO_TEST_DRIVER}" pair
        --server "$<TARGET_FILE:echoserver>"
        --client "$<TARGET_FILE:echoclient>"
)
```

The complete [`tests/CMakeLists.txt`](../../examples/echo/tests/CMakeLists.txt)
also assigns focused labels and 10-second timeouts.

| CTest | What it establishes |
| --- | --- |
| `echo.config-surface` | The actual binaries expose the documented `echoserver/local` and `echoclient/remote` configuration hierarchy. |
| `echo.server-external-peer` | The real server accepts a deterministic Python peer and reflects a binary-safe payload byte for byte. |
| `echo.client-external-peer` | The real client sends the exact greeting to a deterministic Python server and reflects the returned greeting. |
| `echo.pair-smoke` | The real server and client can run together for a bounded smoke interval without either process terminating unexpectedly. |

The test driver creates isolated temporary `XDG_CONFIG_HOME` directories and
uses ephemeral IPv4 loopback ports, so the tests do not depend on the manual
`18001` example port or the developer's existing SNode.C configuration. The
real-pair smoke test suppresses application-level information logging during its
short self-reflecting run; the deterministic peer tests exercise the payload
paths without creating an unbounded echo loop.

Run the complete suite with:

```sh
ctest --test-dir build --output-on-failure
```

Useful focused runs include:

```sh
ctest --test-dir build -R '^echo\.server-external-peer$' --output-on-failure
ctest --test-dir build -L payload --output-on-failure
```

The implementation is in
[`tests/echo_tests.py`](../../examples/echo/tests/echo_tests.py). CTest owns
orchestration and pass/fail reporting; Python only supplies deterministic socket
peers and process control.

SNode.C itself already exercises composition, payload exchange, framed and large
payloads, multiple messages and clients, disconnect lifecycle, controlled close,
connection failure, and effective listener addresses under
[`tests/component/net`](../../tests/component/net/). Those component tests
validate the framework foundation. These four CTests validate the **complete
external application artifact** presented here.

## Continue from here

Once this plain IPv4 path is clear, the same application/context model can be
applied to other SNode.C components by selecting the corresponding installed
headers and exported CMake component. Higher application layers then add their
own protocol semantics above the same event-driven connection foundation.
