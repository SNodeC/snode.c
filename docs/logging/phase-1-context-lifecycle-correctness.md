# Phase 1 context lifecycle correctness

SNode.C runtime lifecycle logging is being normalized in three canonical phases:

1. Context lifecycle correctness.
2. Transport and attempt lifecycle.
3. Protocol, request, and session lifecycle.

This report documents Phase 1 only. The book is intentionally not updated until the three-phase programme is complete.

## Authoritative vocabulary

The authoritative vocabulary separates framework context lifetime from transport/socket lifetime:

- Transport/socket lifetime: `connected`, `ready`, `disconnected`.
- Framework context lifetime: `attached`, `detached`.
- Protocol engine lifetime: `started`, `stopped`.
- Protocol session lifetime: `established`, `ended`.
- Request/response lifetime: `started`, `completed`, `failed`, `aborted`.
- Listener/connect-attempt lifetime: `started`, `succeeded`, `failed`, `timed out`, `cancelled`, `stopped`.

Phase 1 changes only framework-context wording, detach-reason availability, lifecycle-event ordering, matching severities, redundant context-level signal messages, directly affected tests, and this implementation report.

## Context ordering and detach reason

Generic framework context records use Debug-level `Context attached` and `Context detached ...` messages. Attach is emitted before the derived protocol context is notified, and detach remains derived-before-generic:

```text
Context attached
HTTP: context attached
...
HTTP: context detached for context switch
Context detached for context switch
```

This nesting distinguishes context attachment/detachment from transport connection/disconnection. A context switch replaces a framework context over a still-live transport, so it must not claim that a socket or connection disconnected.

The virtual callback names `onConnected()` and `onDisconnected()` remain unchanged to preserve the existing callback contract. Their signatures are not changed; only the wording emitted by their implementations is normalized.

`SocketContext::DetachReason` moved from private to protected visibility, and `getDetachReason() const noexcept` is now a protected accessor. `SocketContext::detach()` stores the reason before invoking `onDisconnected()`, allowing derived contexts to inspect whether the detach is for `ContextSwitch` or `ConnectionClose` while the callback executes. The stored value only needs to remain meaningful for that callback because the context is deleted after detach completes.

## Phase 1 wording and severity

All context attach/detach records introduced or changed in this phase use matching Debug severity.

HTTP client and server contexts now emit:

- `HTTP: context attached`
- `HTTP: context detached for context switch`
- `HTTP: context detached for connection close`

WebSocket upgraded contexts now emit:

- `WebSocket: context attached with subprotocol '<name>'`
- `WebSocket: context detached with subprotocol '<name>' for context switch`
- `WebSocket: context detached with subprotocol '<name>' for connection close`

The canonical TCP echo and TLS legacy examples now emit reason-aware context lifecycle messages through inherited `SocketContext::log()`, so their records carry application-origin, context-bound instance and connection identity:

- `Echo: context attached`
- `Echo: context detached for context switch`
- `Echo: context detached for connection close`
- `TLS legacy: context attached`
- `TLS legacy: context detached for context switch`
- `TLS legacy: context detached for connection close`

## Signal-message deduplication

The EventLoop owns the canonical process-level received-signal record. Phase 1 removes duplicate `HTTP: Received signal <n>` messages from HTTP client and server contexts. The apparently trivial HTTP `onSignal()` overrides remain in place and continue returning `true`, preserving inherited shutdown behaviour and callback participation.

## Source-policy and migration tests

`SocketContextDetachPolicyTest` is a source-policy test. It verifies protected detach-reason API fragments, stored-reason assignment before callback invocation, generic context wording, preserved context-switch tenure diagnostics, Echo/TLS example implementation rules, and the real production switch block ordering in `SocketConnection.hpp`:

```text
socketContext->detach(SocketContext::DetachReason::ContextSwitch);
socketContext = newSocketContext;
newSocketContext = nullptr;
socketContext->attach();
```

The same isolated switch block is checked to contain no `onDisconnect` callback. This is a source-policy invariant for the production switch branch, not runtime transport-callback proof.

`WebSocketMigration07cTest` remains a semantic logger and migration compatibility test. Its representative messages verify formatting and scope preservation; they are not described as runtime lifecycle execution.

Retained affected-string matches are classified as historical forbidden-pattern guards, migration fixtures, active subprotocol-object messages deferred to Phase 3, HTTP upgrade/subprotocol diagnostics outside this context-lifecycle phase, or WebSocket factory/subprotocol naming code. The high-frequency severity test intentionally keeps historical forbidden HTTP fragments as negative regression guards.

## Temporary Info-level visibility

Phase 1 intentionally removes misleading Info-level context-disconnect records and places context lifecycle at Debug. Until Phase 2 and Phase 3 add their respective transport and protocol/session lifecycle records, ordinary HTTP or WebSocket runs may show little context/protocol lifecycle information at the default Info threshold beyond existing endpoint termination summaries. This is intentional: correct absence at Info is preferable to false disconnect information.

## Explicit deferrals

Phase 2 is deferred: no role-aware connection-scoped endpoint logger, transport connected/ready/disconnected cleanup, callback renaming, connect-attempt vocabulary, attempt correlation, retry/reconnect vocabulary, listener lifecycle changes, endpoint activation records, or removal of the generic `SocketConnection::unobservedEvent()` `disconnected` record is included here.

Phase 3 is deferred: no MQTT protocol started/stopped records, protocol-session established/ended records, persistent-session lifecycle records, queued-session replay lifecycle, MQTT-over-WebSocket signal cleanup, WebSocket subprotocol started/stopped normalization, HTTP request/response aborted semantics, or MQTT exactly-once session-state changes are included here.
