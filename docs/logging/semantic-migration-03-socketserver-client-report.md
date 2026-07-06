# Semantic logging migration 03 — SocketServer and SocketClient

## Implementation summary

This is a clean Migration 03 PR from the current base branch after PR #151, PR #154, and PR #155 were present in history. PR #151 already provides the production-threshold repair; PR #154 already completed Migration 1 for `SocketConnection.hpp`; PR #155 already completed Migration 2 for `SocketConnector.h` and `SocketAcceptor.h`. This PR does not implement or duplicate PR #151, PR #154, or PR #155.

Migration 03 converts scoped legacy `LOG` macro call sites in `SocketServer.h` and `SocketClient.h` to the existing object-owned semantic logger. It also adds one focused unit test for the SocketServer/SocketClient owner path.

## Exact changed files

- `docs/logging/semantic-migration-03-socketserver-client-report.md`
- `src/core/socket/stream/SocketClient.h`
- `src/core/socket/stream/SocketServer.h`
- `tests/unit/log/CMakeLists.txt`
- `tests/unit/log/SocketServerClientMigration03Test.cpp`

## Base verification result

The requested `git fetch origin` step could not be completed because this checkout has no `origin` remote configured. The local base branch was the provided clean `work` checkout, whose latest commit is merge commit `2615d0a Merge pull request #155 from SNodeC/codex/create-semantic-logging-migration-02`, with PR #151, PR #154, PR #155, Round 8, and Round 9 visible in the recent log and required files present.

Verified required files:

- `docs/logging/semantic-production-threshold-repair-report.md`
- `tests/unit/log/SemanticProductionThresholdRepairTest.cpp`
- `docs/logging/semantic-migration-01-socketconnection-hpp-report.md`
- `tests/unit/log/SocketConnectionMigration01Test.cpp`
- `docs/logging/semantic-migration-02-socketconnector-acceptor-report.md`
- `tests/unit/log/SocketConnectorAcceptorMigration02Test.cpp`
- `docs/logging/round-08-controlled-subsystem-migration-report.md`
- `tests/unit/log/ControlledMigrationRound8Test.cpp`
- `docs/logging/round-09-compatibility-sanitizer-overhead-report.md`
- `tests/unit/log/SemanticCompatibilityRound9Test.cpp`
- `tests/unit/log/SemanticOverheadRound9Test.cpp`

## Initial inventory command/result

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" src/core/socket/stream/SocketServer.h src/core/socket/stream/SocketClient.h
```

Result:

```text
src/core/socket/stream/SocketClient.h:134:                      LOG(DEBUG) << socketConnection->getConnectionName() << ": OnConnect";
src/core/socket/stream/SocketClient.h:136:                      LOG(DEBUG) << "  Local: " << socketConnection->getLocalAddress().toString();
src/core/socket/stream/SocketClient.h:137:                      LOG(DEBUG) << "   Peer: " << socketConnection->getRemoteAddress().toString();
src/core/socket/stream/SocketClient.h:144:                      LOG(DEBUG) << socketConnection->getConnectionName() << ": OnConnected";
src/core/socket/stream/SocketClient.h:146:                      LOG(DEBUG) << "  Local: " << socketConnection->getLocalAddress().toString();
src/core/socket/stream/SocketClient.h:147:                      LOG(DEBUG) << "   Peer: " << socketConnection->getRemoteAddress().toString();
src/core/socket/stream/SocketClient.h:154:                      LOG(DEBUG) << socketConnection->getConnectionName() << ": OnDisconnect";
src/core/socket/stream/SocketClient.h:156:                      LOG(DEBUG) << "            Local: " << socketConnection->getLocalAddress().toString();
src/core/socket/stream/SocketClient.h:157:                      LOG(DEBUG) << "             Peer: " << socketConnection->getRemoteAddress().toString();
src/core/socket/stream/SocketClient.h:159:                      LOG(DEBUG) << "     Online Since: " << socketConnection->getOnlineSince();
src/core/socket/stream/SocketClient.h:160:                      LOG(DEBUG) << "  Online Duration: " << socketConnection->getOnlineDuration();
src/core/socket/stream/SocketClient.h:162:                      LOG(DEBUG) << "     Total Queued: " << socketConnection->getTotalQueued();
src/core/socket/stream/SocketClient.h:163:                      LOG(DEBUG) << "       Total Sent: " << socketConnection->getTotalSent();
src/core/socket/stream/SocketClient.h:164:                      LOG(DEBUG) << "      Write Delta: " << socketConnection->getTotalQueued() - socketConnection->getTotalSent();
src/core/socket/stream/SocketClient.h:165:                      LOG(DEBUG) << "       Total Read: " << socketConnection->getTotalRead();
src/core/socket/stream/SocketClient.h:166:                      LOG(DEBUG) << "  Total Processed: " << socketConnection->getTotalProcessed();
src/core/socket/stream/SocketClient.h:167:                      LOG(DEBUG) << "       Read Delta: " << socketConnection->getTotalRead() - socketConnection->getTotalProcessed();
src/core/socket/stream/SocketClient.h:197:                        LOG(DEBUG) << config->getInstanceName() << ": Initiating connect";
src/core/socket/stream/SocketClient.h:211:                                        LOG(INFO)
src/core/socket/stream/SocketClient.h:223:                                                    LOG(INFO) << config->getInstanceName() << ": Reconnect disabled during wait";
src/core/socket/stream/SocketClient.h:251:                                        LOG(INFO)
src/core/socket/stream/SocketClient.h:269:                                                    LOG(INFO) << config->getInstanceName() << ": Retry connect disabled during wait";
src/core/socket/stream/SocketClient.h:277:                        LOG(FATAL) << config->getInstanceName() << " required";
src/core/socket/stream/SocketServer.h:128:                      LOG(DEBUG) << socketConnection->getConnectionName() << ": OnConnect";
src/core/socket/stream/SocketServer.h:130:                      LOG(DEBUG) << "  Local: " << socketConnection->getLocalAddress().toString();
src/core/socket/stream/SocketServer.h:131:                      LOG(DEBUG) << "   Peer: " << socketConnection->getRemoteAddress().toString();
src/core/socket/stream/SocketServer.h:138:                      LOG(DEBUG) << socketConnection->getConnectionName() << ": OnConnected";
src/core/socket/stream/SocketServer.h:140:                      LOG(DEBUG) << "  Local: " << socketConnection->getLocalAddress().toString();
src/core/socket/stream/SocketServer.h:141:                      LOG(DEBUG) << "   Peer: " << socketConnection->getRemoteAddress().toString();
src/core/socket/stream/SocketServer.h:148:                      LOG(DEBUG) << socketConnection->getConnectionName() << ": OnDisconnect";
src/core/socket/stream/SocketServer.h:150:                      LOG(DEBUG) << "            Local: " << socketConnection->getLocalAddress().toString();
src/core/socket/stream/SocketServer.h:151:                      LOG(DEBUG) << "             Peer: " << socketConnection->getRemoteAddress().toString();
src/core/socket/stream/SocketServer.h:153:                      LOG(DEBUG) << "     Online Since: " << socketConnection->getOnlineSince();
src/core/socket/stream/SocketServer.h:154:                      LOG(DEBUG) << "  Online Duration: " << socketConnection->getOnlineDuration();
src/core/socket/stream/SocketServer.h:156:                      LOG(DEBUG) << "     Total Queued: " << socketConnection->getTotalQueued();
src/core/socket/stream/SocketServer.h:157:                      LOG(DEBUG) << "       Total Sent: " << socketConnection->getTotalSent();
src/core/socket/stream/SocketServer.h:158:                      LOG(DEBUG) << "      Write Delta: " << socketConnection->getTotalQueued() - socketConnection->getTotalSent();
src/core/socket/stream/SocketServer.h:159:                      LOG(DEBUG) << "       Total Read: " << socketConnection->getTotalRead();
src/core/socket/stream/SocketServer.h:160:                      LOG(DEBUG) << "  Total Processed: " << socketConnection->getTotalProcessed();
src/core/socket/stream/SocketServer.h:161:                      LOG(DEBUG) << "       Read Delta: " << socketConnection->getTotalRead() - socketConnection->getTotalProcessed();
src/core/socket/stream/SocketServer.h:191:                        LOG(DEBUG) << config->getInstanceName() << ": Initiating listen";
src/core/socket/stream/SocketServer.h:222:                                        LOG(INFO)
src/core/socket/stream/SocketServer.h:236:                                                    LOG(INFO) << config->getInstanceName() << ": Retry listen disabled during wait";
src/core/socket/stream/SocketServer.h:244:                        LOG(FATAL) << config->getInstanceName() << " required";
```

## Post-migration inventory command/result

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" src/core/socket/stream/SocketServer.h src/core/socket/stream/SocketClient.h
```

Result: no matches.

## Migrated call-site table

| File | Legacy call sites | Semantic result |
| --- | ---: | --- |
| `src/core/socket/stream/SocketServer.h` | 22 `LOG` calls | Converted to captured `this->log()` owner calls using `debug`, `info`, and `critical`. |
| `src/core/socket/stream/SocketClient.h` | 23 `LOG` calls | Converted to captured `this->log()` owner calls using `debug`, `info`, and `critical`. |

## Deferred call-site table

| File | Deferred call sites | Reason |
| --- | ---: | --- |
| `src/core/socket/stream/SocketServer.h` | 0 | No scoped `LOG`, `PLOG`, or `VLOG` call sites remain. |
| `src/core/socket/stream/SocketClient.h` | 0 | No scoped `LOG`, `PLOG`, or `VLOG` call sites remain. |

## Semantic owner used

All migrated production calls use the existing object-owned semantic logger exposed by `this->log()`. Lambdas capture `log = this->log()` at owner construction or flow start so nested callbacks retain the existing SocketServer/SocketClient semantic scope. No new `BoundaryLogger`, `LogScopeOwner`, inheritance, mixin, constructor parameter, or logging infrastructure was added.

## Severity mapping

- `LOG(DEBUG)` -> `log.debug(...)`
- `LOG(INFO)` -> `log.info(...)`
- `LOG(FATAL)` -> `log.critical(...)`

No `TRACE`, `WARNING`, or `ERROR` call sites were present in the scoped inventory.

## PLOG/sysError errno handling

No `PLOG` call sites were present in the scoped inventory, so no production errno conversion was required. The new unit test exercises `sysError` through the SocketServer semantic owner path and verifies errno code/text preservation.

## VLOG result

No `VLOG` call sites were present in the scoped inventory. No verbose-level behavior was migrated or changed.

## Identity-prefix cleanup result

Manual instance-name prefixes such as `config->getInstanceName() << ": ..."` were removed when the semantic owner already carries the instance identity. Connection names and socket endpoint values were retained because they describe the connected stream peer/local details rather than the SocketServer/SocketClient owner identity.

## Filter/backend/format compatibility

`SocketServerClientMigration03Test` covers enabled server/client owner emission, framework/instance/core.socket.stream scope, server/client role fields, LogManager filtering, backend `Logger::setLogLevel` filtering, JSON format selection, `sysError` errno code/text preservation, sink-taking overload compatibility, and suppressed expensive formatting after PR #151.

## Production-scope confirmations

This PR changes only SocketServer.h, SocketClient.h, tests/unit/log/CMakeLists.txt, SocketServerClientMigration03Test.cpp, and this report, or fewer files if no header changes are needed.
This PR does not modify SemanticLogger.*.
This PR does not modify LogScopeOwner.*.
This PR does not modify Logger.*.
This PR does not modify ConfigInstance.cpp.
This PR does not modify SocketConnection.*.
This PR does not modify SocketConnection.hpp.
This PR does not modify SocketContext.cpp.
This PR does not modify SocketConnector.h.
This PR does not modify SocketAcceptor.h.
This PR does not modify SemanticProductionThresholdRepairTest.cpp.
This PR does not modify SocketConnectionMigration01Test.cpp.
This PR does not modify SocketConnectorAcceptorMigration02Test.cpp.
This PR does not change LOG/PLOG/VLOG macro definitions.
This PR does not change LogManager precedence.
This PR does not change LogManager freeze behavior.
This PR does not change JSON v1 schema.
This PR does not start Round 10.

## Build commands run

- `cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON`
- `cmake --build cmake-build --parallel 2`
- `cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON`
- `cmake --build cmake-build-asan --target SocketServerClientMigration03Test --parallel 2`

## Test commands run

- `ctest --test-dir cmake-build -R "SemanticLoggerRound2Test|LogScopeOwnerRound3Test|ProductionLogApiRound4Test|SocketEndpointLogApiRound5Test|SemanticBackendRound6Test|SemanticFilterRound7Test|ControlledMigrationRound8Test|SemanticCompatibilityRound9Test|SemanticOverheadRound9Test|SemanticProductionThresholdRepairTest|SocketConnectionMigration01Test|SocketConnectorAcceptorMigration02Test|SocketServerClientMigration03Test" --output-on-failure`
- `ctest --test-dir cmake-build --output-on-failure`
- `ASAN=$(gcc -print-file-name=libasan.so) LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan -R SocketServerClientMigration03Test --output-on-failure`

## ASan result or exact reason not run

A focused ASan build and run of `SocketServerClientMigration03Test` passed. A full ASan suite was not run because the non-ASan full suite already built all targets and the focused ASan run covered the new Migration 03 owner test while keeping validation time practical in this container.

## Known follow-ups

- Continue subsequent semantic logging migrations in separate PRs only; this PR does not start Migration 4 or Round 10.
