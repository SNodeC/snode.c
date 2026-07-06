# Semantic logging repair — production threshold wiring and hygiene

## Summary of the defect

Production semantic no-arg loggers were constructing `BoundaryLogger` instances through sink-taking overloads that defaulted the local threshold to `LogLevel::Trace`. Output filtering remained correct because `Logger::emitSemantic()` still applies `LogManager::shouldEmit(record)`, but suppressed format-style calls had already formatted their message and materialized a `LogRecord` before that final policy gate.

This repair fixes production threshold wiring so no-arg semantic loggers use `LogManager::effectiveLevel(scope)`.

This repair ensures format-style semantic calls check `BoundaryLogger::enabled(level)` before formatting.

This repair ensures `sysError` checks `BoundaryLogger::enabled(level)` before formatting or building error payloads.

## Root cause

The no-arg production APIs called their sink-taking overloads with `Logger::semanticSink()` and relied on the explicit-threshold overload default of `LogLevel::Trace`. That default is appropriate for custom sink/testing compatibility, but it is not appropriate for backend-backed production no-arg loggers that should use the frozen startup semantic policy as their local threshold.

## Branch and precondition notes

The requested `origin` remote was not usable in this environment: `git fetch origin` failed with `fatal: 'origin' does not appear to be a git repository`. The checkout was clean before changes, and Round 9 / PR #150 was confirmed present by the required files and `git log --oneline -n 12`, including `9a6f335 Merge pull request #150 from SNodeC/codex/implement-logging-roadmap-round-9`.

## Exact files changed

- `src/log/SemanticLogger.h`
- `src/log/SemanticLogger.cpp`
- `src/log/LogScopeOwner.h`
- `src/log/LogScopeOwner.cpp`
- `src/net/config/ConfigInstance.cpp`
- `src/core/socket/stream/SocketConnection.cpp`
- `src/core/socket/stream/SocketContext.cpp`
- `src/core/socket/stream/SocketServer.h`
- `src/core/socket/stream/SocketClient.h`
- `src/core/socket/stream/SocketConnector.h`
- `src/core/socket/stream/SocketAcceptor.h`
- `tests/unit/log/SemanticProductionThresholdRepairTest.cpp`
- `tests/unit/log/CMakeLists.txt`
- `docs/logging/semantic-production-threshold-repair-report.md`

## Exact production loggers repaired

- `ConfigInstance::log()`
- `SocketConnection::log()`
- `SocketContext::log()`
- `SocketContext::frameworkLog()`
- `SocketServer::log()`
- `SocketClient::log()`
- `SocketConnector::log()`
- `SocketAcceptor::log()`

## LogScopeOwner helper

A `LogScopeOwner::logger(BoundaryLogger::Sink sink, BoundaryLogger::Clock clock = {}) const` helper was added. It resolves `LogManager::effectiveLevel(scope())` and delegates to the preserved explicit-threshold overload.

The explicit-threshold overload remains available and still honors caller-provided thresholds for custom sink and test paths.

## Format-style suppression fix

`BoundaryLogger::emitFormatted()` now checks `enabled(level)` before collecting arguments and formatting the message. This avoids expensive formatting when the local semantic threshold suppresses the call.

## sysError suppression fix

Both `sysError(LogLevel, int, ...)` and `sysError(LogLevel, std::error_code, ...)` now check `enabled(level)` before formatting or constructing `LogError` payloads.

## Stale comment cleanup

Comments that said startup semantic filter configuration was deferred to Round 7 were updated to state that the production default logger uses the frozen startup semantic policy as its local threshold.

## License header cleanup

The standard project dual LGPL/MIT header was added to:

- `src/log/SemanticLogger.h`
- `src/log/SemanticLogger.cpp`
- `src/log/LogScopeOwner.h`
- `src/log/LogScopeOwner.cpp`

## clang-format result

`clang-format -i` completed successfully for the semantic logging files and all changed production/test C++ files.

## Tests added

Added `tests/unit/log/SemanticProductionThresholdRepairTest.cpp` and registered `SemanticProductionThresholdRepairTest` in `tests/unit/log/CMakeLists.txt`.

The test covers:

- suppressed production no-arg `SocketConnection::log().debug("{}", ExpensiveValue)` does not evaluate formatting and emits no backend output;
- enabled production no-arg `SocketConnection::log().debug("{}", ExpensiveValue)` evaluates formatting once, emits backend output, and preserves semantic scope;
- suppressed production no-arg `SocketContext::frameworkLog().debug("{}", ExpensiveValue)` does not evaluate formatting and emits no backend output;
- disabled `sysError` does not format or call the sink;
- enabled `sysError` preserves errno code, errno text, and message;
- explicit-threshold custom sink overloads remain compatible even when `LogManager` global level is higher.

## Exact build commands run

- `cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON`
- `cmake --build cmake-build --parallel 2`
- `cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON`
- `cmake --build cmake-build-asan --parallel 2`

## Exact test commands run

- `ctest --test-dir cmake-build -R "SemanticLoggerRound2Test|LogScopeOwnerRound3Test|ProductionLogApiRound4Test|SocketEndpointLogApiRound5Test|SemanticBackendRound6Test|SemanticFilterRound7Test|ControlledMigrationRound8Test|SemanticCompatibilityRound9Test|SemanticOverheadRound9Test|SemanticProductionThresholdRepairTest" --output-on-failure`
- `ctest --test-dir cmake-build --output-on-failure`
- `ASAN=$(gcc -print-file-name=libasan.so) LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan --output-on-failure`

## ASan result

ASan build and test run passed. The ASan `ctest` command reported 100% tests passed, 0 failed out of 117. The same environment-level skipped component tests remained skipped under ASan.

## Non-goal confirmations

This repair does not migrate additional `LOG`/`PLOG`/`VLOG` production call sites.

This repair does not change `LOG`/`PLOG`/`VLOG` macro definitions.

This repair does not change the JSON v1 schema.

This repair does not start Round 10 book integration.

LogManager precedence was unchanged.

LogManager freeze behavior was unchanged.

Logger::semanticSink still applies final LogManager filtering through `Logger::emitSemantic()`.

No new production call-site migration was done.

Migration 1 was not started.

Round 10 was not started.

## Known follow-ups

- Continue the planned post-roadmap migration work only in a separate future PR.
- Keep future production no-arg semantic logger APIs wired through `LogManager::effectiveLevel(scope)` at construction time.
