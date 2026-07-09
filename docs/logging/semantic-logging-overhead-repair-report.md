# Semantic logging runtime-overhead repair

## Problem summary

After the semantic logging migration and root CLI integration, disabled semantic logs could still be too expensive in hot paths. The main risk was that a helper-created `BoundaryLogger` could locally allow `Trace`, format expensive arguments or payload dumps, materialize a record, timestamp it, and only then be rejected by `LogManager`.

## Valgrind observation

The motivating observation was disproportionate overhead while running `httpclient-tls-in` under Valgrind. Valgrind amplifies logging costs, but the underlying issue is real when disabled semantic diagnostics still construct payload dumps or records before policy rejection.

## Disabled-log cost model

A disabled semantic log must be rejected by `BoundaryLogger::enabled(level)` before expensive work. The disabled path must avoid expensive argument formatting, payload dump construction, `LogRecord` materialization, timestamping, and sink calls.

## Convenience helper threshold issue

Root-level no-threshold helpers in `src/SemanticLog.h` previously shared the explicit-threshold overload default of `LogLevel::Trace`. This made production calls such as `snode::semantic::webHttpLog().debug(...)` locally enabled even when the effective semantic policy would reject them later. This PR adds production no-argument and identity-only overloads that construct loggers with `LogScopeOwner(...).logger(Logger::semanticSink())`, which captures `LogManager::effectiveLevel(...)` for the helper scope.

Explicit sink/threshold overloads remain available for tests and custom capture. Those overloads intentionally keep `Trace` as the explicit-test default when a custom sink is supplied, preserving existing migration tests.

## Guarded expensive diagnostics issue

Repeated-helper guards construct and resolve the same logger twice, for example `mqttLog().enabled(...)` followed by `mqttLog().trace() << toHexString(...)`. This PR updates obvious expensive MQTT and HTTP payload diagnostics to bind the logger once before the guard.

## Bind-once rule

Expensive guarded diagnostics should use:

```cpp
auto log = someSemanticLog();
if (log.enabled(logger::LogLevel::Trace)) {
    log.trace() << expensiveThing();
}
```

This avoids duplicate helper construction and keeps the expensive expression inside the enabled branch.

## Cached logger rule

Cached `BoundaryLogger` members are allowed only when the scope identity is stable and the logger is constructed after semantic policy initialization. This PR did not add class-member caches because the targeted bind-once fixes were smaller and avoid lifecycle ambiguity.

## Why macros were not added

No macro guard API was added. The logging migration is moving away from macro-based logging, and macros can hide control flow or accidentally evaluate logger expressions more than once.

## Why async logging was not added

Async logging was not added because this PR targets disabled-log overhead. Async sinks can only help accepted-log I/O costs; they do not fix formatting, dump construction, or record materialization before rejection.

## Files inspected

- `src/SemanticLog.h`
- `src/log/SemanticLogger.h`
- `src/log/SemanticLogger.cpp`
- `src/log/LogScopeOwner.h`
- `src/log/LogScopeOwner.cpp`
- `src/log/Logger.cpp`
- `src/iot/mqtt/SemanticLog.h`
- `src/web/http/client/SemanticLog.h`
- `src/web/http/server/SemanticLog.h`
- `src/web/websocket/SemanticLog.h`
- `src/iot/mqtt/Mqtt.cpp`
- `src/web/http/client/SocketContext.cpp`
- `src/web/websocket/Receiver.cpp`
- `src/web/websocket/Transmitter.cpp`
- MQTT broker/session files reported by the guard search

## Implementation summary

- Added effective-threshold production overloads for root semantic helpers in `src/SemanticLog.h`.
- Preserved explicit test/custom capture overloads with explicit sink support.
- Bound MQTT payload/fixed-header trace diagnostics to a local logger before guarded `toHexString(...)` work.
- Bound the HTTP client rejected-request dump guard to a local logger before `httputils::toString(...)`.
- Added `SemanticLoggingOverheadTest` to prove disabled formatting and materialization are avoided and component overrides still enable selected debug logs.

## Tests added/updated

- Added `tests/unit/log/SemanticLoggingOverheadTest.cpp`.
- Registered `SemanticLoggingOverheadTest` in `tests/unit/log/CMakeLists.txt`.

## Search commands run

Before changes:

```sh
rg -n "\.enabled\(logger::LogLevel::" src -g '*.h' -g '*.hpp' -g '*.cpp'
rg -n "[A-Za-z0-9_:]*Log\(\)\.enabled|semantic::[A-Za-z0-9_:]*Log\(\)\.enabled" src -g '*.h' -g '*.hpp' -g '*.cpp'
rg -n "toHexString|httputils::toString|dump\(|Frame data|payload|body|header|certificate|packet|buffer|route" src -g '*.h' -g '*.hpp' -g '*.cpp'
rg -n "threshold = logger::LogLevel::Trace|const logger::LogLevel threshold = logger::LogLevel::Trace|LogScopeOwner\(.*\)\.logger\(.*threshold" src -g '*.h' -g '*.hpp' -g '*.cpp'
```

After changes:

```sh
rg -n "[A-Za-z0-9_:]*Log\(\)\.enabled|semantic::[A-Za-z0-9_:]*Log\(\)\.enabled" src -g '*.h' -g '*.hpp' -g '*.cpp'
rg -n "SNODEC_.*LOG|LOG_IF|TRACE_IF|DEBUG_IF|IF_ENABLED" src/log src tests docs -g '*.h' -g '*.hpp' -g '*.cpp' -g '*.md'
rg -n "LogLevel::Trace" src/SemanticLog.h src/iot/mqtt/SemanticLog.h src/web/http src/web/websocket src/log -g '*.h' -g '*.hpp' -g '*.cpp'
rg -n "\b(LOG|PLOG|VLOG)\s*\(" src -g '*.h' -g '*.hpp' -g '*.cpp'
```

Remaining repeated-helper guard matches are MQTT broker/session `Info`/`Debug` bookkeeping guards, not obvious expensive payload dump construction. They are left unchanged to keep this PR focused.

Remaining `LogLevel::Trace` defaults are explicit sink/test/custom capture overloads, `BoundaryLogger::createForTest`, semantic policy defaults, and intentional trace-level methods/guards. Production no-argument root helpers no longer default to unconditional `Trace`.

The macro search found existing compatibility definitions in `src/log/Logger.h` and non-compiled express compatibility-suite macro uses already documented for the final macro-removal/source-gate work. No new macro guard API was introduced.

## Tests run

```sh
cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON
cmake --build cmake-build --parallel 2
ctest --test-dir cmake-build -R "SemanticLoggingOverheadTest|SemanticCliIntegrationTest|SemanticFilterRound7Test|SemanticEndToEndOutputTest|SemanticCompatibilityRound9Test|SemanticOverheadRound9Test|FinalCleanupMigration09Test|HighFrequencyLoggingSeverityTest|SensitiveLoggingRedactionTest" --output-on-failure
ctest --test-dir cmake-build --output-on-failure
```

The full test run passed all 137 registered tests, with environment-dependent integration tests reported as skipped by their own guards.

## Known follow-ups

1. PR G.2 — add instance-scoped semantic `--log-level` to `ConfigInstance`.
2. Final macro removal/source gate.
3. Round 10 — book integration.
4. Optional later async logging backend if accepted-log sink I/O becomes the bottleneck.
