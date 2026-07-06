# Semantic logging migration 02 — SocketConnector and SocketAcceptor

## Implementation summary

This is a clean Migration 02 PR from the current local `master` baseline after PR #151 and PR #154. PR #151 already provides the production-threshold repair. PR #154 already completed Migration 1 for `SocketConnection.hpp`. This PR does not implement, duplicate, cherry-pick from, or rebase either PR #151 or PR #154.

The scoped header inventory for `SocketConnector.h` and `SocketAcceptor.h` contained no legacy `LOG`, `PLOG`, or `VLOG` call sites. Therefore there were no suitable production member macro call sites inside the allowed production files to convert. The production changes are limited to documenting that the existing object-owned semantic `this->log()` scope remains the Migration 02 owner for these endpoint types.

## Exact changed files

This PR changes only:

```text
docs/logging/semantic-migration-02-socketconnector-acceptor-report.md
src/core/socket/stream/SocketAcceptor.h
src/core/socket/stream/SocketConnector.h
tests/unit/log/CMakeLists.txt
tests/unit/log/SocketConnectorAcceptorMigration02Test.cpp
```

## Base verification result

Branch preparation was attempted exactly as requested, with the following local-environment difference: this checkout has no `origin` remote, so `git fetch origin` and `git pull --ff-only origin master` could not be performed. The local baseline commit is already the merged PR #154 commit.

Commands and results:

```text
$ git fetch origin
fatal: 'origin' does not appear to be a git repository
fatal: Could not read from remote repository.
```

```text
$ git branch --show-current
work

$ git status --short

$ git log --oneline -n 40
...
ec7f020 Merge pull request #154 from SNodeC/codex/create-new-migration-branch-for-socketconnection
b8d63d1 Semantic logging migration 01 SocketConnection.hpp
0aa69f6 Merge pull request #151 from SNodeC/codex/repair-semantic-logging-production-thresholds
23b0656 Semantic logging production threshold repair
...
```

PR #151 verification:

```text
$ test -f docs/logging/semantic-production-threshold-repair-report.md
$ test -f tests/unit/log/SemanticProductionThresholdRepairTest.cpp
$ git log --oneline -n 40 | grep "0aa69f6"
0aa69f6 Merge pull request #151 from SNodeC/codex/repair-semantic-logging-production-thresholds
```

PR #154 / Migration 1 verification:

```text
$ test -f docs/logging/semantic-migration-01-socketconnection-hpp-report.md
$ test -f tests/unit/log/SocketConnectionMigration01Test.cpp
$ git log --oneline -n 40 | grep "154"
ec7f020 Merge pull request #154 from SNodeC/codex/create-new-migration-branch-for-socketconnection
```

## Initial inventory command/result

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" src/core/socket/stream/SocketConnector.h src/core/socket/stream/SocketAcceptor.h
```

Result:

```text
<no matches; rg exit status 1>
```

Ownership discovery command:

```sh
rg -n "\blog\(\)|logger::|BoundaryLogger|LogScopeOwner|semantic" src/core/socket/stream/SocketConnector.h src/core/socket/stream/SocketAcceptor.h
```

Result:

```text
src/core/socket/stream/SocketConnector.h:47:#include "log/LogScopeOwner.h"
src/core/socket/stream/SocketConnector.h:92:        logger::BoundaryLogger log() const {
src/core/socket/stream/SocketConnector.h:93:            // Round 6 backend-backed default semantic logger.
src/core/socket/stream/SocketConnector.h:94:            // Migration 02 keeps SocketConnector production logging on this object-owned semantic scope.
src/core/socket/stream/SocketConnector.h:96:            // Production default logger uses the frozen startup semantic policy as its local threshold.
src/core/socket/stream/SocketConnector.h:97:            return logScope.logger(logger::Logger::semanticSink());
src/core/socket/stream/SocketConnector.h:100:        logger::BoundaryLogger log(logger::BoundaryLogger::Sink sink,
src/core/socket/stream/SocketConnector.h:101:                                   logger::LogLevel threshold = logger::LogLevel::Trace,
src/core/socket/stream/SocketConnector.h:102:                                   logger::BoundaryLogger::Clock clock = {}) const {
src/core/socket/stream/SocketConnector.h:130:        static logger::LogScopeOwner makeLogScope(const std::string& instanceName) {
src/core/socket/stream/SocketConnector.h:131:            return logger::LogScopeOwner(logger::LogOrigin::Framework,
src/core/socket/stream/SocketConnector.h:132:                                         logger::LogBoundary::Instance,
src/core/socket/stream/SocketConnector.h:135:                                         logger::LogRole::Client,
src/core/socket/stream/SocketConnector.h:139:        logger::LogScopeOwner logScope;
src/core/socket/stream/SocketAcceptor.h:47:#include "log/LogScopeOwner.h"
src/core/socket/stream/SocketAcceptor.h:97:        logger::BoundaryLogger log() const {
src/core/socket/stream/SocketAcceptor.h:98:            // Round 6 backend-backed default semantic logger.
src/core/socket/stream/SocketAcceptor.h:99:            // Migration 02 keeps SocketAcceptor production logging on this object-owned semantic scope.
src/core/socket/stream/SocketAcceptor.h:101:            // Production default logger uses the frozen startup semantic policy as its local threshold.
src/core/socket/stream/SocketAcceptor.h:102:            return logScope.logger(logger::Logger::semanticSink());
src/core/socket/stream/SocketAcceptor.h:105:        logger::BoundaryLogger log(logger::BoundaryLogger::Sink sink,
src/core/socket/stream/SocketAcceptor.h:106:                                   logger::LogLevel threshold = logger::LogLevel::Trace,
src/core/socket/stream/SocketAcceptor.h:107:                                   logger::BoundaryLogger::Clock clock = {}) const {
src/core/socket/stream/SocketAcceptor.h:134:        static logger::LogScopeOwner makeLogScope(const std::string& instanceName) {
src/core/socket/stream/SocketAcceptor.h:135:            return logger::LogScopeOwner(logger::LogOrigin::Framework,
src/core/socket/stream/SocketAcceptor.h:136:                                         logger::LogBoundary::Instance,
src/core/socket/stream/SocketAcceptor.h:139:                                         logger::LogRole::Server,
src/core/socket/stream/SocketAcceptor.h:143:        logger::LogScopeOwner logScope;
```

## Post-migration inventory command/result

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" src/core/socket/stream/SocketConnector.h src/core/socket/stream/SocketAcceptor.h
```

Result:

```text
<no matches; rg exit status 1>
```

All suitable member `LOG`/`PLOG` call sites in the allowed `SocketConnector.h` and `SocketAcceptor.h` files are migrated or absent. There are no remaining `LOG`, `PLOG`, or `VLOG` call sites in those files.

## Migrated call-site table

| File | Legacy call site | New semantic call | Notes |
| --- | --- | --- | --- |
| `src/core/socket/stream/SocketConnector.h` | none found | none needed | Existing production owner remains `this->log()`. |
| `src/core/socket/stream/SocketAcceptor.h` | none found | none needed | Existing production owner remains `this->log()`. |

## Deferred call-site table

| File | Deferred call site | Reason |
| --- | --- | --- |
| `src/core/socket/stream/SocketConnector.h` | none | No `LOG`, `PLOG`, or `VLOG` matches in the scoped file. |
| `src/core/socket/stream/SocketAcceptor.h` | none | No `LOG`, `PLOG`, or `VLOG` matches in the scoped file. |

## Semantic owner used

The semantic owner remains the existing object-owned `this->log()` API backed by `logScope.logger(logger::Logger::semanticSink())`. No new `BoundaryLogger`, `LogScopeOwner`, inheritance, mixin, or logger ownership was added.

`SocketConnector` uses framework/instance/component `core.socket.stream` with client role. `SocketAcceptor` uses framework/instance/component `core.socket.stream` with server role.

## Severity mapping

No legacy macro call sites were present in the allowed header files, so no severity remapping was required in this PR. The applicable mapping remains:

| Legacy | Semantic |
| --- | --- |
| `LOG(TRACE)` | `this->log().trace(...)` |
| `LOG(DEBUG)` | `this->log().debug(...)` |
| `LOG(INFO)` | `this->log().info(...)` |
| `LOG(WARNING)` | `this->log().warn(...)` |
| `LOG(ERROR)` | `this->log().error(...)` |
| `LOG(FATAL)` | `this->log().critical(...)` |
| `PLOG(level)` | `this->log().sysError(logger::LogLevel::<level>, errnum, ...)` |

## PLOG/sysError errno handling

No `PLOG` call sites were present in the allowed header files. The new Migration 02 unit test still exercises `sysError` through the connector semantic owner to prove errno code/text preservation for the owner path if a future scoped `PLOG` conversion is added.

## VLOG result

No `VLOG` call sites were present in the allowed header files. No `VLOG` call sites were migrated.

## Identity-prefix cleanup result

No identity-prefixed legacy messages were present in the allowed header files, so no prefix cleanup was required. The existing semantic scopes already carry instance and role metadata.

## Filter/backend/format compatibility

The new `SocketConnectorAcceptorMigration02Test` covers:

* connector semantic owner emission when enabled;
* acceptor semantic owner emission when enabled;
* framework/instance/component and client/server role scope;
* LogManager filtering suppression;
* `Logger::setLogLevel` backend suppression;
* JSON format selection;
* `sysError` errno code/text preservation through the connector owner path;
* sink-taking overload compatibility;
* suppressed production formatting not evaluating `ExpensiveValue` after PR #151.

## Production-scope confirmations

* This PR changes only SocketConnector.h, SocketAcceptor.h, tests/unit/log/CMakeLists.txt, SocketConnectorAcceptorMigration02Test.cpp, and this report.
* This PR does not modify SemanticLogger.*.
* This PR does not modify LogScopeOwner.*.
* This PR does not modify Logger.*.
* This PR does not modify ConfigInstance.cpp.
* This PR does not modify SocketConnection.*.
* This PR does not modify SocketConnection.hpp.
* This PR does not modify SocketContext.cpp.
* This PR does not modify SocketServer.h.
* This PR does not modify SocketClient.h.
* This PR does not modify SemanticProductionThresholdRepairTest.cpp.
* This PR does not modify SocketConnectionMigration01Test.cpp.
* This PR does not change LOG/PLOG/VLOG macro definitions.
* This PR does not change LogManager precedence.
* This PR does not change LogManager freeze behavior.
* This PR does not change JSON v1 schema.
* This PR does not start Round 10.

## Build commands run

```sh
git fetch origin
```

Failed because this local checkout has no `origin` remote.

```sh
cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON
cmake --build cmake-build --target SocketConnectorAcceptorMigration02Test --parallel 2
```

Passed after installing missing system packages (`nlohmann-json3-dev`, `libmagic-dev`, and `libbluetooth-dev`) required by the requested apps-enabled configure.

## Additional validation commands run

Passed:

```sh
git status --short
git diff --name-only master...HEAD
git diff --check
cmake --build cmake-build --parallel 2
```

## Test commands run

```sh
ctest --test-dir cmake-build -R "SemanticLoggerRound2Test|LogScopeOwnerRound3Test|ProductionLogApiRound4Test|SocketEndpointLogApiRound5Test|SemanticBackendRound6Test|SemanticFilterRound7Test|ControlledMigrationRound8Test|SemanticCompatibilityRound9Test|SemanticOverheadRound9Test|SemanticProductionThresholdRepairTest|SocketConnectionMigration01Test|SocketConnectorAcceptorMigration02Test" --output-on-failure
```

Passed: 12/12 focused logging tests.

```sh
ctest --test-dir cmake-build --output-on-failure
```

Passed: 119/119 tests reported as pass/skip with 0 failures. Several component tests were skipped by the suite's runtime skip policy.

## ASan result or exact reason not run

```sh
cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON
cmake --build cmake-build-asan --parallel 2
ASAN=$(gcc -print-file-name=libasan.so) LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan --output-on-failure
```

Passed: ASan configure, build, and ctest completed with 0 failures. The linker emitted libasan warnings about `tmpnam*` symbols; those warnings did not fail the ASan build or tests.

## Known follow-ups

* No Migration 03 work was started.
* No Round 10 work was started.
* No HTTP/WebSocket/MQTT/DB/app/MiniGateway migration was performed.
* The user requested an extended general review of SNode.C itself and mqttsuite; this code-change PR remains restricted to the hard five-file Migration 02 allowlist.
