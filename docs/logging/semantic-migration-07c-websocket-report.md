# Semantic logging migration 07c — WebSocket

## Implementation summary

This is a production follow-up inside PR #161, not inventory-only. PR #161 now contains Migration 7a — HTTP client, Migration 7b — HTTP server and upgrade, and Migration 7c — WebSocket. This continues from the existing PR #161 branch/worktree with the 7a and 7b files already present.

Migration 7c migrates production WebSocket common/client/server/subprotocol logging under `src/web/websocket` to semantic `BoundaryLogger` helpers without changing logging infrastructure.

## Exact changed files for the 7c follow-up

- `docs/logging/semantic-migration-07c-websocket-report.md`
- `src/web/websocket/SemanticLog.h`
- `src/web/websocket/SocketContextUpgrade.hpp`
- `src/web/websocket/Receiver.cpp`
- `src/web/websocket/Transmitter.cpp`
- `src/web/websocket/SubProtocol.hpp`
- `src/web/websocket/SubProtocolFactorySelector.hpp`
- `src/web/websocket/client/SubProtocolFactorySelector.cpp`
- `src/web/websocket/server/SubProtocolFactorySelector.cpp`
- `tests/unit/log/CMakeLists.txt`
- `tests/unit/log/WebSocketMigration07cTest.cpp`

## Focused 7c WebSocket inventory

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" \
  src/web/websocket \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

Result: 24 pre-migration LOG call sites, 0 PLOG call sites, 0 VLOG call sites.

Complete focused inventory:

```text
src/web/websocket/SubProtocolFactorySelector.hpp:70:                    LOG(DEBUG) << "WebSocket: SubProtocolFactory create success: " << subProtocolName;
src/web/websocket/SubProtocolFactorySelector.hpp:72:                    LOG(DEBUG) << "WebSocket: SubProtocolFactory create failed: " << subProtocolName;
src/web/websocket/SubProtocolFactorySelector.hpp:76:                LOG(DEBUG) << "WebSocket: Optaining function \"" << subProtocolFactoryFunctionName
src/web/websocket/SubProtocolFactorySelector.hpp:93:            LOG(DEBUG) << "WebSocket subprotocol: plugin '" << subProtocolName << "' selected from dynamic cache";
src/web/websocket/SubProtocolFactorySelector.hpp:98:            LOG(DEBUG) << "WebSocket subprotocol: plugin '" << subProtocolName << "' selected from static cache";
src/web/websocket/SubProtocolFactorySelector.hpp:103:            LOG(DEBUG) << "WebSocket subprotocol: plugin '" << subProtocolName << "' loaded and added to dynamic cache";
src/web/websocket/SubProtocolFactorySelector.hpp:105:            LOG(WARNING) << "WebSocket subprotocol: plugin '" << subProtocolName << "' not found";
src/web/websocket/SocketContextUpgrade.hpp:118:            LOG(DEBUG) << this->getSocketConnection()->getConnectionName() << " WebSocketContext: Subprotocol '" << subProtocol->name
src/web/websocket/SocketContextUpgrade.hpp:189:                    LOG(DEBUG) << getSocketConnection()->getConnectionName() << " WebSocketContext: Subprotocol '" << subProtocol->name
src/web/websocket/SocketContextUpgrade.hpp:194:                    LOG(DEBUG) << getSocketConnection()->getConnectionName() << " WebSocketContext: Subprotocol '" << subProtocol->name
src/web/websocket/SocketContextUpgrade.hpp:222:        LOG(INFO) << getSocketConnection()->getConnectionName() << " WebSocketContext: Subprotocol '" << subProtocol->name << "' connect";
src/web/websocket/SocketContextUpgrade.hpp:229:        LOG(INFO) << getSocketConnection()->getConnectionName() << " WebSocketContext:  Subprotocol '" << subProtocol->name
src/web/websocket/server/SubProtocolFactorySelector.cpp:83:            LOG(WARNING) << "WebSocket: Overriding websocket subprotocol library dir";
src/web/websocket/Receiver.cpp:130:                LOG(ERROR) << "WebSocket: Error opcode in continuation frame";
src/web/websocket/Receiver.cpp:284:            LOG(TRACE) << "WebSocket receive: Frame data\n" << utils::hexDump(payloadChunk, payloadChunkLen, 32, true);
src/web/websocket/SubProtocol.hpp:72:                        LOG(WARNING) << this->subProtocolContext->getSocketConnection()->getConnectionName() << " Subprotocol '"
src/web/websocket/SubProtocol.hpp:132:        LOG(DEBUG) << subProtocolContext->getSocketConnection()->getConnectionName() << " Subprotocol '" << name << "': Ping sent";
src/web/websocket/SubProtocol.hpp:144:        LOG(DEBUG) << subProtocolContext->getSocketConnection()->getConnectionName() << " Subprotocol '" << name << "': start";
src/web/websocket/SubProtocol.hpp:153:        LOG(DEBUG) << subProtocolContext->getSocketConnection()->getConnectionName() << " Subprotocol '" << name << "': stopped";
src/web/websocket/SubProtocol.hpp:155:        LOG(DEBUG) << "       Total Payload sent: " << getPayloadTotalSent();
src/web/websocket/SubProtocol.hpp:156:        LOG(DEBUG) << "  Total Payload processed: " << getPayloadTotalRead();
src/web/websocket/SubProtocol.hpp:161:        LOG(DEBUG) << subProtocolContext->getSocketConnection()->getConnectionName() << " Subprotocol '" << name << "': Pong received";
src/web/websocket/Transmitter.cpp:142:            LOG(TRACE) << "WebSocket send: Frame data\n" << utils::hexDump(payload, payloadLength, 32, true);
src/web/websocket/client/SubProtocolFactorySelector.cpp:83:            LOG(WARNING) << "WebSocket: Overriding websocket subprotocol library dir";
```

## References

- Migration 7 inventory PR #160 established HTTP/WebSocket split planning.
- Migration 7a and 7b work is already present in PR #161 and covers HTTP client, EventSource URL validation helpers, HTTP server, and HTTP server upgrade logging.

## Focused inventory classification table

| Class | Files | Count | Result |
| --- | --- | ---: | --- |
| A. WebSocket SocketContextUpgrade.hpp lifecycle/subprotocol selection | `src/web/websocket/SocketContextUpgrade.hpp` | 5 | Migrated |
| B. WebSocket Receiver.cpp frame/protocol diagnostics | `src/web/websocket/Receiver.cpp` | 2 | Migrated |
| C. WebSocket Transmitter.cpp frame/protocol diagnostics | `src/web/websocket/Transmitter.cpp` | 1 | Migrated |
| D. WebSocket SubProtocol.hpp lifecycle/ping/pong/payload totals | `src/web/websocket/SubProtocol.hpp` | 7 | Migrated |
| E. WebSocket SubProtocolFactorySelector.hpp plugin/cache/loader diagnostics | `src/web/websocket/SubProtocolFactorySelector.hpp` | 7 | Migrated |
| F. WebSocket client/server SubProtocolFactorySelector.cpp library-dir warning | client/server selector `.cpp` files | 2 | Migrated |
| G. PLOG / errno-sensitive call sites | focused 7c scope | 0 | None present |
| H. VLOG / verbose diagnostics | focused 7c scope | 0 | None present |
| I. deferred/unclear call sites | focused 7c scope | 0 | None deferred |

## Ownership discovery summary

Command:

```sh
rg -n "\blog\(\)|logger::|BoundaryLogger|LogScopeOwner|semantic|scope|getInstanceName|getConnectionName|SocketContext|SocketConnection|WebSocket|SubProtocol|Receiver|Transmitter|Upgrade" \
  src/web/websocket \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

Summary: `SocketContextUpgrade.hpp` and `SubProtocol.hpp` have access to socket connection identity and preserve `getSocketConnection()->getConnectionName()` manually. Receiver/transmitter/factory code does not expose a safe existing semantic owner without broad API churn, so 7c adds small WebSocket helper scopes in `src/web/websocket/SemanticLog.h`.

## Migrated call-site table

| Area | Semantic owner/helper | Severity mapping | Payload preservation |
| --- | --- | --- | --- |
| `SocketContextUpgrade.hpp` lifecycle and close paths | `webSocketSubProtocolLog()` | DEBUG/INFO -> debug/info | connectionName, subprotocol name, close sent/confirmed/request received, connect/disconnect wording |
| `SubProtocol.hpp` lifecycle/ping/pong/payload totals | `webSocketSubProtocolLog()` | WARNING/DEBUG -> warn/debug | connectionName, subprotocol name, max flying pings, ping sent, start/stopped, payload totals, pong received |
| `Receiver.cpp` frame/protocol diagnostics | `webSocketFrameLog()` | ERROR/TRACE -> error/trace | continuation-frame error wording and receive hex dump |
| `Transmitter.cpp` frame diagnostics | `webSocketFrameLog()` | TRACE -> trace | send frame hex dump |
| `SubProtocolFactorySelector.hpp` plugin/cache/loader diagnostics | `webSocketFactoryLog()` | DEBUG/WARNING -> debug/warn | plugin name, factory function name, dynamic/static cache distinction, loaded/selected/not-found states |
| client/server selector `.cpp` library-dir warning | `webSocketFactoryLog()` | WARNING -> warn | library-dir override warning text |

## Deferred call-site table

No focused Migration 7c LOG/PLOG/VLOG call sites were deferred.

## Semantic owner/helper used

- Helper component `web.websocket`: general WebSocket diagnostics and test coverage.
- Helper component `web.websocket.frame`: receiver/transmitter frame and protocol diagnostics.
- Helper component `web.websocket.subprotocol`: socket-context upgrade and subprotocol lifecycle diagnostics.
- Helper component `web.websocket.factory`: subprotocol plugin/factory/cache and library-dir diagnostics.

## Severity mapping

- `LOG(TRACE)` -> semantic `trace()`
- `LOG(DEBUG)` -> semantic `debug()`
- `LOG(INFO)` -> semantic `info()`
- `LOG(WARNING)` -> semantic `warn()`
- `LOG(ERROR)` -> semantic `error()`
- No `LOG(FATAL)` was present.

## PLOG/sysError errno handling

No PLOG call sites were present in the focused 7c inventory. The new unit test still covers `sysError` helper behavior to confirm errno code/text preservation for future WebSocket errno-sensitive use.

## VLOG result

No VLOG call sites were present in the focused 7c inventory, and no VLOG semantics were changed.

## WebSocket payload preservation notes

The migration preserves connectionName, subprotocol name, selected/rejected subprotocol information, plugin name, factory function name, dynamic/static cache distinction, loaded/selected/not-found plugin state, opcode/frame payload detail in representative tests, continuation-frame error wording, frame payload hex dump, ping sent, pong received, start/stopped lifecycle messages, payload total sent/processed, library-dir override warning, and connect/disconnect wording.

## Hex dump / expensive payload guard notes

Receiver and Transmitter hex-dump call sites now create a `webSocketFrameLog()` and check `log.enabled(logger::LogLevel::Trace)` before constructing `utils::hexDump(...)`. `WebSocketMigration07cTest` includes a sink-taking helper check proving the guarded branch is not evaluated when trace is disabled.

## Post-migration inventory

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" \
  src/web/websocket \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

Result: no remaining LOG/PLOG/VLOG call sites in Migration 7c scope.

## Filter/backend/format compatibility

`WebSocketMigration07cTest` covers enabled emission, framework component scopes (`web.websocket`, `web.websocket.frame`, `web.websocket.subprotocol`, `web.websocket.factory`), LogManager filtering, backend filtering, JSON v1 output, sink-taking helper overloads, suppressed formatting, sysError code/text, representative subprotocol/frame/factory/lifecycle payloads, and a guarded hex-dump branch.

## Production-scope confirmations

- This follow-up changes only allowed Migration 7c files plus tests/unit/log/CMakeLists.txt.
- This follow-up is not inventory-only.
- This follow-up modifies production WebSocket logging.
- This follow-up does not modify SemanticLogger.*.
- This follow-up does not modify LogScopeOwner.*.
- This follow-up does not modify Logger.*.
- This follow-up does not modify ConfigInstance.cpp.
- This follow-up does not modify src/net/config/stream/tls.
- This follow-up does not modify src/iot or MQTT-over-WebSocket code.
- This follow-up does not modify DB/apps/examples.
- This follow-up does not change LOG/PLOG/VLOG macro definitions.
- This follow-up does not change LogManager precedence.
- This follow-up does not change LogManager freeze behavior.
- This follow-up does not change JSON v1 schema.
- This follow-up does not start Round 10.

## Changed-file check

- `git diff --name-only master...HEAD` was attempted, but the local worktree does not have a `master` ref in this Codex environment, so Git reports `fatal: ambiguous argument 'master...HEAD'`.
- Equivalent local changed-file inspection uses `git diff --name-only HEAD -- src/web/websocket tests/unit/log/CMakeLists.txt docs/logging/semantic-migration-07c-websocket-report.md tests/unit/log/WebSocketMigration07cTest.cpp` plus `git ls-files --others --exclude-standard ...` and shows only the 7c files listed above.

## Build commands run

- `cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON`
- `cmake --build cmake-build --parallel 2 --target WebSocketMigration07cTest websocket websocket-client websocket-server`
- `cmake --build cmake-build --parallel 2 --target WebSocketMigration07cTest`
- `cmake --build cmake-build --parallel 2`
- `cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON`
- `cmake --build cmake-build-asan --parallel 2 --target WebSocketMigration07cTest`

## Test commands run

- `ctest --test-dir cmake-build -R WebSocketMigration07cTest --output-on-failure` passed.
- `ctest --test-dir cmake-build -R "SemanticLoggerRound2Test|LogScopeOwnerRound3Test|ProductionLogApiRound4Test|SocketEndpointLogApiRound5Test|SemanticBackendRound6Test|SemanticFilterRound7Test|ControlledMigrationRound8Test|SemanticCompatibilityRound9Test|SemanticOverheadRound9Test|SemanticProductionThresholdRepairTest|SocketConnectionMigration01Test|SocketConnectorAcceptorMigration02Test|SocketServerClientMigration03Test|CoreRuntimeMigration04Test|NetPhysicalSocketMigration05Test|TlsSocketStreamMigration06Test|HttpClientMigration07aTest|HttpServerMigration07bTest|WebSocketMigration07cTest" --output-on-failure` passed.
- `ctest --test-dir cmake-build --output-on-failure` passed with 126/126 tests successful; skipped component tests were reported as skipped by the test harness, not failed.
- `ASAN=$(gcc -print-file-name=libasan.so) && LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan -R WebSocketMigration07cTest --output-on-failure` passed.

## ASan result

ASan focused `WebSocketMigration07cTest` passed in `cmake-build-asan` with `LD_PRELOAD` set to `gcc -print-file-name=libasan.so`.

## Known follow-ups

- Migration 8 — MQTT
- Migration 9 — DB / apps / examples / remaining cleanup
- Round 10 — book integration
