# Example applications

The programs in this directory are runnable examples of SNode.C composition and
configuration. They are useful as source references, but they are not a support
matrix: a target existing in the tree does not by itself mean that every
transport, connection mode, protocol, and platform combination has equivalent
runtime qualification.

Start with the plain IPv4 echo pair below. It is deliberately small enough to
trace from C++ types through CMake, configuration, connection establishment,
semantic application logging, and per-connection callbacks without introducing
HTTP, TLS, WebSocket, or another application protocol first.

[Project README](../../README.md) ·
[API documentation](https://snodec.github.io/snode.c-doc/html/index.html) ·
[Echo sources](echo/) ·
[Network component tests](../../tests/component/net/README.md)

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
> publication qualification. That recorded run established listener startup and
> connection formation. The current source adds sparse information-level
> semantic messages inside `EchoSocketContext` so the application behavior is
> visible during an ordinary learning run. The verification recipes later on
> this page are reproducible checks, not claims that every recipe has been run in
> every environment.

## Files in the path

The complete example contains transport-independent echo behavior plus the
client/server wrappers that select a concrete network target:

| File | Responsibility |
| --- | --- |
| [`echo/model/EchoSocketContext.h`](echo/model/EchoSocketContext.h) | Declares the per-connection echo context, its small observation state, and the server/client context factories. |
| [`echo/model/EchoSocketContext.cpp`](echo/model/EchoSocketContext.cpp) | Emits semantic application messages, sends the client greeting, and reflects every received chunk. |
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
    bool firstPayloadObserved = false;
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

### Semantic output belongs to the context

The context uses the inherited `SocketContext::log()` logger. That is a semantic
logger with **application** origin and **context** boundary; the framework also
attaches the instance name and connection identity to its log scope. The example
does not use legacy `LOG`/`VLOG` macros for this behavior.

The information-level messages are intentionally sparse. A learner should see
when the context attaches, what the client initially sends, and that the first
payload is actually reflected. Logging every reflected chunk at information
level would be a bad default because the supplied client and server both echo:
once the client sends its greeting, the same bytes continue travelling between
the two contexts until the pair is stopped.

The behavioral core is therefore:

```cpp
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

std::size_t EchoSocketContext::onReceivedFromPeer() {
    char chunk[4096];
    const std::size_t chunklen = readFromPeer(chunk, 4096);

    if (chunklen > 0) {
        const char* roleName = role == Role::CLIENT ? "client" : "server";
        const auto contextLog = log();

        if (!firstPayloadObserved) {
            contextLog.info(
                "Echo {}: received {} bytes; reflecting first payload",
                roleName,
                chunklen);
            firstPayloadObserved = true;
        }

        if (contextLog.enabled(logger::LogLevel::Trace)) {
            contextLog.trace(
                "Echo {}: data to reflect: {}",
                roleName,
                std::string(chunk, chunklen));
        }

        sendToPeer(chunk, chunklen);
    }

    return chunklen;
}
```

`onDisconnected()` emits one corresponding information-level context-detach
message. The full payload remains a trace-level diagnostic because arbitrary
network input should not be dumped continuously into ordinary application logs.

The factories themselves stay deliberately small:

```cpp
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

The client sends the initial 30-byte greeting when its context attaches. Every
received chunk is then read from the connection and sent back unchanged.
Because both the server and the supplied client use the same reflecting context,
the greeting is reflected back and forth until the pair is stopped. The
`firstPayloadObserved` member controls only information-level presentation; it
does not change the echo behavior.

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
        // The real source semantically logs OK, DISABLED, ERROR, and FATAL.
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
        // The real source semantically logs OK, DISABLED, ERROR, and FATAL.
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
| `core::socket::stream::SocketContext` | Base class for connection-local protocol/application callbacks and the context-scoped semantic application logger. |
| `logger::BoundaryLogger` | Semantic logger returned by `SocketContext::log()`; filtering keeps information output concise while trace can show payload contents. |
| `EchoSocketContext::Role` | Lets the same context behave as server or client; only the client sends the initial greeting. |

The endpoint's `getConfig()` exposes the concrete configuration assembled for
that selected role and transport. The server command uses its `local` section;
the client command uses its `remote` section.

## Essential includes

The current example uses macro-selected wrapper headers so one source can build
several variants. For the plain IPv4 path, the networking- and behavior-relevant
includes reduce conceptually to:

```cpp
#include "core/SNodeC.h"
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "log/SemanticLogger.h"
#include "net/in/stream/legacy/SocketServer.h"
#include "net/in/stream/legacy/SocketClient.h"
```

Their responsibilities are distinct:

- `core/SNodeC.h` provides framework initialization and the event-loop entry
  point.
- `SocketContext.h` and `SocketContextFactory.h` define the application-facing
  per-connection extension point.
- `log/SemanticLogger.h` provides the semantic levels and `BoundaryLogger` used
  for the context's learning-oriented application output.
- `net/in/stream/legacy/SocketServer.h` and `SocketClient.h` select IPv4 plus the
  plain stream connection implementation and the corresponding configuration
  types.
- `SemanticLog.h`, used by the real server/client wrapper sources, supplies their
  application-level semantic log access. The context itself uses its inherited
  `log()` so its records carry context, instance, and connection scope.

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

The listener callback emits its ordinary application status. After a client
connects, `EchoSocketContext` also emits context-local semantic information. With
logger metadata omitted, the meaningful application messages include:

```text
echoserver: listening on '127.0.0.1:18001'
Echo server context attached
Echo server: received 30 bytes; reflecting first payload
```

The last line is emitted once for each server-side echo context, not for every
subsequent reflected chunk.

## 7. Run the client

In a second terminal, using the same isolated configuration root:

```sh
XDG_CONFIG_HOME="$PWD/cmake-build-release/echo-config" \
  ./cmake-build-release/src/apps/echo/echoclient-legacy-in \
  --monochrom=true \
  echoclient remote --host 127.0.0.1 --port 18001
```

Here `remote` is the client destination section. Again omitting logger metadata,
the client-side application messages include:

```text
echoclient: connected to '127.0.0.1:18001 (127.0.0.1)'
Echo client context attached
Echo client: sending initial greeting: 'Hello peer! Nice to see you!!!'
Echo client: received 30 bytes; reflecting first payload
```

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

### Inspect every reflected payload

Information-level output deliberately reports only the first received payload
per context. To inspect the actual contents of every reflected chunk, add this
root option to either run command:

```sh
--log-origin-level application=trace
```

The context-scoped semantic trace then includes messages such as:

```text
Echo server: data to reflect: Hello peer! Nice to see you!!!
Echo client: data to reflect: Hello peer! Nice to see you!!!
```

Because both supplied peers reflect every received chunk, trace output is
continuous. Use it briefly for diagnosis or learning; ordinary information
logging is intentionally bounded.

## 8. Verify the worked example

The repository already has extensive IPv4/plain component coverage under
[`tests/component/net`](../../tests/component/net/), including composition,
payload exchange, framed and large payloads, multiple messages, multiple
clients, disconnect lifecycle, controlled close, connection failure, and
effective listener-address tests. For example:

- [`InetLegacyServerClientCompositionTest.cpp`](../../tests/component/net/InetLegacyServerClientCompositionTest.cpp)
- [`InetLegacyServerClientPayloadExchangeTest.cpp`](../../tests/component/net/InetLegacyServerClientPayloadExchangeTest.cpp)
- [`InetLegacyServerClientLargePayloadExchangeTest.cpp`](../../tests/component/net/InetLegacyServerClientLargePayloadExchangeTest.cpp)
- [`InetLegacyServerClientDisconnectLifecycleTest.cpp`](../../tests/component/net/InetLegacyServerClientDisconnectLifecycleTest.cpp)

The checks below serve a different purpose: they exercise the **actual example
executables** and their application wiring. They are intentionally small enough
to copy from this page after building the pair above. The socket-peer checks use
Python 3 only; they do not require another SNode.C program.

### Test 1 — generated configuration surface

First verify that the executable names, instance names, and endpoint sections
shown by this walkthrough really exist in the generated interface:

```sh
SERVER=./cmake-build-release/src/apps/echo/echoserver-legacy-in
CLIENT=./cmake-build-release/src/apps/echo/echoclient-legacy-in
CFG="$PWD/cmake-build-release/echo-config"

XDG_CONFIG_HOME="$CFG" "$SERVER" --monochrom=true --help=expanded \
  > /tmp/snodec-echo-server-help.txt
XDG_CONFIG_HOME="$CFG" "$CLIENT" --monochrom=true --help=expanded \
  > /tmp/snodec-echo-client-help.txt

grep -F 'echoserver' /tmp/snodec-echo-server-help.txt
grep -F 'local'      /tmp/snodec-echo-server-help.txt
grep -F 'echoclient' /tmp/snodec-echo-client-help.txt
grep -F 'remote'     /tmp/snodec-echo-client-help.txt
```

A failing `grep` gives a non-zero status, making this useful in a shell script as
well as interactively. This check is deliberately about the **application
artifact**: lower-level socket tests cannot prove that the demo binary retained
its documented instance names and generated command hierarchy.

For a configuration-focused follow-up, inspect the same binaries with
`--show-config` and `--command-line=complete`.

### Test 2 — real echo server with a deterministic external peer

This test starts the real `echoserver-legacy-in`, connects with a small Python
TCP peer, sends a byte sequence containing a NUL byte, and requires exactly the
same bytes back. That makes the assertion independent of human-readable log
text and demonstrates that the server executable reaches its context factory,
`EchoSocketContext::onReceivedFromPeer()`, and `sendToPeer()` path.

```sh
python3 - <<'PY'
import os
import socket
import subprocess
import time
from pathlib import Path

server = "./cmake-build-release/src/apps/echo/echoserver-legacy-in"
config = str(Path("cmake-build-release/echo-config").resolve())
address = ("127.0.0.1", 18001)
payload = b"SNode.C echo\x00payload\n"
env = os.environ | {"XDG_CONFIG_HOME": config}

proc = subprocess.Popen(
    [server, "--monochrom=true",
     "echoserver", "local", "--host", address[0], "--port", str(address[1])],
    env=env,
    stdout=subprocess.DEVNULL,
    stderr=subprocess.STDOUT,
)

def stop(process):
    if process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=2)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()

try:
    peer = None
    deadline = time.monotonic() + 5
    while time.monotonic() < deadline:
        if proc.poll() is not None:
            raise RuntimeError(f"server exited early with {proc.returncode}")
        try:
            peer = socket.create_connection(address, timeout=0.25)
            break
        except OSError:
            time.sleep(0.05)
    if peer is None:
        raise TimeoutError("server did not become reachable")

    with peer:
        peer.settimeout(2)
        peer.sendall(payload)
        received = bytearray()
        while len(received) < len(payload):
            chunk = peer.recv(len(payload) - len(received))
            if not chunk:
                raise RuntimeError("server closed before the payload was complete")
            received.extend(chunk)

    assert bytes(received) == payload, (bytes(received), payload)
    print(f"PASS server reflected {len(payload)} bytes exactly")
finally:
    stop(proc)
PY
```

Using a byte-exact peer is important here: an echo application should preserve
payload length and contents, not merely establish a TCP connection or print a
success message.

### Test 3 — real echo client with a deterministic external peer

The reciprocal test starts a Python listener first and then launches the real
`echoclient-legacy-in`. It checks two application-specific facts: the client
sends the exact greeting from `onConnected()`, and after the peer echoes that
greeting once, the client reflects it back through `onReceivedFromPeer()`.

```sh
python3 - <<'PY'
import os
import socket
import subprocess
from pathlib import Path

client = "./cmake-build-release/src/apps/echo/echoclient-legacy-in"
config = str(Path("cmake-build-release/echo-config").resolve())
address = ("127.0.0.1", 18002)
expected = b"Hello peer! Nice to see you!!!"
env = os.environ | {"XDG_CONFIG_HOME": config}

listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
listener.bind(address)
listener.listen(1)
listener.settimeout(5)

proc = subprocess.Popen(
    [client, "--monochrom=true",
     "echoclient", "remote", "--host", address[0], "--port", str(address[1])],
    env=env,
    stdout=subprocess.DEVNULL,
    stderr=subprocess.STDOUT,
)

def recv_exact(sock, size):
    data = bytearray()
    while len(data) < size:
        chunk = sock.recv(size - len(data))
        if not chunk:
            raise RuntimeError("peer closed before the payload was complete")
        data.extend(chunk)
    return bytes(data)

def stop(process):
    if process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=2)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()

try:
    peer, _ = listener.accept()
    with peer:
        peer.settimeout(2)
        greeting = recv_exact(peer, len(expected))
        assert greeting == expected, (greeting, expected)

        peer.sendall(greeting)
        reflected = recv_exact(peer, len(expected))
        assert reflected == expected, (reflected, expected)

    print("PASS client sent the greeting and reflected the echoed greeting")
finally:
    listener.close()
    stop(proc)
PY
```

This is more deterministic than using the two echo programs alone: the Python
peer controls when the exchange stops and can assert the exact application
bytes before closing.

### Test 4 — bounded real-pair smoke test

Finally run the two actual example executables together. Because both contexts
reflect every received chunk, an unattended test must be bounded rather than
leaving the pair running indefinitely. This smoke test waits for the server to
listen, starts the client, and checks the new information-level semantic output
from both contexts before terminating them.

```sh
python3 - <<'PY'
import os
import subprocess
import tempfile
import time
from pathlib import Path

server = "./cmake-build-release/src/apps/echo/echoserver-legacy-in"
client = "./cmake-build-release/src/apps/echo/echoclient-legacy-in"
config = str(Path("cmake-build-release/echo-config").resolve())
address = ("127.0.0.1", 18003)
env = os.environ | {"XDG_CONFIG_HOME": config}


def stop(process):
    if process is not None and process.poll() is None:
        process.terminate()
        try:
            process.wait(timeout=2)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()


def wait_for(path, required, processes, timeout=5):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        text = path.read_text(errors="replace") if path.exists() else ""
        if all(item in text for item in required):
            return text
        for process in processes:
            if process is not None and process.poll() is not None:
                raise RuntimeError(f"process exited early with {process.returncode}\n{text}")
        time.sleep(0.05)
    raise TimeoutError(f"missing expected output in {path}\n{text}")

with tempfile.TemporaryDirectory(prefix="snodec-echo-") as tmp:
    tmp = Path(tmp)
    server_log = tmp / "server.log"
    client_log = tmp / "client.log"
    server_proc = None
    client_proc = None

    try:
        with server_log.open("w") as out:
            server_proc = subprocess.Popen(
                [server, "--monochrom=true", "--log-origin-level", "application=info",
                 "echoserver", "local", "--host", address[0], "--port", str(address[1])],
                env=env, stdout=out, stderr=subprocess.STDOUT)

        wait_for(server_log, [f"listening on '{address[0]}:{address[1]}'"],
                 [server_proc])

        with client_log.open("w") as out:
            client_proc = subprocess.Popen(
                [client, "--monochrom=true", "--log-origin-level", "application=info",
                 "echoclient", "remote", "--host", address[0], "--port", str(address[1])],
                env=env, stdout=out, stderr=subprocess.STDOUT)

        server_text = wait_for(
            server_log,
            ["Echo server context attached",
             "Echo server: received 30 bytes; reflecting first payload"],
            [server_proc, client_proc])
        client_text = wait_for(
            client_log,
            ["Echo client context attached",
             "Echo client: sending initial greeting: 'Hello peer! Nice to see you!!!'",
             "Echo client: received 30 bytes; reflecting first payload"],
            [server_proc, client_proc])

        print("PASS real echo pair reached both contexts and exchanged the greeting")
    finally:
        stop(client_proc)
        stop(server_proc)
PY
```

This test intentionally checks application messages rather than timestamps or
full logger prefixes. Semantic metadata and formatting may evolve independently
from the behavior the example is teaching.

### Relationship to the component suite

These four recipes do not replace the repository's CTest coverage. The
component tests exercise the reusable network layers directly and cover cases
that this tutorial should not reproduce. If you configure a separate test build
with `SNODEC_BUILD_TESTS=ON`, the IPv4/plain network family can be selected by
CTest name, for example:

```sh
ctest --test-dir cmake-build-test \
  -R '^InetLegacy' \
  --output-on-failure
```

The distinction is useful:

```text
component tests                         worked-example checks
      │                                         │
      ├─ composition                            ├─ generated CLI/config surface
      ├─ payload sizes/framing                  ├─ real server executable
      ├─ lifecycle/failure                      ├─ real client greeting behavior
      └─ multiple clients                       └─ real-pair semantic smoke test
```

Together they answer both questions a developer should ask: *does the reusable
network layer behave correctly?* and *is this example application actually
wired the way the documentation says it is?*

## What this example establishes

This path demonstrates how a real SNode.C application is assembled:

1. choose a concrete endpoint family and stream mode;
2. implement one connection-local `SocketContext`;
3. use context-scoped semantic logging to make application lifecycle and the
   first data action visible without flooding ordinary output;
4. provide factories that create that context for established connections;
5. parameterize `SocketServer` and `SocketClient` with those factories;
6. link the matching SNode.C component and the application context code;
7. configure named endpoint instances through the generated hierarchy;
8. enter the shared event loop and handle lifecycle/data callbacks per
   connection.

The recorded August 2026 IPv4 qualification establishes buildability, listener
startup, and connection formation for the cited `bf01683` revision. The current
source makes the application context behavior visible at information level and
offers the deterministic verification recipes above for a fresh checkout. Do
not extend one local run into a claim about IPv6, Unix-domain sockets, TLS,
Bluetooth, or higher protocol layers.

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
