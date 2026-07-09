# Semantic logger cache report

## Problem summary

PR G.1b targets the remaining disabled semantic logging overhead in hot stable paths. PR #175 made guarded diagnostics bind helpers once locally, but a single helper call such as `iot::mqtt::semantic::mqttLog()` still resolved `LogManager::effectiveLevel(scope)` on each function invocation before checking `enabled(level)`.

## What PR #175 fixed

- No-threshold production semantic helpers now use effective startup thresholds.
- Disabled fmt-style semantic logs avoid formatting expensive arguments.
- Selected repeated-helper guarded diagnostics in MQTT and HTTP client code were changed to local bind-once form.

## What PR #175 did not fix

PR #175 intentionally did not store resolved `BoundaryLogger` values on hot objects. Therefore hot accessors/helpers could still allocate/copy scope lookup data and call `LogManager::effectiveLevel(...)` for disabled logs.

## LogScopeOwner vs BoundaryLogger caching distinction

`LogScopeOwner` caches stable semantic identity: origin, boundary, component, instance, role, and connection. It does not cache the resolved effective threshold. `BoundaryLogger` owns a copied `OwnedLogScope`, sink, threshold, and clock, so caching a `BoundaryLogger` also caches the resolved threshold selected when the logger is constructed.

## BoundaryLogger value/copy safety verification

- `BoundaryLogger::createForTest()` and `LogScopeOwner::logger(...)` construct a `BoundaryLogger` from `copyLogScope(...)` or from a `LogScopeOwner`'s owned scope.
- `OwnedLogScope` stores strings and optionals rather than borrowing temporary `std::string_view` data.
- `BoundaryLogger` has value members only (`OwnedLogScope`, `std::function` sink, threshold, and clock) and uses compiler-generated copy/move operations safely for the cached-by-value pattern.
- This PR does not return references to temporary `BoundaryLogger` objects. The MQTT cache is a class member and guarded calls use it directly.
- Returning/copying an already constructed `BoundaryLogger` does not call `LogManager::effectiveLevel(...)`; that call occurs in `LogScopeOwner::logger(sink, clock)` before the `BoundaryLogger` is constructed.

## Freeze safety rule

Caching a resolved logger is allowed only when semantic identity is stable, construction is post CLI/config policy application, the log policy is frozen or demonstrably in runtime post-freeze code, and no later policy update is expected.

## Construction-order verification per inspected target

### MQTT core (`iot::mqtt::Mqtt`)

Caching was added only to the core MQTT object. MQTT server contexts are created by MQTT `SocketContextFactory::create(...)` and shared factory `create(...)`, which allocate contexts during accepted connection handling. HTTP/WebSocket/MQTT factories are runtime socket-context factories, not configuration objects. Root configuration applies semantic policy and calls `logger::LogManager::freeze()` in `ConfigRoot::bootstrap(...)` before normal runtime starts. The cached MQTT logger has framework/connection component identity `iot.mqtt`, which is stable for the object lifetime.

### MQTT broker/session construction

Broker tree/session code was not converted to member cached loggers in this PR. These classes can be tied to broker/session-store lifecycle and offline session state; the PR only performs mechanical local bind-once cleanup for the remaining guarded expensive diagnostics.

### Core `SocketContext`

`SocketContext` has stable `applicationLogScope` and `frameworkLogScope` members that include instance and connection identity. However it is a base class for many protocol/application contexts. Because caching there would affect every subclass, this PR leaves it unchanged unless each subclass path is verified and measured in a future rollout.

### HTTP client `SocketContext`

HTTP client contexts are created by `web::http::client::SocketContextFactory::create(...)` during connection establishment. This appears runtime/post-freeze in normal application flow, but because the base `SocketContext` cache would be inherited broadly and because direct HTTP-client-only caching would require API churn around `frameworkLog()`, no caching was added in this PR.

### HTTP server `SocketContext`

HTTP server contexts are created by `web::http::server::SocketContextFactory::create(...)` for accepted connections. This appears runtime/post-freeze in normal application flow, but no caching was added for the same conditional/base-class reasons as HTTP client.

### WebSocket context

WebSocket send/receive frame logging uses semantic helper functions in frame transmitter/receiver code rather than the core `SocketContext` accessors. WebSocket upgrade/context creation spans HTTP upgrade factories; this PR inspected those paths but did not add caching because the requested safe, minimal proof point is MQTT core.

## Classes/files inspected

- `src/log/SemanticLogger.h`
- `src/log/SemanticLogger.cpp`
- `src/log/LogScopeOwner.cpp`
- `src/iot/mqtt/Mqtt.h`
- `src/iot/mqtt/Mqtt.cpp`
- `src/iot/mqtt/SemanticLog.h`
- `src/iot/mqtt/server/SocketContextFactory.cpp`
- `src/iot/mqtt/server/SharedSocketContextFactory.cpp`
- `src/iot/mqtt/server/broker/RetainTree.cpp`
- `src/iot/mqtt/server/broker/SubscriptionTree.cpp`
- `src/iot/mqtt/server/broker/Session.cpp`
- `src/core/socket/stream/SocketContext.h`
- `src/core/socket/stream/SocketContext.cpp`
- `src/web/http/client/SocketContext.cpp`
- `src/web/http/client/SocketContextFactory.cpp`
- `src/web/http/server/SocketContext.cpp`
- `src/web/http/server/SocketContextFactory.cpp`
- `src/web/websocket`
- `src/utils/Config.cpp`

## Where caching was added

- `iot::mqtt::Mqtt` now stores a resolved `logger::BoundaryLogger log_` member initialized from `iot::mqtt::semantic::mqttLog()` in both constructors.
- MQTT trace guards in send/publish/receive dump paths use `log_.enabled(...)` and `log_.trace()` instead of reconstructing the helper/accessor logger on every call.

## Where caching was deliberately not added and why

- Core `SocketContext` was not cached because it is a broad base class with many subclasses; this PR treats SocketContext caching as conditional on per-subclass verification and future measurement.
- HTTP client/server contexts were not cached directly because doing so without base-class changes would create broader API churn than this focused PR needs.
- WebSocket frame helpers were not cached because their transmitter/receiver construction and upgrade paths need a separate minimal design if measurements identify them as the next bottleneck.
- MQTT broker trees/sessions were not cached as members because the remaining issue there was the repeated-helper guard pattern, fixed locally without changing object layout.

## Remaining MQTT broker bind-once cleanup

The remaining repeated-helper guarded diagnostics in `RetainTree.cpp`, `SubscriptionTree.cpp`, and `Session.cpp` now bind the semantic logger once to a local `auto log` before `enabled(...)` and the expensive `Mqtt::toHexString(...)` formatting path.

## Tests added/updated

- Added `tests/unit/log/SemanticCachedLoggerOverheadTest.cpp`.
- Registered `SemanticCachedLoggerOverheadTest` in `tests/unit/log/CMakeLists.txt`.
- The test checks global threshold rejection, component override acceptance, disabled expensive-argument suppression, enabled expensive-argument formatting exactly once, cached logger copy/value threshold behavior, and the source-policy absence of repeated MQTT broker helper guards.

## Search commands run

Before changes:

```sh
rg -n "\.enabled\(logger::LogLevel::" src -g '*.h' -g '*.hpp' -g '*.cpp'
rg -n "[A-Za-z0-9_:]*Log\(\)\.enabled|semantic::[A-Za-z0-9_:]*Log\(\)\.enabled" src -g '*.h' -g '*.hpp' -g '*.cpp'
rg -n "auto log = .*Log\(\)|auto log = .*frameworkLog\(\)|auto log = .*\.log\(\)" src -g '*.h' -g '*.hpp' -g '*.cpp'
rg -n "LogScopeOwner|BoundaryLogger" src/iot/mqtt src/core/socket/stream src/web/http src/web/websocket -g '*.h' -g '*.hpp' -g '*.cpp'
rg -n "toHexString|httputils::toString|dump\(|payload|body|header|packet|buffer|retain|subscription" src/iot/mqtt src/web/http src/web/websocket src/core/socket/stream -g '*.h' -g '*.hpp' -g '*.cpp'
```

After changes:

```sh
rg -n "[A-Za-z0-9_:]*Log\(\)\.enabled|semantic::[A-Za-z0-9_:]*Log\(\)\.enabled" src -g '*.h' -g '*.hpp' -g '*.cpp'
rg -n "cached|BoundaryLogger.*log_|logger::BoundaryLogger" src/iot/mqtt src/core/socket/stream src/web/http src/web/websocket docs/logging tests/unit/log -g '*.h' -g '*.hpp' -g '*.cpp' -g '*.md'
rg -n "SNODEC_.*LOG|LOG_IF|TRACE_IF|DEBUG_IF|IF_ENABLED" src/log src tests docs -g '*.h' -g '*.hpp' -g '*.cpp' -g '*.md'
```

## Tests run

The validation commands and results are recorded in the PR body/final summary for this change.

## Known follow-ups

1. PR G.2 — ConfigInstance instance-scoped `--log-level`.
2. Optional broader cache rollout if measurements show remaining overhead.
3. Optional async logging backend only if accepted-log sink I/O becomes the bottleneck.
4. Final macro removal/source gate.
5. Round 10 book integration.
