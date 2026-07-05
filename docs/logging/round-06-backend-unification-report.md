# Logging roadmap round 06 — backend unification report

## Implementation summary

Round 6 replaces the Round 4/5 no-op default semantic loggers with backend-backed default semantic loggers.

The implementation adds a narrow semantic backend path to `logger::Logger` and wires default no-argument semantic logger APIs to that backend. Semantic records are formatted with the existing Round 2 text formatter and emitted through the same `Logger.cpp` stdout/file sink infrastructure used by `LOG`, `PLOG`, and `VLOG`.

The environment had no usable `origin` remote: `git fetch origin` failed with `fatal: 'origin' does not appear to be a git repository`. The checked-out commit already contained merged Round 5 (`b09bffa Merge pull request #146 from SNodeC/codex/implement-logging-for-socket-endpoints`), the working tree was clean, and the Round 6 branch was created from that current commit.

## Exact files changed

- `src/log/Logger.h`
- `src/log/Logger.cpp`
- `src/net/config/ConfigInstance.cpp`
- `src/core/socket/stream/SocketConnection.cpp`
- `src/core/socket/stream/SocketContext.cpp`
- `src/core/socket/stream/SocketServer.h`
- `src/core/socket/stream/SocketClient.h`
- `src/core/socket/stream/SocketConnector.h`
- `src/core/socket/stream/SocketAcceptor.h`
- `tests/unit/log/CMakeLists.txt`
- `tests/unit/log/SemanticBackendRound6Test.cpp`
- `docs/logging/round-06-backend-unification-report.md`

## Semantic backend API added

`logger::Logger` now exposes:

- `static void emitSemantic(const logger::LogRecord& record);`
- `static logger::BoundaryLogger::Sink semanticSink();`

`emitSemantic` is the narrow backend bridge. `semanticSink` returns a `BoundaryLogger` sink that forwards materialized semantic records to `emitSemantic`.

## LogLevel to legacy Level mapping

Round 6 maps semantic levels to existing legacy levels as follows:

- `logger::LogLevel::Trace` -> `logger::Level::TRACE`
- `logger::LogLevel::Debug` -> `logger::Level::DEBUG`
- `logger::LogLevel::Info` -> `logger::Level::INFO`
- `logger::LogLevel::Warn` -> `logger::Level::WARNING`
- `logger::LogLevel::Error` -> `logger::Level::ERROR`
- `logger::LogLevel::Critical` -> `logger::Level::FATAL`
- `logger::LogLevel::Off` -> no emission

Semantic backend emission uses `Logger::shouldLog(...)` through that mapping. Round 6 does not add a second semantic threshold system.

## Default no-argument semantic logger result

The following no-argument APIs now return backend-backed semantic loggers instead of no-op loggers:

- `ConfigInstance::log()`
- `SocketConnection::log()`
- `SocketContext::log()`
- `SocketContext::frameworkLog()`
- `SocketServer::log()`
- `SocketClient::log()`
- `SocketConnector::log()`
- `SocketAcceptor::log()`

Each default logger uses `logger::Logger::semanticSink()` and therefore emits through the existing logger backend when `Logger::setLogLevel(...)` enables the mapped legacy level.

## Sink-taking overload compatibility result

The sink-taking overloads remain available and continue to bypass/capture semantic records for tests and custom integrations. Existing Round 4 and Round 5 API-shape tests pass, and `SemanticBackendRound6Test` adds a direct custom-sink compatibility assertion.

## Legacy macro compatibility result

`LOG`, `PLOG`, and `VLOG` macro syntax was not changed. No production `LOG`, `PLOG`, or `VLOG` call sites were migrated. A Round 6 test confirms legacy `LOG(INFO)` still emits through the backend while semantic emission uses the same file sink.

Round 6 does not migrate production LOG/PLOG/VLOG call sites. Controlled subsystem migration remains Round 8 work.

## CLI behavior preservation result

Round 6 adds no CLI options, renames no CLI options, and changes no CLI defaults. Existing numeric log-level behavior remains the filtering mechanism for semantic backend emission. Verbose-level behavior and `VLOG` remain unchanged.

## stdout/file/quiet/color/tick behavior result

Semantic default backend emission reuses the existing `Logger.cpp` `emitLine` path, so it inherits:

- stdout logging when not quiet,
- file logging when `Logger::logToFile(...)` is enabled,
- `Logger::setQuiet(...)` stdout suppression,
- existing color-disable behavior for legacy level labels,
- existing tick resolver / timestamp pattern behavior,
- existing rotating file sink setup.

The focused Round 6 test uses a controlled file sink and controlled tick resolver to verify file emission and tick resolver use without depending on wall-clock output.

## Filtering result

Filtering is only the existing numeric `Logger::setLogLevel(...)` behavior. There is no semantic precedence and no independent semantic runtime threshold configuration in Round 6.

Round 6 does not implement startup-only semantic filtering. The semantic filter precedence instance > component > boundary > origin > global remains Round 7 work.

## Confirmation of non-goals

- No startup semantic filter configuration was implemented.
- No global/origin/boundary/component/instance precedence was implemented.
- No `LogManager::freeze()` or `freeze()` equivalent was implemented.
- No runtime reload, signals, admin endpoints, or mutable atomic semantic thresholds were implemented.
- No production call-site migration was done.
- Backend unification did not start Round 7 or Round 8.

## Exact build commands run

```sh
git fetch origin
git checkout master
git pull --ff-only origin master
git status --short
git checkout -b codex/implement-logging-roadmap-round-6
git status --short
git diff --check
cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON
cmake --build cmake-build --parallel 2
cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON
cmake --build cmake-build-asan --parallel 2
```

`git fetch origin` failed because the environment has no usable `origin` remote. `git checkout master` also could not be used because this environment only had the `work` branch, but that branch pointed at the current Round 5 merge commit listed above.

The first configure attempt failed before installing missing optional/development packages because `nlohmann-json3-dev` was absent. The environment allowed installing `nlohmann-json3-dev`, `libmagic-dev`, and `libbluetooth-dev`, after which the required apps-enabled configure and build succeeded.

## Exact test commands run

```sh
ctest --test-dir cmake-build -R "SemanticLoggerRound2Test|LogScopeOwnerRound3Test|ProductionLogApiRound4Test|SocketEndpointLogApiRound5Test|SemanticBackendRound6Test" --output-on-failure
ctest --test-dir cmake-build --output-on-failure
ASAN=$(gcc -print-file-name=libasan.so); LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan --output-on-failure
```

## Sanitizer result

ASan was run successfully. The ASan build completed with linker warnings from `libasan.so.8` about `tmpnam`/`tempnam` use, and the ASan `ctest` run reported 100% tests passed with skipped integration tests matching the normal test run behavior.

## Deliberate deferrals

- Startup-only semantic filter configuration remains Round 7 work.
- Semantic filter precedence `instance > component > boundary > origin > global` remains Round 7 work.
- Runtime format selection between text and JSON remains deferred.
- Controlled production call-site migration remains Round 8 work.
- Broader CLI-facing semantic configuration remains deferred.

## Known non-blocking follow-ups

- Consider a future runtime format selection API when Round 7 introduces semantic startup configuration.
- Consider additional direct stdout/quiet/color assertions if a future test sink hook is accepted; Round 6 used controlled file sinks to avoid broad public backend replacement APIs.
- The broader SNode.C and MQTT suite still have many integration tests skipped in this container due the existing test environment gating; those are not introduced by Round 6.
