# Build something with SNode.C

<p>
  <a href="../../README.md"><img src="media/menu/back-snode-c.svg" alt="← SNode.C" width="96" height="24"></a>
</p>

The landing page contains complete [Factory/Context](../../README.md#your-first-program-a-factory-and-a-context), [HTTP](../../README.md#a-small-web-service), [SSE](../../README.md#receive-a-server-sent-event) and [WebSocket](../../README.md#talk-both-ways-with-a-websocket) examples, including their build files and run commands. This guide adds message framing and explains composing roles. Use a separate directory for each project.

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

**Go further:** [resource and deployment policy](deployment.md).

## Compose roles in one application

Multiple named clients and servers can coexist between one `core::SNodeC::init()` and one `core::SNodeC::start()`. Keep shared application state separate from per-peer contexts, give each instance a distinct name, and keep callbacks non-blocking. HTTP, custom protocols and database operations can then cooperate through one event loop without starting a separate process for every interface.
