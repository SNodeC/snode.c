# Semantic logging migration 08 — MQTT

## Implementation summary

This is the complete Migration 8 in one PR. It is a production migration, not inventory-only, and it modifies production MQTT logging under `src/iot/mqtt`. The branch was prepared from the current repository tip that contains merge commit `9b052c8` for PR #161, so Migration 7a HTTP client, Migration 7b HTTP server/upgrade, and Migration 7c WebSocket are already merged. This PR does not duplicate previous migrations and does not split Migration 8.

## Exact changed files

- `docs/logging/semantic-migration-08-mqtt-report.md`
- `src/iot/mqtt/SemanticLog.h`
- `src/iot/mqtt/Mqtt.cpp`
- `src/iot/mqtt/SubProtocol.hpp`
- `src/iot/mqtt/client/Mqtt.cpp`
- `src/iot/mqtt/server/Mqtt.cpp`
- `src/iot/mqtt/server/broker/Broker.cpp`
- `src/iot/mqtt/server/broker/RetainTree.cpp`
- `src/iot/mqtt/server/broker/Session.cpp`
- `src/iot/mqtt/server/broker/SubscriptionTree.cpp`
- `tests/unit/log/CMakeLists.txt`
- `tests/unit/log/MqttMigration08Test.cpp`

## Focused MQTT inventory command/result

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" src/iot/mqtt -g '*.h' -g '*.hpp' -g '*.cpp'
```

Focused inventory result before migration: 181 call sites.

Complete focused inventory by file:

| File | Count |
| --- | ---: |
| `src/iot/mqtt/Mqtt.cpp` | 46 |
| `src/iot/mqtt/client/Mqtt.cpp` | 19 |
| `src/iot/mqtt/server/Mqtt.cpp` | 35 |
| `src/iot/mqtt/server/broker/Broker.cpp` | 11 |
| `src/iot/mqtt/server/broker/RetainTree.cpp` | 23 |
| `src/iot/mqtt/server/broker/Session.cpp` | 9 |
| `src/iot/mqtt/server/broker/SubscriptionTree.cpp` | 30 |
| `src/iot/mqtt/SubProtocol.hpp` | 8 |

## Focused inventory classification table

| Class | Scope | Files / representative payloads | Count |
| --- | --- | --- | ---: |
| A | MQTT core / MqttContext / SocketContext lifecycle | `Mqtt.cpp`: connected, disconnected, keep-alive, packet availability and parse errors | 12 |
| B | MQTT client lifecycle and control-packet flow | `client/Mqtt.cpp`: session store, CONNACK, SUBACK/UNSUBACK, CONNECT/SUBSCRIBE/UNSUBSCRIBE | 19 |
| C | MQTT server lifecycle and control-packet flow | `server/Mqtt.cpp`: session selection, CONNECT protocol fields, SUBSCRIBE/UNSUBSCRIBE packet ids/topics | 35 |
| D | MQTT broker/session/subscription/retained-message flow | `server/broker/*.cpp`: session store, retained releases, subscriptions, publish distribution | 73 |
| E | MQTT packet parsing/serialization/fixed-header diagnostics | `Mqtt.cpp`: send/receive packet names, fixed header, variable header/payload dumps | 19 |
| F | MQTT topic/filter/matching diagnostics | `SubscriptionTree.cpp`, `RetainTree.cpp`: topic filters, wildcard placement, matches | 15 |
| G | MQTT-over-WebSocket subprotocol logging inside `src/iot/mqtt` | `SubProtocol.hpp`: connect, opcode, frame data, message end/error, signal exit | 8 |
| H | PLOG / errno-sensitive call sites | `client/Mqtt.cpp`, `server/broker/Broker.cpp` session-store read/write failures | 4 |
| I | VLOG / verbose diagnostics | None found in focused inventory | 0 |
| J | Deferred/unclear call sites | None | 0 |

## Ownership discovery summary

Command:

```sh
rg -n "\blog\(\)|logger::|BoundaryLogger|LogScopeOwner|semantic|scope|getInstanceName|getConnectionName|Mqtt|MQTT|Session|SocketContext|SubProtocol|Broker|Topic|ControlPacket|FixedHeader" src/iot/mqtt -g '*.h' -g '*.hpp' -g '*.cpp'
```

The MQTT classes carry useful payload identity such as `connectionName`, `clientId`, session identifiers, packet identifiers, topics, filters, packet types, and WebSocket subprotocol names, but they did not expose existing semantic owners. To avoid broad constructor/API churn, Migration 8 adds a single MQTT-local helper header, `src/iot/mqtt/SemanticLog.h`, with framework/connection scoped helpers for `iot.mqtt`, `iot.mqtt.client`, `iot.mqtt.server`, `iot.mqtt.broker`, `iot.mqtt.session`, `iot.mqtt.packet`, `iot.mqtt.topic`, and `iot.mqtt.websocket`.

The user-requested extended general review covered SNode.C migration boundaries and mqttsuite references: no external mqttsuite repository is present in this checkout, and no external mqttsuite files were changed.

## Migrated call-site table

| Area | Files | Migrated call sites | Semantic helper |
| --- | --- | ---: | --- |
| MQTT core/packet flow | `src/iot/mqtt/Mqtt.cpp` | 46 | `mqttLog()` |
| MQTT client | `src/iot/mqtt/client/Mqtt.cpp` | 19 | `mqttClientLog()` |
| MQTT server | `src/iot/mqtt/server/Mqtt.cpp` | 35 | `mqttServerLog()` |
| Broker persistence and publish | `src/iot/mqtt/server/broker/Broker.cpp` | 11 | `mqttBrokerLog()` |
| Retained messages | `src/iot/mqtt/server/broker/RetainTree.cpp` | 23 | `mqttBrokerLog()` |
| Broker session delivery | `src/iot/mqtt/server/broker/Session.cpp` | 9 | `mqttSessionLog()` |
| Subscription tree | `src/iot/mqtt/server/broker/SubscriptionTree.cpp` | 30 | `mqttBrokerLog()` |
| MQTT-over-WebSocket | `src/iot/mqtt/SubProtocol.hpp` | 8 | `mqttWebSocketLog()` |
| Total |  | 181 |  |

## Deferred call-site table

No focused MQTT call sites were deferred. No VLOG call sites were present.

## Semantic owner/helper used

`src/iot/mqtt/SemanticLog.h` is the single MQTT-local semantic helper. It uses `logger::LogOrigin::Framework` and `logger::LogBoundary::Connection` with component names:

- `iot.mqtt`
- `iot.mqtt.client`
- `iot.mqtt.server`
- `iot.mqtt.broker`
- `iot.mqtt.session`
- `iot.mqtt.packet`
- `iot.mqtt.topic`
- `iot.mqtt.websocket`

Each helper has a production sink overload and a sink-taking overload for tests and compatibility.

## Severity mapping

- `LOG(TRACE)` -> `trace()`
- `LOG(DEBUG)` -> `debug()`
- `LOG(INFO)` -> `info()`
- `LOG(WARNING)` -> `warn()`
- `LOG(ERROR)` -> `error()`
- `LOG(FATAL)` -> `critical()`; no focused MQTT `LOG(FATAL)` call sites were present.
- `PLOG(WARNING)` and `PLOG(ERROR)`/`PLOG(DEBUG)` -> `sysError(logger::LogLevel::Warn/Error/Debug, errnum, ...)`.

## PLOG/sysError errno handling

Four errno-sensitive `PLOG` call sites were migrated:

- MQTT client session-store read failure: captures `errno` immediately and emits `sysError(Warn, errnum, ...)`.
- MQTT client session-store write failure: captures `errno` immediately and emits `sysError(Debug, errnum, ...)`.
- MQTT broker session-store read failure: captures `errno` immediately and emits `sysError(Warn, errnum, ...)`.
- MQTT broker session-store write failure: captures `errno` immediately and emits `sysError(Error, errnum, ...)`.

`MqttMigration08Test` verifies errno code/text preservation through the MQTT broker helper.

## VLOG result

No `VLOG` call sites were found in the focused MQTT inventory. No verbose logging semantics were changed.

## MQTT payload preservation notes

Migration 8 preserves the existing MQTT technical diagnostics: connection name, client id, session state, protocol name/version, connect flags, keep-alive, will fields, username/password where already logged, packet identifiers, packet types, fixed-header flags and remaining length, topic names/filters, QoS, DUP, RETAIN, retained-message state, broker/session identity, publish/subscribe/unsubscribe/ack flow, ping lifecycle, disconnect lifecycle, and parse/deserialization failure reasons. Manual `MQTT`, `MQTT Client`, and `MQTT Broker` prefixes remain where they carry protocol readability beyond the semantic component.

## MQTT-over-WebSocket notes

The WebSocket subprotocol call sites inside `src/iot/mqtt/SubProtocol.hpp` were migrated to `iot.mqtt.websocket`. This PR did not modify `src/web/http` or `src/web/websocket` framework files.

## Expensive payload / dump guard notes

Hex-dump and serialized-packet payloads that call `toHexString(...)` or `serializeVP()` are guarded with `enabled(...)` checks before constructing expensive strings. Retained-message and subscription-message dumps are guarded at the corresponding info/debug levels. The MQTT-over-WebSocket `WsMqtt: Frame Data` dump in `src/iot/mqtt/SubProtocol.hpp` is guarded at `Debug` before constructing `std::vector<char>`, calling `utils::hexDump(...)`, or building the padded string. `MqttMigration08Test` includes explicit disabled expensive-dump guard assertions, including a WebSocket frame-dump guard simulation.

## Post-migration inventory command/result

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" src/iot/mqtt -g '*.h' -g '*.hpp' -g '*.cpp'
```

Post-migration result: no remaining `LOG`, `PLOG`, or `VLOG` call sites in `src/iot/mqtt`. All suitable focused MQTT call sites are migrated.

## Filter/backend/format compatibility

`MqttMigration08Test` covers:

- MQTT semantic logger emission when enabled.
- Framework/connection component scope for `iot.mqtt` and narrower helpers.
- `LogManager` filtering suppression.
- `Logger::setLogLevel` backend filtering suppression.
- JSON v1 format selection.
- `sysError` errno code/text preservation.
- Sink-taking helper overload compatibility.
- Suppressed production formatting not evaluating `ExpensiveValue`.
- Representative client/server/broker/session/packet/topic/WebSocket payload preservation.
- Expensive payload/dump guard behavior.

## Production-scope confirmations

- This PR changes only allowed Migration 8 files.
- This PR is not inventory-only.
- This PR modifies production MQTT logging.
- This PR does not split Migration 8.
- This PR does not modify `SemanticLogger.*`.
- This PR does not modify `LogScopeOwner.*`.
- This PR does not modify `Logger.*`.
- This PR does not modify `ConfigInstance.cpp`.
- This PR does not modify `src/net/config/stream/tls`.
- This PR does not modify `src/web/http`.
- This PR does not modify `src/web/websocket`.
- This PR does not modify DB/apps/examples.
- This PR does not modify external mqttsuite repository.
- This PR does not modify previous migration tests except `tests/unit/log/CMakeLists.txt` registration.
- This PR does not change `LOG`/`PLOG`/`VLOG` macro definitions.
- This PR does not change `LogManager` precedence.
- This PR does not change `LogManager` freeze behavior.
- This PR does not change JSON v1 schema.
- This PR does not start Round 10.

## Build commands run

- `cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON`
- `cmake --build cmake-build --parallel 2 --target MqttMigration08Test`
- `cmake --build cmake-build --parallel 2`

The first configure attempt failed because `nlohmann-json3-dev` was missing; it was installed with `apt-get update && apt-get install -y nlohmann-json3-dev`, after which configure and build completed.

## Test commands run

- `ctest --test-dir cmake-build -R "SemanticLoggerRound2Test|LogScopeOwnerRound3Test|ProductionLogApiRound4Test|SocketEndpointLogApiRound5Test|SemanticBackendRound6Test|SemanticFilterRound7Test|ControlledMigrationRound8Test|SemanticCompatibilityRound9Test|SemanticOverheadRound9Test|SemanticProductionThresholdRepairTest|SocketConnectionMigration01Test|SocketConnectorAcceptorMigration02Test|SocketServerClientMigration03Test|CoreRuntimeMigration04Test|NetPhysicalSocketMigration05Test|TlsSocketStreamMigration06Test|HttpClientMigration07aTest|HttpServerMigration07bTest|WebSocketMigration07cTest|MqttMigration08Test" --output-on-failure`
- `ctest --test-dir cmake-build --output-on-failure`

## ASan result

ASan commands were run for the focused MQTT migration test:

- `cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON`
- `cmake --build cmake-build-asan --parallel 2 --target MqttMigration08Test`
- `ASAN=$(gcc -print-file-name=libasan.so); LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan -R MqttMigration08Test --output-on-failure`

Full ASan was not run because the non-ASan full suite is already large in this environment; the required focused ASan test passed.

## Known follow-ups

- Migration 9 — DB / apps / examples / remaining cleanup.
- Round 10 — book integration.
- TLS configuration/SNI policy logs under `src/net/config/stream/tls` remain deferred outside Migration 8.
