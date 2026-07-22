# Logging lifecycle Phase 2: transport and attempt consistency

## Status and relationship to earlier work

This document is the reviewed production-call-site audit and implementation
report for logging lifecycle Phase 2. It builds on, and does not replace, the
contracts in:

- `semantic-logging-contract.md`;
- `phase-1-context-lifecycle-correctness.md`;
- `semantic-identity-callsite-audit.md`; and
- `../core-stream-shutdown-lifecycle.md`.

The Phase 1 framework-context lifecycle remains `attached` / `detached`.
Attachment is not a transport connection, and detachment is not a transport
disconnection.

## Audit method and accounting

The audit searched all production files under `src/`, first for logging calls
and then more broadly for callbacks, state transitions, and messages containing
`connect`, `disconnect`, `accept`, `listen`, `open`, `close`, `ready`, `retry`,
`reconnect`, `timeout`, `cancel`, `spawn`, `child`, `exit`, `terminate`, `pipe`,
`handshake`, and `shutdown`. The broad review produced 488 candidate source
locations in 93 files. Every candidate was inspected in its owning state
machine; the numbers are search-review counts, not a claim that every textual
match is a lifecycle record.

The resulting accounting is:

| Disposition | Count | Meaning |
| --- | ---: | --- |
| Phase 2 lifecycle decisions/emissions changed or introduced | 43 | Canonical listener, attempt, transport, continuation, TLS-ready, child-process, and stdio records, including distinct terminal branches |
| Phase 2 owner-scope records retained | 96 | Detailed socket/TLS/descriptor/process progress, diagnostics, policy decisions, and summaries in core socket/pipe/eventreceiver, network, and Codex stdio code |
| Phase 3 or application-facing candidates deferred/retained | 237 | Protocol/session/request vocabulary and application messages selected by the broad protocol/application review |
| Not lifecycle log records | 112 | Declarations, callbacks, exceptions, state names, comments, and non-log textual matches |
| **Total classified** | **488** | No broad-search candidate is left unclassified |

The reproducible count for the final canonical Phase 2 sites is the number of
production matches for the exact vocabulary listed below. The 96 retained
owner-scope records are the remaining broad logging-call matches in
`src/core/socket`, `src/core/pipe`, `src/core/eventreceiver`, `src/net`, and
`src/ai/openai/codex/stdio`. The larger broad search supplies the deferred and
not-a-log classifications. Grouping related specializations in the tables
below avoids turning this report into a generated search dump.

## Canonical vocabulary and severity

| Lifecycle | Records | Level |
| --- | --- | --- |
| Listener | `listener started`, `listener stopped` | Info |
| Listener startup terminal | `listener start failed` | Debug |
| Connection attempt | `connection attempt started`, `connection attempt succeeded`, `connection attempt failed`, `connection attempt timed out`, `connection attempt cancelled` | Debug |
| Established transport | `transport connected`, `transport disconnected` | Info |
| TLS preparation completion | `transport ready` | Info |
| Retry | `retry scheduled`, `retry dispatched`, `retry cancelled` | Debug |
| Reconnect | `reconnect scheduled`, `reconnect dispatched`, `reconnect cancelled` | Debug |
| Child process | `child process spawned`, `child process exited`, `child process terminated` | Info |
| Child spawn terminal | `child process spawn failed` | Debug |
| Codex stdio transport | `stdio transport started`, `stdio transport stopped` | Info |
| Codex stdio startup terminal | `stdio transport start failed` | Debug |

Long-lived, externally meaningful resource boundaries are visible at Info.
Attempt and timer mechanics are Debug. Trace continues to carry syscall and
state-machine progress. Warn and Error remain separate diagnostics; an error
does not change the level of a canonical lifecycle end record.

`listener start timed out` and `listener start cancelled` are not emitted
because the current listener architecture has neither a startup timer nor an
asynchronous cancellation state. They are intentionally not fabricated.

## Semantic ownership and identity

| Owner | Logger and identity | Why it owns the record |
| --- | --- | --- |
| `SocketAcceptor` | Endpoint instance scope, server role | Owns listener admission and the registered accept receiver |
| `SocketConnector` | Endpoint instance scope, client role | Owns one dispatched physical connection candidate and its terminal outcome |
| `SocketConnection` | Connection scope with instance, role, and connection id | Exists only after an accepted or outgoing physical transport is established and remains alive through final teardown |
| TLS acceptor/connector callbacks | The same connection logger | Successful handshake is the point at which TLS becomes application-ready |
| Client/server continuation context | Endpoint instance scope with client/server role | Owns retry/reconnect timer scheduling and dispatch counters |
| Codex stdio `Session` | Existing `ai.openai.codex.stdio` connection scope | Already owns descriptors, child observation, shutdown escalation, and reaping |

`SocketConnection::getConnectionName()` remains a compatibility/display value.
It is not redefined and is not copied into the new transport message text.
Connection roles now come from the existing configuration role so both ends of
the lifecycle have the same full semantic identity.

## Reviewed core transport call sites

| Owner and call site | Previous record / level / logger | Classification | Pair or alternatives and duplication risk | Final action |
| --- | --- | --- | --- | --- |
| `SocketAcceptor::init`, successful descriptor enable | Low-level `open`/`bind`/`listen`/`enable` details through the static core logger | `LIFECYCLE_START` plus retained `PROGRESS` | Startup can fail before registration; internal operations are not separate resources | Add endpoint Info `listener started`; retain detailed diagnostics |
| `SocketAcceptor::init`, all unsuccessful completion paths, including TLS context setup | Error/debug syscall and TLS details | `TERMINAL_ALTERNATIVE` and `DIAGNOSTIC` | A failed candidate must not later stop | Add endpoint Debug `listener start failed`; retain the cause separately |
| `SocketAcceptor::unobservedEvent` | No canonical listener end | `LIFECYCLE_END` | Only a successfully registered receiver reaches this existing exactly-once owner | Emit endpoint Info `listener stopped` immediately before destruction |
| `SocketAcceptor::acceptEvent` after connection construction | Static `accept ... success` Debug detail | `LIFECYCLE_START` plus retained `PROGRESS` | Accepted connection must share identity with final teardown | Add connection-bound Info `transport connected`; retain accept/address detail |
| `SocketConnector::init` and `connectEvent` | Static connect/open/bind/getsockopt details with status callback terminals | `LIFECYCLE_START`, `TERMINAL_ALTERNATIVE`, `PROGRESS`, `DIAGNOSTIC` | Immediate, asynchronous, timeout, address-fallback, disable, and teardown paths converge differently | Add one endpoint Debug attempt start and exactly one success/failure/timeout/cancel terminal per candidate |
| `SocketConnector` immediate and event success | Static `connect ... success` Debug | `LIFECYCLE_START` | A constructed connection is established; a failed candidate constructs none | Emit connection-bound Info `transport connected` after attempt success |
| `SocketConnection::unobservedEvent` | Bare `disconnected` Debug on connection logger | `LIFECYCLE_END` | Reader/writer halves, repeated shutdown, and framework shutdown already converge on this one unobserved owner | Replace with connection-bound Info `transport disconnected`, once, after context detach and the existing disconnect callback, before destruction |
| Plain stream connection callbacks | No separate readiness record | Intentionally not paired | Plain transport is usable at connection construction | Do not add redundant `transport ready` |
| TLS handshake success callbacks | `SSL/TLS: Handshake success` Debug | `PROGRESS` plus readiness `LIFECYCLE_START` | Generic physical connection already emitted; handshake failure must never be ready | Retain handshake detail and add connection-bound Info `transport ready` only on success |
| TLS handshake/shutdown failures and close-notify progress | Error/Warn/Trace details | `DIAGNOSTIC` / `PROGRESS` | These report causes and state-machine progress, not extra lifecycle objects | Retain unchanged apart from canonical lifecycle companions |
| Client/server retry timer admission and callback | Info `Retry ... in`, counter increment on dispatch | Continuation lifecycle | Timer admission is not a dispatch; shutdown/config changes can cancel it | Emit Debug scheduled/dispatched/cancelled; move timing detail to Trace; increment existing counter only on dispatch |
| Client reconnect timer admission and callback | Info `Reconnect in`, counter increment on dispatch | Continuation lifecycle | Same distinction as retry | Emit Debug scheduled/dispatched/cancelled; preserve counter and policy |
| Client/server `OnConnect`/`OnConnected`/`OnDisconnect` address/statistics messages | Debug endpoint callback details including connection display name | `DIAGNOSTIC` | Compatibility/debug callback surface, not canonical state ownership | Retain; do not promote or mechanically rewrite |
| Stream reader/writer close, half-close, drain, timeout, and descriptor messages | Trace/Warn/Error | `PROGRESS` / `DIAGNOSTIC` | Coordinated stream shutdown owns behavior; these are not independent transports | Retain and rely on common connection end owner |

The connector uses one private `attemptActive` flag. The status callback could
not be used as an ownership token because the architecture deliberately copies
it to the next address candidate. The flag is reset in the copy so every
candidate starts and ends once; it changes no callback, timeout, or fallback
behavior and creates no second ownership graph.

## Network specializations, pipes, and event-loop infrastructure

| Area | Classification | Decision |
| --- | --- | --- |
| `net/in`, `net/in6`, `net/un`, `net/rc`, and `net/l2` stream specializations | `PROGRESS`, `DIAGNOSTIC`, or no local lifecycle record | The common acceptor, connector, connection, and TLS layers own canonical records; no duplicate specialization records were added |
| Unix path locking/unlinking and address selection | `DIAGNOSTIC` / `POLICY_DECISION` | Retained as physical-address detail |
| `core/pipe` source, sink, and pipe helper | `NOT_A_LOG_RECORD` for construction/close paths; exceptions are diagnostics | Pipe halves are temporary descriptor helpers, including inside Codex stdio, and are not independently presented as long-lived transports; no noisy pipe pair was invented |
| Descriptor receivers and multiplexer admission/removal | `PROGRESS` / `DIAGNOSTIC` | Retained at low level; existing registration facts provide exactly-once ownership |
| EventLoop start/stop/shutdown records | `SUMMARY` / `PROGRESS` outside Phase 2 | Retained; EventLoop shutdown semantics are unchanged |

## Codex stdio and child-process audit

| Call site | Classification | Final action and exactly-once basis |
| --- | --- | --- |
| Successful `posix_spawn` followed by parent ownership setup | Child `LIFECYCLE_START` | Info `child process spawned` after a real pid is owned; pid detail remains Trace |
| `posix_spawn` error | `TERMINAL_ALTERNATIVE` plus `DIAGNOSTIC` | Debug `child process spawn failed`; existing Error carries errno/text; no child exit is possible |
| Descriptor/pidfd/poll setup failure after spawn | Stdio startup terminal plus process cleanup | Debug `stdio transport start failed`; spawned child still gets its actual reaped process terminal record |
| All descriptor sides successfully registered | Stdio `LIFECYCLE_START` | Info `stdio transport started` once |
| Reaped `WIFEXITED` status | Child `LIFECYCLE_END` | Info `child process exited`; status and requested-shutdown detail remain Debug |
| Reaped `WIFSIGNALED` status | Child terminal alternative | Info `child process terminated`; signal/requested/forced detail remains Debug |
| First framework SIGTERM | `POLICY_DECISION` | Debug `child process termination requested` |
| First escalation to SIGKILL | `POLICY_DECISION` / abnormal diagnostic | Warn `child process forced termination requested` |
| Final descriptor teardown after a started transport | Stdio `LIFECYCLE_END` | Info `stdio transport stopped` once, guarded by the session's private started fact |
| Launch rejected or setup failure | Stdio startup terminal | Debug `stdio transport start failed`; detailed Error remains separate; never emits a fictional stop |

The existing `childPid = -1`, observer detachment, and reaped-state flow remain
the single child terminal owner. Pidfd and polling fallback both feed that same
path, so they cannot emit duplicate exits. Descriptor closure is deliberately
not called process exit. A process is only `exited` or `terminated` after
`waitpid` has reaped it.

## Exactly-once proof summary

- An acceptor startup failure calls direct destruction; only an observed,
  registered acceptor reaches `unobservedEvent` and emits `listener stopped`.
- Each connector candidate sets `attemptActive` at dispatch and exchanges it
  to false in every terminal helper. Destruction can therefore add cancellation
  only when no earlier terminal path ran.
- A `SocketConnection` is constructed only after accept/connect success.
  Consequently failed attempts cannot reach the common transport-disconnect
  owner.
- Both stream receiver halves converge on the connection's existing
  unobserved event. Coordinated shutdown, an already active shutdown, and later
  destructor cleanup do not create another end site.
- TLS readiness exists only inside the successful handshake callback. TLS
  shutdown remains generic transport teardown and cannot add another end.
- Retry/reconnect booleans are consumed by dispatch or cancellation. Existing
  counters are called only from dispatch.
- Codex process terminals are emitted only from the reaped-status path;
  `transportStarted` permits one stdio stop and forbids stop after startup
  failure.

## Compatibility and behavior preservation

This phase changes log message vocabulary, severity, and the completeness of
semantic role metadata. It does not change EventLoop shutdown, coordinated
stream shutdown, callback ordering, context attach/detach ordering or lifetime,
retry/reconnect policy, timeout values, timer or descriptor admission, TLS
handshake/shutdown behavior, connection/process ownership, child reaping,
public callbacks, public socket APIs, the logger backend/schema/formatters, the
global logging policy, ABI, or SOVERSION.

The only new state is private lifecycle bookkeeping: one connector-candidate
boolean and three Codex session booleans. No public or protected API is added.

## Deliberate Phase 3 deferrals

The following reviewed records remain intentionally unchanged unless they were
merely transport duplicates:

- MQTT protocol engine and MQTT session `Connected` / `Disconnected` records;
- HTTP request/response and HTTP protocol-engine lifecycle;
- EventSource protocol lifecycle;
- WebSocket protocol and subprotocol lifecycle;
- database protocol/command/session lifecycle;
- Codex app-server protocol/session lifecycle, JSON-RPC requests and
  notifications, threads, turns, model/session/user-message state, and
  cancellation;
- application-domain sessions and ordinary user-facing application messages.

Reference applications may still say that they are listening or connected as
user-facing status. Those messages neither own nor replace the framework
records and are classified `APPLICATION_USER_FACING`.

## Validation surface

Runtime JSON capture tests cover accepted and outgoing plain transports,
attempt success/failure without fictional disconnect, listener success/failure
and stop, graceful/dual-receiver shutdown, retry/reconnect schedule and
dispatch, and Codex spawn/setup/normal-exit/termination/fallback behavior.
Narrow source-policy tests cover terminal placement that is impractical to
force deterministically, including timeout/cancellation convergence, TLS-ready
placement, Phase 1 vocabulary preservation, and the absence of obsolete bare
transport records. Exact build, CTest, compiler, sanitizer, and CI results are
reported in the pull request because they describe the tested revision rather
than the semantic contract.
