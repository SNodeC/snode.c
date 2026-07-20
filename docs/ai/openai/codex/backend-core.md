# Codex BackendCore

`ai::openai::codex::backend::BackendCore` is the reusable, stateful layer above
the typed Codex App Server API. It owns one `AppServerClient`, reduces typed
operation results, notifications, lifecycle changes, diagnostics, and server
requests into canonical state, and exposes deterministic snapshots and
transport-neutral frontend sessions.

The dependency direction is deliberately one way:

```text
frontend protocol or another in-process consumer
                    ↓
        ai-openai-codex-backend
                    ↓
             ai-openai-codex
                    ↓
                   core
```

The backend library has no dependency on Unix sockets, `net/un`, socket
contexts, JSONL framing, Qt, WebSocket, or browser code. In particular, a Unix
socket path is not backend state. Concrete listener and framing code belongs in
`src/apps/codex-backend`.

## Ownership and construction

`BackendCore` takes exclusive ownership of exactly one client:

```cpp
#include "ai/openai/codex/backend/BackendCore.h"
#include "ai/openai/codex/stdio/Client.h"

auto appServer = std::make_unique<ai::openai::codex::stdio::Client>();
ai::openai::codex::backend::BackendCore backend(std::move(appServer));

backend.start();
```

`stdio::Client` is only one possible composition. The backend constructor
accepts `std::unique_ptr<AppServerClient>` and never constructs or assumes the
stdio transport. Deterministic tests can supply an `AppServerClient` backed by
a fake transport while using the same typed API and correlation registry.

The backend installs the client's lifecycle, diagnostic, typed-event, and
typed-server-request handlers. It does not instantiate a raw protocol engine,
does not allocate App Server client request IDs, and does not duplicate the
typed client's request-correlation registry. Destruction invalidates backend
callbacks, stops the owned client, suppresses queued frontend callbacks, and
then destroys the client.

The main API is:

```cpp
class BackendCore {
public:
    explicit BackendCore(
        std::unique_ptr<AppServerClient> client,
        BackendCoreOptions options = {});

    void start();
    void stop();

    BackendState state() const;
    Snapshot snapshot() const;
    bool isReady() const noexcept;

    FrontendSession openSession(FrontendSessionCallbacks callbacks);
    BackendObserverSubscription subscribe(BackendObserverCallbacks callbacks);
};
```

`state()` is intended for trusted in-process diagnostics and tests. It includes
the exact typed pending request needed to preserve occurrence ownership.
Transport and UI integrations should consume `snapshot()`, which deliberately
removes occurrence tokens and sensitive implementation data.

## Canonical state and identifiers

`BackendState` uses the strong `SessionId`, `PendingRequestId`, and
`SequenceNumber` types. Session and pending-request IDs reserve zero as an
invalid value. They increase monotonically and do not silently wrap.

The state contains:

- backend lifecycle and the last lifecycle error;
- a bounded diagnostic summary;
- threads in deterministic first-seen order;
- each thread's typed summary and its known turns;
- each turn's typed status, deterministic item order, terminal/failure state,
  token usage, and a bounded model-reroute history;
- each item's typed representation, lifecycle timestamps, and separately
  accumulated agent text, reasoning text, reasoning summary, and command
  output;
- exact pending typed server requests, indexed by backend-generated
  `PendingRequestId`;
- connected sessions and controller ownership;
- thread-list pagination and completeness information; and
- the current backend revision plus bounded unknown-extension records.

Maps provide ID-based upsert semantics while explicit order vectors preserve
the server's deterministic first-seen order. An operation result and a later
notification for the same thread, turn, or item update the same entity; they do
not create duplicate state.

The default reducer retains 64 diagnostics and 64 Codex extensions.
Individual diagnostic messages are capped at 16 KiB. Canonical extension
records cap the method at 4 KiB, the serialized payload at 64 KiB, and a
decoding error at 16 KiB. Model reroutes are capped at 64 per turn, and each
accumulated item-content stream at 4 MiB. When accumulated content exceeds its bound, the reducer retains the
newest suffix and increments `droppedContentBytes`. Snapshots expose both the
dropped byte count and `contentTruncated`, so a consumer never mistakes a
bounded suffix for complete output. These bounds are configurable through
`ReducerOptions`; ordinary 1,000-delta test bursts remain below the defaults
and reconstruct exactly.

## Reducer semantics

All ordinary domain transitions pass through `Reducer::apply()`. Typed Codex
events first pass through `Reducer::translate()` and become deliberately named
backend events:

- `LifecycleChanged` and `DiagnosticReceived`;
- `ThreadUpserted`, `ThreadListUpdated`, and `ThreadStatusUpdated`;
- `TurnUpserted`, `TurnCompleted`, `TurnFailed`, and `TurnErrorUpdated`;
- `ItemUpserted`, `ItemContentChanged`, and `FileChangeUpdated`;
- `TokenUsageUpdated` and `ModelRerouted`;
- `PendingRequestAdded` and `PendingRequestRemoved`;
- `ControllerChanged` and `SessionChanged`; and
- `CodexExtensionReceived`.

Unknown typed items with a stable ID remain items. Unknown events, or malformed
future item events that cannot identify an owning entity, remain observable as
bounded `CodexExtensionReceived` records with their original method or
deliberate extension name, payload, and optional decoding error. They do not
fail the backend and are not silently discarded.

The immutable public snapshot retains the newest 64 extension records under
stricter frontend-safe bounds: 1 KiB of UTF-8 method, 32 KiB of serialized
parameters, and 2 KiB of decoding error. Oversized parameters become an
explicit omission record with their original serialized byte count. Obvious
credential, authorization, password, token, answer, and secret-value keys are
redacted recursively (case-insensitively). The same sanitizer protects unknown
pending-request parameters. Thus neither an extension snapshot nor an unknown
request snapshot exposes App Server occurrence tokens, access tokens, or secret
answers.

The reducer updates canonical content immediately for every text or output
delta. Backend events describe transitions that have already been applied:
when an observer receives an event, an immediately obtained snapshot contains
that transition. Terminal turns, failed turns, completed items, pending
interactive requests, controller changes, and lifecycle failures are marked as
immediate-flush transitions for the frontend normalization layer.

## Snapshots and sequence semantics

`Snapshot` is an immutable-by-value view assembled deterministically from the
canonical maps and explicit order vectors. Two snapshots of unchanged state
compare equal. It contains the current backend revision, lifecycle and error,
diagnostic summary, ordered threads, turns and items, accumulated bounded
content, pending request summaries, controller, connected sessions,
thread-list completeness, and sequence-exhaustion state.

Snapshot creation never exposes pointers, callbacks, App Server client request
IDs, server-request occurrence tokens, authentication access tokens, or
user-input answers. Raw typed envelopes are not copied into normal snapshot
data. Known pending requests expose only the fields needed to render and answer
them. Unknown request details are explicitly labelled and bounded to 64 KiB.

The backend sequence starts at zero and increases once for each visible
backend-domain transition. It is an in-process revision, not the Codex
Frontend Protocol replay sequence. The frontend layer allocates its own
sequence only after normalization and coalescing and owns the bounded replay
journal. `Snapshot::replayRange` is therefore left unset by `BackendCore` and
may be populated by a higher layer that owns such a journal. Exhaustion fails
explicitly by marking the backend failed; the number never wraps.

## Hydration and lifecycle recovery

Successful operations hydrate state before their command completion is
delivered:

- thread start and resume upsert the returned thread summary;
- thread list merges the returned page by ID and retains its cursors;
- thread read upserts the returned thread, turns, and items, marking it fully
  loaded when turns were explicitly requested;
- turn start upserts the returned turn; and
- turn interrupt completes from its typed result while later authoritative
  events determine terminal turn state.

On each `Ready` generation, the backend submits at most one initial thread-list
request. Its default limit is 50 threads. It never walks subsequent cursors
automatically, so startup cannot load unbounded history. Set
`initialThreadListLimit` to zero to disable this refresh or to another bounded
value for an application-specific policy.

`stop()` is explicit; disconnecting a frontend does not call it. A later
`start()` reuses the owned client for explicit recovery. Every accepted typed
operation captures the current backend generation. Stop, connection failure,
and restart invalidate that generation, complete still-attached operations as
cancelled, and prevent late old-generation typed completions from mutating new
state. Backend and session callbacks also use weak lifetime guards, so queued
work cannot enter a destroyed backend.

## Sessions, observers, and callback ordering

`FrontendSession` is a move-only RAII handle. Destruction calls `close()` and
detaches the session. Every new session begins as an observer; opening a
session never grants controller authority implicitly.

```cpp
auto session = backend.openSession({
    .onEvents = [](const std::vector<SequencedBackendEvent>& events) {
        // Events are ordered backend-domain transitions.
    },
    .onSnapshot = [](const Snapshot& snapshot) {},
    .onCommandCompleted = [](const CommandCompletion& completion) {},
    .onClosed = [](const std::string& reason) {}
});

session.submit("client-1", ControllerAcquire{});
session.requestSnapshot();
```

Outbound session callbacks are scheduled through the SNode.C event loop by
default, are ordered, and never run inline as command completion callbacks.
One scheduled-drain guard collects a burst of backend events rather than
scheduling one callback per token delta. Consecutive events are delivered in
batches of at most 512 by default. Public callback exceptions are caught at the
boundary. A callback may submit another command, close its session, stop the
backend, destroy an adapter, or throw without unwinding into the event loop or
invalidating another session.

`BackendObserverSubscription` provides the same ordered, batched event feed to
one transport-neutral normalization adapter without creating controller state.
It is also move-only RAII. If its bounded queue overflows, intermediate backend
events are dropped and `onResynchronize` receives a current snapshot. This is
safe because the frontend layer derives normalized events from already reduced
state.

The default per-session queue bound is 4,096 entries and 8 MiB. The observer
queue has the same defaults. Both entry and approximate serialized-byte bounds
are configurable. A session that exceeds either limit is closed and all its
queued data is released; its controller role is released if necessary. That
does not stop `BackendCore`, the App Server, another observer, or the
controller. Observer-subscription overflow uses snapshot resynchronization
instead of allowing unbounded growth.

Tests can replace the default `core::EventReceiver::atNextTick()` scheduler via
`BackendCoreOptions::scheduler`. A deterministic scheduler can append callbacks
to a deque and advance one event-loop tick explicitly. Production code should
retain event-loop scheduling rather than introduce threads or blocking waits.

## Controller policy and commands

There is at most one controller and any number of observers. The backend
commands are C++ value variants independent of JSON:

- `ControllerAcquire`, `ControllerRelease`, `SnapshotGet`, and `ReplayAfter`;
- `ThreadStart`, `ThreadResume`, `ThreadList`, and `ThreadRead`;
- `TurnStart` and `TurnInterrupt`;
- `ApprovalRespond`, `UserInputRespond`, and `AuthenticationRespond`; and
- `UnknownRequestRespondRaw` and `UnknownRequestReject`.

Observers may obtain snapshots/replay coordination and issue thread list/read
operations. All other App Server operations require the controller. Acquisition
succeeds when there is no controller or idempotently for the same session. A
different session receives `conflict`; release succeeds only for the current
controller.

When the controller disconnects, ownership is released and remaining sessions
are notified. The App Server keeps running. Pending requests remain pending,
are neither approved nor rejected, and can be answered by a later controller.

Each submitted command has a nonempty, session-local request ID. A duplicate ID
is rejected while its earlier command is pending. IDs in different sessions do
not conflict. An accepted command produces exactly one asynchronous
`CommandCompletion` unless that session closes first. Closing a session
suppresses later completions but does not cancel the App Server operation; a
successful late result can still hydrate shared state.

The stable command error categories are:

```text
permission_denied
invalid_command
not_found
conflict
local_submission_failure
typed_decoding_failure
remote_app_server_error
cancelled
backend_unavailable
```

They preserve an optional remote code and bounded details without reducing the
contract to `errno`.

## Pending request ownership

Each incoming typed server request receives a monotonically increasing
`PendingRequestId`. Canonical state retains the exact typed request, including
its `ServerRequestToken`, so the existing typed layer continues to protect
against App Server JSON-RPC ID reuse. The token is never a frontend identifier
and never appears in a snapshot.

A pending request is removed only after the typed `respond`, `respondRaw`, or
`reject` call successfully enqueues the response. Validation, encoding, or
enqueue failure returns `local_submission_failure` and retains the request for
retry. App Server stop, failure, or other connection invalidation clears all
pending requests because their typed occurrence ownership is no longer valid.

Controller disconnect is not connection invalidation. It does not remove,
approve, decline, reject, or otherwise answer any request. BackendCore contains
no automatic approval or rejection policy.
