# Generic Codex App Server protocol engine

`AppServerClient::raw()` exposes the transport-independent App Server protocol
after initialization reaches `State::Ready`. It intentionally models protocol
ownership and correlation without assigning thread, turn, item, approval,
authentication, or model semantics to JSON payloads.

The public value type is `ai::openai::codex::Json`, an alias of
`nlohmann::json`. The generic codec retains every complete decoded envelope.
Public notifications, server requests, unknown messages, and the initialization
result expose their retained raw JSON in addition to their common fields.

## Caller-owned requests

`RawProtocol::request()` accepts a nonempty method, arbitrary JSON parameters,
and a response handler only while the client is `Ready`. `initialize` and
`initialized` are reserved for the internal handshake and cannot be submitted
through the raw facade.

Each accepted request receives a `ClientRequestId`. Client IDs are signed
64-bit integers generated exclusively by SNode.C. The allocator is shared with
the internal initialization request, increases monotonically, detects
exhaustion, and is not reset when the same client is stopped and restarted.
Client IDs are never constructed from server requests.

The client registers ownership before enqueueing the encoded document. If
encoding or transport enqueue fails, the registry entry is rolled back and the
submission reports a local error. An accepted request completes at most once,
including when responses arrive out of order or a test transport injects a
response synchronously from `send()`.

Successful results retain any JSON value, including objects, arrays, strings,
numbers, Booleans, and `null`. App Server error responses are reported as
`Response::Kind::RemoteError` with their optional JSON `data`. Local lifecycle,
submission, transport, and cancellation failures are not converted into remote
protocol errors. `ProtocolError::code` is signed 64-bit, matching the installed
App Server schema rather than narrowing valid remote codes to the platform
`int` width.

The client holds at most 4,096 caller-owned pending requests. A request beyond
that limit is rejected with a capacity error and creates no pending entry.

## Notifications

`RawProtocol::notify()` sends an arbitrary non-reserved method and arbitrary
JSON parameters while the connection is ready. Incoming notifications are
delivered through `setOnNotification()` without method classification. Current
and future method names therefore follow the same path, and their method,
parameters, complete raw object, and wire order are preserved.

The raw layer does not build state snapshots or reduce streamed events. A later
typed layer may interpret notifications, but it does not change their ownership
in this generic engine.

## Server-initiated requests

An incoming envelope with both a method and an ID becomes a neutral
`ServerRequest`. `ServerRequestId` preserves the exact signed-integer or string
representation supplied by the App Server. It is a different strong type from
`ClientRequestId`, preventing a caller from accidentally answering the wrong
side of the protocol.

The request remains pending until the caller explicitly uses `respond()` or
`reject()`. A successful response encodes a result; a rejection encodes a
`ProtocolError`, including optional JSON data. The request is removed only after
the transport accepts the encoded response. Encoding or enqueue failure retains
it for retry, while a second answer after successful enqueue is rejected.

Disconnect, failure, stop, or destruction never implies acceptance or approval.
Pending server requests are simply cleared when their connection ends. A
duplicate ID that is still pending is ownership-ambiguous and causes a protocol
failure rather than replacing the original request.

The client retains at most 1,024 pending server requests. Exceeding that limit
is a protocol failure because accepting another request would lose safe response
ownership.

## Initialization ownership

The client exclusively owns this sequence:

```text
initialize request
-> initialize response
-> initialized notification
-> Ready
```

Initialization uses the ordinary encoder, decoder, and client request-ID
allocator, but it is never exposed as a caller request and does not invoke a raw
response handler. `getInitializeResult()` returns the cached typed platform
fields together with the complete raw initialization result.

Structurally valid non-initialization messages received during the handshake are
retained in a 1,024-message pre-ready queue. After the transition to `Ready`,
they are dispatched asynchronously through the normal response, notification,
server-request, or unknown-message path in original wire order. Overflow is a
protocol failure rather than silent loss.

Input delivered reentrantly by a transport while `send()` is still active is
held in a separate 4,096-message deferred-input queue so public callbacks remain
asynchronous and submission rollback stays transactional. During initialization,
the stricter 1,024-message pre-ready limit still applies to the combined queued
input. Either overflow fails the connection as a protocol error.

## Unknown and malformed messages

A structurally valid response that is not owned by a current client request is
nonfatal and is delivered through `setOnUnknownMessage()`. This includes unknown
integer response IDs and string response IDs, because SNode.C client IDs are
always integers. The callback receives the complete raw envelope and a reason.

Malformed JSON, invalid or ambiguous envelopes, invalid ID types, responses
containing both `result` and `error`, duplicate pending server IDs, and bounded
pre-ready overflow use the existing fatal protocol-failure lifecycle. Unknown
method names and unknown object fields are not malformed and remain preserved.
There is deliberately no public `sendRawObject()` escape hatch that could bypass
reserved methods, correlation, or ownership.

## Cancellation and connection generations

Each active transport connection has a monotonically increasing generation.
Pending client requests record that generation and their deterministic
submission sequence. When the active connection stops, fails, exits, or is
replaced, every still-pending caller request completes once as
`Response::Kind::Cancelled`, in submission order, with a local cancellation
error. Pending server requests and pre-ready messages are cleared without being
answered.

Deferred protocol work is generation-checked. Work retained from an old
connection cannot dispatch into a restarted connection, and stale responses
cannot match new requests. Installed notification, server-request, and unknown
handlers belong to the public client object and remain installed across the
explicit recovery sequence:

```text
Failed -> stop() -> Stopping -> Stopped -> start() -> Ready
```

There is no automatic restart policy.

## Callback context and reentrancy

All public protocol callbacks execute asynchronously in the SNode.C event-loop
context. None executes inline from `request()`, `notify()`, `respond()`, or
`reject()`, even when a fake transport injects input synchronously during
`send()`.

Incoming callback scheduling preserves wire order, and cancellation callbacks
preserve request submission order. A callback may reenter the raw facade, call
`stop()`, or destroy the client. Lifetime tokens and generation checks prevent
deferred work from accessing destroyed or replaced client state. Exceptions
escaping user protocol callbacks are caught and logged at the callback boundary
so they cannot unwind through the event loop.

## Boundary for future typed APIs

The raw engine owns envelope validation, request IDs, correlation, enqueue
results, callback delivery, cancellation, and connection generations. It does
not interpret method-specific parameters or results, classify server requests
as approvals, choose models, persist backend state, or own a UI controller.
Future typed APIs should build on this facade rather than duplicate its
transport, framing, initialization, or request-ownership machinery.
