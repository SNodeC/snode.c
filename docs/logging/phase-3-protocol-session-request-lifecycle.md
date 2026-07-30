# Logging lifecycle Phase 3: protocol, session, and request consistency

## Scope and contracts

This phase audits production logging under `src/` and normalizes lifecycle records above the transport layer. It is primarily a mechanical semantic-logging normalization: existing lifecycle messages receive canonical vocabulary and severity and are emitted through existing semantic scopes at authoritative protocol transitions. It does not redesign protocol state machines. It preserves the semantic logging contract, Phase 1 framework-context ownership, Phase 2 listener/connection-attempt/transport ownership, coordinated stream shutdown, and the TLS state machine. In particular, this work does not rename, reorder, duplicate, or change the severity of `listener started/stopped`, connection-attempt outcomes, `transport connected/ready/disconnected`, or framework-context `attached/detached` records.

The audit reviewed the implementations and state transitions, not only message
text. The ownership decisions below were checked against HTTP parser and
request queues, EventSource ready-state and retry configuration, WebSocket
upgraded-context attachment, MQTT CONNECT/CONNACK and broker session retention,
and the MariaDB asynchronous command queue.

## Canonical vocabulary and severity

The implemented vocabulary is:

- HTTP: `request started`, `request completed`, `request failed`, `request aborted`.
- EventSource: `event source started`, `event stream established`, `event source reconnect scheduled`, `event source reconnect dispatched`, `event source reconnect cancelled`, `event source stopped`.
- WebSocket: `websocket established`, `websocket ended`, `websocket failed`.
- selected WebSocket subprotocol: `subprotocol started`, `subprotocol stopped`.
- MQTT: `mqtt protocol started`, `mqtt protocol stopped`, `mqtt session established`, `mqtt session resumed`, `mqtt session rejected`, `mqtt session ended`.
- MariaDB: `database session established`, `database session ended`; `database request started`, `database request completed`, `database request failed`, `database request cancelled`.

Info is used for long-lived protocol, session, WebSocket, subprotocol,
EventSource, and database-session boundaries. Debug is used for bounded
request/command outcomes, rejected sessions, failed WebSocket establishment,
and EventSource reconnect policy. Trace remains the level for WebSocket
close-handshake progress, MQTT-over-WebSocket adapter attachment,
parser/frame/packet progress, and state-machine transitions. Warn and Error
records retain detailed abnormal causes and remain separate from the canonical
terminal record.

No HTTP `request timed out` or `request cancelled` record is fabricated: the
HTTP layer has no distinct deadline/cancellation callback. A connection
teardown while a started request is active is therefore `request aborted`;
Phase 2 still owns any lower-level timeout diagnostic.

## Identity ownership

| Owner | Semantic logger and correlation |
| --- | --- |
| HTTP server request | connection-bound HTTP framework/server-role logger plus a private monotonically increasing request correlation within that socket context |
| HTTP client request | connection-bound HTTP framework/client-role logger plus the existing `MasterRequest::count` |
| EventSource | `web.http.client.eventsource`; endpoint origin and path identify the protocol instance before a transport connection exists |
| WebSocket/subprotocol | connection-bound `web.websocket.subprotocol` scope; selected subprotocol is present in the lifecycle message |
| MQTT protocol/session | connection-bound `iot.mqtt` scope; client ID identifies the session and the connection scope carries instance and connection IDs |
| MariaDB | existing MariaDB semantic scope; connection name and command description identify the session/operation |

No public request-ID API or second identity framework was added. HTTP server correlation is private and socket-local because pipelined requests otherwise share all available semantic scope fields.

### Identity-prefix closure

Architectural identity is carried by semantic fields rather than repeated in message text. The MQTT closure reviewed 184 production message sites: 51 connection-bound duplicates, 44 static sites that could use an existing connection, 11 pre-bound or destruction-order fallbacks, five canonical/domain-identity records, and 73 process-wide component diagnostics. It mechanically changed 178 sites. Connection-bound common, client-role, server-role, and MQTT-over-WebSocket records now begin with the event or diagnostic payload; process-wide broker diagnostics likewise omit the redundant component label. MQTT client IDs, topics, packet identifiers and types, QoS, DUP/retain state, session-present state, protocol version, keep-alive values, store paths, and error detail remain domain payload.

The 11 retained `PREBOUND_FALLBACK` message sites are narrowly limited to seven client session-store diagnostics (constructor load, corrupt/empty data, restore, read failure, and absent filename; destructor write failure and absent filename) and four server session-release diagnostics reached from MQTT object destruction (delete/retain decisions and their session-pointer detail). They retain only the configured connection display name because no safe bound socket scope exists at those points; the client/server component label is removed. Live DISCONNECT uses the connection-bound server scope. All common-engine records were verified to execute after MQTT context attachment.

Frozen HTTP/WebSocket context lifecycle text, Phase 2 transport records,
protocol-version and subprotocol payload, and application-facing output were
excluded from the identity cleanup. The cleanup changes no lifecycle outcome,
protocol behavior, callback, timer, persistence decision, logger schema, or
formatter. It adds no public/protected API, class state, layout, or vtable
entry.

Thirteen fallback paths remain: the 11 MQTT constructor/destructor sites above,
the HTTP server's unexpected-disconnect branch after its response has no
SocketContext, and the EventSource `OnRequestEnd` branch reached when its weak
owner is gone after HTTP-context disconnect. Only 12 retain textual connection
identity: the 11 MQTT display-name fallbacks and that EventSource request
fallback; the disconnected HTTP server record contains no connection text.
Normal EventSource request end is connection-bound. HTTP methods, URLs and
versions, selected/requested upgrade protocols, MQTT client IDs, topics, packet
IDs, QoS, session values, database session identity, and error detail remain
domain payload. This final correction changes only logger selection and message
prefixes; it adds no logger abstraction or overload, public/protected API,
persistent logger state, lifecycle flag, class-layout/vtable change, protocol
behavior, or lifecycle outcome.

The HTTP server record is specifically a `PREBOUND_FALLBACK` with a
post-disconnect subtype: `Response::isConnected()` is equivalent to
`socketContext != nullptr`, and `Response::disconnect()` clears that pointer.
The disconnected `Response::upgrade()` branch therefore intentionally uses the
parameterless HTTP server logger, carries no fabricated connection identity,
and emits `Unexpected disconnect during upgrade` without the redundant
`Upgrade:` label. The connected missing-request path remains connection-bound
but returns before any request-derived access: it emits
`Upgrade request has gone away`, sets `Connection: close` and HTTP 500, invokes
the status callback exactly once with an empty selected protocol, and performs
no factory selection or context switch. This narrow HTTP correctness
correction leaves normal upgrade behavior and the fallback count unchanged.

### Parameterless-helper closure

The policy lexically inspects every literal empty call to the discovered Phase
3 semantic helpers under HTTP, WebSocket, MQTT, and MariaDB production code.
The two `SubProtocol::detach()` payload-total summaries reuse the same
connection-bound `webSocketSubProtocolLog` as `subprotocol stopped`. The
remaining 77 calls are intentionally parameterless and appear individually,
with a stable source marker, classification, and reason, in
`ParameterlessSemanticLoggerPolicyTest`. That machine-readable allowlist is the
authoritative per-call-site classification artifact; the table below is its
human-readable summary.

| Module | Helper | Sites | Classification | Reason/category |
| --- | --- | ---: | --- | --- |
| HTTP generic | `webHttpLog` | 9 | `GLOBAL_COMPONENT_DIAGNOSTIC` | eight protocol-selector/registry diagnostics and `MimeTypes::contentType` |
| HTTP client/EventSource | `httpClientEventSourceLog` | 14 | `PREBOUND_FALLBACK` | URL/configuration validation in legacy, TLS, RFCOMM, and Unix-domain EventSource templates before a connection exists |
| HTTP client/EventSource | `httpClientEventSourceLog` | 6 | `DOMAIN_OR_PROTOCOL_SCOPE` | long-lived EventSource start/stop, stream establishment, and SSE retry lifecycle between transports |
| HTTP client/EventSource | `httpClientLog` | 2 | `PREBOUND_FALLBACK` | initial EventSource origin/path configuration diagnostics |
| HTTP client/EventSource | `httpClientLog` | 1 | `POSTDISCONNECT_FALLBACK` | `OnRequestEnd` weak-owner fallback after the connection-owned request is gone |
| HTTP client/EventSource | `httpClientLog` | 4 | `FROZEN_PHASE1_PHASE2` | accepted EventSource connector-state diagnostics |
| HTTP upgrade registries | `httpClientUpgradeLog`, `httpServerUpgradeLog` | 2 | `GLOBAL_COMPONENT_DIAGNOSTIC` | one client and one server dynamic-upgrade-directory override |
| HTTP server | `httpServerLog` | 1 | `POSTDISCONNECT_FALLBACK` | `Response::upgrade` after `disconnect()` has cleared `socketContext` |
| WebSocket | `webSocketFactoryLog` | 9 | `GLOBAL_COMPONENT_DIAGNOSTIC` | seven factory-selector/registry diagnostics and two dynamic-directory overrides |
| WebSocket | `webSocketFrameLog` | 2 | `DOMAIN_OR_PROTOCOL_SCOPE` | receiver/transmitter frame-level logger caches |
| MQTT common | `mqttLog` | 1 | `HELPER_FALLBACK_IMPLEMENTATION` | fallback return in `Mqtt::log()` when no safe bound scope is available |
| MQTT client | `mqttClientLog` | 5 | `PREBOUND_FALLBACK` | constructor/session-store load and configuration diagnostics |
| MQTT client | `mqttClientLog` | 2 | `POSTDISCONNECT_FALLBACK` | destructor/session-store write diagnostics |
| MQTT client | `mqttClientLog` | 1 | `HELPER_FALLBACK_IMPLEMENTATION` | fallback return in local `clientLog()` |
| MQTT server | `mqttServerLog` | 1 | `POSTDISCONNECT_FALLBACK` | destructor-time session release after safe connection ownership ends |
| MQTT server | `mqttServerLog` | 1 | `HELPER_FALLBACK_IMPLEMENTATION` | fallback return in local `serverLog()` |
| MQTT broker | `mqttBrokerLog` | 5 | `GLOBAL_COMPONENT_DIAGNOSTIC` | broker, retained-tree, and subscription-tree process-wide caches |
| MQTT session | `mqttSessionLog` | 1 | `DOMAIN_OR_PROTOCOL_SCOPE` | protocol-session logger cache rather than socket identity |
| MariaDB library | `mariaDbLog` | 1 | `GLOBAL_COMPONENT_DIAGNOSTIC` | process-wide client-library initialization |
| MariaDB connection | `mariaDbLog` | 9 | `DOMAIN_OR_PROTOCOL_SCOPE` | existing database session/command owner and descriptor diagnostics |

The classification totals are 21 `PREBOUND_FALLBACK`, five
`POSTDISCONNECT_FALLBACK`, 26 `GLOBAL_COMPONENT_DIAGNOSTIC`, 18
`DOMAIN_OR_PROTOCOL_SCOPE`, four `FROZEN_PHASE1_PHASE2`, three
`HELPER_FALLBACK_IMPLEMENTATION`, and zero `APPLICATION_FACING` or
`UNRESOLVED_STATIC_BUT_BINDABLE`. Object-bound `log()`/`frameworkLog()` calls,
semantic helpers with nonempty arguments, and helper definitions are
deliberately excluded. Domain identities—including MQTT client
ID/topic/packet/QoS fields, HTTP request and upgrade data, and database
operation data—remain in their messages. This policy changes no lifecycle or
protocol behavior and introduces no API, logger owner/cache, persistent logger
state, or class-layout change.

## Exactly-once strategy

Duplicate suppression uses existing owner state and narrow private flags only where necessary. These guarantees apply to the actual framework lifecycle; they do not promise arbitrary event-replay or event-sourcing idempotency.

- The HTTP server stores terminal state beside each pending request. Parser failure marks the request terminal, successful response completion removes it, and context teardown aborts only remaining nonterminal entries. A parser that fails before message-begin produces no fabricated start/terminal pair. The private `Response::sourceFailed` terminal-disambiguation flag is reset for each response and set by `onSourceError()` before stream EOF/close cleanup. `sendCompleted()` then reports failure rather than completion, preventing one exact-length streaming request from receiving both failed and completed terminals. Successful streaming behavior is unchanged. This is a narrow HTTP correctness change discovered during lifecycle normalization, not observability state exposed through an API.
- The HTTP client treats membership in its private started-request set as terminal ownership. Response delivery, parse failure, delivery failure, and disconnect erase that membership; later cleanup cannot emit another outcome.
- EventSource start/stop and reconnect-pending bits live in private per-instance shared configuration. `close()` is idempotent, disables Phase 2 reconnect/retry before shutdown, cancels an outstanding SSE retry record, and prevents disconnect cleanup from scheduling another retry or stop.
- WebSocket establishment/end are emitted only by upgraded-context attach/detach. Close-frame progress does not terminate the lifecycle, so the later transport teardown cannot duplicate the WebSocket end. Selected subprotocol start/stop uses the same attach/detach convergence point.
- MQTT protocol/session flags live in the `Mqtt` protocol engine. Private helpers, friend-limited to the existing concrete client and server implementations, are invoked only after accepted CONNACK or broker session creation. Resume is limited to the retained-session renewal branch or a client CONNACK with `sessionPresent`. Rejection prevents establishment, and disconnect clears the active session before stopping the protocol. Broker retention remains a persistence decision, not a second end.
- MariaDB command state is attached to the existing `currentCommand`. The error path marks it failed before the shared completion cleanup; destruction terminates only a command that actually started. Queued, unstarted commands do not receive fabricated terminals.

These are existing domain state and local private flags on lifecycle owners, not
a general lifecycle registry or alternate ownership graph.

## Per-module decisions

### HTTP and Express

HTTP request start is parser message-begin on the server and successful dispatch initiation on the client. A malformed request fails only if message-begin occurred. Response parser failure and request delivery failure are `failed`; connection loss while active is `aborted`; successful response/upgrade handoff is `completed`. Server streaming completion remains tied to the existing source EOF/send-completion point. Express routing and middleware logs remain Trace progress and do not create another request lifecycle.

`Response::sourceFailed` is a private terminal-disambiguation flag and a separate narrow HTTP correctness correction. `Response::init()` resets it for every response; `onSourceError()` marks it before stream EOF and connection-close cleanup; and `sendCompleted()` uses it to report the active request as failed rather than completed. This prevents a source that fails after sending its exact declared content length, followed by later EOF cleanup, from producing a successful terminal after failure. It changes no successful streaming path and is not public domain or logging state.

### EventSource

The EventSource object owns the long-lived protocol. Stream establishment may repeat after an SSE retry. `event source reconnect ...` describes WHATWG/EventSource retry policy only; generic socket retry/reconnect remains Phase 2. `Last-Event-ID`, parsed `retry`, ready states, reconnect timing, and explicit close behavior are unchanged. A small safety correction initializes the private configuration pointer to null, guards configuration access in `close()`, and makes destruction call the same idempotent close path. This prevents a post-destruction reconnect and supplies one stop for a started object without adding another shutdown state machine.

### WebSocket and subprotocols

HTTP upgrade completion and WebSocket establishment are separate boundaries. Upgrade validation failure emits `websocket failed` without establishment. A normal close handshake and abnormal transport loss both converge on one `websocket ended`; the detailed cause remains in existing diagnostics. Close-frame progress was reduced to Trace. There is no separate `subprotocol start failed` record because no subprotocol object/callback failure state exists: factory/plugin selection failures precede subprotocol ownership and are retained as diagnostics while WebSocket initialization fails.

### MQTT

Physical transport connection is not an MQTT session. Protocol start occurs when the MQTT context attaches. Session establishment follows accepted CONNECT/CONNACK only. Server-side `resumed` is limited to renewal of an actual retained broker session; client-side `resumed` requires CONNACK `sessionPresent`. CONNECT rejection has no fabricated establishment/end. Explicit DISCONNECT, keep-alive close, context detach, and transport loss all converge on the same guarded protocol/session stop. The MQTT-over-WebSocket adapter logs only Trace progress and relies on the WebSocket, subprotocol, MQTT, and Phase 2 owners for canonical records. Logging-only session helpers are private; the installed class exposes no new public or protected member function for them.

### MariaDB

A database session is established only after successful asynchronous connect completion and ends only if that establishment occurred. Existing asynchronous command logging is normalized as database request lifecycle. Synchronous statements, which did not previously have a lifecycle surface, remain uninstrumented to avoid per-statement noise. No PostgreSQL behavior was added.

### Applications

Sample `OnRequest`, upgrade-status, endpoint-connected, and MariaDB demo
messages use application-facing log scopes or are explicit sample status
output, not canonical framework truth. Reference WebSocket echo
`connected/disconnected` callback messages and database sample
`MySQL connected/disconnected` messages remain ordinary application output.

## Deliberately retained diagnostics and deferrals

Retained diagnostics include HTTP parse/response details, upgrade
bootstrap/plugin selection, EventSource callbacks, WebSocket frame and
close-handshake detail, MQTT packet and broker persistence detail, and MariaDB
driver/descriptor errors. Their existing data and user value are retained;
where high-frequency protocol progress is lifecycle-like, it is Trace.

No relevant Phase 3 lifecycle record is deferred. Detailed diagnostics remain
outside the canonical lifecycle vocabulary.

## Compatibility assessment

Apart from the narrow EventSource destruction/configuration correction
described above, protocol behavior, parsing, routing, middleware order,
callbacks, HTTP/1.x behavior, WebSocket framing and close behavior, MQTT
QoS/persistence/reconnect behavior, database execution, Phase 1/2 transport
behavior, TLS, logger schema/backend/formatting/filtering, and book content are
unchanged.

Existing public class-member signatures and protected APIs are unchanged. The
installed function surface is not literally unchanged: four additive
namespace-scope semantic logger overloads were added for connection-bound MQTT
and HTTP logging (`mqttLogScope(connection)`, `mqttLog(connection)`,
`httpClientLog(connection)`, and `httpServerLog(connection)`). Existing source
consumers remain source-compatible.

The identity-prefix closure adds no namespace helper or public/protected API. It changes the signature of the private, nonvirtual server `releaseSession` implementation so its safe caller selects either the bound logger or destruction fallback; this adds no member state, layout change, or vtable effect.

Binary compatibility is **not claimed**. Installed HTTP client/server context
and response classes, MQTT `Mqtt`, and `MariaDBConnection` have private layout
changes, so consumers must rebuild. In particular, the private HTTP response
`sourceFailed` flag contributes to the already-disclosed response-layout change
and rebuild requirement. The parameterless-helper policy adds no further state
or layout change. EventSource's private shared allocation and inline destructor
behavior changed, but no virtual slot was added. No vtable entry was added,
removed, or reordered. SOVERSION is intentionally unchanged.
