# Logging roadmap round 07 — startup-only semantic filter configuration

## Implementation summary

Round 7 implements startup-only semantic filtering and format selection.

Round 7 implements precedence:
instance > component > boundary > origin > global.

The implementation adds `logger::LogManager` as the startup semantic policy surface. `LogManager::init()` resets policy to permissive defaults; startup setters configure global, origin, boundary, component, instance, and semantic output format; `freeze()` makes the policy immutable; `effectiveLevel(...)`, `shouldEmit(...)`, `format()`, and `formatRecord(...)` expose frozen lookups for semantic emission.

`logger::Logger::emitSemantic(...)` now applies the semantic policy before mapping to the existing legacy backend level. Records must still pass the existing `Logger::setLogLevel(...)` numeric backend gate before output reaches existing spdlog-backed sinks. No second backend was added.

## Files changed

- `src/log/SemanticLogger.h` — added the `logger::LogManager` public API and `Format { Text, Json }`.
- `src/log/SemanticLogger.cpp` — added startup policy storage, precedence lookup, freeze enforcement, `shouldEmit`, and semantic format selection.
- `src/log/Logger.cpp` — integrated semantic filtering and selected Text/JSON formatting into `Logger::emitSemantic(...)` before legacy backend emission.
- `tests/unit/log/SemanticFilterRound7Test.cpp` — added focused Round 7 coverage.
- `tests/unit/log/CMakeLists.txt` — registered `SemanticFilterRound7Test`.
- `docs/logging/round-07-startup-filter-configuration-report.md` — this report.

## Build commands run

- `git fetch origin` — failed because this Codex environment has no usable `origin` remote: `'origin' does not appear to be a git repository`.
- `git checkout master` — failed because this Codex environment has no local `master` branch.
- `git status --short` — clean before changes.
- `git log --oneline -- docs/logging/round-06-backend-unification-report.md | head` — confirmed current checkout includes merged Round 6 commit `a3da6b7 Logging roadmap round 06 backend unification` and HEAD `38388e0 Merge pull request #147...`.
- `git checkout -b codex/implement-logging-roadmap-round-7`
- `apt-get update && apt-get install -y nlohmann-json3-dev` — installed missing configure dependency needed by `SNODEC_BUILD_APPS=ON`.
- `git diff --check`
- `cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON`
- `cmake --build cmake-build --parallel 2`
- `cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON`
- `cmake --build cmake-build-asan --parallel 2`

## Test commands run

- `ctest --test-dir cmake-build -R "SemanticLoggerRound2Test|LogScopeOwnerRound3Test|ProductionLogApiRound4Test|SocketEndpointLogApiRound5Test|SemanticBackendRound6Test|SemanticFilterRound7Test" --output-on-failure`
- `ctest --test-dir cmake-build --output-on-failure`
- `ASAN=$(gcc -print-file-name=libasan.so) LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan --output-on-failure`

## Sanitizer result

ASan configure, build, and full `ctest` completed successfully. The ASan test run reported `100% tests passed, 0 tests failed out of 113`; many component tests skipped by existing environment/group gating, consistent with the non-ASan run.

## LogManager API added

Added startup-only API:

- `LogManager::init()`
- `setGlobalLevel(LogLevel)`
- `setOriginLevel(LogOrigin, LogLevel)`
- `setBoundaryLevel(LogBoundary, LogLevel)`
- `setComponentLevel(std::string, LogLevel)`
- `setInstanceLevel(std::string, LogLevel)`
- `setFormat(LogManager::Format)`
- `freeze()`
- `isFrozen()`
- `effectiveLevel(LogScope)` / `effectiveLevel(LogRecord)`
- `shouldEmit(LogRecord)`
- `format()` / `formatRecord(LogRecord)`

## Startup-only freeze behavior

Before `freeze()`, setters mutate startup policy. `freeze()` marks the policy immutable. After freeze, repeated effective-level lookups are stable because no setter can modify the policy. No runtime reload, signal handler, admin endpoint, mutable atomic threshold, or dynamic runtime reconfiguration was added.

## Post-freeze setter behavior

Post-freeze setters reject changes by throwing `std::logic_error`. The Round 7 test verifies the throw and verifies the effective level and selected format remain unchanged afterward.

## Default policy

`LogManager::init()` resets to:

- global level: `Trace`
- format: `Text`
- no origin, boundary, component, or instance overrides
- not frozen

This preserves Round 6 default behavior unless a user configures semantic filtering.

## Precedence implementation result

Lookup is exactly:

1. instance override
2. component override
3. boundary override
4. origin override
5. global level fallback

Threshold ordering uses existing semantic enum order: `Trace < Debug < Info < Warn < Error < Critical < Off`. `Off` is treated only as a suppression threshold and is not emitted as a severity.

## Component/instance matching rule

Component and instance overrides use exact string matching only. No regex, glob, or prefix matching was added. Empty component and instance override keys are rejected with `std::invalid_argument`, so they cannot become surprising wildcard keys. An absent record instance does not match instance overrides.

## Text/Json format selection result

`LogManager::Format::Text` uses existing `formatText(record)`. `LogManager::Format::Json` uses existing `formatJsonV1(record)`. Format selection is part of startup policy and is frozen by `freeze()`.

## JSON v1 compatibility result

JSON output continues to use the existing JSON v1 formatter. Mandatory fields remain `v`, `ts`, `level`, `origin`, `boundary`, `component`, and `message`. Optional fields remain omitted when absent rather than emitted as `null`.

## Integration with Logger::emitSemantic result

`Logger::emitSemantic(record)` now first calls `LogManager::shouldEmit(record)`. If the semantic policy allows the record, it maps the semantic level to the existing legacy level and still applies `Logger::shouldLog(...)` / `Logger::setLogLevel(...)` before using existing backend sinks.

## Default no-argument object logger filtering result

No-argument object loggers continue to route through `Logger::semanticSink()` and therefore through `Logger::emitSemantic(...)`; they now respect the semantic policy without storing `BoundaryLogger` in production objects or broad constructor churn.

## Sink-taking overload compatibility result

Sink-taking overloads remain available and unchanged. Explicit test/custom capture sinks still receive records according to their supplied `BoundaryLogger` threshold and are not forced through the default backend bridge.

## Legacy Logger::setLogLevel backend-gate result

The existing numeric backend gate remains the final gate. Round 7 tests configure semantic global `Trace`, freeze policy, then set `Logger::setLogLevel(3)` and verify semantic `Info` is suppressed while semantic `Warn` is emitted.

## Legacy macro compatibility result

Legacy `LOG`/`PLOG`/`VLOG` syntax and backend behavior are unchanged. Round 7 tests verify legacy `LOG(INFO)` and `LOG(WARNING)` still obey the existing numeric backend gate and are not affected by semantic Text/JSON format selection.

## CLI behavior preservation result

No CLI parsing, CLI defaults, app startup behavior, `Logger::logToFile(...)`, `Logger::disableLogToFile()`, `Logger::setQuiet(...)`, `Logger::setTickResolver(...)`, or `Logger::setDisableColor(...)` semantics were changed.

## Production call-site migration / Round 8 status

Round 7 does not migrate production LOG/PLOG/VLOG call sites.

Round 7 does not add runtime reload, signals, admin endpoints, or mutable atomic runtime thresholds.

Round 7 does not start controlled subsystem migration; that remains Round 8.

No protocol-specific logging migration was done for HTTP, WebSocket, MQTT, DB, application examples, or MiniGateway.

## Deliberate deferrals

- Prefix/glob/regex component matching is deferred; Round 7 uses exact string matches only.
- Runtime reload, signals, admin endpoints, and mutable atomic runtime thresholds are intentionally not implemented.
- Controlled subsystem migration remains deferred to Round 8.
- Protocol-specific logging migration remains deferred.

## Known non-blocking follow-ups

- Future rounds can decide whether startup configuration should be wired to CLI/config-file surfaces.
- Future rounds can decide whether component hierarchy prefix matching is useful after exact-match operational behavior is validated.
- Future Round 8 work can begin controlled subsystem migration using the now-frozen semantic policy surface.

## General SNode.C and mqttsuite review note

As part of the requested broader review, this Round 7 pass built the full SNode.C tree with apps and tests enabled, including MQTT-related targets (`mqtt-types`, `mqtt-packets`, `mqtt-broker`, `mqtt`, `mqtt-server`, `mqtt-client`, `mqtt-server-websocket`, and `mqtt-client-websocket`) in both the normal and ASan build configurations. The semantic logging change deliberately did not migrate MQTT or mqttsuite production call sites; no mqttsuite-specific source edits were made in Round 7.
