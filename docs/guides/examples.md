<!-- snodec:begin page-header -->
<a id="page-overview"></a>
<p>
  <a href="../../README.md#project-overview" title="SNode.C repository"><img src="../media/page-banner.svg" alt="SNode.C repository" width="100%"></a>
</p>
<!-- snodec:end page-header -->

# Build something with SNode.C

Start with the complete [native echo and Python interoperability example](../../README.md#your-first-program-a-factory-and-a-context). Continue here with HTTP, SSE, WebSocket and message framing. Each example includes prerequisites, build files, commands, expected results and boundaries; use a separate directory for each project.

## A small web service

HTTP uses a ready-made context factory. The Express-style `WebApp` adds routes on top, so application code can work with requests and responses instead of parsing bytes. This service offers a greeting and a small SSE endpoint used in the next example.

**You need:** the `http-server-express-legacy-in` development component, C++20, CMake, curl, and free loopback port **18081**. Use a new directory separate from the echo example.

**Code — `main.cpp`:**

```cpp
#include <express/legacy/in/WebApp.h>
#include <core/socket/State.h>
#include <iostream>
#include <memory>

int main(int argc, char* argv[]) {
    using WebApp = express::legacy::in::WebApp;
    WebApp::init(argc, argv);

    const WebApp app("hello");
    app.get("/hello", [](const std::shared_ptr<WebApp::Request>&,
                         const std::shared_ptr<WebApp::Response>& res) {
        res->send("Hello from SNode.C!\n");
    });
    app.get("/events", [](const std::shared_ptr<WebApp::Request>&,
                          const std::shared_ptr<WebApp::Response>& res) {
        res->set("Content-Type", "text/event-stream")
           .set("Cache-Control", "no-cache")
           .sendHeader();
        res->sendFragment("event: measurement\nid: 1\n"
                          "data: {\"sensor\":\"temperature\",\"value\":23.5}\n\n");
    });

    app.listen("127.0.0.1", 18081,
               [](const WebApp::SocketAddress& address,
                  const core::socket::State& state) {
        if (state == core::socket::State::OK)
            std::cout << "Listening on " << address.toString() << '\n';
        else
            std::cerr << state.what() << '\n';
    });

    return WebApp::start();
}
```

**Build — `CMakeLists.txt`:**

```cmake
cmake_minimum_required(VERSION 3.18)
project(snodec_hello LANGUAGES CXX)
find_package(snodec REQUIRED COMPONENTS http-server-express-legacy-in)
add_executable(hello main.cpp)
target_compile_features(hello PRIVATE cxx_std_20)
target_link_libraries(hello PRIVATE snodec::http-server-express-legacy-in)
```

**Run — terminal 1:**

```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/hello --config-file /dev/null
```

**Run — terminal 2:**

```text
curl http://127.0.0.1:18081/hello
```

**Expected result:** `Hello from SNode.C!` followed by a newline. Leave the server running for the SSE example, or stop it with Ctrl+C.

**Boundaries:** loopback HTTP, without TLS or authentication. For HTTPS use `<express/tls/in/WebApp.h>`, `express::tls::in::WebApp` and component `http-server-express-tls-in`, then configure certificates and trust.

[![deployment and resource limits](../media/menu/further-deployment.svg)](deployment.md#page-overview)

## Receive a server-sent event

SSE delivers named events over HTTP without a WebSocket connection. This native client consumes the `/events` route shown above and closes after the first measurement.

**You need:** the running HTTP example, the `http-client` and `net-in-stream-legacy` development components, C++20 and CMake. Create another empty directory.

**Code — `main.cpp`:**

```cpp
#include <core/SNodeC.h>
#include <net/in/SocketAddress.h>
#include <web/http/legacy/in/EventSource.h>
#include <iostream>

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);
    auto events = web::http::legacy::in::EventSource(
        "http", net::in::SocketAddress{"127.0.0.1", 18081}, "/events");
    events->addEventListener("measurement", [&events](const auto& event) {
        std::cout << event.data << std::endl;
        events->close();
    });
    events->onError([] { std::cerr << "SSE connection error\n"; });
    return core::SNodeC::start();
}
```

**Build — `CMakeLists.txt`:**

```cmake
cmake_minimum_required(VERSION 3.18)
project(snodec_events LANGUAGES CXX)
find_package(snodec REQUIRED COMPONENTS http-client net-in-stream-legacy)
add_executable(events main.cpp)
target_compile_features(events PRIVATE cxx_std_20)
target_link_libraries(events PRIVATE snodec::http-client snodec::net-in-stream-legacy)
```

**Run — terminal 2:**

```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/events --config-file /dev/null
```

**Expected result:** `{"sensor":"temperature","value":23.5}`. You can also inspect the wire format with `curl -N http://127.0.0.1:18081/events`.

**Boundaries:** the server sends one event and leaves the response open; the native client closes after receiving it. Stop curl with Ctrl+C. A continuous feed sends further fragments and removes subscriber callbacks on disconnect. SSE records use newline separators and a blank line after each event; HTTP chunk boundaries are not SSE record boundaries. The first `EventSource` argument is the URL scheme (`"http"`), not an instance name. Reconnection and `Last-Event-ID` support do not supply application-level durable replay.

[![SSE and WebSocket capabilities](../media/menu/further-sse-websocket.svg)](capabilities.md#server-sent-events)

## Talk both ways with a WebSocket

HTTP serves a small page, then upgrades `/ws` to a bidirectional WebSocket connection. The installed `echo` subprotocol supplies the message handling; the route handles HTTP upgrade rather than implementing another frame parser.

**You need:** the `http-server-express-legacy-in` development component, the framework's installed HTTP/WebSocket upgrade and **server-side echo subprotocol plugins** (package `snodec-apps`, included in `snodec`), C++20, CMake and a browser. Port **18082** must be free. Keep plugins and libraries from the same installation.

**Code — `main.cpp`:**

```cpp
#include <core/socket/State.h>
#include <express/legacy/in/WebApp.h>
#include <iostream>
#include <memory>
#include <string>

int main(int argc, char* argv[]) {
    using WebApp = express::legacy::in::WebApp;
    WebApp::init(argc, argv);
    const WebApp app("websocket");
    app.get("/", [](const std::shared_ptr<WebApp::Request>&,
                    const std::shared_ptr<WebApp::Response>& res) {
        res->set("Content-Type", "text/html; charset=utf-8").send(R"HTML(
<!doctype html>
<html lang="en"><meta charset="utf-8"><title>SNode.C WebSocket echo</title>
<h1>WebSocket echo</h1><pre id="output">Connecting...</pre>
<script>
const output = document.getElementById('output');
const peer = new WebSocket('ws://' + location.host + '/ws', 'echo');
peer.onopen = () => { output.textContent = 'Connected'; peer.send('Hello, WebSocket!'); };
peer.onmessage = event => { output.textContent += '\n' + event.data; peer.close(); };
peer.onerror = () => { output.textContent += '\nConnection failed'; };
</script></html>
)HTML");
    });
    app.get("/ws", [](const std::shared_ptr<WebApp::Request>& req,
                      const std::shared_ptr<WebApp::Response>& res) {
        res->upgrade(req, [res](const std::string& selected) {
            if (selected.empty()) res->sendStatus(400);
            else res->end();
        });
    });
    app.listen("127.0.0.1", 18082,
               [](const WebApp::SocketAddress&, const core::socket::State& state) {
        if (state != core::socket::State::OK) std::cerr << state.what() << '\n';
    });
    return WebApp::start();
}
```

**Build — `CMakeLists.txt`:**

```cmake
cmake_minimum_required(VERSION 3.18)
project(snodec_websocket LANGUAGES CXX)
find_package(snodec REQUIRED COMPONENTS http-server-express-legacy-in)
add_executable(websocket main.cpp)
target_compile_features(websocket PRIVATE cxx_std_20)
target_link_libraries(websocket PRIVATE snodec::http-server-express-legacy-in)
```

**Run — terminal 1:**

```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/websocket --config-file /dev/null
```

Open **[http://127.0.0.1:18082/](http://127.0.0.1:18082/)** in a browser.

**Expected result:** the page displays `Connected` and then `Hello, WebSocket!`. The browser sends a text frame, receives the echo and closes its connection. Stop the server with Ctrl+C.

**Boundaries:** the `echo` demonstration plugin broadcasts received text to its connected clients; this page opens one client. Missing plugins make the upgrade fail. For deployment, use TLS/WSS, authenticate access and configure message limits. SSE is simpler when data only needs to flow from server to client.

[![WebSocket capabilities](../media/menu/further-websocket.svg)](capabilities.md#websockets) [![framework's subprotocol implementation](../media/menu/further-subprotocol-source.svg)](../../src/apps/websocket/subprotocol)

## A line-oriented protocol

TCP delivers bytes, not application messages. This context buffers partial input until a newline arrives and echoes each completed line. It accepts at most 4096 bytes per line, including its newline.

**You need:** installed `net-in-stream-legacy` development libraries, C++20, CMake 3.18+, Python 3, and free loopback port **18002**.

**Code — `main.cpp`:**

```cpp
#include <core/SNodeC.h>
#include <core/socket/State.h>
#include <core/socket/stream/SocketContext.h>
#include <core/socket/stream/SocketContextFactory.h>
#include <net/in/stream/legacy/SocketServer.h>
#include <cstddef>
#include <iostream>
#include <string>

class LineContext final : public core::socket::stream::SocketContext {
public:
    explicit LineContext(core::socket::stream::SocketConnection* connection)
        : SocketContext(connection) {}
private:
    std::string pending;
    void onConnected() override {}
    void onDisconnected() override {}
    bool onSignal(int) override { return true; }
    std::size_t onReceivedFromPeer() override {
        char bytes[4096];
        const std::size_t count = readFromPeer(bytes, sizeof(bytes));
        for (std::size_t i = 0; i < count; ++i) {
            if (pending.size() == 4096) {
                close();
                return count;
            }
            pending += bytes[i];
            if (bytes[i] == '\n') {
                sendToPeer(pending);
                pending.clear();
            }
        }
        return count;
    }
};

class LineFactory final : public core::socket::stream::SocketContextFactory {
    core::socket::stream::SocketContext*
    create(core::socket::stream::SocketConnection* connection) override {
        return new LineContext(connection);
    }
};

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);
    using Server = net::in::stream::legacy::SocketServer<LineFactory>;
    const Server server("lines");
    server.listen("127.0.0.1", 18002,
                  [](const Server::SocketAddress&, core::socket::State state) {
        if (state != core::socket::State::OK) std::cerr << state.what() << '\n';
    });
    return core::SNodeC::start();
}
```

**Build — `CMakeLists.txt`:**

```cmake
cmake_minimum_required(VERSION 3.18)
project(snodec_lines LANGUAGES CXX)
find_package(snodec REQUIRED COMPONENTS net-in-stream-legacy)
add_executable(lines main.cpp)
target_compile_features(lines PRIVATE cxx_std_20)
target_link_libraries(lines PRIVATE snodec::net-in-stream-legacy)
```

**Run — terminal 1:**

```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/lines --config-file /dev/null
```

**Run — terminal 2:**

```text
python3 - <<'PY'
import socket
with socket.create_connection(('127.0.0.1', 18002)) as peer:
    peer.sendall(b'first')
    peer.sendall(b' line\nsecond line\n')
    with peer.makefile('rb') as reply:
        print(reply.readline().decode(), end='')
        print(reply.readline().decode(), end='')
PY
```

**Expected result:** `first line` and `second line`, each on its own line. Network reads may combine or split the writes; neither changes the result. Stop the server with Ctrl+C.

**Boundaries:** an overlong line closes the connection, and an unfinished line is discarded on disconnect. This example uses LF delimiters and unencrypted loopback TCP. Applications should also configure idle and write-queue limits.

[![resource and deployment policy](../media/menu/further-deployment.svg)](deployment.md#page-overview)

## Compose roles in one application

Multiple named clients and servers can coexist between one `core::SNodeC::init()` and one `core::SNodeC::start()`. Keep shared application state separate from per-peer contexts, give each instance a distinct name, and keep callbacks non-blocking. HTTP, custom protocols and database operations can then cooperate through one event loop without starting a separate process for every interface.

For a complete compilable multi-instance exercise, follow [one-process instance composition](../../examples/echo/README.md#compose-instances-in-one-process). It reuses the standalone pair’s context and verifies two separately named loopback listeners in the same event loop.
