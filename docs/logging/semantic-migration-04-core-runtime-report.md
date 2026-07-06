# Semantic logging migration 04 — core runtime report

## Implementation summary

This is a clean Migration 4 PR from current `master` after the already-merged PR #151, PR #154, PR #155, and PR #156. It does not duplicate those migrations. It migrates suitable core runtime lifecycle call sites in `EventLoop`, `TimerEventReceiver`, `DescriptorEventReceiver`, and the epoll/poll/select mux backend constructor announcements to semantic logging. It does not migrate HTTP, WebSocket, MQTT, DB, apps, examples, MiniGateway, or previous socket-stream migration files.

## Exact changed files

- `src/core/EventLoop.h`
- `src/core/EventLoop.cpp`
- `src/core/DescriptorEventReceiver.h`
- `src/core/DescriptorEventReceiver.cpp`
- `src/core/TimerEventReceiver.h`
- `src/core/TimerEventReceiver.cpp`
- `src/core/multiplexer/epoll/EventMultiplexer.cpp`
- `src/core/multiplexer/poll/EventMultiplexer.cpp`
- `src/core/multiplexer/select/EventMultiplexer.cpp`
- `tests/unit/log/CMakeLists.txt`
- `tests/unit/log/CoreRuntimeMigration04Test.cpp`
- `docs/logging/semantic-migration-04-core-runtime-report.md`

## Base verification result

`git fetch origin` could not run because this workspace has no `origin` remote. The current base commit is `2c69151 Merge pull request #156 from SNodeC/codex/start-semantic-logging-migration-03`, which includes the prerequisite merged PRs in the visible history. All requested prerequisite files were present:

- `docs/logging/semantic-production-threshold-repair-report.md`
- `tests/unit/log/SemanticProductionThresholdRepairTest.cpp`
- `docs/logging/semantic-migration-01-socketconnection-hpp-report.md`
- `tests/unit/log/SocketConnectionMigration01Test.cpp`
- `docs/logging/semantic-migration-02-socketconnector-acceptor-report.md`
- `tests/unit/log/SocketConnectorAcceptorMigration02Test.cpp`
- `docs/logging/semantic-migration-03-socketserver-client-report.md`
- `tests/unit/log/SocketServerClientMigration03Test.cpp`
- `docs/logging/round-08-controlled-subsystem-migration-report.md`
- `tests/unit/log/ControlledMigrationRound8Test.cpp`
- `docs/logging/round-09-compatibility-sanitizer-overhead-report.md`
- `tests/unit/log/SemanticCompatibilityRound9Test.cpp`
- `tests/unit/log/SemanticOverheadRound9Test.cpp`

## Initial inventory command/result

Command run exactly as requested:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" \
  src/core \
  -g '*.h' -g '*.hpp' -g '*.cpp' \
  -g '!src/core/socket/stream/SocketConnection.h' \
  -g '!src/core/socket/stream/SocketConnection.cpp' \
  -g '!src/core/socket/stream/SocketConnection.hpp' \
  -g '!src/core/socket/stream/SocketConnector.h' \
  -g '!src/core/socket/stream/SocketAcceptor.h' \
  -g '!src/core/socket/stream/SocketServer.h' \
  -g '!src/core/socket/stream/SocketClient.h' \
  -g '!src/core/socket/stream/SocketContext.cpp'
```

Complete initial inventory:

- `src/core/DynamicLoader.cpp`: 14 `LOG(TRACE)` call sites.
- `src/core/multiplexer/poll/EventMultiplexer.cpp:166`: `LOG(DEBUG)`.
- `src/core/multiplexer/epoll/EventMultiplexer.cpp:90`: `LOG(DEBUG)`.
- `src/core/multiplexer/select/EventMultiplexer.cpp:71`: `LOG(DEBUG)`.
- `src/core/TimerEventReceiver.cpp:88`: `LOG(WARNING)`.
- `src/core/TimerEventReceiver.cpp:104`: `LOG(TRACE)`.
- `src/core/DescriptorEventReceiver.cpp:102`: `LOG(TRACE)`.
- `src/core/DescriptorEventReceiver.cpp:104`: `LOG(WARNING)`.
- `src/core/DescriptorEventReceiver.cpp:114`: `LOG(TRACE)`.
- `src/core/DescriptorEventReceiver.cpp:116`: `LOG(WARNING)`.
- `src/core/DescriptorEventReceiver.cpp:126`: `LOG(WARNING)`.
- `src/core/DescriptorEventReceiver.cpp:129`: `LOG(WARNING)`.
- `src/core/DescriptorEventReceiver.cpp:140`: `LOG(WARNING)`.
- `src/core/DescriptorEventReceiver.cpp:143`: `LOG(WARNING)`.
- `src/core/EventLoop.cpp:122`: `LOG(TRACE)`.
- `src/core/EventLoop.cpp:176`: `PLOG(FATAL)`.
- `src/core/EventLoop.cpp:210`: `LOG(TRACE)`.
- `src/core/EventLoop.cpp:218`: `LOG(TRACE)`.
- `src/core/EventLoop.cpp:221`: `LOG(TRACE)`.
- `src/core/EventLoop.cpp:224`: `LOG(TRACE)`.
- `src/core/EventLoop.cpp:227`: `PLOG(FATAL)`.
- `src/core/EventLoop.cpp:261`: `LOG(TRACE)`.
- `src/core/EventLoop.cpp:270`: `LOG(TRACE)`.
- `src/core/EventLoop.cpp:287`: `LOG(TRACE)`.
- `src/core/EventLoop.cpp:291`: `LOG(TRACE)`.
- `src/core/EventLoop.cpp:293`: `LOG(TRACE)`.
- `src/core/EventLoop.cpp:297`: `LOG(TRACE)`.
- Remaining initial matches were under socket-stream TLS helpers, socket-stream writer, `SocketAcceptor.hpp`, and `SocketConnector.hpp`; these are deferred as socket-stream/TLS prior-migration or separate-scope work.

## Inventory classification table

| Class | Files | Initial production macro call sites | Decision |
| --- | --- | ---: | --- |
| A. EventLoop / runtime lifecycle | `src/core/EventLoop.cpp` | 13 | Migrated |
| B. Timer | `src/core/TimerEventReceiver.cpp` | 2 | Migrated |
| C. Descriptor / eventreceiver | `src/core/DescriptorEventReceiver.cpp` | 8 | Migrated |
| D. pipe | `src/core/pipe/**` | 0 | No call sites |
| E. mux backends | epoll/poll/select `EventMultiplexer.cpp` | 3 | Migrated |
| F. other `src/core` out of Migration 4 scope | `DynamicLoader.cpp`, socket-stream/TLS files, prior socket stream templates | Many | Deferred |

## Ownership discovery summary

Command run:

```sh
rg -n "\blog\(\)|logger::|BoundaryLogger|LogScopeOwner|semantic|LOG_SCOPE|scope" \
  src/core \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

Existing semantic owners were found only in prior socket-stream migration files (`SocketConnection`, `SocketContext`, `SocketConnector`, `SocketAcceptor`, `SocketServer`, `SocketClient`). No owner existed for EventLoop, descriptor receivers, timer receivers, or mux backend constructors.

## Stop/split decision and reason

The allowed Migration 4 scope contained 26 production macro call sites, below the stop/split threshold of 35. The PR proceeds without splitting. Out-of-scope socket-stream/TLS and `DynamicLoader` matches are documented as deferred and were not used in the threshold decision.

## Post-migration inventory command/result

The same inventory command was rerun. All migrated Migration 4 files are absent from the post-migration results. Remaining results are deferred `LOG`/`PLOG` call sites in socket-stream/TLS or socket-stream template files and `src/core/DynamicLoader.cpp`; there were no remaining `VLOG` matches.

## Migrated call-site table

| File | Count | Mapping |
| --- | ---: | --- |
| `src/core/EventLoop.cpp` | 11 `LOG(TRACE)`, 2 `PLOG(FATAL)` | `log().trace(...)`, `log().sysError(LogLevel::Critical, errnum, ...)` |
| `src/core/DescriptorEventReceiver.cpp` | 2 `LOG(TRACE)`, 6 `LOG(WARNING)` | `log().trace(...)`, `log().warn(...)` |
| `src/core/TimerEventReceiver.cpp` | 1 `LOG(WARNING)`, 1 `LOG(TRACE)` | `log().warn(...)`, `log().trace(...)` |
| `src/core/multiplexer/epoll/EventMultiplexer.cpp` | 1 `LOG(DEBUG)` | `muxLog().debug(...)` |
| `src/core/multiplexer/poll/EventMultiplexer.cpp` | 1 `LOG(DEBUG)` | `muxLog().debug(...)` |
| `src/core/multiplexer/select/EventMultiplexer.cpp` | 1 `LOG(DEBUG)` | `muxLog().debug(...)` |

## Deferred call-site table

| Area | Reason |
| --- | --- |
| `src/core/DynamicLoader.cpp` | Dynamic loader is not part of the requested core runtime layer (`EventLoop`, timer, descriptor/eventreceiver, pipe, mux). It needs a separate ownership decision. |
| `src/core/socket/stream/tls/**` | TLS socket-stream logging is outside Migration 4 and should not be mixed with core runtime migration. |
| `src/core/socket/stream/SocketWriter.cpp` | Socket-stream writer is outside this migration and prior socket-stream owner migrations should not be duplicated. |
| `src/core/socket/stream/SocketAcceptor.hpp` and `SocketConnector.hpp` | Socket stream acceptor/connector implementation templates are prior migration/separate scope and intentionally not modified here. |

## Semantic owner(s) used or added

- Added minimal `LogScopeOwner` ownership to `core::EventLoop` with `Framework`, `System`, component `core.event-loop`.
- Added minimal object-owned semantic logging to `DescriptorEventReceiver` with `Framework`, `System`, component `core.eventreceiver`, and receiver name as instance.
- Added minimal object-owned semantic logging to `TimerEventReceiver` with `Framework`, `System`, component `core.timer`, and receiver name as instance.
- Added file-local mux backend semantic owners with `Framework`, `System`, component `core.mux`, and backend instance (`epoll`, `poll`, `select`).
- `LogBoundary::Subsystem` does not exist in this codebase, so `LogBoundary::System` is the closest existing boundary for core-runtime subsystem messages. No new enum values were added.

## Severity mapping

- `LOG(TRACE)` -> `trace`
- `LOG(DEBUG)` -> `debug`
- `LOG(WARNING)` -> `warn`
- `PLOG(FATAL)` -> `sysError(LogLevel::Critical, errnum, ...)`

## PLOG/sysError errno handling

Two `PLOG(FATAL)` sites in `EventLoop.cpp` were migrated to `sysError(LogLevel::Critical, errnum, ...)`. Each captures `errno` into `const int errnum` immediately before semantic logging in the failure branch/case, preserving the errno code and generic-category text in semantic records.

## VLOG result

No `VLOG` call sites were migrated. No `VLOG` matches remain in the migration inventory.

## Message/identity preservation notes

The migration preserves lifecycle messages, signal names/numbers, mux backend names, receiver names, and timer dispatch delta values. Existing textual prefixes were kept where they contain useful operational identity; semantic scope adds structured component and instance metadata without deleting technical payload.

## Filter/backend/format compatibility

`CoreRuntimeMigration04Test` validates enabled emission, framework/system/component scope, `LogManager` filtering, `Logger::setLogLevel` backend filtering, JSON format selection, `sysError` errno code/text preservation, sink overload compatibility, and suppressed formatting avoiding `ExpensiveValue` evaluation after PR #151.

## Production-scope confirmations

- This PR changes only allowed Migration 4 files.
- This PR does not modify `SemanticLogger.*`.
- This PR does not modify `LogScopeOwner.*`.
- This PR does not modify `Logger.*`.
- This PR does not modify `ConfigInstance.cpp`.
- This PR does not modify previous socket-stream migration files.
- This PR does not modify previous migration tests.
- This PR does not change `LOG`/`PLOG`/`VLOG` macro definitions.
- This PR does not change `LogManager` precedence.
- This PR does not change `LogManager` freeze behavior.
- This PR does not change JSON v1 schema.
- This PR does not start Round 10.

## Build commands run

- `git diff --check`
- `cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON`
- `cmake --build cmake-build --parallel 2`
- `cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON`
- `cmake --build cmake-build-asan --parallel 2 --target CoreRuntimeMigration04Test`

## Test commands run

- `ctest --test-dir cmake-build -R "SemanticLoggerRound2Test|LogScopeOwnerRound3Test|ProductionLogApiRound4Test|SocketEndpointLogApiRound5Test|SemanticBackendRound6Test|SemanticFilterRound7Test|ControlledMigrationRound8Test|SemanticCompatibilityRound9Test|SemanticOverheadRound9Test|SemanticProductionThresholdRepairTest|SocketConnectionMigration01Test|SocketConnectorAcceptorMigration02Test|SocketServerClientMigration03Test|CoreRuntimeMigration04Test" --output-on-failure`
- `ctest --test-dir cmake-build --output-on-failure`
- `ASAN=$(gcc -print-file-name=libasan.so) LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan -R CoreRuntimeMigration04Test --output-on-failure`

## ASan result or exact reason not run

A focused ASan build and `CoreRuntimeMigration04Test` run passed. Full ASan was not run because the full non-ASan build already took many minutes in this environment and a full ASan build/test of all apps and component tests would materially exceed the practical runtime for this PR; the migration-specific ASan target was built and executed instead.

## Known follow-ups

- Consider a separate, explicitly-scoped migration for `DynamicLoader` ownership.
- Consider a separate TLS/socket-stream migration for the remaining TLS and socket-stream template call sites.
