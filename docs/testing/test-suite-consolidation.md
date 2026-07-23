# Test-suite consolidation audit

This audit records the test suite at the start of the consolidation from
`master` commit `6fb9171f08a68d79771187155873a448b47a4aee`.

## Baseline

- Registered tests: 254
- Passing tests: 253
- Skipped tests: 1 (`CodexTypedAppServerIntegrationTest`, return code 77)
- Failing tests: 0 when local IPv4, IPv6, and Unix-domain sockets are permitted
- Baseline command: `ctest --test-dir build --output-on-failure`
- A sandboxed run produced 91 expected failures because socket creation returned
  `EPERM`; the unrestricted baseline above is the comparison baseline.
- Historical labels present at baseline: `round2` through `round9`,
  `migration01` through `migration09`, `phase3`, `repair`, and
  `source-policy`.

The classification vocabulary is:

`KEEP`, `RENAME`, `MERGE`, `EXTRACT_THEN_DELETE`, `DELETE_REDUNDANT`,
`DELETE_MIGRATION_ONLY`, `MOVE_TO_POLICY`, `MOVE_TO_SUBSYSTEM`, and
`REPLACE_WITH_RUNTIME_TEST`.

## Inventory conventions

Every name in the first column below is a separately registered CTest test.
Rows group names only when CMake intentionally registers several scenarios
from one source file. In such a grouped row, the category, contract, overlap,
decision, destination, and reason apply to every explicitly enumerated name.
“Complementary variants” means the scenarios share a fixture but protect
different state transitions, transports, payload shapes, or error paths; it
does not mean one scenario replaces another.

## Stable unit, install, and component inventory

These tests already have stable subsystem ownership. They are kept without
stylistic churn.

| Current test name | Current source file | Category | Permanent contract | Overlap | Decision | Destination or replacement | Reason |
|---|---|---|---|---|---|---|---|
| `StagedInstalledConsumerTest` | `tests/StagedInstalledConsumerTest.cmake` | install/API | A staged install can be consumed by downstream CMake projects | Complements header-level ABI tests | KEEP | unchanged | Installed-consumer behavior cannot be replaced by in-tree linkage |
| `SNodeCInitFreeSmokeTest` | `tests/unit/core/SNodeCInitFreeSmokeTest.cpp` | unit/core | Core initialization and cleanup smoke path | None | KEEP | unchanged | Stable public runtime behavior |
| `TLSResultClassificationTest` | `tests/unit/core/TLSResultClassificationTest.cpp` | unit/core/TLS | TLS result classification | Complements fatal path and state machine | KEEP | unchanged | Stable TLS decision behavior |
| `TLSIoFatalPathTest` | `tests/unit/core/TLSIoFatalPathTest.cpp` | unit/core/TLS | Fatal TLS I/O follows the connection-owned failure path | Complements result classification | KEEP | unchanged | Stable TLS error behavior |
| `TLSLifecycleHelperOwnershipTest` | `tests/unit/core/TLSLifecycleHelperOwnershipTest.cpp` | unit/core/TLS/lifecycle | TLS helpers preserve lifecycle ownership | Complements state-machine scenarios | KEEP | unchanged | Durable ownership contract |
| `TLSTransportStateMachineTest` | `tests/unit/core/TLSTransportStateMachineTest.cpp` | unit/core/TLS | TLS handshake, shutdown, close-notify, and I/O transitions | Complementary cases within one executable | KEEP | unchanged | Authoritative TLS state-machine coverage |
| `TLSLifecycleAbiSourceCompatibilityTest` | `tests/unit/core/TLSLifecycleAbiSourceCompatibilityTest.cpp` | unit/core/TLS/ABI | TLS lifecycle API and ABI source compatibility | Complements runtime TLS tests | KEEP | unchanged | Compile-time compatibility cannot be inferred from runtime tests |
| `StreamFrameworkShutdown_plain_requested_empty`, `StreamFrameworkShutdown_plain_requested_queued`, `StreamFrameworkShutdown_plain_signal_false_veto`, `StreamFrameworkShutdown_plain_no_observer`, `StreamFrameworkShutdown_plain_noncooperating_timeout`, `StreamFrameworkShutdown_reentrant_context_cleanup`, `StreamFrameworkShutdown_active_detach_context_visibility`, `StreamFrameworkShutdown_destruction_logging_order` | `tests/unit/core/StreamFrameworkShutdownTest.cpp` | unit/core/shutdown | Plain-stream shutdown admission, timeout, reentrancy, context visibility, and destruction ordering | Complementary variants | KEEP | unchanged | Stable exactly-once shutdown behavior |
| `TLSFrameworkShutdown_active_helper_requested`, `TLSFrameworkShutdown_active_helper_no_observer`, `TLSFrameworkShutdown_active_helper_signal`, `TLSFrameworkShutdown_active_helper_forced`, `TLSFrameworkShutdown_publisher_mutation_same_list`, `TLSFrameworkShutdown_publisher_mutation_cross_publisher`, `TLSFrameworkShutdown_signal_false_tls`, `TLSFrameworkShutdown_outer_timeout_long`, `TLSFrameworkShutdown_outer_timeout_multiple` | `tests/unit/core/TLSFrameworkShutdownTest.cpp` | unit/core/TLS/shutdown | TLS framework shutdown, publisher mutation, signal veto, and outer timeout behavior | Complementary variants | KEEP | unchanged | Stable TLS lifecycle and mutation-safety coverage |
| `InetSocketAddressTest`, `InetSocketAddressNativeTest`, `Inet6SocketAddressTest`, `Inet6SocketAddressNativeTest`, `UnixSocketAddressTest`, `UnixSocketAddressInitTest`, `InetSocketAddressInvalidInputTest`, `Inet6SocketAddressInvalidInputTest`, `UnixSocketAddressPathLengthTest`, `SocketAddressFormattingTest` | Corresponding file under `tests/unit/net/` | unit/net/address | IPv4, IPv6, Unix address parsing, native conversion, validation, limits, and formatting | Complementary address variants | KEEP | unchanged | Stable network value-type behavior |
| `HttpMessageParserTest`, `HttpRequestFormatterRawWireTest`, `HttpResponseFormatterRawWireTest`, `HttpMessagePresentationTest`, `HttpMessageHeaderCasingTest`, `HttpMessageTargetQueryEdgeTest` | Corresponding file under `tests/unit/http/` | unit/HTTP | HTTP parsing, raw-wire formatting, presentation, casing, and target/query edges | Complementary HTTP message layers | KEEP | unchanged | Stable protocol correctness |
| `WebSocketReceiverValidationTest` | `tests/unit/websocket/WebSocketReceiverValidationTest.cpp` | unit/WebSocket | Receiver rejects invalid frames and state | Complements component handshakes | KEEP | unchanged | Deterministic protocol validation |
| `Mqtt311PacketValidationTest` | `tests/unit/mqtt/Mqtt311PacketValidationTest.cpp` | unit/MQTT | MQTT 3.1.1 packet validation | Complements MQTT lifecycle | KEEP | unchanged | Stable protocol validation |
| `ConfigFormatterCommentMetadataTest`, `ConfigRequirementStateTest`, `HexDumpPresentationTest` | Corresponding file under `tests/unit/utils/` | unit/utils | Config metadata, requirement state, and hex-dump presentation | None beyond shared utility layer | KEEP | unchanged | Stable utility behavior |
| `SingleshotTimerTest`, `IntervalTimerStopableTest`, `TimerCancelBeforeFireTest`, `IntervalTimerCancelFromCallbackTest` | Corresponding file under `tests/component/core/` | component/core/timer | Timer fire, stop, pre-fire cancellation, and callback cancellation | Complementary timer transitions | KEEP | unchanged | Observable event-loop behavior |
| `SNodeCStopFromCallbackTest` | `tests/component/core/SNodeCStopFromCallbackTest.cpp` | component/core/lifecycle | Framework stop is safe from a callback | Complements shutdown tests | KEEP | unchanged | Stable reentrancy contract |
| `PipeBoundedQueueTest`, `PipeOwnershipTest`, `PipeImmediateCloseTest`, `PipeSinkFairnessTest`, `PipeTimeoutTest` | Corresponding file under `tests/component/core/` | component/core/pipe | Queue bounds, ownership, close, fairness, and timeout | Complementary pipe behaviors | KEEP | unchanged | Stable component behavior |
| `DescriptorRegistrationFailureTest` | `tests/component/core/DescriptorRegistrationFailureTest.cpp` | component/core/descriptor | Descriptor registration failure is surfaced and cleaned up | Complements syscall policies | KEEP | unchanged | Runtime failure coverage |
| `ShutdownReceiverNotification_requested`, `ShutdownReceiverNotification_signal` | `tests/component/core/ShutdownReceiverNotificationTest.cpp` | component/core/shutdown | Requested and signal shutdown notify receivers | Complementary triggers | KEEP | unchanged | Stable shutdown behavior |
| `InetLegacyServerClientCompositionTest`, `Inet6LegacyServerClientCompositionTest`, `UnixLegacyServerClientCompositionTest` | Matching composition source under `tests/component/net/` | component/net | Server/client composition works on IPv4, IPv6, and Unix transports | Transport variants | KEEP | unchanged | Stable public composition behavior |
| `InetLegacyServerMultipleClientsCompositionTest`, `Inet6LegacyServerMultipleClientsCompositionTest`, `UnixLegacyServerMultipleClientsCompositionTest` | Matching multiple-client composition source under `tests/component/net/` | component/net | One server owns multiple concurrent clients on each transport | Transport variants | KEEP | unchanged | Stable multiplicity and ownership behavior |
| `InetLegacyServerClientMultiMessagePayloadExchangeTest`, `Inet6LegacyServerClientMultiMessagePayloadExchangeTest`, `UnixLegacyServerClientMultiMessagePayloadExchangeTest` | Matching multi-message source under `tests/component/net/` | component/net | Ordered bidirectional multi-message exchange | Transport variants | KEEP | unchanged | Stable streaming behavior |
| `InetLegacyServerMultipleClientsPayloadExchangeTest`, `Inet6LegacyServerMultipleClientsPayloadExchangeTest`, `UnixLegacyServerMultipleClientsPayloadExchangeTest` | Matching multiple-client payload source under `tests/component/net/` | component/net | Payload isolation across multiple clients | Transport variants | KEEP | unchanged | Stable connection isolation |
| `UnixLegacyServerClientPayloadExchangeTest`, `InetLegacyServerClientPayloadExchangeTest`, `Inet6LegacyServerClientPayloadExchangeTest` | Matching payload source under `tests/component/net/` | component/net | Basic payload exchange on each transport | Transport variants | KEEP | unchanged | Stable transport behavior |
| `UnixLegacyServerClientDisconnectLifecycleTest`, `InetLegacyServerClientDisconnectLifecycleTest`, `Inet6LegacyServerClientDisconnectLifecycleTest` | Matching disconnect source under `tests/component/net/` | component/net/lifecycle | Disconnect callbacks and cleanup occur once | Transport variants | KEEP | unchanged | Durable lifecycle behavior |
| `UnixLegacyClientConnectFailureTest`, `InetLegacyClientConnectFailureTest`, `Inet6LegacyClientConnectFailureTest` | Matching connect-failure source under `tests/component/net/` | component/net | Connect failures are reported without false success | Transport variants | KEEP | unchanged | Stable failure behavior |
| `InetLegacyServerEffectiveListenAddressTest`, `InetPhysicalServerEffectiveBindAddressTest`, `UnixPhysicalSocketPathSafetyTest` | Corresponding file under `tests/component/net/` | component/net | Effective addresses and Unix path safety are correct | Complementary address layers | KEEP | unchanged | Observable transport/security behavior |
| `InetLegacyServerClientLargePayloadExchangeTest`, `Inet6LegacyServerClientLargePayloadExchangeTest`, `UnixLegacyServerClientLargePayloadExchangeTest` | Matching large-payload source under `tests/component/net/` | component/net | Large payloads traverse each transport intact | Transport variants | KEEP | unchanged | Stable buffering behavior |
| `InetLegacyServerClientFramedPayloadExchangeTest`, `Inet6LegacyServerClientFramedPayloadExchangeTest`, `UnixLegacyServerClientFramedPayloadExchangeTest` | Matching framed-payload source under `tests/component/net/` | component/net | Framing preserves message boundaries | Transport variants | KEEP | unchanged | Stable framing behavior |
| `InetLegacyServerControlledCloseLifecycleTest`, `Inet6LegacyServerControlledCloseLifecycleTest`, `UnixLegacyServerControlledCloseLifecycleTest` | Matching controlled-close source under `tests/component/net/` | component/net/lifecycle | Server-controlled close completes once | Transport variants | KEEP | unchanged | Stable close lifecycle |
| `InetLegacyServerMultipleClientsDisconnectLifecycleTest`, `Inet6LegacyServerMultipleClientsDisconnectLifecycleTest`, `UnixLegacyServerMultipleClientsDisconnectLifecycleTest` | Matching multiple-client disconnect source under `tests/component/net/` | component/net/lifecycle | Multiple disconnects remain isolated and exactly once | Transport variants | KEEP | unchanged | Stable ownership/lifecycle behavior |
| `InetHttpServerClientGetRoundTripTest`, `Inet6HttpServerClientGetRoundTripTest`, `UnixHttpServerClientGetRoundTripTest` | Matching GET source under `tests/component/http/` | component/HTTP | GET request/response round trip | Transport variants | KEEP | unchanged | Stable protocol behavior |
| `InetHttpServerClientPostBodyRoundTripTest`, `Inet6HttpServerClientPostBodyRoundTripTest`, `UnixHttpServerClientPostBodyRoundTripTest` | Matching POST source under `tests/component/http/` | component/HTTP | POST body round trip | Transport variants | KEEP | unchanged | Stable body handling |
| `InetHttpServerClientStatusVariantTest`, `Inet6HttpServerClientStatusVariantTest`, `UnixHttpServerClientStatusVariantTest` | Matching status source under `tests/component/http/` | component/HTTP | Status variants survive round trip | Transport variants | KEEP | unchanged | Stable status behavior |
| `InetHttpServerClientRepeatedRequestTest`, `InetHttpServerClientDisconnectLifecycleTest`, `InetHttpClientPrematureServerCloseTest`, `InetHttpServerClientLargeResponseBodyTest`, `InetHttpServerClientLargeRequestBodyTest`, `InetHttpServerClientHeaderHeavyRequestTest`, `InetHttpServerClientHeaderHeavyResponseTest`, `InetHttpClientConnectFailureTest`, `InetHttpServerClientChunkedResponseTest`, `InetHttpServerClientPipeliningTest`, `InetHttpServerMalformedRequestBehaviorTest`, `InetHttpClientMalformedResponseBehaviorTest`, `InetHttpServerClientChunkedRequestTest` | Corresponding file under `tests/component/http/` | component/HTTP | Reuse, disconnect, premature close, size, headers, connect failure, chunking, pipelining, and malformed-message behavior | Complementary protocol scenarios | KEEP | unchanged | Stable HTTP state-machine coverage |
| `InetExpressRoutingMiddlewareTest`, `Inet6ExpressTransportSmokeTest`, `UnixExpressTransportSmokeTest`, `InetExpressMiddlewareShortCircuitTest`, `InetExpressRoutingEdgeBasicsTest`, `InetExpressMountedRouterBasicsTest`, `InetExpressNestedRouterBasicsTest`, `InetExpressRouteParameterRegexTest`, `InetExpressMiddlewareMountOrderTest`, `InetExpressQueryMountInteractionTest`, `InetExpressFallbackNotFoundTest` | Corresponding file under `tests/component/express/` | component/Express | Middleware and router ordering, mounting, parameters, query interaction, fallback, and transport smoke | Complementary routing cases | KEEP | unchanged | Stable framework behavior |
| `InetWebSocketServerClientTextEchoTest`, `Inet6WebSocketServerClientTextEchoTest`, `UnixWebSocketServerClientTextEchoTest`, `InetWebSocketServerClientMultiTextMessageTest`, `InetWebSocketServerClientBinaryEchoTest`, `InetWebSocketServerClientCloseHandshakeTest`, `InetWebSocketServerClientPingPongTest`, `InetWebSocketServerInitiatedCloseTest`, `InetWebSocketUnexpectedCloseLifecycleTest`, `InetWebSocketLargeTextMessageTest`, `InetWebSocketLargeBinaryMessageTest` | Corresponding file under `tests/component/websocket/` | component/WebSocket | Echo, fragmentation/message multiplicity, close, ping/pong, unexpected close, and large messages | Complementary protocol scenarios | KEEP | unchanged | Stable WebSocket behavior |
| `InetSseEventSourceBasicEventTest`, `InetSseEventSourceMultiEventSequenceTest`, `InetSseEventSourceMultiLineDataTest`, `InetSseEventSourceCommentIgnoredTest`, `InetSseEventSourceDefaultMessageEventTest`, `InetSseEventSourceClientCloseAfterEventTest`, `InetSseEventSourceRetryFieldTest`, `InetSseEventSourceReconnectLifecycleTest`, `InetSseEventSourceDestructionLifecycleTest` | Corresponding file under `tests/component/eventsource/` | component/EventSource | SSE parsing, sequencing, comments, defaults, close, retry, reconnect, and destruction | Complementary protocol/lifecycle cases | KEEP | unchanged | Stable EventSource behavior |
| `CodexStdioLifecycleTest` | `tests/component/codex/CodexStdioLifecycleTest.cpp` | component/Codex | stdio child lifecycle | Complements scenario matrix | KEEP | unchanged | Stable process lifecycle |
| `CodexStdioProtocolFlow_flow`, `CodexStdioProtocolFlow_typed_flow`, `CodexStdioProtocolFlow_exit_restart` | `tests/component/codex/CodexStdioProtocolFlowTest.cpp` | component/Codex/protocol | Raw, typed, and restart protocol flows | Complementary variants | KEEP | unchanged | Stable protocol behavior |
| `CodexProtocolCodecTest`, `CodexProtocolEngineTest`, `CodexTypedThreadCodecTest`, `CodexTypedTurnCodecTest`, `CodexTypedItemDecoderTest`, `CodexTypedEventDecoderTest`, `CodexTypedServerRequestDecoderTest`, `CodexTypedProtocolEngineTest` | Corresponding file under `tests/component/codex/` | component/Codex/protocol | Raw and typed codec, decoder, and protocol engine behavior | Complementary layers/types | KEEP | unchanged | Stable protocol correctness |
| `CodexBackendReducerTest`, `CodexBackendSessionTest`, `CodexBackendCoreTest` | Corresponding file under `tests/component/codex/` | component/Codex/backend | Reducer, session, and backend-core state | Complementary backend layers | KEEP | unchanged | Stable backend behavior |
| `CodexFrontendProtocolV1Test`, `CodexFrontendAdapterTest`, `CodexFrontendJsonLineFramerTest` | Corresponding file under `tests/component/codex/` | component/Codex/frontend | Frontend v1 protocol, adapter, and framing | Complementary frontend layers | KEEP | unchanged | Stable protocol/API behavior |
| `CodexBackendClientCommandTest`, `CodexBackendClientCommandDrainTest`, `CodexBackendClientPresenterTest`, `CodexBackendClientJsonLineFramerTest`, `CodexBackendClientStdinReaderTest` | Corresponding file under `tests/component/codex/` | component/Codex/client | Command parsing/drain, presentation, framing, and stdin lifecycle | Complementary client layers | KEEP | unchanged | Stable client behavior |
| `CodexBackendClientUnixAcceptanceTest`, `CodexBackendClientRealBackendAcceptanceTest`, `CodexBackendClientThreadWorkflowAcceptanceTest`, `CodexBackendUnixAcceptanceTest` | Corresponding acceptance source under `tests/component/codex/`; thread workflow shares `CodexBackendClientRealBackendAcceptanceTest.cpp` | component/Codex/acceptance | Unix frontend/backend and interactive/thread workflow acceptance | Complementary end-to-end modes | KEEP | unchanged | Stable integration behavior |
| `CodexGlobalShutdown_forced_polling_normal`, `CodexGlobalShutdown_forced_polling_ignore_sigterm`, `CodexGlobalShutdown_ignore_sigterm`, `CodexGlobalShutdown_destroy_client` | `tests/component/codex/CodexGlobalShutdownTest.cpp` | component/Codex/shutdown | Global shutdown across polling, signal-ignore, and client destruction | Complementary variants | KEEP | unchanged | Stable shutdown behavior |
| `CodexShutdownAdmissionTest` | `tests/component/codex/CodexShutdownAdmissionTest.cpp` | component/Codex/shutdown | Shutdown admission rejects or accepts work correctly | Complements global shutdown | KEEP | unchanged | Stable lifecycle contract |
| `CodexDescriptorCollision_stdin`, `CodexDescriptorCollision_stdout`, `CodexDescriptorCollision_stderr`, `CodexDescriptorCollision_all` | `tests/component/codex/CodexDescriptorCollisionTest.cpp` | component/Codex/descriptor | stdio descriptor collisions are handled | Complementary descriptors | KEEP | unchanged | Stable descriptor safety |
| `CodexStdioScenario_exit_before_initialize`, `CodexStdioScenario_close_stdout`, `CodexStdioScenario_initialization_error`, `CodexStdioScenario_malformed_json`, `CodexStdioScenario_launch_failure`, `CodexStdioScenario_post_spawn_setup_failure`, `CodexStdioScenario_registration_lifecycle_failure`, `CodexStdioScenario_registration_pidfd_fallback`, `CodexStdioScenario_registration_stdin_failure`, `CodexStdioScenario_registration_stdout_failure`, `CodexStdioScenario_registration_stderr_failure`, `CodexStdioScenario_bounded_overflow`, `CodexStdioScenario_slow_stdin`, `CodexStdioScenario_ignore_shutdown`, `CodexStdioScenario_blocked_shutdown`, `CodexStdioScenario_blocked_ignore_shutdown`, `CodexStdioScenario_stop_starting`, `CodexStdioScenario_stop_initializing`, `CodexStdioScenario_stdio_framing`, `CodexStdioScenario_close_stderr`, `CodexStdioScenario_recover_after_failure`, `CodexStdioScenario_forced_fallback_normal`, `CodexStdioScenario_forced_fallback_ignore_shutdown`, `CodexStdioScenario_forced_fallback_close_stdout`, `CodexStdioScenario_forced_fallback_exit_before_initialize` | `tests/component/codex/CodexStdioScenarioTest.cpp` | component/Codex/stdio | Launch, registration, framing, overflow, shutdown, recovery, and fallback failure scenarios | Complementary variants | KEEP | unchanged | Durable process, descriptor, and shutdown regressions |
| `CodexAppServerIntegrationTest`, `CodexTypedAppServerIntegrationTest` | Corresponding file under `tests/component/codex/` | integration/Codex | Optional real raw and typed app-server integration | Raw/typed variants | KEEP | unchanged | External integration coverage; skip 77 preserved |

## Consolidation-area inventory and decisions

| Current test name | Current source file | Category | Permanent contract | Overlap with other tests | Decision | Destination or replacement | Reason |
|---|---|---|---|---|---|---|---|
| `SemanticLoggerRound2Test` | `tests/unit/log/SemanticLoggerRound2Test.cpp` | unit/log/API/format | Owned materialization; JSON v1; deterministic text/ANSI/multiline/control handling; format and stream APIs; scope lifetime | Ownership overlaps `LogScopeOwnerRound3Test`; `sysError` overlaps disabled-path test | RENAME | `SemanticLoggerFormattingTest` | Contracts are permanent; historical name is not |
| `LogScopeOwnerRound3Test` | `tests/unit/log/LogScopeOwnerRound3Test.cpp` | unit/log/ownership | `LogScopeOwner` copy/move/lifetime/role/logger construction | Borrowed-string ownership complements formatter materialization | RENAME | `LogScopeOwnerTest` | Permanent ownership contract |
| `ProductionLogApiRound4Test` | `tests/unit/log/ProductionLogApiRound4Test.cpp` | unit/log/scope/API | `ConfigInstance`, connection, and context public logger APIs and structured scopes | Overlaps endpoint API and connection identity tests | MERGE | `SemanticLoggerScopeTest` | One authoritative structured-scope test |
| `SocketEndpointLogApiRound5Test` | `tests/unit/log/SocketEndpointLogApiRound5Test.cpp` | unit/log/scope/API | Server, client, connector, and acceptor logger API/scope | Overlaps production API and connector migration owner checks | MERGE | `SemanticLoggerScopeTest` | One authoritative structured-scope test |
| `SemanticBackendRound6Test` | `tests/unit/log/SemanticBackendRound6Test.cpp` | unit/log/backend | Semantic sink reaches backend; legacy numeric gating remains legacy-only; no-argument semantic logger uses semantic sink | Compatibility test repeats coexistence and sink checks | RENAME | `SemanticLoggerBackendTest` | Permanent backend ownership |
| `SemanticFilterRound7Test` | `tests/unit/log/SemanticFilterRound7Test.cpp` | unit/log/filter | Global/origin/boundary/component/instance precedence, `Off`, freeze, generation, format | Formatter assertions overlap formatting test | RENAME | `SemanticLoggerFilteringTest` | Permanent filter policy |
| `ControlledMigrationRound8Test` | `tests/unit/log/ControlledMigrationRound8Test.cpp` | unit/log/migration | Selected converted calls emit, filter, use JSON, carry `sysError`, and accept sinks | All generic contracts are stronger in backend/filter/format/disabled-path tests | DELETE_MIGRATION_ONLY | Named replacements in deletion map | Only chronology and selected migrated spellings are unique |
| `SemanticCompatibilityRound9Test` | `tests/unit/log/SemanticCompatibilityRound9Test.cpp` | unit/log/compatibility | Legacy `LOG`/`PLOG` and numeric thresholds coexist independently with semantic `Off` and accepted semantic output | Repeats formatting, sink, string lifetime, error fields, and Round 8 calls | RENAME | `SemanticLoggerCompatibilityTest`, trimmed to unique compatibility assertions | Compatibility remains public behavior |
| `SemanticOverheadRound9Test` | `tests/unit/log/SemanticOverheadRound9Test.cpp` | unit/log/overhead | Sink/`Off` suppression, no backend formatting, legacy threshold independence, guarded stream, threshold stability, scope lifetime | Duplicates overhead, backend, filtering, and formatter ownership tests | MERGE | `SemanticLoggerDisabledPathTest`, `SemanticLoggerBackendTest`, `SemanticLoggerFilteringTest`, `SemanticLoggerFormattingTest` | Remove repeated ownership of generic behavior |
| `SemanticLoggingOverheadTest` | `tests/unit/log/SemanticLoggingOverheadTest.cpp` | unit/log/overhead | Disabled format/stream evaluation, sink suppression, enabled exactly once, representative production helper | Overlaps Round 9 overhead and repair test | MERGE | `SemanticLoggerDisabledPathTest` | Single disabled-path authority |
| `SemanticProductionThresholdRepairTest` | `tests/unit/log/SemanticProductionThresholdRepairTest.cpp` | unit/log/overhead/system-error | Production connection/context suppression and enabled path; disabled/enabled `sysError`; explicit threshold overload | Overlaps both overhead tests and migration fixtures | RENAME | `SemanticLoggerDisabledPathTest` | Best base for authoritative generic plus one production-owner path |
| `SocketConnectionMigration01Test` | `tests/unit/log/SocketConnectionMigration01Test.cpp` | unit/log/core scope | Semantic connection identity is `getConnectionId()`, not fd or display name; instance and anonymous value remain present | Generic filter/JSON/error/sink/overhead checks overlap logger authorities | EXTRACT_THEN_DELETE | Identity assertions in `SemanticLoggerScopeTest` | Numeric identity is permanent; migrated-call checks are historical |
| `SocketConnectorAcceptorMigration02Test` | `tests/unit/log/SocketConnectorAcceptorMigration02Test.cpp` | unit/log/core lifecycle | Real attempt owner emits one start, one timeout, no late cancellation; Debug client endpoint identity | Generic owner/filter/JSON/error/sink/overhead overlaps logger authorities | EXTRACT_THEN_DELETE | `tests/unit/core/ConnectionAttemptLifecycleTest.cpp` | Exactly-once terminal state is a permanent lifecycle regression |
| `ListenerLifecyclePhase2Test` | `tests/unit/log/ListenerLifecyclePhase2Test.cpp` | unit/log/core lifecycle | Failed listener reports status/init once, one start failure, and no false start/stop | Static transport policy partly overlaps source structure only | MOVE_TO_SUBSYSTEM | `tests/unit/core/ListenerLifecycleTest.cpp` | Runtime listener behavior belongs to core |
| `SocketServerClientMigration03Test` | `tests/unit/log/SocketServerClientMigration03Test.cpp` | unit/log/migration | Selected server/client owners emit with roles plus generic logger behavior | Scope test and generic logger authorities are stronger | DELETE_MIGRATION_ONLY | `SemanticLoggerScopeTest` plus logger authorities | No unique lifecycle/protocol scenario |
| `EndpointLifetimeCountersTest` | `tests/unit/log/EndpointLifetimeCountersTest.cpp` | unit/log/core lifecycle | Endpoint terminal summaries, retry/reconnect counters, cancellation, IDs, and ordering | Complements listener and connection-attempt lifecycle | MOVE_TO_SUBSYSTEM | `tests/unit/core/EndpointLifetimeCountersTest.cpp` | Core ownership rather than logger behavior |
| `CoreRuntimeMigration04Test` | `tests/unit/log/CoreRuntimeMigration04Test.cpp` | unit/log/migration | Selected event-loop owner plus generic logging behavior | Backend/filter/format/disabled-path tests are stronger | DELETE_MIGRATION_ONLY | Logger authority tests | Synthetic owner emission has no unique runtime scenario |
| `NetPhysicalSocketMigration05Test` | `tests/unit/log/NetPhysicalSocketMigration05Test.cpp` | unit/log/migration | Selected physical socket owner plus generic logging behavior | Network component tests cover physical behavior; logger authorities cover mechanics | DELETE_MIGRATION_ONLY | Network component suite plus logger authorities | Synthetic logging payload is migration chronology |
| `TlsSocketStreamMigration06Test` | `tests/unit/log/TlsSocketStreamMigration06Test.cpp` | unit/log/TLS | Emitted OpenSSL helper drains queue; suppressed error/warning/info helpers do not drain it | Real TLS state-machine tests own handshake/SNI/close behavior; generic checks overlap logger authorities | EXTRACT_THEN_DELETE | `tests/unit/core/TlsOpenSslLoggingTest.cpp` | Error-queue side effects are unique observable behavior |
| `HttpClientMigration07aTest` | `tests/unit/log/HttpClientMigration07aTest.cpp` | unit/log/HTTP migration | Synthetic HTTP client/EventSource/upgrade payloads plus generic logger mechanics | HTTP/EventSource components own real behavior; logger authorities own mechanics | DELETE_MIGRATION_ONLY | HTTP/EventSource suites plus logger authorities | No unique runtime regression |
| `HttpServerMigration07bTest` | `tests/unit/log/HttpServerMigration07bTest.cpp` | unit/log/HTTP migration | Null request upgrade callback/status/close/no-context/no-factory/error identity safety | Other payload and generic logger checks overlap HTTP components and logger authorities | EXTRACT_THEN_DELETE | `tests/unit/http/HttpServerUpgradeMissingRequestTest.cpp` | Null dereference/correct terminal response is a permanent regression |
| `WebSocketMigration07cTest` | `tests/unit/log/WebSocketMigration07cTest.cpp` | unit/log/WebSocket migration | Synthetic subprotocol/frame/factory/lifecycle payloads plus generic mechanics | WebSocket component tests and logger authorities are stronger | DELETE_MIGRATION_ONLY | WebSocket component suite plus logger authorities | No unique real state-machine scenario |
| `MqttMigration08Test` | `tests/unit/log/MqttMigration08Test.cpp` | unit/log/MQTT migration | Synthetic MQTT payloads plus generic mechanics | MQTT validation/lifecycle and logger authorities are stronger | DELETE_MIGRATION_ONLY | MQTT unit/lifecycle tests plus logger authorities | No unique real protocol scenario |
| `FinalCleanupMigration09Test` | `tests/unit/log/FinalCleanupMigration09Test.cpp` | unit/log/migration | Synthetic DB/TLS/app payloads and generic filter/format/error/sink/overhead checks | All mechanics overlap stable logger authorities | DELETE_MIGRATION_ONLY | Stable logger authority tests | Selected payload preservation is migration chronology |
| `SemanticEndToEndOutputTest` | `tests/unit/log/SemanticEndToEndOutputTest.cpp` | unit/log/e2e | Final file/stdout semantic output integration | Complements pure format/backend tests | KEEP | unchanged | Observable end-to-end routing |
| `SemanticTerminalColorRoutingTest` | `tests/unit/log/SemanticTerminalColorRoutingTest.cpp` | unit/log/output | TTY color, plain file, JSON, escape safety, invalid presentation fallback, quiet routing | Formatter test owns grammar; this owns destination behavior | KEEP | unchanged | Observable and security-relevant terminal behavior |
| `ContextLifecyclePhase1Test` | `tests/unit/log/ContextLifecyclePhase1Test.cpp` | unit/log/core lifecycle | Real context attach/switch/detach ordering, reasons, callbacks, identity, and obsolete duplicate suppression | Complements framework shutdown context visibility | MOVE_TO_SUBSYSTEM | `tests/unit/core/SocketContextLifecycleTest.cpp` | Core lifecycle ownership |
| `SocketContextDetachPolicyTest` | `tests/unit/log/SocketContextDetachPolicyTest.cpp` | unit/log/source policy | Exact source spellings for detach calls | Runtime context lifecycle is stronger | REPLACE_WITH_RUNTIME_TEST | `SocketContextLifecycleTest` | Layout/spelling snapshot is not a durable independent contract |
| `TransportLifecyclePhase2PolicyTest` | `tests/unit/log/TransportLifecyclePhase2PolicyTest.cpp` | unit/log/source policy | Exact transport lifecycle source spellings | Runtime connection/listener/endpoint tests are stronger | REPLACE_WITH_RUNTIME_TEST | `ConnectionAttemptLifecycleTest`, `ListenerLifecycleTest`, `EndpointLifetimeCountersTest`, existing shutdown tests | Runtime state transitions are falsifiable without source snapshots |
| `Phase3LifecyclePolicyTest` | `tests/unit/log/Phase3LifecyclePolicyTest.cpp` | unit/log lifecycle source policy | Lifecycle exactly-once and identity behavior | Runtime HTTP/MQTT/Codex/WebSocket/EventSource lifecycle tests are stronger | REPLACE_WITH_RUNTIME_TEST | `HttpStreamingSourceFailureTest`, `MqttLifecycleTest`, `CodexBackendLifecycleTest`, `AppServerClientRequestLifecycleTest`, and existing WebSocket/EventSource lifecycle tests | Runtime scenarios protect observable lifecycle behavior without retaining message-spelling snapshots |
| `Phase3LifecyclePolicyTest` | `tests/unit/log/Phase3LifecyclePolicyTest.cpp` | logging architecture/API policy | MQTT logging/lifecycle helpers remain private; Codex backend event/state models exclude logging-only bookkeeping | Runtime lifecycle tests cannot protect API access or domain-model shape | EXTRACT_THEN_DELETE | `LoggingApiSurfacePolicyTest` | Durable API and model-pollution invariants require a focused structural policy |
| `Phase3ParameterlessSemanticLoggerPolicyTest` | `tests/unit/log/Phase3ParameterlessSemanticLoggerPolicyTest.cpp` | unit/log/architecture policy | Complete masked scan and exact justified allowlist for literal parameterless semantic logger calls | No runtime equivalent can prove whole-source classification | MOVE_TO_POLICY | `tests/policy/log/ParameterlessSemanticLoggerPolicyTest.cpp` | Durable explicit-scope architecture invariant |
| `Phase3CodexLifecycleTest` | `tests/unit/log/Phase3CodexLifecycleTest.cpp` | unit/log/Codex lifecycle | Codex thread/turn lifecycle exactly-once and identity behavior | Complements component protocol engine tests | MOVE_TO_SUBSYSTEM | `tests/component/codex/CodexBackendLifecycleTest.cpp` | Codex behavior belongs to Codex |
| `Phase3AppServerClientLifecycleTest` | `tests/unit/log/Phase3AppServerClientLifecycleTest.cpp` | unit/log/Codex lifecycle | App-server client request lifecycle exactly-once behavior | Complements Codex stdio and protocol tests | MOVE_TO_SUBSYSTEM | `tests/component/codex/AppServerClientRequestLifecycleTest.cpp` | Codex request lifecycle belongs to Codex |
| `Phase3MqttLifecycleTest` | `tests/unit/log/Phase3MqttLifecycleTest.cpp` | unit/log/MQTT lifecycle | MQTT session lifecycle exactly-once and identity behavior | Complements packet validation | MOVE_TO_SUBSYSTEM | `tests/unit/mqtt/MqttLifecycleTest.cpp` | MQTT behavior belongs to MQTT |
| `Phase3HttpStreamingSourceFailureTest` | `tests/unit/log/Phase3HttpStreamingSourceFailureTest.cpp` | unit/log/HTTP lifecycle | One request start and failure; no completion/abort/late EOF terminal; close and matching identity | No equivalent HTTP component failure source test | MOVE_TO_SUBSYSTEM | `tests/unit/http/HttpStreamingSourceFailureTest.cpp` | Permanent HTTP lifecycle regression |
| `SensitiveLoggingRedactionTest` | `tests/unit/log/SensitiveLoggingRedactionTest.cpp` | unit/log/security source scan | Prevent credentials, tokens, passwords, private material, MQTT/database secrets in logging calls | Existing exact fragments are too narrow; no generic redaction layer exists | MOVE_TO_POLICY | `tests/policy/security/SensitiveLoggingPolicyTest.cpp`, broadened and documented | Security invariant must remain explicit without claiming runtime redaction |
| `HighFrequencyLoggingSeverityTest` | `tests/unit/log/HighFrequencyLoggingSeverityTest.cpp` | unit/log/source policy | Exact severity/source spelling for selected hot calls | Disabled-path/cache tests own overhead; subsystem tests own behavior | DELETE_MIGRATION_ONLY | `SemanticLoggerDisabledPathTest`, `SemanticLoggerCacheTest` | Exact migrated severity locations are not durable |
| `InnerEpollDiagnosticsTest` | `tests/unit/log/InnerEpollDiagnosticsTest.cpp` | unit/log/core policy | Epoll return checks, errno capture, EEXIST/EBADF handling, underflow prevention, and non-success diagnostics | Descriptor failure runtime test covers one path only | MOVE_TO_POLICY | `tests/policy/core/EpollDescriptorPublisherPolicyTest.cpp` | Remaining deterministic structural invariants are broader than logging |
| `BroaderSyscallDisciplineTest` | `tests/unit/log/BroaderSyscallDisciplineTest.cpp` | unit/log/core policy | Signal ordering/restoration, monitor returns, syscall wrappers, and failure diagnostics | Component core tests cover some runtime paths | MOVE_TO_POLICY | `tests/policy/core/EventLoopSyscallDisciplinePolicyTest.cpp` | Durable correctness policy belongs to core |
| `SemanticCliIntegrationTest` | `tests/unit/log/SemanticCliIntegrationTest.cpp` | unit/log/CLI | Parsing, validation, precedence, freeze, and semantic filtering | Makes source option-literal scan redundant | KEEP | unchanged | Observable CLI behavior |
| `SemanticCliSourcePolicyTest` | `tests/unit/log/SemanticCliSourcePolicyTest.cpp` | unit/log/source policy | Presence of selected option literals in source | Runtime CLI integration is stronger | DELETE_REDUNDANT | `SemanticCliIntegrationTest` | Exact option spelling/location is not a durable separate contract |
| `SemanticCachedLoggerOverheadTest` | `tests/unit/log/SemanticCachedLoggerOverheadTest.cpp` | unit/log/cache plus source policy | MQTT cached thresholds/override/copy plus exact member/call spellings | Runtime portion overlaps structural cache test; source layout is brittle | MERGE | `SemanticLoggerCacheTest` | Preserve observable cache behavior only |
| `SemanticStructuralLoggerCacheTest` | `tests/unit/log/SemanticStructuralLoggerCacheTest.cpp` | unit/log/cache plus source policy | Thresholds, overrides, generation refresh plus exact private storage layout | Runtime portion complements cached MQTT path; structural details overlap implementation | RENAME | `SemanticLoggerCacheTest`, stripped of private-name snapshots | Cache contract is behavior, not member spelling |
| `ScopedSemanticLoggingMigrationTest` | `tests/unit/log/ScopedSemanticLoggingMigrationTest.cpp` | unit/log/migration | Selected helpers carry scopes and selected production source calls use them | Scope test and parameterless policy are stronger | DELETE_MIGRATION_ONLY | `SemanticLoggerScopeTest`, `ParameterlessSemanticLoggerPolicyTest` | Selected-call chronology is not permanent |

## Discovery results

- Registered tests with missing sources: none.
- Test source files no longer registered: none. Four Codex scenario sources
  and the three stream/TLS/shutdown scenario sources have a basename that is
  not itself a CTest name; each is registered through an explicit CMake
  scenario loop. The five typed Codex codec/decoder sources are registered by
  the `typed_test` loop.
- Unused support headers: none.
- `tests/support/TestResult.h` is shared broadly.
- `tests/support/DescriptorRegistrationFailure.h` is used by three
  descriptor-registration tests.
- The former `tests/support/Phase3SemanticLogCapture.h` had ten consumers and
  was the only shared semantic JSON capture helper. It is now
  `tests/support/SemanticLogCapture.h`; every consumer uses the stable name and
  no duplicate helper was introduced.
- Logger-state guards, temporary paths, JSON parsing, and expensive-argument
  fixtures are heavily duplicated in migration tests. Consolidating generic
  logger behavior removes most of those fixtures. The remaining lifecycle
  tests will use the shared semantic capture helper where practical.
- Misplaced tests under `tests/unit/log`: core connection/listener/context and
  endpoint lifecycle, TLS/OpenSSL, HTTP, MQTT, Codex, epoll, syscall, and
  security-policy tests. Their destinations are recorded above.

## Deleted-test assertion disposition

Every survivor named below was built and run before its historical source was
deleted.

| Deleted current test | Meaningful assertion | Exact survivor |
|---|---|---|
| `SemanticLoggerRound2Test` | Borrowed scope strings are materialized; JSON v1 fields/omission/escaping; deterministic text, ANSI, multiline, control sanitization, level labels; format and stream APIs; scope lifetime | `SemanticLoggerFormattingTest`; owner copy/move lifetime is independently protected by `LogScopeOwnerTest` |
| `LogScopeOwnerRound3Test` | Owner construction, owned materialization, role, copy, move, lifetime, and logger construction | `LogScopeOwnerTest` |
| `ProductionLogApiRound4Test` | `ConfigInstance`, `SocketConnection`, and application/framework `SocketContext` structured fields | `SemanticLoggerScopeTest` |
| `SocketEndpointLogApiRound5Test` | `SocketServer`, `SocketClient`, `SocketConnector`, and `SocketAcceptor` structured fields | `SemanticLoggerScopeTest` |
| `SemanticBackendRound6Test` | Semantic backend delivery, no legacy numeric re-filtering, legacy/semantic coexistence, and production no-argument sink | `SemanticLoggerBackendTest` |
| `SemanticFilterRound7Test` | Global/origin/boundary/component/instance thresholds, precedence, `Off`, freeze, generation, and format selection | `SemanticLoggerFilteringTest` |
| `ControlledMigrationRound8Test` | Context/connection helpers emit through semantic backend | Migration-only selected-call evidence; generic backend delivery survives in `SemanticLoggerBackendTest` |
| `ControlledMigrationRound8Test` | Global filter suppresses; accepted semantic output is not legacy-double-gated | `SemanticLoggerFilteringTest`; `SemanticLoggerBackendTest` |
| `ControlledMigrationRound8Test` | JSON v1 selection | `SemanticLoggerFormattingTest` and format selection in `SemanticLoggerFilteringTest` |
| `ControlledMigrationRound8Test` | `sysError` code/text and sink overload | `SemanticLoggerDisabledPathTest`; `SemanticLoggerBackendTest` |
| `SemanticCompatibilityRound9Test` | `LOG`/`PLOG`, numeric legacy thresholds, semantic `Off` independence, and accepted semantic output independence | `SemanticLoggerCompatibilityTest` |
| `SemanticCompatibilityRound9Test` | Repeated schema, formatting, sink, borrowed-string, `sysError`, and selected migrated-call assertions | `SemanticLoggerFormattingTest`, `SemanticLoggerBackendTest`, `SemanticLoggerDisabledPathTest`; selected-call evidence is migration-only |
| `SemanticOverheadRound9Test` | Sink/`Off` suppression, guarded stream evaluation, accepted semantic independence, stable thresholds, and scope lifetime | `SemanticLoggerDisabledPathTest`, `SemanticLoggerBackendTest`, `SemanticLoggerFilteringTest`, `SemanticLoggerFormattingTest` |
| `SemanticLoggingOverheadTest` | Disabled format/stream arguments, zero sink calls, enabled evaluation exactly once, and representative production-owner suppression | `SemanticLoggerDisabledPathTest` |
| `SemanticProductionThresholdRepairTest` | Connection/context enabled and disabled paths, disabled/enabled `sysError`, and explicit threshold overload | `SemanticLoggerDisabledPathTest` |
| `SocketConnectionMigration01Test` | Numeric `getConnectionId()` is used instead of fd/display name; instance and anonymous value survive | `SemanticLoggerScopeTest` |
| `SocketConnectionMigration01Test` | Filter, JSON, `sysError`, sink, and disabled formatting | `SemanticLoggerFilteringTest`, `SemanticLoggerFormattingTest`, `SemanticLoggerDisabledPathTest`, `SemanticLoggerBackendTest` |
| `SocketConnectorAcceptorMigration02Test` | One real attempt start, one timeout, no late cancellation; Debug client endpoint identity | `ConnectionAttemptLifecycleTest` |
| `SocketConnectorAcceptorMigration02Test` | Connector/acceptor structured scope | `SemanticLoggerScopeTest` |
| `SocketConnectorAcceptorMigration02Test` | Generic filter/backend/JSON/error/sink/disabled path | Stable logger authority tests named above |
| `ListenerLifecyclePhase2Test` | Failed listener status/init, one start failure, and no false start/stop terminal | `ListenerLifecycleTest` |
| `SocketServerClientMigration03Test` | Server/client structured roles and instances | `SemanticLoggerScopeTest` |
| `SocketServerClientMigration03Test` | Generic logger mechanics | Stable logger authority tests named above |
| `EndpointLifetimeCountersTest` | Endpoint terminal summaries, retry/reconnect/cancellation counters, identity, and ordering | `EndpointLifetimeCountersTest` under `tests/unit/core/` |
| `CoreRuntimeMigration04Test` | Event-loop selected owner emits | Migration-only; event-loop cache behavior survives in `SemanticLoggerCacheTest` and runtime core behavior in core component tests |
| `CoreRuntimeMigration04Test` | Generic logger mechanics | Stable logger authority tests named above |
| `NetPhysicalSocketMigration05Test` | Selected physical-socket diagnostic emits | Migration-only; physical socket behavior survives in `InetPhysicalServerEffectiveBindAddressTest` and `UnixPhysicalSocketPathSafetyTest` |
| `NetPhysicalSocketMigration05Test` | Generic logger mechanics | Stable logger authority tests named above |
| `TlsSocketStreamMigration06Test` | Emitted `ssl_log_*` drains the OpenSSL error queue | `TlsOpenSslLoggingTest` |
| `TlsSocketStreamMigration06Test` | Suppressed error/warning/info helpers do not drain the OpenSSL queue | `TlsOpenSslLoggingTest` |
| `TlsSocketStreamMigration06Test` | Synthetic SNI/handshake/shutdown/payload and generic logger checks | Migration-only synthetic text; real TLS behavior survives in `TLSTransportStateMachineTest`, `TLSIoFatalPathTest`, `TLSLifecycleHelperOwnershipTest`, and TLS shutdown tests; mechanics survive in logger authority tests |
| `HttpClientMigration07aTest` | Synthetic request/response/upgrade/EventSource payload formatting | Migration-only; real HTTP and EventSource component tests remain |
| `HttpClientMigration07aTest` | Generic logger mechanics | Stable logger authority tests named above |
| `HttpServerMigration07bTest` | Null request invokes callback once with empty protocol; status 500 and `Connection: close`; no context switch/factory; one bound-identity error; safe null handling | `HttpServerUpgradeMissingRequestTest` |
| `HttpServerMigration07bTest` | Synthetic HTTP payloads and generic logger mechanics | Migration-only payload evidence; real HTTP component tests and stable logger authority tests remain |
| `WebSocketMigration07cTest` | Synthetic subprotocol/frame/factory/ping payloads | Migration-only; real WebSocket component tests remain |
| `WebSocketMigration07cTest` | Generic logger mechanics and guarded hex dump | Stable logger authority tests, especially `SemanticLoggerDisabledPathTest` and `SemanticTerminalColorRoutingTest` |
| `MqttMigration08Test` | Synthetic client/server/broker/session/packet/WebSocket payloads | Migration-only; `Mqtt311PacketValidationTest` and `MqttLifecycleTest` remain |
| `MqttMigration08Test` | Generic logger mechanics | Stable logger authority tests |
| `FinalCleanupMigration09Test` | Synthetic DB/TLS/app payload strings | Migration-only selected payload preservation |
| `FinalCleanupMigration09Test` | Backend/filter/JSON/error/sink/disabled/dump behavior | Stable logger authority tests |
| `ContextLifecyclePhase1Test` | Context attach/switch/detach ordering, reasons, callbacks, identity, and duplicate-terminal suppression | `SocketContextLifecycleTest` |
| `SocketContextDetachPolicyTest` | Detach reason and ordering | `SocketContextLifecycleTest` runtime assertions |
| `TransportLifecyclePhase2PolicyTest` | Attempt/listener/endpoint terminal uniqueness and ordering | `ConnectionAttemptLifecycleTest`, `ListenerLifecycleTest`, `EndpointLifetimeCountersTest`, and stream/TLS shutdown suites |
| `Phase3LifecyclePolicyTest` | Lifecycle exactly-once and identity behavior | `HttpStreamingSourceFailureTest`, `MqttLifecycleTest`, `CodexBackendLifecycleTest`, `AppServerClientRequestLifecycleTest`, and existing WebSocket/EventSource component lifecycle tests |
| `Phase3LifecyclePolicyTest` | MQTT logging/lifecycle helpers remain private; Codex backend events/state exclude logging-only lifecycle bookkeeping | `LoggingApiSurfacePolicyTest` |
| `Phase3ParameterlessSemanticLoggerPolicyTest` | Masked whole-source scan, exact stable-marker allowlist, duplicate/stale/missing/reason validation, unresolved-category rejection, and negative self-tests | `ParameterlessSemanticLoggerPolicyTest` |
| `Phase3CodexLifecycleTest` | Codex backend thread/turn exactly-once transitions and identity | `CodexBackendLifecycleTest` |
| `Phase3AppServerClientLifecycleTest` | App-server client request exactly-once transitions | `AppServerClientRequestLifecycleTest` |
| `Phase3MqttLifecycleTest` | MQTT session exactly-once transitions and identity | `MqttLifecycleTest` |
| `Phase3HttpStreamingSourceFailureTest` | One start/one failure, no completion/abort/late-EOF terminal, close, and matching request/connection identity | `HttpStreamingSourceFailureTest` |
| `SensitiveLoggingRedactionTest` | Direct credentials, tokens, passwords, private material, MQTT credentials, and database secrets must not appear in log expressions | `SensitiveLoggingPolicyTest`, broadened to the complete production source tree and explicitly documented as a static guard rather than a runtime redaction layer |
| `HighFrequencyLoggingSeverityTest` | Hot disabled diagnostics avoid work | `SemanticLoggerDisabledPathTest` and `SemanticLoggerCacheTest` runtime assertions |
| `InnerEpollDiagnosticsTest` | Return checks, immediate errno capture, EEXIST/EBADF branches, interest-count underflow prevention, and no success diagnostic after setup failure | `EpollDescriptorPublisherPolicyTest`; the executable descriptor-registration failure path remains in `DescriptorRegistrationFailureTest` |
| `BroaderSyscallDisciplineTest` | Signal setup/restoration ordering, monitor return handling, syscall-wrapper checks, and failure diagnostics | `EventLoopSyscallDisciplinePolicyTest` plus existing core component tests |
| `SemanticCliSourcePolicyTest` | Semantic CLI options exist | `SemanticCliIntegrationTest` parsing, validation, precedence, freeze, and filtering assertions |
| `SemanticCachedLoggerOverheadTest` | Cached MQTT logger threshold, component override, expensive argument, and copy validity | `SemanticLoggerCacheTest` |
| `SemanticCachedLoggerOverheadTest` | Exact `log_` member/helper spellings | Intentionally deleted implementation-layout assertions; observable cache behavior is stronger and storage is not ABI |
| `SemanticStructuralLoggerCacheTest` | Thresholds, component/instance overrides, generation refresh, pre-freeze safety | `SemanticLoggerCacheTest` |
| `SemanticStructuralLoggerCacheTest` | Exact optional member names, generation member names, access placement, and local expressions | Intentionally deleted layout assertions; no documented ABI requires these private spellings |
| `ScopedSemanticLoggingMigrationTest` | Structured production scopes | `SemanticLoggerScopeTest` |
| `ScopedSemanticLoggingMigrationTest` | Selected production source calls are scoped | Migration-only selected-call snapshot; whole-tree literal parameterless call classification survives in `ParameterlessSemanticLoggerPolicyTest` |

## Final suite and validation

- Registered tests: 236
- Expected passing tests: 235
- Expected skipped tests: 1 (`CodexTypedAppServerIntegrationTest`, return
  code 77)
- Historical registered names and labels: none
- Missing registered sources: none
- Orphan test sources: none; the seven scenario-source basename exceptions
  and five typed Codex sources are registered through CMake loops as described
  above
- Unused support headers: none
- Production files changed: none
- Shared capture helper: `tests/support/SemanticLogCapture.h`, with all former
  consumers updated

The final policy suite contains:

- `ParameterlessSemanticLoggerPolicyTest` (`policy;log;architecture`);
- `LoggingApiSurfacePolicyTest` (`policy;log;architecture;api`);
- `EpollDescriptorPublisherPolicyTest` (`policy;core;epoll`);
- `EventLoopSyscallDisciplinePolicyTest` (`policy;core;syscall`);
- `SensitiveLoggingPolicyTest` (`policy;security;log`).

`SensitiveLoggingPolicyTest` recursively scans production C++ sources,
extracts logging statements, masks string-literal labels, permits only
boolean/length metadata access for known secret values, and rejects direct
secret expressions. It is a secondary static security guard: SNode.C does not
have a generic runtime redaction layer, and the test does not claim otherwise.

Validation performed:

| Validation | Result |
|---|---|
| GCC 15.3 Debug configure and complete build | Pass |
| Consolidated replacement targets | 22/22 pass |
| All `log` and `policy` tests | 23/23 pass |
| Core, HTTP, WebSocket, EventSource, MQTT, Codex, TLS, shutdown, ABI, and install label union | 161 pass, 1 expected skip |
| `StagedInstalledConsumerTest` | Pass |
| `TLSLifecycleAbiSourceCompatibilityTest` | Pass |
| Clang 21.1 focused build, strict project flags | Blocked in unchanged production sources: `Chunked.cpp` raises `-Wtautological-type-limit-compare`, and `ItemDecoder.cpp` raises `-Wnrvo`; production was not changed |
| Clang 21.1 focused build with only those diagnostics demoted | Build pass; 22/22 focused tests pass |
| ASan plus LSan, five required lifecycle/regression targets | 5/5 pass; LSan required execution outside the ptrace sandbox |
| UBSan, five required lifecycle/regression targets | 5/5 pass with `vptr` excluded; vptr instrumentation introduces an unresolved circular RTTI link from `core-socket-stream` to `ConfigInstance` |
| Complete GCC CTest suite | 235 pass, 1 expected skip, 0 failures (236 registered) |
| `git diff --check` | Pass |

CI state is recorded in the pull request after the committed branch is pushed.
