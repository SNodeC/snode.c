# Logging lifecycle Phase 3: protocol, session, request, and turn consistency

## Scope and frozen contracts

This phase audits production logging under `src/` and normalizes lifecycle records above the transport layer. It preserves the semantic logging contract, Phase 1 framework-context ownership, Phase 2 listener/connection-attempt/transport ownership, coordinated stream shutdown, and the TLS state machine. In particular, this work does not rename, reorder, duplicate, or change the severity of `listener started/stopped`, connection-attempt outcomes, `transport connected/ready/disconnected`, or framework-context `attached/detached` records.

The audit reviewed the implementations and state transitions, not only message text. The ownership decisions below were checked against HTTP parser and request queues, EventSource ready-state and retry configuration, WebSocket upgraded-context attachment, MQTT CONNECT/CONNACK and broker session retention, the Codex pending-request maps and typed reducer, and the MariaDB asynchronous command queue.

## Reviewed audit methodology and accounting

The initial case-insensitive production search covered the requested lifecycle and protocol terms (`connected`, `disconnected`, `started`, `stopped`, `established`, `ended`, `completed`, `failed`, `aborted`, `cancelled`, `timeout`, `request`, `response`, `session`, `protocol`, `upgrade`, `websocket`, `subprotocol`, `mqtt`, `connect`, `connack`, `disconnect`, `eventsource`, `reconnect`, `thread`, `turn`, `notification`, `database`, `query`, and `command`). It produced 998 candidate source lines. Each candidate was then inspected in its containing function and classified as follows:

| Classification | Count |
| --- | ---: |
| `LIFECYCLE_START` | 7 |
| `LIFECYCLE_END` | 6 |
| `TERMINAL_ALTERNATIVE` | 7 |
| `PROGRESS` | 46 |
| `DIAGNOSTIC` | 188 |
| `POLICY_DECISION` | 13 |
| `SUMMARY` | 9 |
| `PHASE1_CONTEXT` | 18 |
| `PHASE2_TRANSPORT` | 32 |
| `APPLICATION_USER_FACING` | 119 |
| `NOT_A_LOG_RECORD` | 553 |
| **Total** | **998** |

The 20 pre-change Phase 3 lifecycle candidates were expanded or replaced at their true ownership points. The final production tree contains 48 canonical Phase 3 emission expressions; 39 emission expressions were added or replaced by this phase. Detailed parser, frame, packet, callback, queue, persistence, and failure-cause records remain diagnostics or progress. There are zero unresolved relevant Phase 3 production lifecycle sites.

This accounting is a reviewed classification, not an unfiltered search dump. Matches in comments, symbols, callback names, packet fields, application text, and non-logging state checks are included only in `NOT_A_LOG_RECORD`.

## Canonical vocabulary and severity

The implemented vocabulary is:

- HTTP: `request started`, `request completed`, `request failed`, `request aborted`.
- EventSource: `event source started`, `event stream established`, `event source reconnect scheduled`, `event source reconnect dispatched`, `event source reconnect cancelled`, `event source stopped`.
- WebSocket: `websocket established`, `websocket ended`, `websocket failed`.
- selected WebSocket subprotocol: `subprotocol started`, `subprotocol stopped`.
- MQTT: `mqtt protocol started`, `mqtt protocol stopped`, `mqtt session established`, `mqtt session resumed`, `mqtt session rejected`, `mqtt session ended`.
- Codex App Server: `app-server session started`, `app-server session stopped`, `app-server session start failed`; `request started`, `request completed`, `request failed`, `request cancelled`; `thread created`; `turn started`, `turn completed`, `turn failed`, `turn cancelled`.
- MariaDB: `database session established`, `database session ended`; `database request started`, `database request completed`, `database request failed`, `database request cancelled`.

Info is used for long-lived protocol, session, WebSocket, subprotocol, EventSource, thread-creation, and database-session boundaries. Debug is used for bounded request/command/turn outcomes, rejected sessions, failed WebSocket establishment, App Server session-start failure, and EventSource reconnect policy. Trace remains the level for WebSocket close-handshake progress, MQTT-over-WebSocket adapter attachment, parser/frame/packet progress, and state-machine transitions. Warn and Error records retain detailed abnormal causes and remain separate from the canonical terminal record.

No HTTP `request timed out` or `request cancelled` record is fabricated: the HTTP layer has no distinct deadline/cancellation callback. A connection teardown while a started request is active is therefore `request aborted`; Phase 2 still owns any lower-level timeout diagnostic. Likewise, Codex emits no `request timed out` because its client currently implements no request deadline.

## Identity ownership

| Owner | Semantic logger and correlation |
| --- | --- |
| HTTP server request | connection-bound HTTP framework/server-role logger plus a private monotonically increasing request correlation within that socket context |
| HTTP client request | connection-bound HTTP framework/client-role logger plus the existing `MasterRequest::count` |
| EventSource | `web.http.client.eventsource`; endpoint origin and path identify the protocol instance before a transport connection exists |
| WebSocket/subprotocol | connection-bound `web.websocket.subprotocol` scope; selected subprotocol is present in the lifecycle message |
| MQTT protocol/session | connection-bound `iot.mqtt` scope; client ID identifies the session and the connection scope carries instance and connection IDs |
| Codex session/request | the existing `ai.openai.codex` scope; JSON-RPC ID and method correlate request start and terminal records |
| Codex thread/turn | `ai.openai.codex.backend`; thread and turn IDs are carried by every applicable record |
| MariaDB | existing MariaDB semantic scope; connection name and command description identify the session/operation |

No public request-ID API or second identity framework was added. HTTP server correlation is private and socket-local because pipelined requests otherwise share all available semantic scope fields.

## Exactly-once strategy

Exactly-once behavior follows existing ownership data structures:

- The HTTP server stores terminal state beside each pending request. Parser failure marks the request terminal, successful response completion removes it, and context teardown aborts only remaining nonterminal entries. A parser that fails before message-begin produces no fabricated start/terminal pair. Streaming source failure prevents a later completion.
- The HTTP client treats membership in its private started-request set as terminal ownership. Response delivery, parse failure, delivery failure, and disconnect erase that membership; later cleanup cannot emit another outcome.
- EventSource start/stop and reconnect-pending bits live in the existing shared ready-state owner. `close()` is idempotent, disables Phase 2 reconnect/retry before shutdown, cancels an outstanding SSE retry record, and prevents disconnect cleanup from scheduling another retry or stop.
- WebSocket establishment/end are emitted only by upgraded-context attach/detach. Close-frame progress does not terminate the lifecycle, so the later transport teardown cannot duplicate the WebSocket end. Selected subprotocol start/stop uses the same attach/detach convergence point.
- MQTT protocol/session flags live in the `Mqtt` protocol engine. Session establishment is called only after accepted CONNACK or broker session creation; resume is called only in the retained-session renewal branch or for a client CONNACK with `sessionPresent`. Rejection prevents establishment, and disconnect clears the active session before stopping the protocol. Broker retention remains a persistence decision, not a second end.
- Codex client requests are terminal exactly when the existing pending map erases them or invalidation clears accepted requests. Notifications never enter that map. Server-originated requests follow the same rule. Intentional stop/destruction cancels pending requests; protocol, process, or transport failure fails them. Session stop is guarded by successful initialization. Turn terminal logging uses the existing reducer-owned `TurnState`, preventing completion after a failure or cancellation.
- MariaDB command state is attached to the existing `currentCommand`. The error path marks it failed before the shared completion cleanup; destruction terminates only a command that actually started. Queued, unstarted commands do not receive fabricated terminals.

These are local flags on lifecycle owners, not a general lifecycle registry or alternate ownership graph.

## Per-module decisions

### HTTP and Express

HTTP request start is parser message-begin on the server and successful dispatch initiation on the client. A malformed request fails only if message-begin occurred. Response parser failure and request delivery failure are `failed`; connection loss while active is `aborted`; successful response/upgrade handoff is `completed`. Server streaming completion remains tied to the existing source EOF/send-completion point. Express routing and middleware logs remain Trace progress and do not create another request lifecycle.

### EventSource

The EventSource object owns the long-lived protocol. Stream establishment may repeat after an SSE retry. `event source reconnect ...` describes WHATWG/EventSource retry policy only; generic socket retry/reconnect remains Phase 2. `Last-Event-ID`, parsed `retry`, ready states, reconnect timing, and explicit close behavior are unchanged.

### WebSocket and subprotocols

HTTP upgrade completion and WebSocket establishment are separate boundaries. Upgrade validation failure emits `websocket failed` without establishment. A normal close handshake and abnormal transport loss both converge on one `websocket ended`; the detailed cause remains in existing diagnostics. Close-frame progress was reduced to Trace. There is no separate `subprotocol start failed` record because no subprotocol object/callback failure state exists: factory/plugin selection failures precede subprotocol ownership and are retained as diagnostics while WebSocket initialization fails.

### MQTT

Physical transport connection is not an MQTT session. Protocol start occurs when the MQTT context attaches. Session establishment follows accepted CONNECT/CONNACK only. Server-side `resumed` is limited to renewal of an actual retained broker session; client-side `resumed` requires CONNACK `sessionPresent`. CONNECT rejection has no fabricated establishment/end. Explicit DISCONNECT, keep-alive close, context detach, and transport loss all converge on the same guarded protocol/session stop. The MQTT-over-WebSocket adapter logs only Trace progress and relies on the WebSocket, subprotocol, MQTT, and Phase 2 owners for canonical records.

### Codex App Server

The App Server session begins only after successful initialize response and initialized-notification admission. Initialization failure records only `app-server session start failed`; an established session stops once during invalidation. Accepted client and server JSON-RPC requests carry their IDs from start to their one map-removal terminal. Notifications remain notifications. The typed backend reducer records real thread-start and turn events; no `thread closed` is invented because the protocol exposes no thread-close event. A turn terminal bit suppresses completion after failure/cancellation.

### MariaDB

A database session is established only after successful asynchronous connect completion and ends only if that establishment occurred. Existing asynchronous command logging is normalized as database request lifecycle. Synchronous statements, which did not previously have a lifecycle surface, remain uninstrumented to avoid per-statement noise. No PostgreSQL behavior was added.

### Applications

The 119 application-facing candidates were retained. Examples include sample `OnRequest`, upgrade-status, endpoint-connected, and MariaDB demo messages. They use application-facing log scopes or are explicit sample status output, not canonical framework truth. Reference WebSocket echo `connected/disconnected` callback messages and database sample `MySQL connected/disconnected` messages remain ordinary application output.

## Deliberately retained diagnostics and deferrals

The 188 diagnostics include HTTP parse/response details, upgrade bootstrap/plugin selection, EventSource callbacks, WebSocket frame and close-handshake detail, MQTT packet and broker persistence detail, Codex state transitions and public-callback failures, and MariaDB driver/descriptor errors. Their existing data and user value are retained; where high-frequency protocol progress was lifecycle-like, it is Trace.

No relevant Phase 3 lifecycle record is deferred. The final closure audit may reassess general diagnostic wording, application copy, and unrelated whole-tree severity consistency, but it has no unresolved Phase 3 ownership work.

## Compatibility assessment

Protocol behavior, parsing, routing, middleware order, callbacks, public/protected function signatures, HTTP/1.x behavior, WebSocket framing and close behavior, MQTT QoS/persistence/reconnect behavior, Codex wire schemas/reducers, database execution, Phase 1/2 transport behavior, TLS, logger schema/backend/formatting/filtering, SOVERSION, and book content are unchanged.

Source compatibility is preserved. Binary compatibility is **not claimed**: private state was added to installed/polymorphic HTTP, EventSource template, MQTT, MariaDB, and Codex backend types, so consumers must rebuild. No vtable entry or public/protected API was added, removed, or reordered, and SOVERSION is intentionally unchanged for this in-tree lifecycle normalization.

## Validation surface and results

The final GCC build used `cmake --build build -j2` and completed all targets successfully. The focused structured lifecycle matrix used:

```text
ctest --test-dir build --output-on-failure -R '^(Phase3LifecyclePolicyTest|Phase3CodexLifecycleTest|Phase3MqttLifecycleTest|InetHttpServerClientGetRoundTripTest|InetHttpServerMalformedRequestBehaviorTest|InetHttpClientPrematureServerCloseTest|InetWebSocketServerClientCloseHandshakeTest|InetWebSocketUnexpectedCloseLifecycleTest|InetSseEventSourceClientCloseAfterEventTest|InetSseEventSourceReconnectLifecycleTest)$'
```

Result: 10/10 passed. The tests use structured semantic JSON for HTTP request success, malformed-input failure, transport-loss abort, WebSocket upgrade/normal end/abnormal end/subprotocol ownership, EventSource establishment/retry/explicit close, MQTT established/resumed/rejected/ended exactly-once behavior, and Codex thread/turn terminal exclusion. The Phase 3 source-policy test covers notification separation, obsolete MQTT wording, MQTT-over-WebSocket duplication, absence of fabricated thread close, and database outcome consistency.

The final combined logging, lifecycle, TLS, and shutdown label matrix passed 76/76. The final complete suite passed 249 tests with zero failures out of 250. `CodexTypedAppServerIntegrationTest` was skipped and is not counted as passing because `SNODEC_RUN_CODEX_TYPED_INTEGRATION=1` was not set; that opt-in may use configured credentials and quota.

Clang 21 compiled every changed production library and the focused targets with `CHECK_INCLUDES=OFF` and only these compatibility demotions for warnings introduced by newer Clang diagnostics:

```text
-Wno-error=tautological-type-limit-compare -Wno-error=nrvo -Wno-error=implicit-int-conversion
```

The Clang focused matrix passed 10/10. A strict Clang 21 build remains gated by three pre-existing `-Werror` diagnostics outside Phase 3 changes: a tautological type-limit comparison in `src/web/http/decoder/Chunked.cpp`, `-Wnrvo` in `src/ai/openai/codex/detail/ItemDecoder.cpp`, and implicit integer conversion in `src/web/websocket/Receiver.cpp`.

The AddressSanitizer/LeakSanitizer configuration used `SNODEC_ENABLE_ASAN=ON`. Seven non-WebSocket lifecycle tests passed 7/7 with leak detection enabled, covering MQTT, Codex, HTTP, and EventSource. The two WebSocket lifecycle tests passed 2/2 with address checking enabled and leak reporting disabled. With leak reporting enabled, both expose the existing process-lifetime WebSocket upgrade/subprotocol factory registries; no invalid access is reported, but those two runs are not called leak-clean.

There is no configured MariaDB service or credentials in this environment, so no live database session/query integration was run; the MariaDB library compiled under GCC and Clang and its lifecycle invariants are covered by the source-policy test. MQTT-over-WebSocket libraries compiled under both compilers and their ownership boundary is source-policy checked, but the repository has no broker-backed MQTT-over-WebSocket lifecycle integration fixture. These are validation limitations, not passing integration results.
