# Logging lifecycle Phase 3: protocol, session, request, and turn consistency

## Scope and frozen contracts

This phase audits production logging under `src/` and normalizes lifecycle records above the transport layer. It is primarily a mechanical semantic-logging normalization: existing lifecycle messages receive canonical vocabulary and severity and are emitted through existing semantic scopes at authoritative protocol transitions. It does not redesign protocol state machines. It preserves the semantic logging contract, Phase 1 framework-context ownership, Phase 2 listener/connection-attempt/transport ownership, coordinated stream shutdown, and the TLS state machine. In particular, this work does not rename, reorder, duplicate, or change the severity of `listener started/stopped`, connection-attempt outcomes, `transport connected/ready/disconnected`, or framework-context `attached/detached` records.

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

### Identity-prefix closure

Architectural identity is carried by semantic fields rather than repeated in message text. The MQTT closure reviewed 184 production message sites: 51 connection-bound duplicates, 44 static sites that could use an existing connection, 11 pre-bound or destruction-order fallbacks, five canonical/domain-identity records, and 73 process-wide component diagnostics. It mechanically changed 178 sites. Connection-bound common, client-role, server-role, and MQTT-over-WebSocket records now begin with the event or diagnostic payload; process-wide broker diagnostics likewise omit the redundant component label. MQTT client IDs, topics, packet identifiers and types, QoS, DUP/retain state, session-present state, protocol version, keep-alive values, store paths, and error detail remain domain payload.

The 11 retained `PREBOUND_FALLBACK` message sites are narrowly limited to seven client session-store diagnostics (constructor load, corrupt/empty data, restore, read failure, and absent filename; destructor write failure and absent filename) and four server session-release diagnostics reached from MQTT object destruction (delete/retain decisions and their session-pointer detail). They retain only the configured connection display name because no safe bound socket scope exists at those points; the client/server component label is removed. Live DISCONNECT uses the connection-bound server scope. All common-engine records were verified to execute after MQTT context attachment.

The secondary Phase 3 closure changed 78 verified duplicate component prefixes: 40 HTTP/EventSource sites, 29 WebSocket/subprotocol sites, four Codex App Server diagnostics, and five MariaDB diagnostics. Frozen HTTP/WebSocket context lifecycle text, Phase 2 transport/process records, protocol-version and subprotocol payload, and application-facing output were excluded. This identity cleanup changes no lifecycle outcome, protocol behavior, callback, timer, persistence decision, logger schema or formatter. It adds no public/protected API, class state, layout, or vtable entry.

A final connection-scope audit reviewed 243 identity-bearing production record sites across the Phase 3 surface. At the reviewed head, 95 connection-bound records were already compliant (51 formerly `BOUND_DUPLICATE` and 44 formerly `STATIC_BUT_BINDABLE` MQTT records), with zero residual `BOUND_DUPLICATE`; the remaining classification was 38 `STATIC_BUT_BINDABLE`, 12 `PREBOUND_FALLBACK` call sites, 14 `DOMAIN_IDENTITY`, 74 `GLOBAL_COMPONENT_DIAGNOSTIC`, and 10 `FROZEN_PHASE1_PHASE2`. The correction changed all 38 bindable emissions: 13 connected HTTP server-upgrade records in `Response.cpp`, 17 HTTP client upgrade/SSE-response records in `Request.cpp`, and eight EventSource connection/request/stream diagnostics. Thirty removed a textual identity prefix; eight adjacent protocol/subprotocol detail emissions had no textual prefix but were moved with their operation to the same bound logger. No MQTT, WebSocket, Codex, MariaDB, or other non-HTTP production file required another change.

Thirteen fallback paths remain: the 11 MQTT constructor/destructor sites above, the HTTP server's unexpected-disconnect branch after its response has no SocketContext, and the EventSource `OnRequestEnd` branch reached when its weak owner is gone after HTTP-context disconnect. Only 12 retain textual connection identity: the 11 MQTT display-name fallbacks and that EventSource request fallback; the disconnected HTTP server record contains no connection text. Normal EventSource request end is connection-bound. HTTP methods, URLs and versions, selected/requested upgrade protocols, MQTT client IDs, topics, packet IDs, QoS, session values, database session identity, Codex request/thread/turn IDs, and error detail remain domain payload. This final correction changes only logger selection and message prefixes; it adds no logger abstraction or overload, public/protected API, persistent logger state, lifecycle flag, class-layout/vtable change, protocol behavior, or lifecycle outcome.

## Exactly-once strategy

Duplicate suppression uses existing owner state and narrow private flags only where necessary. These guarantees apply to the actual framework lifecycle; they do not promise arbitrary event-replay or event-sourcing idempotency.

- The HTTP server stores terminal state beside each pending request. Parser failure marks the request terminal, successful response completion removes it, and context teardown aborts only remaining nonterminal entries. A parser that fails before message-begin produces no fabricated start/terminal pair. Streaming source failure prevents a later completion.
- The HTTP client treats membership in its private started-request set as terminal ownership. Response delivery, parse failure, delivery failure, and disconnect erase that membership; later cleanup cannot emit another outcome.
- EventSource start/stop and reconnect-pending bits live in private per-instance shared configuration. `close()` is idempotent, disables Phase 2 reconnect/retry before shutdown, cancels an outstanding SSE retry record, and prevents disconnect cleanup from scheduling another retry or stop.
- WebSocket establishment/end are emitted only by upgraded-context attach/detach. Close-frame progress does not terminate the lifecycle, so the later transport teardown cannot duplicate the WebSocket end. Selected subprotocol start/stop uses the same attach/detach convergence point.
- MQTT protocol/session flags live in the `Mqtt` protocol engine. Private helpers, friend-limited to the existing concrete client and server implementations, are invoked only after accepted CONNACK or broker session creation. Resume is limited to the retained-session renewal branch or a client CONNACK with `sessionPresent`. Rejection prevents establishment, and disconnect clears the active session before stopping the protocol. Broker retention remains a persistence decision, not a second end.
- Codex client requests are terminal when the existing pending map erases them or connection invalidation clears accepted requests. Notifications never enter that map, and server-originated requests follow the same ownership rule. Explicit stop and unusable-connection invalidation cancel pending local requests, matching the existing callback result; pending server-originated requests retain the triggering stop/failure outcome. A separate Error diagnostic retains the underlying protocol/process/transport cause. Session stop is guarded by successful initialization. `typed::ThreadStarted` and `typed::TurnStarted` are the authoritative starts. Turn terminal logging inspects the existing reducer-owned `TurnState::terminal` before applying a terminal event, preventing completion after failure or cancellation.
- MariaDB command state is attached to the existing `currentCommand`. The error path marks it failed before the shared completion cleanup; destruction terminates only a command that actually started. Queued, unstarted commands do not receive fabricated terminals.

These are existing domain state and local private flags on lifecycle owners, not a general lifecycle registry or alternate ownership graph. AppServerClient destruction intentionally does not manufacture terminal records for pending work.

## Per-module decisions

### HTTP and Express

HTTP request start is parser message-begin on the server and successful dispatch initiation on the client. A malformed request fails only if message-begin occurred. Response parser failure and request delivery failure are `failed`; connection loss while active is `aborted`; successful response/upgrade handoff is `completed`. Server streaming completion remains tied to the existing source EOF/send-completion point. Express routing and middleware logs remain Trace progress and do not create another request lifecycle.

### EventSource

The EventSource object owns the long-lived protocol. Stream establishment may repeat after an SSE retry. `event source reconnect ...` describes WHATWG/EventSource retry policy only; generic socket retry/reconnect remains Phase 2. `Last-Event-ID`, parsed `retry`, ready states, reconnect timing, and explicit close behavior are unchanged. A small safety correction initializes the private configuration pointer to null, guards configuration access in `close()`, and makes destruction call the same idempotent close path. This prevents a post-destruction reconnect and supplies one stop for a started object without adding another shutdown state machine.

### WebSocket and subprotocols

HTTP upgrade completion and WebSocket establishment are separate boundaries. Upgrade validation failure emits `websocket failed` without establishment. A normal close handshake and abnormal transport loss both converge on one `websocket ended`; the detailed cause remains in existing diagnostics. Close-frame progress was reduced to Trace. There is no separate `subprotocol start failed` record because no subprotocol object/callback failure state exists: factory/plugin selection failures precede subprotocol ownership and are retained as diagnostics while WebSocket initialization fails.

### MQTT

Physical transport connection is not an MQTT session. Protocol start occurs when the MQTT context attaches. Session establishment follows accepted CONNECT/CONNACK only. Server-side `resumed` is limited to renewal of an actual retained broker session; client-side `resumed` requires CONNACK `sessionPresent`. CONNECT rejection has no fabricated establishment/end. Explicit DISCONNECT, keep-alive close, context detach, and transport loss all converge on the same guarded protocol/session stop. The MQTT-over-WebSocket adapter logs only Trace progress and relies on the WebSocket, subprotocol, MQTT, and Phase 2 owners for canonical records. Logging-only session helpers are private; the installed class exposes no new public or protected member function for them.

### Codex App Server

The App Server session begins only after successful initialize response and initialized-notification admission. Initialization failure records only `app-server session start failed`; an established session stops once during explicit stop or connection invalidation. Accepted client and server JSON-RPC requests carry their IDs from start to their one map-removal terminal. Remote JSON-RPC errors are `request failed`; accepted pending requests invalidated with an unusable connection are `request cancelled`, matching the existing callback semantics. Notifications remain notifications.

Destruction-time invalidation was removed. `AppServerClient::Impl` retains its pre-Phase-3 destruction behavior: invalidate callback lifetime, clear transport callbacks, and stop the transport. It does not synthesize request/session terminals or schedule callbacks during destruction.

The typed backend reducer emits creation/start records only for actual `typed::ThreadStarted` and `typed::TurnStarted` events. Generic upserts and snapshot hydration do not fabricate starts. Before applying `typed::TurnCompleted` or `typed::TurnFailed`, terminal logging checks the existing domain `terminal` state; no logging flags live in `ThreadUpserted`, `TurnUpserted`, `ThreadState`, `TurnState`, snapshots, or frontend state. No `thread closed` is invented because the protocol exposes no thread-close event.

### MariaDB

A database session is established only after successful asynchronous connect completion and ends only if that establishment occurred. Existing asynchronous command logging is normalized as database request lifecycle. Synchronous statements, which did not previously have a lifecycle surface, remain uninstrumented to avoid per-statement noise. No PostgreSQL behavior was added.

### Applications

The 119 application-facing candidates were retained. Examples include sample `OnRequest`, upgrade-status, endpoint-connected, and MariaDB demo messages. They use application-facing log scopes or are explicit sample status output, not canonical framework truth. Reference WebSocket echo `connected/disconnected` callback messages and database sample `MySQL connected/disconnected` messages remain ordinary application output.

## Deliberately retained diagnostics and deferrals

The 188 diagnostics include HTTP parse/response details, upgrade bootstrap/plugin selection, EventSource callbacks, WebSocket frame and close-handshake detail, MQTT packet and broker persistence detail, Codex state transitions and public-callback failures, and MariaDB driver/descriptor errors. Their existing data and user value are retained; where high-frequency protocol progress was lifecycle-like, it is Trace.

No relevant Phase 3 lifecycle record is deferred. The final closure audit may reassess general diagnostic wording, application copy, and unrelated whole-tree severity consistency, but it has no unresolved Phase 3 ownership work.

## Compatibility assessment

Apart from the narrow EventSource destruction/configuration correction described above, protocol behavior, parsing, routing, middleware order, callbacks, HTTP/1.x behavior, WebSocket framing and close behavior, MQTT QoS/persistence/reconnect behavior, Codex wire schemas and reducer domain semantics, database execution, Phase 1/2 transport behavior, TLS, logger schema/backend/formatting/filtering, and book content are unchanged. AppServerClient destruction is restored to its pre-Phase-3 behavior.

Existing public class-member signatures and protected APIs are unchanged, and the installed Codex backend aggregate shapes match `master`. The installed function surface is not literally unchanged: four additive namespace-scope semantic logger overloads were added for connection-bound MQTT and HTTP logging (`mqttLogScope(connection)`, `mqttLog(connection)`, `httpClientLog(connection)`, and `httpServerLog(connection)`). Existing source consumers remain source-compatible.

The identity-prefix closure adds no namespace helper or public/protected API. It changes the signature of the private, nonvirtual server `releaseSession` implementation so its safe caller selects either the bound logger or destruction fallback; this adds no member state, layout change, or vtable effect.

Binary compatibility is **not claimed**. Installed HTTP client/server context and response classes, MQTT `Mqtt`, and `MariaDBConnection` have private layout changes, so consumers must rebuild. AppServerClient's PIMPL and the restored Codex aggregates add no exposed layout change. EventSource's private shared allocation and inline destructor behavior changed, but no virtual slot was added. No vtable entry was added, removed, or reordered. SOVERSION is intentionally unchanged.

## Validation surface and results

The GCC 15 Debug build, with include-what-you-use checks enabled, completed every target:

```text
cmake --build build -j2
```

The direct Phase 3 policy, Codex reducer, fake-transport AppServerClient, and MQTT lifecycle tests passed 4/4. The MQTT test captures a common packet diagnostic and a server-role diagnostic after connection binding, checks their structured instance/connection/component/role fields and retained domain payload, and verifies that text formatting places architectural identity before the formatter separator while the payload begins with the diagnostic. The final HTTP closure also extends the real in-process WebSocket upgrade test to verify connection-bound HTTP server metadata and payload, and adds a formatter assertion using the existing bound HTTP server logger. The Phase 3 tests otherwise continue to use structured semantic JSON for vocabulary, level, count, ordering, and available semantic identity fields.

Additional GCC results were:

- all logging tests: 44/44 passed;
- Phase 1/2 context, listener, and transport policy tests: 4/4 passed;
- HTTP tests: 28/28 passed;
- WebSocket tests: 12/12 passed;
- EventSource tests: 9/9 passed;
- TLS lifecycle/state-machine tests: 14/14 passed;
- coordinated shutdown tests: 24/24 passed;
- all 65 Codex-labelled tests: 64 passed and one skipped;
- all three MQTT-named tests: 3/3 passed;
- staged installed consumers and the TLS ABI/source check: 2/2 passed;
- complete CTest: 251 passed, zero failed, and one skipped out of 252.

`CodexTypedAppServerIntegrationTest` was skipped and is not counted as passing because `SNODEC_RUN_CODEX_TYPED_INTEGRATION=1` was not set. That opt-in uses a real Codex App Server and may consume configured credentials and quota.

Clang 21.1.8 compiled 16 focused changed targets, including all MQTT roles/adapters, HTTP, WebSocket, Codex, MariaDB, and the two identity-policy tests. Those tests passed 2/2 with `CHECK_INCLUDES=OFF` and the existing compatibility demotions for three newer diagnostics:

```text
-Wno-error=tautological-type-limit-compare -Wno-error=nrvo -Wno-error=implicit-int-conversion
```

A strict Clang 21 build remains gated by those three pre-existing `-Werror` diagnostics outside Phase 3 changes: a tautological type-limit comparison in `src/web/http/decoder/Chunked.cpp`, `-Wnrvo` in `src/ai/openai/codex/detail/ItemDecoder.cpp`, and implicit integer conversion in `src/web/websocket/Receiver.cpp`.

For the final HTTP identity closure, Clang recompiled `http-client`, `http-server`, both representative EventSource-instantiating HTTP clients, and the policy, formatter, and in-process upgrade tests; the three tests passed 3/3.

The AddressSanitizer/LeakSanitizer configuration used `SNODEC_ENABLE_ASAN=ON`. With the compiler's `libasan.so` explicitly preloaded so that the runtime is first, the focused policy, HTTP formatter, MQTT identity, EventSource reconnect, and EventSource destruction tests passed 5/5 with `detect_leaks=1:halt_on_error=1`. The in-process WebSocket upgrade test passed under AddressSanitizer with leak detection disabled; with LeakSanitizer enabled it reports the test harness's pre-existing static factory-cache leak (262 bytes in seven allocations), so that run is not claimed as passing. LeakSanitizer and socket tests required the unrestricted local-socket/ptrace test environment.

There is no MariaDB CTest fixture or configured service/credentials in this environment, so no live database session/query integration was run; the MariaDB library compiled in the full GCC build and its lifecycle invariants are covered by the source-policy test. MQTT-over-WebSocket libraries compiled, and their ownership boundary is source-policy checked, but the repository has no broker-backed MQTT-over-WebSocket lifecycle fixture. These are limitations, not passing integration results. `git diff --check` is clean.
