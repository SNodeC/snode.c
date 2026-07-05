# Logging Roadmap Round 04 Report: Object Log APIs

Branch/report: `codex/implement-logging-roadmap-round-4` / Round 04 object log API plumbing.

## Implementation summary

Round 04 adds object-owned semantic logging API surfaces to the three agreed production areas only:

* `net::config::ConfigInstance`
* `core::socket::stream::SocketConnection`
* `core::socket::stream::SocketContext`

Each area uses `logger::LogScopeOwner` by composition. No logging inheritance, logging superclass, mixin, or `LogSuperClass` was introduced.

## Exact files changed

* `src/net/config/ConfigInstance.h`
* `src/net/config/ConfigInstance.cpp`
* `src/core/socket/stream/SocketConnection.h`
* `src/core/socket/stream/SocketConnection.cpp`
* `src/core/socket/stream/SocketContext.h`
* `src/core/socket/stream/SocketContext.cpp`
* `tests/unit/log/CMakeLists.txt`
* `tests/unit/log/ProductionLogApiRound4Test.cpp`
* `docs/logging/round-04-object-log-api-report.md`

## Exact build commands run

Fresh-branch preparation was attempted with the requested commands. This checkout has no configured `origin` remote, so `git fetch origin` failed with `fatal: 'origin' does not appear to be a git repository`. The working tree was clean on the merged Round 3 commit, so the Round 4 branch was created locally from the current checkout.

```sh
git fetch origin
```

```sh
git checkout -b codex/implement-logging-roadmap-round-4
```

The first normal configure failed because `nlohmann_json>=3.11` was missing from the container:

```sh
git diff --check
cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON
```

The missing dependency was installed and the normal configure/build was rerun successfully:

```sh
apt-get update && apt-get install -y nlohmann-json3-dev
cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON
cmake --build cmake-build --parallel 2
```

## Exact test commands run

```sh
ctest --test-dir cmake-build -R "SemanticLoggerRound2Test|LogScopeOwnerRound3Test|ProductionLogApiRound4Test" --output-on-failure
```

Result: passed, 3/3 tests.

```sh
ctest --test-dir cmake-build --output-on-failure
```

Result: passed, 110/110 tests, with the existing environment-dependent component tests reported as skipped by the existing harness.

## Sanitizer result

AddressSanitizer configure/build/test was started with:

```sh
cmake -S . -B cmake-build-asan \
  -DSNODEC_BUILD_TESTS=ON \
  -DSNODEC_BUILD_APPS=ON \
  -DSNODEC_ENABLE_ASAN=ON
cmake --build cmake-build-asan --parallel 2
ASAN=$(gcc -print-file-name=libasan.so) LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan --output-on-failure
```

Initial Round 4 result: ASan configure/build/test passed, 110/110 tests, with the existing environment-dependent component tests reported as skipped by the existing harness. The ASan build emitted linker warnings from libasan about tmpnam/tempnam usage, but the ASan ctest run completed successfully. Follow-up result: ASan was rerun after the ConfigInstance role/no-op documentation fix and passed, 110/110 tests, with the same existing environment-dependent skips.

## ConfigInstance log API result

`ConfigInstance` now owns a `logger::LogScopeOwner` member and exposes:

```cpp
logger::BoundaryLogger log() const;
logger::BoundaryLogger log(logger::BoundaryLogger::Sink sink,
                           logger::LogLevel threshold = logger::LogLevel::Trace,
                           logger::BoundaryLogger::Clock clock = {}) const;
```

The zero-argument API intentionally returns a no-op `BoundaryLogger` in Round 4 because backend unification is deferred to Round 6. It exists to establish the object API shape only. No production call sites were migrated to it, and broad migration must not happen before a real default backend sink exists. The sink-taking overload is used by tests to prove emitted semantic records contain the expected object scope.

## SocketConnection log API result

`SocketConnection` now owns a `logger::LogScopeOwner` member and exposes the same `log()` and sink-taking `log(...)` API shape. The zero-argument API intentionally returns a no-op `BoundaryLogger` in Round 4 because backend unification is deferred to Round 6. It exists to establish the object API shape only. No production call sites were migrated to it, and broad migration must not happen before a real default backend sink exists. The owned scope is initialized after `instanceName` and `connectionName` have been constructed.

## SocketContext log/frameworkLog API result

`core::socket::stream::SocketContext` now owns two `logger::LogScopeOwner` members by composition:

* `applicationLogScope`, returned by `log()` with `logger::LogOrigin::Application`
* `frameworkLogScope`, returned by `frameworkLog()` with `logger::LogOrigin::Framework`

The no-argument `log()`/`frameworkLog()` overloads intentionally return no-op `BoundaryLogger` instances in Round 4 because backend unification is deferred to Round 6. They exist to establish the object API shape only. No production call sites were migrated to them, and broad migration must not happen before a real default backend sink exists. The public APIs are:

```cpp
logger::BoundaryLogger log() const;
logger::BoundaryLogger log(logger::BoundaryLogger::Sink sink,
                           logger::LogLevel threshold = logger::LogLevel::Trace,
                           logger::BoundaryLogger::Clock clock = {}) const;
logger::BoundaryLogger frameworkLog() const;
logger::BoundaryLogger frameworkLog(logger::BoundaryLogger::Sink sink,
                                    logger::LogLevel threshold = logger::LogLevel::Trace,
                                    logger::BoundaryLogger::Clock clock = {}) const;
```

## Origin/boundary/component/instance/role/connection choices

### ConfigInstance

* origin: `framework`
* boundary: `configuration`
* component: `configuration`
* instance: configured instance name when non-empty
* role: server/client mapped from `ConfigInstance::Role`
* connection: absent

The configuration boundary was chosen because `ConfigInstance` represents configuration identity in the net configuration tree. No fake instance name is invented for empty instance names.

### SocketConnection

* origin: `framework`
* boundary: `connection`
* component: `core.socket.stream`
* instance: configured instance name when non-empty
* role: unknown/absent for Round 04 because the base connection object does not reliably know server/client role
* connection: existing `connectionName`

### SocketContext

* `log()` origin: `application`
* `frameworkLog()` origin: `framework`
* boundary: `context`
* component: `core.socket.context`
* instance: parent connection instance name when non-empty
* role: unknown/absent for Round 04 because the base context does not reliably know server/client role
* connection: parent connection `connectionName`

## Lifetime/ownership result

All Round 04 object-owned identity is stored in `logger::LogScopeOwner`. The added production objects do not store caller-owned `std::string_view` identity data. Returned `BoundaryLogger` instances are created from `LogScopeOwner::logger(...)`, which copies the owned scope into the logger. Temporary `LogScope` views are not retained by these objects.

## Macro compatibility result

Round 04 did not edit `src/log/Logger.h`, did not remove or rewrite legacy macro definitions, and did not migrate existing production macro call sites. Existing Round 2 macro smoke coverage still passes through `SemanticLoggerRound2Test`.

## Non-goal confirmations

* `LOG`, `PLOG`, and `VLOG` were not changed.
* No broad production call-site migration was done.
* `SocketServer`, `SocketClient`, `SocketAcceptor`, and `SocketConnector` were not integrated.
* HTTP server/client objects, WebSocket objects, MQTT objects, DB objects, application examples, and MiniGateway were not integrated.
* Backend unification was not started.
* CLI logging behavior, startup filter configuration, global/origin/boundary/component/instance precedence, `freeze()`, runtime reload, signals, admin endpoints, and mutable atomic thresholds were not implemented.

## Deliberate deferrals

* Semantic backends and integration with legacy logger sinks remain deferred.
* Server/client role derivation for connection and context scopes remains deferred until a later round can safely integrate acceptor/connector ownership without touching excluded objects in Round 04.
* Protocol-specific contexts such as MQTT, HTTP, and WebSocket remain deferred.
* Broad replacement of hand-prefixed `LOG`/`PLOG` messages remains deferred.

## Known non-blocking follow-ups

* A future round can wire object semantic loggers into the selected backend once backend unification is in scope.
* A future round can add role propagation when `SocketServer`/`SocketClient`/`SocketAcceptor`/`SocketConnector` integration is explicitly in scope.
* A future round can migrate targeted call sites from textual identity prefixes to semantic fields.
