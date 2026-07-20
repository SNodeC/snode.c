# Codex App Server client

The `ai::openai::codex` module provides an asynchronous SNode.C client for the
Codex App Server. Its public center is the transport-independent
`ai::openai::codex::AppServerClient`. The first concrete transport is
`ai::openai::codex::stdio::Client`, which starts `codex app-server` locally and
communicates over standard input and standard output.

The client implements process and transport lifecycle, the Codex initialization
handshake, a generic raw protocol engine, and typed facades over that engine.
Callers can submit arbitrary raw App Server messages or use the typed thread,
turn, event, item, approval, and user-input API after the client reaches
`Ready`. See [Typed Codex App Server API](typed-api.md) for the typed boundary
and forward-compatibility rules.

## Public interface

Include the local client with:

```cpp
#include "ai/openai/codex/stdio/Client.h"
```

Raw protocol values and strong request-ID types are available from:

```cpp
#include "ai/openai/codex/Protocol.h"
```

The client exposes:

```cpp
void start();
void stop();

ai::openai::codex::State getState() const noexcept;
bool isReady() const noexcept;

void setOnStateChanged(ai::openai::codex::Callbacks::StateChanged callback);
void setOnDiagnostic(ai::openai::codex::Callbacks::DiagnosticReceived callback);

ai::openai::codex::AppServerClient::RawProtocol& raw() noexcept;
std::optional<ai::openai::codex::InitializeResult> getInitializeResult() const;
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

All callbacks run in the SNode.C event-loop context. State and protocol
callbacks are queued and ordered. Public protocol callbacks never run inline
from `request()`, `notify()`, `respond()`, or `reject()`. Protocol callbacks may
call those methods, call `stop()`, or destroy the client. Exceptions escaping a
protocol callback are caught and logged instead of unwinding through the event
loop. `start()` and `stop()` initiate work and return before completion callbacks
are dispatched.

See [Generic raw protocol engine](raw-protocol-engine.md) for request ownership,
strong ID semantics, capacity limits, cancellation, generation safety, and
unknown-message handling.

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

`StateChange::error` is populated on the transition to `Failed` and distinguishes
launch, transport, protocol, initialization, and process failures. The shared
public `Error` type also represents invalid-state, capacity, cancellation, and
enqueue failures returned by raw operations. Remote App Server errors remain
distinct from all of these local errors.

## Initialization

After the process and parent descriptors are ready, the client:

1. sends one `initialize` request with the configured `ClientInfo`;
2. correlates the response by integer request ID;
3. validates the minimal initialization result;
4. sends the `initialized` notification; and
5. transitions to `Ready`.

Initialization uses the same monotonically increasing request-ID allocator as
caller requests but remains internally owned. Raw callers cannot send the
reserved `initialize` or `initialized` operations. The typed initialization
fields and complete raw result are cached for `getInitializeResult()`.
Structurally valid non-initialization messages received during the handshake are
held in a bounded queue and dispatched in wire order after `Ready`.

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

The target exports a C++20 compile-feature requirement. The public
`Protocol.h` uses `nlohmann::json`, so the module also exposes the include path
provided by `nlohmann_json >= 3.11` to build-tree and installed consumers. A
regular build and test run does not require a Codex installation or
authentication; the real App Server integration test is optional.
