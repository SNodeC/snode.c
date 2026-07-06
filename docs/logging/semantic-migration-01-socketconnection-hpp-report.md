# Semantic logging migration 01 — SocketConnection.hpp

## Implementation summary

Migration 1 migrates only `SocketConnection.hpp` production member call sites from legacy `LOG(...)`/`PLOG(...)` to the existing object-owned `SocketConnection::log()` semantic owner. The free helper address-discovery functions at the top of `SocketConnection.hpp` were deliberately deferred because they do not have safe access to a `SocketConnection` object or its semantic owner without broader constructor/API churn.

Migration 1 does not change LOG/PLOG/VLOG macro definitions.

Migration 1 does not change startup semantic policy.

Migration 1 does not change JSON v1.

Migration 1 does not add runtime reload, signals, admin endpoints, or mutable atomic runtime thresholds.

Migration 1 does not start Round 10 book integration.


## Rebase after production-threshold repair

Migration 1 was originally completed before the semantic logging production-threshold repair. This branch was recreated on top of repaired `origin/master` after PR #151 (`0aa69f6f0 Merge pull request #151 from SNodeC/codex/repair-semantic-logging-production-thresholds`). The migration preserves the repair behavior and does not revert production no-arg logger threshold wiring.

PR #151 repair files are present on the base branch:

- `docs/logging/semantic-production-threshold-repair-report.md`
- `tests/unit/log/SemanticProductionThresholdRepairTest.cpp`

Rebase/repair compatibility confirmations:

- Migration 1 was recreated on top of repaired master because this environment initially only had the old local `work` branch; after adding `https://github.com/SNodeC/snode.c.git` as `origin`, `master` was reset to `origin/master` and the Migration 1 changes were cherry-picked onto the repaired base.
- No repair behavior was reverted.
- No production no-arg logger was changed back to the old `Trace`-default behavior.
- BoundaryLogger pre-format suppression remains intact.
- `SemanticProductionThresholdRepairTest` still passes together with Migration 1 tests.
- Migration 1 still migrates only `SocketConnection.hpp`.
- No Round 10 book integration was started.

## Exact files changed

- `src/core/socket/stream/SocketConnection.hpp`
- `tests/unit/log/CMakeLists.txt`
- `tests/unit/log/SocketConnectionMigration01Test.cpp`
- `docs/logging/semantic-migration-01-socketconnection-hpp-report.md`

## Base verification result

The GitHub repository `https://github.com/SNodeC/snode.c.git` was added as `origin`, `origin/master` was fetched, and local `master` was reset/tracked to the repaired upstream master. Verification commands confirmed the PR #151 repair files, Round 9 files, and Round 8 files exist:

- `docs/logging/semantic-production-threshold-repair-report.md`
- `tests/unit/log/SemanticProductionThresholdRepairTest.cpp`
- `docs/logging/round-08-controlled-subsystem-migration-report.md`
- `tests/unit/log/ControlledMigrationRound8Test.cpp`
- `docs/logging/round-09-compatibility-sanitizer-overhead-report.md`
- `tests/unit/log/SemanticCompatibilityRound9Test.cpp`
- `tests/unit/log/SemanticOverheadRound9Test.cpp`

Recent base history included `0aa69f6f0 Merge pull request #151 from SNodeC/codex/repair-semantic-logging-production-thresholds`, `9a6f33572 Merge pull request #150 from SNodeC/codex/implement-logging-roadmap-round-9`, and `a5f44780b Merge pull request #149 from SNodeC/codex/implement-logging-roadmap-round-8-migration`.

## Candidate inventory command and result

Command:

```sh
rg -n "\\b(LOG|PLOG|VLOG)\\s*\\(" src/core/socket/stream/SocketConnection.hpp
```

Initial result: 24 legacy macro call sites, all `LOG` or `PLOG`, no `VLOG`.

Post-migration result: 6 deferred legacy macro call sites remain in free helper functions only; no `VLOG` call sites exist in the file.

## Migrated call-site table

| file | old call form | new call form | reason migrated | semantic owner used | expected semantic scope | errno preserved? | test coverage |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `src/core/socket/stream/SocketConnection.hpp` | `LOG(TRACE) << connectionName << " OnReadError: EOF received"` | `this->log().trace("OnReadError: EOF received")` | Member lambda captures `this`; connection identity is scope-owned. | `SocketConnection::log()` | framework / connection / core.socket.stream / instance / connection identity | not applicable | indirect |
| `src/core/socket/stream/SocketConnection.hpp` | `PLOG(TRACE) << connectionName << " OnReadError"` | `this->log().sysError(logger::LogLevel::Trace, errnum, "OnReadError")` | Existing callback receives `errnum`; semantic sysError preserves code/text without relying on later errno. | `SocketConnection::log()` | framework / connection / core.socket.stream / instance / connection identity | yes | indirect |
| `src/core/socket/stream/SocketConnection.hpp` | `PLOG(TRACE) << connectionName << " OnWriteError"` | `this->log().sysError(logger::LogLevel::Trace, errnum, "OnWriteError")` | Existing callback receives `errnum`; semantic sysError preserves code/text without relying on later errno. | `SocketConnection::log()` | framework / connection / core.socket.stream / instance / connection identity | yes | indirect |
| `src/core/socket/stream/SocketConnection.hpp` | `LOG(TRACE) << connectionName << " ReadFromPeer: New SocketContext != nullptr: SocketContextSwitch still in progress"` | `this->log().trace("ReadFromPeer: New SocketContext != nullptr: SocketContextSwitch still in progress")` | Member function has safe owner access; identity prefix removed. | `SocketConnection::log()` | framework / connection / core.socket.stream / instance / connection identity | not applicable | indirect |
| `src/core/socket/stream/SocketConnection.hpp` | `LOG(TRACE) << connectionName << ": Shutdown (RD)"` | `this->log().trace("Shutdown (RD)")` | Member function has safe owner access; identity prefix removed. | `SocketConnection::log()` | framework / connection / core.socket.stream / instance / connection identity | not applicable | direct |
| `src/core/socket/stream/SocketConnection.hpp` | `LOG(DEBUG) << connectionName << " Shutdown (RD): success"` | `this->log().debug("Shutdown (RD): success")` | Member function has safe owner access; identity prefix removed. | `SocketConnection::log()` | framework / connection / core.socket.stream / instance / connection identity | not applicable | direct |
| `src/core/socket/stream/SocketConnection.hpp` | `PLOG(ERROR) << connectionName << " Shutdown (RD)"` | `const int errnum = errno; this->log().sysError(logger::LogLevel::Error, errnum, "Shutdown (RD)")` | Captures errno immediately after failed shutdown and emits semantic sysError. | `SocketConnection::log()` | framework / connection / core.socket.stream / instance / connection identity | yes | direct |
| `src/core/socket/stream/SocketConnection.hpp` | `LOG(TRACE) << connectionName << ": Stop writing"` | `this->log().trace("Stop writing")` | Member function has safe owner access; identity prefix removed. | `SocketConnection::log()` | framework / connection / core.socket.stream / instance / connection identity | not applicable | indirect |
| `src/core/socket/stream/SocketConnection.hpp` | `LOG(TRACE) << connectionName << ": Shutdown (WR)"` | `this->log().trace("Shutdown (WR)")` | Member function has safe owner access; identity prefix removed. | `SocketConnection::log()` | framework / connection / core.socket.stream / instance / connection identity | not applicable | indirect |
| `src/core/socket/stream/SocketConnection.hpp` | `LOG(DEBUG) << connectionName << " Shutdown (WR): success"` | `this->log().debug("Shutdown (WR): success")` | Member function has safe owner access; identity prefix removed. | `SocketConnection::log()` | framework / connection / core.socket.stream / instance / connection identity | not applicable | indirect |
| `src/core/socket/stream/SocketConnection.hpp` | `PLOG(ERROR) << connectionName << " Shutdown (WR)"` | `const int errnum = errno; this->log().sysError(logger::LogLevel::Error, errnum, "Shutdown (WR)")` | Captures errno immediately after failed shutdown and emits semantic sysError. | `SocketConnection::log()` | framework / connection / core.socket.stream / instance / connection identity | yes | indirect |
| `src/core/socket/stream/SocketConnection.hpp` | `LOG(TRACE) << connectionName << ": Data available: " << available << " but nothing read"` | `this->log().trace("Data available: {} but nothing read", available)` | Member function has safe owner access; identity prefix removed; simple value formatting preserved. | `SocketConnection::log()` | framework / connection / core.socket.stream / instance / connection identity | not applicable | indirect |
| `src/core/socket/stream/SocketConnection.hpp` | `LOG(DEBUG) << connectionName << " SocketConnection: switch completed"` | `this->log().debug("SocketConnection: switch completed")` | Member function has safe owner access; identity prefix removed. | `SocketConnection::log()` | framework / connection / core.socket.stream / instance / connection identity | not applicable | indirect via Round 8/9 |
| `src/core/socket/stream/SocketConnection.hpp` | signal shutdown `LOG(DEBUG)` stream expression | `this->log().debug("Shutting down due to signal '{}' (SIG{} [{}])", ...)` | Member function has safe owner access; string composition preserved with semantic formatting. | `SocketConnection::log()` | framework / connection / core.socket.stream / instance / connection identity | not applicable | indirect |
| `src/core/socket/stream/SocketConnection.hpp` | `LOG(WARNING) << connectionName << ": Read timeout"` | `this->log().warn("Read timeout")` | Member function has safe owner access; severity mapped to warn. | `SocketConnection::log()` | framework / connection / core.socket.stream / instance / connection identity | not applicable | indirect |
| `src/core/socket/stream/SocketConnection.hpp` | `LOG(WARNING) << connectionName << ": Write timeout"` | `this->log().warn("Write timeout")` | Member function has safe owner access; severity mapped to warn. | `SocketConnection::log()` | framework / connection / core.socket.stream / instance / connection identity | not applicable | indirect |
| `src/core/socket/stream/SocketConnection.hpp` | `LOG(DEBUG) << connectionName << ": disconnected"` | `this->log().debug("disconnected")` | Member function has safe owner access; identity prefix removed. | `SocketConnection::log()` | framework / connection / core.socket.stream / instance / connection identity | not covered |

## Deferred call-site table

| file | call sites | reason deferred |
| --- | --- | --- |
| `src/core/socket/stream/SocketConnection.hpp` | `getLocalSocketAddress(...)`: local peer address success/warning/PLOG warning | Helper lacks safe access to `SocketConnection::log()`; migrating would require broader constructor/API churn or a new logger parameter. |
| `src/core/socket/stream/SocketConnection.hpp` | `getRemoteSocketAddress(...)`: remote peer address success/warning/PLOG warning | Helper lacks safe access to `SocketConnection::log()`; migrating would require broader constructor/API churn or a new logger parameter. |

## Semantic owner used

All migrated production call sites use `this->log()`, the existing object-owned `SocketConnection::log()` semantic owner. No new logger member, stored `BoundaryLogger`, `LogScopeOwner`, inheritance, mixin, or `LogSuperClass` was added.

## Severity mapping result

- `LOG(TRACE)` became `this->log().trace(...)`.
- `LOG(DEBUG)` became `this->log().debug(...)`.
- `LOG(WARNING)` became `this->log().warn(...)`.
- `PLOG(TRACE)` became `this->log().sysError(logger::LogLevel::Trace, ...)`.
- `PLOG(ERROR)` became `this->log().sysError(logger::LogLevel::Error, ...)`.

## PLOG/sysError errno preservation result

PLOG-equivalent member migrations use semantic `sysError`. Error callbacks use the provided `errnum` directly. Shutdown failure sites capture `const int errnum = errno;` immediately after the failing `physicalSocket.shutdown(...)` call and before semantic emission.

## VLOG result

No `VLOG(...)` call sites were present in `SocketConnection.hpp`; none were migrated.

## Identity-prefix cleanup result

Connection identity prefixes were removed only from migrated member messages because `SocketConnection::log()` carries the connection identity in semantic scope. The deferred free helper function messages still retain legacy identity text.

## Compatibility results

- LogManager filter compatibility: covered by `SocketConnectionMigration01Test` with semantic global level filtering.
- Production-threshold repair compatibility: covered by `SocketConnectionMigration01Test` with an `ExpensiveValue` formatting check for suppressed `SocketConnection::log()` format-style logging, and by `SemanticProductionThresholdRepairTest` for repaired production no-arg logger threshold wiring and `BoundaryLogger` pre-format suppression.
- Logger::setLogLevel backend-gate compatibility: covered by `SocketConnectionMigration01Test` with numeric backend filtering.
- Text/Json format compatibility: text output is covered by semantic emission checks and JSON v1 output is covered by `SocketConnectionMigration01Test`.
- Legacy macro compatibility: macro definitions and legacy macro call sites outside the allowed production file were not changed; Round 9 compatibility tests remain in the focused validation set.

## Production-scope confirmations

- Macro definitions were not changed.
- No production files outside `SocketConnection.hpp` were migrated.
- No protocol/application/MQTT/HTTP/WebSocket/DB/MiniGateway migration was done.
- No book/manuscript files were changed.

## Exact build commands run

```sh
git status --short
git diff --check
cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON
cmake --build cmake-build --parallel 2
```

## Exact test commands run

```sh
ctest --test-dir cmake-build -R SocketConnectionMigration01Test --output-on-failure
```

Required full-build, focused Round 2-9 + repair + migration, full test suite, and ASan validation completed successfully after this follow-up update.

Additional commands run:

```sh
ctest --test-dir cmake-build -R "SemanticLoggerRound2Test|LogScopeOwnerRound3Test|ProductionLogApiRound4Test|SocketEndpointLogApiRound5Test|SemanticBackendRound6Test|SemanticFilterRound7Test|ControlledMigrationRound8Test|SemanticCompatibilityRound9Test|SemanticOverheadRound9Test|SemanticProductionThresholdRepairTest|SocketConnectionMigration01Test" --output-on-failure
ctest --test-dir cmake-build --output-on-failure
cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON
cmake --build cmake-build-asan --parallel 2
ASAN=$(gcc -print-file-name=libasan.so) LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan --output-on-failure
```

## Sanitizer result or reason not run

ASan build and ASan ctest suite completed successfully: 118/118 tests passed, with environment-controlled skipped integration/component tests reported by CTest as skipped.

## Deliberate deferrals

Six helper-function call sites remain on legacy macros because the helper lacks safe access to `SocketConnection::log()` and migration would require broader API churn.

## Known non-blocking follow-ups

A future migration can consider passing a semantic owner into the address-discovery helpers, but that is intentionally outside Migration 1 because it would affect construction/API shape beyond the allowed production file scope.
