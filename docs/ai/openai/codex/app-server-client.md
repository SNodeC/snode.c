# Codex App Server client

The `ai::openai::codex` module provides an asynchronous SNode.C client for the
Codex App Server. Its public center is the transport-independent
`ai::openai::codex::AppServerClient`. The first concrete transport is
`ai::openai::codex::stdio::Client`, which starts `codex app-server` locally and
communicates over standard input and standard output.

This first milestone implements process and transport lifecycle plus the Codex
initialization handshake. Thread, turn, item, approval, and WebSocket APIs are
intentionally not part of the public interface yet.

## Public interface

Include the local client with:

```cpp
#include "ai/openai/codex/stdio/Client.h"
```

The client exposes:

```cpp
void start();
void stop();

ai::openai::codex::State getState() const noexcept;
bool isReady() const noexcept;

void setOnStateChanged(ai::openai::codex::Callbacks::StateChanged callback);
void setOnDiagnostic(ai::openai::codex::Callbacks::DiagnosticReceived callback);
```

The default constructor runs:

```text
codex app-server
```

An executable, argument vector, and client identity can be supplied explicitly:

```cpp
ai::openai::codex::stdio::Client client(
    "/opt/codex/bin/codex",
    {"app-server"},
    {.name = "my_client", .title = "My Client", .version = "1.0"});
```

`AppServerClient` owns its transport through composition. It is not a socket
client, and future transports can preserve this public lifecycle API.

## Example

```cpp
#include "ai/openai/codex/stdio/Client.h"
#include "core/SNodeC.h"

#include <memory>

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    auto client = std::make_unique<ai::openai::codex::stdio::Client>();

    client->setOnStateChanged([](const ai::openai::codex::StateChange& change) {
        if (change.current == ai::openai::codex::State::Failed && change.error) {
            // Inspect change.error->category, code, and message.
        }
    });

    client->setOnDiagnostic([](const ai::openai::codex::Diagnostic& diagnostic) {
        // stderr diagnostics are delivered here and are never parsed as protocol.
    });

    client->start();
    const int result = core::SNodeC::start();
    client.reset();
    core::SNodeC::free();
    return result;
}
```

All callbacks run in the SNode.C event-loop context. State callbacks are queued,
ordered, and non-reentrant. `start()` and `stop()` initiate work and return before
completion callbacks are dispatched.

## State model

The successful lifecycle is:

```text
Stopped -> Starting -> Initializing -> Ready
Ready   -> Stopping -> Stopped
```

Fatal launch, stdin/stdout transport, protocol, initialization, or unexpected
process-exit errors lead to `Failed`. Recovery is explicit:

```text
Failed -> stop() -> Stopping -> Stopped -> start() -> Starting
```

There is no automatic restart or reconnect. Calls to `start()` outside
`Stopped`, calls to `stop()` in `Stopped` or `Stopping`, and startup attempted
while SNode.C is stopping have no effect.

`StateChange::error` is populated on the transition to `Failed`. Error
categories distinguish launch, transport, protocol, initialization, and process
failures.

## Initialization

After the process and parent descriptors are ready, the client:

1. sends one `initialize` request with the configured `ClientInfo`;
2. correlates the response by integer request ID;
3. validates the minimal initialization result;
4. sends the `initialized` notification; and
5. transitions to `Ready`.

Protocol encoding produces pure JSON documents. The stdio transport adds exactly
one newline to each outgoing document. Incoming stdout is framed as JSON Lines;
split reads and multiple messages per read are supported.

## Diagnostics and failures

Child stdout is exclusively the Codex JSONL protocol stream. Child stderr is an
independent line-oriented diagnostic stream.

Closing stderr or encountering a stderr read error is nonfatal. Any trailing
diagnostic line is emitted, the stderr receiver is detached, and protocol
communication continues. Unexpected stdin or stdout failures remain fatal unless
they are explained by an expected shutdown or child exit.

## Build integration

The installed CMake target is:

```cmake
target_link_libraries(my_target PRIVATE snodec::ai-openai-codex)
```

The module requires `nlohmann_json >= 3.11` at build time. A regular build and
test run does not require a Codex installation or authentication; the real App
Server integration test is optional.

