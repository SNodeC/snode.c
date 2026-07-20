# Codex backend reference application

`codex-backend` is a small local reference composition for
[Codex Frontend Protocol v1](frontend-protocol-v1.md). It owns one local Codex
App Server client and accepts several frontend clients on a Unix-domain stream
socket. It is an adapter and example, not a second backend implementation or a
production daemon. The companion
[`codex-backend-client`](../../../../src/apps/codex-backend-client/README.md) is a
small terminal and diagnostic client for that frontend protocol.

The concrete transport is intentionally confined to
`src/apps/codex-backend`:

```text
ai::openai::codex::stdio::Client
                  ↓
             BackendCore
                  ↓
      frontend::BackendAdapter
                  ↓
net::un::stream::legacy::SocketServer
                  ↓
       compact JSONL clients
```

Reusable `ai-openai-codex-backend` code has no socket dependency, and reusable
`ai-openai-codex-frontend` code has no Unix or JSONL dependency. Only this
application links `net-un-stream-legacy`. That boundary keeps future in-process,
Qt, CLI, WebSocket, or other transports from depending on a Unix socket path or
`SocketContext`.

## Composition and lifetime

The listener uses the actual SNode.C legacy helper:

```cpp
ai::openai::codex::backend::BackendCore<
    ai::openai::codex::stdio::Client
> backend;

auto socketServer =
    net::un::stream::legacy::Server<
        apps::codex_backend::CodexFrontendSocketContextFactory
    >("codex-backend", configure, backend);
```

The helper yields a
`net::un::stream::legacy::SocketServer<CodexFrontendSocketContextFactory,
BackendCore<stdio::Client>&>`. The factory derives from
`core::socket::stream::SocketContextFactory`, owns one reusable
`frontend::BackendAdapter`, and constructs one
`CodexFrontendSocketContext` per accepted socket.

`main()` performs this ordering:

1. initialize `core::SNodeC`;
2. directly construct `backend::BackendCore<stdio::Client>`;
3. create and listen with the Unix socket server;
4. start `BackendCore`, then enter the SNode.C event loop;
5. destroy the socket server and all contexts;
6. destroy `BackendCore`, whose non-template runtime stops before its directly
   owned App Server client is destroyed; and
7. perform the final idempotent `core::SNodeC::free()` cleanup.

This construction order prevents an accepted context from outliving the
backend. Frontend callbacks additionally use weak lifetime gates. Disconnecting
one frontend closes its transport-neutral `FrontendConnection` and backend
`FrontendSession`; it does not call `BackendCore::stop()` and does not stop the
Codex App Server. An explicit backend stop/start is the recovery boundary.

## Build and install

The executable follows the repository's `src/apps` convention and is built
only when `SNODEC_BUILD_APPS` is enabled:

```sh
cmake -S . -B build \
  -DSNODEC_BUILD_APPS=ON \
  -DSNODEC_BUILD_TESTS=ON
cmake --build build --parallel --target codex-backend codex-backend-client
```

With a conventional single-configuration generator, run the in-tree binaries
from `build/src/apps/codex-backend/codex-backend` and
`build/src/apps/codex-backend-client/codex-backend-client`. Installing the
`apps` component places both executables in the configured binary install
directory:

```sh
cmake --install build --component apps
```

Ordinary library builds may set `-DSNODEC_BUILD_APPS=OFF`; the exported
`snodec::ai-openai-codex-backend` and
`snodec::ai-openai-codex-frontend` components remain independently reusable.
The executable also requires the `codex` command expected by the existing
stdio client. Authentication and quota are properties of that local Codex
installation, not of the frontend protocol.

Start the server and client in separate terminals. The client sends `hello`
automatically and provides typed `snapshot`, `acquire`, `threads`, and `read`
commands as well as the other commands documented in its README:

```sh
codex-backend
codex-backend-client
```

## Socket path

The application registers an SNode.C server instance named `codex-backend`.
Its local address exposes the standard `--sun-path` CLI/config-file option.
The option is authoritative when supplied. The exact nested CLI form is:

```sh
build/src/apps/codex-backend/codex-backend \
  codex-backend local \
  --sun-path /run/user/1000/my-codex-backend.sock
```

Use `--help=expanded` at the application root, or
`codex-backend local --help`, to inspect the generated configuration help.

Without an override, the safe application default is:

```text
$XDG_RUNTIME_DIR/snodec-codex-backend.sock
```

when `XDG_RUNTIME_DIR` is non-empty, otherwise:

```text
/tmp/snodec-codex-backend-<numeric-uid>.sock
```

The default never embeds a developer-specific UID. A filesystem socket uses a
locked sidecar containing a versioned SNode.C ownership marker, so competing
listeners cannot both claim it. A new marker is created exclusively. If the
socket path already exists at that point, startup refuses to infer ownership
retroactively and preserves the path, including an unmarked stale socket or a
socket that has been bound but has not begun listening.

The v1 marker payload is the exact UTF-8 byte sequence
`snodec.unix-socket-lock:v1\n`; extra or missing bytes make a sidecar
unrecognized.

Only a pre-existing sidecar whose marker is recognized can authorize crash
recovery. After acquiring its advisory lock, startup validates the marker,
confirms that the existing node is a socket, and probes it with a nonblocking
Unix connection. It removes only an `ECONNREFUSED`/`ENOENT` stale case, after
rechecking the socket type, device, and inode immediately before unlinking.
An active or unverifiable socket is preserved, as is an unrecognized sidecar,
regular file, symlink, replacement inode, or other unrelated filesystem
object. Clean shutdown likewise removes only the socket and marker identities
owned by this listener. Bind/listen failure is reported and stops the
application cleanly.

## Unix stream framing

The Unix transport uses one compact JSON object followed by one newline:

```text
{"protocol":"snodec.codex-frontend","version":1,"kind":"hello"}\n
```

The newline is transport framing and is not part of Frontend Protocol v1.
`Codec` itself consumes and produces JSON values or compact strings, so a
future transport can select another framing scheme.

`JsonLineFramer` accepts a line split over any number of reads and any number
of complete lines in one read. A trailing carriage return before the record
newline is removed. A JSON string containing an escaped newline uses the two
wire characters `\` and `n`, so it does not terminate a frame. Server messages
are serialized compactly and never pretty-printed.

The default maximum frame payload is exactly 1 MiB (1,048,576 bytes), excluding
the terminating newline. A payload of exactly that size is accepted if its
next byte is the newline. A further non-newline byte clears the bounded
accumulator, yields `frame_too_large`, sends a bounded local protocol error
where possible, and closes only that connection. The same isolation applies to
malformed JSON. `SocketFrontendOptions::maximumFrameSize` makes the bound
deterministic and configurable for embedding and tests; this reference
`main()` uses the default.

Example using a Unix-capable client such as `socat` (each input line must be
compact JSON):

```sh
printf '%s\n' \
  '{"protocol":"snodec.codex-frontend","version":1,"kind":"hello"}' \
  | socat - UNIX-CONNECT:"${XDG_RUNTIME_DIR:-/tmp}/snodec-codex-backend.sock"
```

When `XDG_RUNTIME_DIR` is absent, substitute the UID-bearing fallback path;
the shorthand command above intentionally does not guess it.

## Handshake and commands

The context opens a transport-neutral `FrontendConnection` when the socket is
accepted, but no backend `FrontendSession` exists until a valid hello. Send:

```json
{"protocol":"snodec.codex-frontend","version":1,"kind":"hello","resumeAfter":41}
```

The server sends `welcome`, replay batches or a snapshot, and
`sync.complete`. Every new session is an observer. Controller acquisition is a
separate correlated command:

```json
{"protocol":"snodec.codex-frontend","version":1,"kind":"command","requestId":"role-1","method":"controller.acquire","params":{}}
```

Other observers remain connected and receive the same ordered normalized
event batches. An observer can list/read threads and synchronize, but receives
`permission_denied` for thread/turn mutation or request answers. See the
[v1 protocol document](frontend-protocol-v1.md) and its
[JSON Schema](frontend-protocol-v1.schema.json) for every envelope and method.

Messages before hello, malformed JSON, unsupported protocol versions, unknown
kinds, and command validation failures never reach the raw Codex protocol. A
pre-hello error and identity/version/framing error close that client after a
bounded `protocol.error`. Post-handshake command errors normally keep the
connection open.

## Coalescing and outbound backpressure

The factory's shared `BackendAdapter` subscribes once to BackendCore. Raw text,
reasoning, and command-output deltas have already accumulated in canonical
state before the adapter sees their backend transition. The adapter marks the
specific entity/channel dirty, schedules only one next-tick flush, replaces
obsolete intermediate state for the same key, and emits normalized events in
batches of at most 64 events and 256 KiB by default. Terminal and interactive
updates flush immediately. The replay journal holds at most 4,096 normalized
events and 8 MiB, never the raw token stream.

There are two independent per-client backpressure boundaries:

- `BackendAdapter` allows at most 512 queued protocol messages and 11 MiB of
  compact serialized JSON per connection, delivering at most 64 messages in
  one event-loop callback;
- `CodexFrontendSocketContext` allows at most 13 MiB outstanding in that Unix
  connection's socket writer, including the newline framing byte.

The limits intentionally have headroom in dependency order: the 8 MiB journal
counts event objects, the 11 MiB adapter queue also accommodates bounded replay
batch and synchronization envelopes, and the 13 MiB Unix writer additionally
accommodates JSONL framing and data already handed off by the adapter. Thus a
new Unix connection can replay a full default journal without being rejected
solely because downstream accounting includes envelope overhead. All three
limits remain finite. Deployments that customize one limit must adjust the
downstream limits consistently; a complete snapshot is one separate message
and a configured writer too small for it closes only that frontend.

No unbounded application queue is added. If either boundary cannot accept the
next message, the adapter closes that frontend, clears its queued data, and
detaches its session. A throwing send callback has the same local effect. One
slow observer cannot add data to the controller's queue, delay another client,
stop BackendCore, or stop the App Server. A disconnected controller releases
its role; pending approvals, user-input prompts, authentication requests, and
unknown requests remain pending and are never automatically answered.

All receive, delivery, coalescing, and cleanup work runs through the SNode.C
event loop. The application adds no `std::thread`, blocking descriptor I/O,
direct `fork()`/`vfork()`, or blocking `waitpid()`.

## Shutdown and operational limits

Stopping the global SNode.C loop closes accepted contexts and the listening
socket before BackendCore is destroyed. The listener removes its own socket
and lock nodes on clean shutdown, subject to inode-identity checks. BackendCore
then stops the stdio App Server client, whose existing nonblocking lifecycle
owns child termination and reaping.

This application is deliberately local and minimal. It adds no daemon or
production-service policy (even though generic framework-wide CLI options may
appear in SNode.C help), frontend-user authentication, TLS, systemd socket
activation, snapshot/replay persistence, or automatic App Server restart.
Unix filesystem permissions and the runtime-directory policy are the local
access boundary.

No credentialed real-backend integration is registered in this milestone:
`SNODEC_RUN_CODEX_BACKEND_INTEGRATION=1` is therefore documented as a proposed
gate, not an implemented test switch. The ordinary deterministic suite uses a
fake App Server and requires no credentials, quota, network service, or real
model turn.

Task 5 behavior is explicitly deferred: no Qt event multiplexer, Qt UI,
WebSocket/TCP adapter, browser frontend, UI-product migration, remote
authentication, multi-controller policy, or forced controller takeover is
implemented here. Any later adapter must retain Protocol v1's state reduction,
coalescing, bounded batching, replay fallback, and slow-client isolation.
