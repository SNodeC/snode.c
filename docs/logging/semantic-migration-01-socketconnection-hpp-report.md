# Semantic logging migration 01 — SocketConnection.hpp

## Implementation summary

This is a clean Migration 1 PR from current `master` after PR #151. PR #151 already provides the production-threshold repair. This PR does not implement or duplicate PR #151.

This PR migrates only suitable production member `LOG`/`PLOG` call sites in `src/core/socket/stream/SocketConnection.hpp` to the existing `SocketConnection::log()` semantic owner. It adds `SocketConnectionMigration01Test` and registers it in the unit log CMake file.

## Exact changed files

This PR changes only:

- `docs/logging/semantic-migration-01-socketconnection-hpp-report.md`
- `src/core/socket/stream/SocketConnection.hpp`
- `tests/unit/log/CMakeLists.txt`
- `tests/unit/log/SocketConnectionMigration01Test.cpp`

## Base verification result

Base verification was run from the local `master` state because this container has no `origin` remote configured. The local `master` contains PR #151 and the required Round 8 and Round 9 files.

Commands/results:

```sh
test -f docs/logging/semantic-production-threshold-repair-report.md
test -f tests/unit/log/SemanticProductionThresholdRepairTest.cpp
git log --oneline -n 30 | grep "0aa69f6"
```

Result:

```text
0aa69f6 Merge pull request #151 from SNodeC/codex/repair-semantic-logging-production-thresholds
```

```sh
test -f docs/logging/round-08-controlled-subsystem-migration-report.md
test -f tests/unit/log/ControlledMigrationRound8Test.cpp
test -f docs/logging/round-09-compatibility-sanitizer-overhead-report.md
test -f tests/unit/log/SemanticCompatibilityRound9Test.cpp
test -f tests/unit/log/SemanticOverheadRound9Test.cpp
```

Result: all required files are present.

## Initial inventory command/result

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" src/core/socket/stream/SocketConnection.hpp
```

Result before editing:

```text
67:                LOG(TRACE) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
70:                LOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
74:            PLOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
90:                LOG(TRACE) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
93:                LOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
97:            PLOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
115:                          LOG(TRACE) << connectionName << " OnReadError: EOF received";
117:                          PLOG(TRACE) << connectionName << " OnReadError";
132:                      PLOG(TRACE) << connectionName << " OnWriteError";
204:            LOG(TRACE) << connectionName << " ReadFromPeer: New SocketContext != nullptr: SocketContextSwitch still in progress";
227:        LOG(TRACE) << connectionName << ": Shutdown (RD)";
232:            LOG(DEBUG) << connectionName << " Shutdown (RD): success";
234:            PLOG(ERROR) << connectionName << " Shutdown (RD)";
241:            LOG(TRACE) << connectionName << ": Stop writing";
292:        LOG(TRACE) << connectionName << ": Shutdown (WR)";
295:            LOG(DEBUG) << connectionName << " Shutdown (WR): success";
297:            PLOG(ERROR) << connectionName << " Shutdown (WR)";
308:            LOG(TRACE) << connectionName << ": Data available: " << available << " but nothing read";
322:            LOG(DEBUG) << connectionName << " SocketConnection: switch completed";
346:                LOG(DEBUG) << connectionName << ": Shutting down due to signal '" << utils::system::strsignal(signum) << "' (SIG"
358:        LOG(WARNING) << connectionName << ": Read timeout";
364:        LOG(WARNING) << connectionName << ": Write timeout";
376:        LOG(DEBUG) << connectionName << ": disconnected";
```

## Post-migration inventory command/result

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" src/core/socket/stream/SocketConnection.hpp
```

Result after editing:

```text
67:                LOG(TRACE) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
70:                LOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
74:            PLOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
90:                LOG(TRACE) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
93:                LOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
97:            PLOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
```

All suitable member `LOG`/`PLOG` call sites are migrated. Only free helper function `LOG`/`PLOG` call sites remain deferred. No `VLOG` call sites are migrated.

## Migrated call-site table

| Old call site | New semantic call |
| --- | --- |
| read callback EOF `LOG(TRACE)` | `this->log().trace("OnReadError: EOF received")` |
| read callback error `PLOG(TRACE)` | `this->log().sysError(logger::LogLevel::Trace, errnum, "OnReadError")` |
| write callback error `PLOG(TRACE)` | `this->log().sysError(logger::LogLevel::Trace, errnum, "OnWriteError")` |
| context-switch-in-progress trace | `this->log().trace(...)` |
| shutdown-read trace/debug/error | `this->log().trace`, `debug`, and `sysError(Error, errnum, ...)` |
| shutdown-write start trace | `this->log().trace("Stop writing")` |
| do-write-shutdown trace/debug/error | `this->log().trace`, `debug`, and `sysError(Error, errnum, ...)` |
| data-available trace | `this->log().trace("Data available: {} but nothing read", available)` |
| switch-completed debug | `this->log().debug("SocketConnection: switch completed")` |
| signal debug | `this->log().debug("Shutting down due to signal '{}' (SIG{} [{}])", ...)` |
| read/write timeout warnings | `this->log().warn(...)` |
| disconnect debug | `this->log().debug("disconnected")` |

## Deferred call-site table

| Deferred site | Reason |
| --- | --- |
| `getLocalSocketAddress(...)` legacy `LOG(TRACE)` | Free helper lacks safe access to `SocketConnection::log()` without broader API/constructor churn. |
| `getLocalSocketAddress(...)` legacy `LOG(WARNING)` | Free helper lacks safe access to `SocketConnection::log()` without broader API/constructor churn. |
| `getLocalSocketAddress(...)` legacy `PLOG(WARNING)` | Free helper lacks safe access to `SocketConnection::log()` without broader API/constructor churn. |
| `getRemoteSocketAddress(...)` legacy `LOG(TRACE)` | Free helper lacks safe access to `SocketConnection::log()` without broader API/constructor churn. |
| `getRemoteSocketAddress(...)` legacy `LOG(WARNING)` | Free helper lacks safe access to `SocketConnection::log()` without broader API/constructor churn. |
| `getRemoteSocketAddress(...)` legacy `PLOG(WARNING)` | Free helper lacks safe access to `SocketConnection::log()` without broader API/constructor churn. |

## Semantic owner used

All migrated member call sites use the existing `SocketConnection::log()` semantic owner through `this->log()`. This PR does not add another logger owner, logger member, `BoundaryLogger` storage, `LogScopeOwner`, inheritance, mixin, or `LogSuperClass`.

## Severity mapping

- `LOG(TRACE)` -> `this->log().trace(...)`
- `LOG(DEBUG)` -> `this->log().debug(...)`
- `LOG(WARNING)` -> `this->log().warn(...)`
- `PLOG(TRACE)` -> `this->log().sysError(logger::LogLevel::Trace, errnum, ...)`
- `PLOG(ERROR)` -> `this->log().sysError(logger::LogLevel::Error, errnum, ...)`

No `LOG(INFO)`, `LOG(ERROR)`, `LOG(FATAL)`, or `VLOG` member call sites were present in `SocketConnection.hpp` for this migration.

## PLOG/sysError errno handling

Callbacks that already receive `errnum` pass that value directly to `sysError`. Immediate failing shutdown calls capture `errno` into `const int errnum` immediately after the failing system call and before logging or formatting.

## VLOG result

The initial inventory found no `VLOG` call sites in `SocketConnection.hpp`; no `VLOG` call sites were migrated.

## Identity-prefix cleanup result

Migrated member call sites remove manual `connectionName` prefixes where the semantic `SocketConnection::log()` scope already carries connection identity. Deferred free helper legacy calls retain their existing identity text.

## Filter/backend/format compatibility

`SocketConnectionMigration01Test` covers enabled semantic emission, framework/connection/core.socket.stream scope, `LogManager` filtering, `Logger::setLogLevel` backend filtering, JSON format selection, `sysError` errno code/text preservation, sink-taking overload compatibility, and suppressed production formatting not evaluating an expensive value after PR #151.

## Production-scope confirmations

- This PR changes only `SocketConnection.hpp`, `tests/unit/log/CMakeLists.txt`, `SocketConnectionMigration01Test.cpp`, and this report.
- This PR does not modify `SemanticLogger.*`.
- This PR does not modify `LogScopeOwner.*`.
- This PR does not modify `ConfigInstance.cpp`.
- This PR does not modify `SocketConnection.cpp`.
- This PR does not modify `SocketContext.cpp`.
- This PR does not modify `SocketServer.h`.
- This PR does not modify `SocketClient.h`.
- This PR does not modify `SocketConnector.h`.
- This PR does not modify `SocketAcceptor.h`.
- This PR does not modify `SemanticProductionThresholdRepairTest.cpp`.
- This PR does not change `LOG`/`PLOG`/`VLOG` macro definitions.
- This PR does not change `LogManager` precedence.
- This PR does not change `LogManager` freeze behavior.
- This PR does not change JSON v1 schema.
- This PR does not start Round 10.
- This PR does not migrate HTTP, WebSocket, MQTT, DB, app, or MiniGateway call sites.

## Build commands run

```sh
cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON
cmake --build cmake-build --parallel 2
```

Both commands completed successfully after installing missing container packages `nlohmann-json3-dev`, `libmagic-dev`, and `libbluetooth-dev`.

## Test commands run

```sh
git diff --check
ctest --test-dir cmake-build -R "SemanticLoggerRound2Test|LogScopeOwnerRound3Test|ProductionLogApiRound4Test|SocketEndpointLogApiRound5Test|SemanticBackendRound6Test|SemanticFilterRound7Test|ControlledMigrationRound8Test|SemanticCompatibilityRound9Test|SemanticOverheadRound9Test|SemanticProductionThresholdRepairTest|SocketConnectionMigration01Test" --output-on-failure
ctest --test-dir cmake-build --output-on-failure
```

Focused logging tests passed: 11/11. Full CTest passed: 118/118 reported as successful, with environment-gated component tests skipped by their existing skip conditions.

## ASan result

ASan was not run in this container due to time constraints after completing the full non-ASan configure, build, focused logging tests, and full CTest suite. No ASan-specific failure was observed because no ASan build was attempted.

## Known follow-ups

- Migrate the deferred free helper call sites only in a future migration that can safely provide a semantic owner without broad API/constructor churn.
- Continue later migrations separately; this PR does not start Migration 2 or Round 10.
