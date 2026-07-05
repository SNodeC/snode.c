# Logging roadmap round 08 — controlled socket-stream subsystem migration

## Implementation summary

Round 8 performs only a controlled core socket-stream subsystem migration.

Round 8 migrates a small, reviewable subset of existing production logging call sites from legacy `LOG`/`PLOG` macros to existing object-owned semantic loggers in `SocketConnection` and `SocketContext`.

Round 8 does not migrate protocol-specific subsystems.

Round 8 does not change LOG/PLOG/VLOG macro definitions.

Round 8 does not change startup semantic policy.

Round 8 does not add runtime reload, signals, admin endpoints, or mutable atomic runtime thresholds.

The Codex environment had no usable `origin` remote: `git fetch origin` failed with `fatal: 'origin' does not appear to be a git repository`. The local checkout had no `master` branch, but the checked-out commit was `b946e5c Merge pull request #148 from SNodeC/codex/implement-startup-only-semantic-filters`, which contains merged Round 7. The working tree was clean before the Round 8 branch was created from that commit.

## Exact files changed

- `src/core/socket/stream/SocketConnection.cpp`
- `src/core/socket/stream/SocketContext.cpp`
- `tests/unit/log/ControlledMigrationRound8Test.cpp`
- `tests/unit/log/CMakeLists.txt`
- `docs/logging/round-08-controlled-subsystem-migration-report.md`

## Exact build commands run

```sh
git fetch origin
```

Failed because no usable `origin` remote exists in this environment.

```sh
git checkout master
```

Failed because no local `master` branch exists in this environment.

```sh
git status --short
git checkout -b codex/implement-logging-roadmap-round-8
cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON
cmake --build cmake-build --parallel 2
cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON
cmake --build cmake-build-asan --parallel 2
```

The first non-ASan configure attempt initially failed because `nlohmann-json3-dev` was missing. I installed it with:

```sh
apt-get update && apt-get install -y nlohmann-json3-dev
```

Then the required non-ASan configure/build succeeded.

## Exact test commands run

```sh
git diff --check
ctest --test-dir cmake-build -R "SemanticLoggerRound2Test|LogScopeOwnerRound3Test|ProductionLogApiRound4Test|SocketEndpointLogApiRound5Test|SemanticBackendRound6Test|SemanticFilterRound7Test|ControlledMigrationRound8Test" --output-on-failure
ctest --test-dir cmake-build --output-on-failure
ASAN=$(gcc -print-file-name=libasan.so)
LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan --output-on-failure
```

Focused tests initially failed while the Round 8 assertions expected older `origin=...` text labels. The production code was unchanged; the test assertions were corrected to match the current semantic text output (`framework connection ...`, `framework context ...`), and the focused test command then passed.

## Sanitizer result

ASan configure, build, and test run succeeded with:

```sh
cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON
cmake --build cmake-build-asan --parallel 2
ASAN=$(gcc -print-file-name=libasan.so)
LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan --output-on-failure
```

The linker emitted libasan warnings about `tmpnam_r`, `tmpnam`, and `tempnam`; no ASan test failures were reported.

## Migration scope

Round 8 only touches the core socket-stream production files below:

- `src/core/socket/stream/SocketConnection.cpp`
- `src/core/socket/stream/SocketContext.cpp`

No HTTP, WebSocket, MQTT, DB, application example, MiniGateway, OAuth, `apps/`, `src/iot/`, `src/web/`, `src/db/`, or protocol-family wrapper production code was migrated.

## Candidate inventory summary

Before editing, the allowed files were inventoried with:

```sh
rg -n "\b(P?LOG|VLOG)\s*\(" src/core/socket/stream/SocketConnection.cpp src/core/socket/stream/SocketConnection.hpp src/core/socket/stream/SocketContext.cpp src/core/socket/stream/SocketConnector.hpp src/core/socket/stream/SocketAcceptor.hpp src/core/socket/stream/SocketServer.h src/core/socket/stream/SocketClient.h src/core/socket/stream/SocketConnector.h src/core/socket/stream/SocketAcceptor.h
```

Summary:

- `SocketConnection.cpp`: 3 legacy `LOG` candidates; all 3 migrated.
- `SocketContext.cpp`: 8 legacy `LOG`/`PLOG` candidates; all 8 migrated.
- `SocketConnection.hpp`: many template/lifecycle candidates, including `PLOG`, deferred to avoid a broad template migration in Round 8.
- `SocketConnector.hpp` and `SocketAcceptor.hpp`: many endpoint lifecycle and syscall candidates, deferred to keep Round 8 deliberately small.
- `SocketServer.h` and `SocketClient.h`: many endpoint callback candidates, deferred to keep Round 8 deliberately small.
- No `VLOG(...)` candidates were found in the inventoried allowed files.

## Migrated call-site table

| File | Old call form | New call form | Reason migrated | Semantic owner used | origin/boundary/component/instance/role/connection expectation | errno preserved? |
| --- | --- | --- | --- | --- | --- | --- |
| `src/core/socket/stream/SocketConnection.cpp` | `LOG(DEBUG) << connectionName << ": SocketContext created successful"` | `log().debug("SocketContext created successful")` | Direct connection lifecycle diagnostic with existing connection owner | `SocketConnection::log()` | framework / connection / `core.socket.stream` / instance when present / no role / connection present | not applicable |
| `src/core/socket/stream/SocketConnection.cpp` | `LOG(ERROR) << connectionName << ": SocketContext failed to create"` | `log().error("SocketContext failed to create")` | Direct connection lifecycle diagnostic with existing connection owner | `SocketConnection::log()` | framework / connection / `core.socket.stream` / instance when present / no role / connection present | not applicable |
| `src/core/socket/stream/SocketConnection.cpp` | `LOG(DEBUG) << connectionName << " SocketContext: switch"` | `log().debug("SocketContext: switch")` | Direct context-switch diagnostic with existing connection owner | `SocketConnection::log()` | framework / connection / `core.socket.stream` / instance when present / no role / connection present | not applicable |
| `src/core/socket/stream/SocketContext.cpp` | `PLOG(DEBUG) << socketConnection->getConnectionName() << " SocketContext: onWriteError"` | `frameworkLog().sysError(logger::LogLevel::Debug, errnum, "SocketContext: onWriteError")` | Framework-internal write-error diagnostic with errno payload | `SocketContext::frameworkLog()` | framework / context / `core.socket.context` / instance when present / no role / connection present | yes |
| `src/core/socket/stream/SocketContext.cpp` | `LOG(DEBUG) << socketConnection->getConnectionName() << " SocketContext: EOF received"` | `frameworkLog().debug("SocketContext: EOF received")` | Framework-internal EOF read diagnostic | `SocketContext::frameworkLog()` | framework / context / `core.socket.context` / instance when present / no role / connection present | not applicable |
| `src/core/socket/stream/SocketContext.cpp` | `PLOG(DEBUG) << socketConnection->getConnectionName() << " SocketContext: onReadError"` | `frameworkLog().sysError(logger::LogLevel::Debug, errnum, "SocketContext: onReadError")` | Framework-internal read-error diagnostic with errno payload | `SocketContext::frameworkLog()` | framework / context / `core.socket.context` / instance when present / no role / connection present | yes |
| `src/core/socket/stream/SocketContext.cpp` | `LOG(DEBUG) << socketConnection->getConnectionName() << " SocketContext: detached"` | `frameworkLog().debug("SocketContext: detached")` | Framework-internal detach lifecycle diagnostic | `SocketContext::frameworkLog()` | framework / context / `core.socket.context` / instance when present / no role / connection present | not applicable |
| `src/core/socket/stream/SocketContext.cpp` | `LOG(DEBUG) << "     Online Since: " << getOnlineSince()` | `frameworkLog().debug("     Online Since: {}", getOnlineSince())` | Framework-internal detach summary detail | `SocketContext::frameworkLog()` | framework / context / `core.socket.context` / instance when present / no role / connection present | not applicable |
| `src/core/socket/stream/SocketContext.cpp` | `LOG(DEBUG) << "  Online Duration: " << getOnlineDuration()` | `frameworkLog().debug("  Online Duration: {}", getOnlineDuration())` | Framework-internal detach summary detail | `SocketContext::frameworkLog()` | framework / context / `core.socket.context` / instance when present / no role / connection present | not applicable |
| `src/core/socket/stream/SocketContext.cpp` | `LOG(DEBUG) << "       Total Sent: " << getTotalQueued()` | `frameworkLog().debug("       Total Sent: {}", getTotalQueued())` | Framework-internal detach summary detail | `SocketContext::frameworkLog()` | framework / context / `core.socket.context` / instance when present / no role / connection present | not applicable |
| `src/core/socket/stream/SocketContext.cpp` | `LOG(DEBUG) << "  Total Processed: " << getTotalProcessed()` | `frameworkLog().debug("  Total Processed: {}", getTotalProcessed())` | Framework-internal detach summary detail | `SocketContext::frameworkLog()` | framework / context / `core.socket.context` / instance when present / no role / connection present | not applicable |

Migrated production call-site count: 11.

## Deferred call-site table

| File | Candidate call form | Deferral reason |
| --- | --- | --- |
| `src/core/socket/stream/SocketConnection.hpp` | Template constructor address-discovery `LOG`/`PLOG` calls | Template helper path requires a separate review to avoid broad constructor/template churn and to preserve errno at syscall boundaries. |
| `src/core/socket/stream/SocketConnection.hpp` | Read/write shutdown, timeout, signal, and disconnect `LOG`/`PLOG` calls | Suitable but deferred to keep Round 8 below a broad production migration and within a small reviewable slice. |
| `src/core/socket/stream/SocketConnector.hpp` | Connector open/bind/connect/getsockopt `LOG`/`PLOG` calls | Endpoint migration is suitable but includes many syscall/error paths; deferred to keep Round 8 focused. |
| `src/core/socket/stream/SocketAcceptor.hpp` | Acceptor open/bind/listen/accept `LOG`/`PLOG` calls | Endpoint migration is suitable but includes many syscall/error paths; deferred to keep Round 8 focused. |
| `src/core/socket/stream/SocketServer.h` | Default endpoint callback and listen retry `LOG` calls | Suitable endpoint call sites, but deferred because Round 8 already migrated 11 connection/context call sites. |
| `src/core/socket/stream/SocketClient.h` | Default endpoint callback and reconnect retry `LOG` calls | Suitable endpoint call sites, but deferred because Round 8 already migrated 11 connection/context call sites. |

No `VLOG(...)` candidates were found in the inventoried allowed files; verbose migration remains deferred as a general policy item.

## Semantic owners used

- `SocketConnection` call sites use the existing `SocketConnection::log()` owner.
- `SocketContext` framework-internal call sites use the existing `SocketContext::frameworkLog()` owner.
- No `BoundaryLogger` is stored in production objects.
- No new `LogScopeOwner` members were added.

## Severity mapping result

- `LOG(DEBUG)` migrated to `.debug(...)`.
- `LOG(ERROR)` migrated to `.error(...)`.
- `PLOG(DEBUG)` migrated to `.sysError(logger::LogLevel::Debug, errnum, ...)`.
- No `LOG(TRACE)`, `LOG(INFO)`, `LOG(WARNING)`, `LOG(FATAL)`, or `VLOG(...)` call sites were migrated in this slice.

## PLOG/sysError result

Two `PLOG(DEBUG)` call sites in `SocketContext` were migrated to semantic `sysError` calls. The existing `errnum` parameter is used directly, so errno code and text are preserved without relying on later global `errno` state. The production methods still assign `errno = errnum` before logging, preserving legacy side effects.

## VLOG deferral result

No `VLOG(...)` candidates were found in the allowed files. No verbose-level migration was performed.

## Identity-prefix cleanup result

Connection-name prefixes were removed only where semantic scope already carries connection identity:

- `SocketConnection::log()` carries the connection boundary and connection identity.
- `SocketContext::frameworkLog()` carries context boundary, instance identity when present, and connection identity.

Other message text was preserved where it described the event or metric.

## LogManager filter compatibility result

`ControlledMigrationRound8Test` verifies a migrated `SocketContext` debug call is suppressed when `LogManager` global level is `Error`.

## Logger::setLogLevel backend-gate compatibility result

`ControlledMigrationRound8Test` verifies a migrated semantic debug call is suppressed by `Logger::setLogLevel(3)` even when `LogManager` permits trace/debug records.

## Text/Json format compatibility result

`ControlledMigrationRound8Test` verifies a migrated semantic call reaches the text backend with semantic fields and reaches JSON output when `LogManager::Format::Json` is selected.

## Legacy macro compatibility result

Legacy `LOG`, `PLOG`, and `VLOG` macro definitions were not changed. Existing Round 2 through Round 7 semantic logging tests still pass, including the Round 7 legacy macro compatibility coverage.

## Confirmation of non-goals

- Macro definitions were not changed.
- No broad production migration was done.
- HTTP, WebSocket, MQTT, DB, apps, MiniGateway, OAuth examples, and protocol-specific subsystems were not migrated.
- Startup semantic policy was not changed.
- No runtime reload, signals, admin endpoints, or mutable atomic runtime thresholds were added.
- JSON schema and format selection behavior were not changed.
- No `LogSuperClass`, logging inheritance, or logging mixin base was added.

## Deliberate deferrals

- `SocketConnection.hpp` template call sites remain for a follow-up because they include syscall/PLOG paths and broader lifecycle flow.
- `SocketConnector.hpp` and `SocketAcceptor.hpp` endpoint syscall paths remain for a follow-up so errno handling can be reviewed carefully.
- `SocketServer.h` and `SocketClient.h` default callback logs remain for a follow-up endpoint-specific round.
- A broader SNode.C and mqttsuite review was not folded into this production migration because Round 8 explicitly forbids protocol-specific migration and must remain small.

## Known non-blocking follow-ups

- Continue controlled migration in `SocketConnection.hpp` with focused tests around shutdown/read/write paths.
- Continue endpoint migration in `SocketConnector.hpp` and `SocketAcceptor.hpp`, especially syscall `PLOG` sites with explicit errno preservation.
- Consider endpoint default callback logs in `SocketServer.h` and `SocketClient.h` once endpoint role assertions are added for migrated production calls.
- Keep verbose-level migration as a separate policy decision if `VLOG(...)` candidates are encountered later.
