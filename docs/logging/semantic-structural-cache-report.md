# Semantic structural logger cache report

## Problem summary

Several hot framework paths still resolved semantic loggers repeatedly. Each direct helper call or uncached accessor created a fresh `BoundaryLogger`, forcing effective-level lookup work before discovering that a message was disabled.

## What #175 fixed

#175 reduced disabled semantic logging overhead by moving expensive formatting behind enabled checks and by introducing local bind-once guard patterns where immediate structural ownership was not yet available.

## What #176 fixed

#176 structurally cached the stable `iot::mqtt::Mqtt` semantic logger and completed the direct same-scope cleanup in `Mqtt.cpp`.

## Why bind-once was not enough

A local `auto log = ...Log()` avoids repeated helper calls within one block, but it still resolves the effective level every time the function is entered. Hot objects with stable semantic identity need a member cache so repeated runtime calls reuse an already-resolved threshold.

## For real structural optimization

For real structural optimization, stable framework objects now keep cached `BoundaryLogger` members or lazy post-freeze cached accessors. Disabled logging follows the cheap shape: check `cachedLog.enabled(level)` first, then format only inside the enabled path.

## Safety model

A cached `BoundaryLogger` captures the threshold selected when it is constructed. Caching is safe only when semantic identity is stable and policy has already frozen, or when the accessor keeps a pre-freeze dynamic fallback and populates the cache only after `LogManager::isFrozen()`.

## EventLoop special case

`EventLoop` is a Meyers singleton and can be initialized before configuration bootstrap freezes logging policy. It therefore does not cache in its constructor. Its default `log()` accessor remains dynamic before freeze and lazily caches the resolved semantic logger after freeze.

## SocketConnection cache

`SocketConnection` now stores a lazy cached logger. The accessor preserves dynamic behavior before freeze and reuses the cached logger after freeze for shutdown, close, read/write error, and context-switch diagnostics.

## SocketContext cache

`SocketContext` now stores separate lazy cached application and framework loggers. `log()` and `frameworkLog()` retain sink-taking overloads for tests/custom capture while avoiding repeated effective-level resolution on post-freeze hot paths.

## HTTP client/server impact

HTTP client/server socket contexts call `frameworkLog()` in request/response lifecycle paths. Those call sites now benefit from the core `SocketContext` structural cache without logging wording or protocol behavior changes.

## WebSocket frame cache

`Receiver` and `Transmitter` now cache `webSocketFrameLog()` as `frameLog_`. `readPayload()` and `sendFrame()` no longer create a fresh frame logger per payload chunk/frame, while `hexDump()` remains guarded by the trace-enabled check.

## MQTT broker/session/tree cache

`Broker`, `RetainTree`, `RetainTree::TopicLevel`, `SubscriptionTree`, `SubscriptionTree::TopicLevel`, and `Session` now keep cached MQTT broker/session loggers. Hot publish, retain, subscription, and queued-session paths use those cached members rather than direct helper resolution.

## Files inspected

The audit covered `src/core`, `src/core/socket`, `src/web/http`, `src/web/websocket`, `src/iot/mqtt`, existing logging tests, and prior logging reports.

## Implementation summary

- Added lazy post-freeze caches to `EventLoop`, `SocketConnection`, and `SocketContext`.
- Added frame logger members to WebSocket receiver/transmitter.
- Added broker/session logger members to MQTT broker tree/session classes.
- Added a source-policy and behavioral regression test for structural caching.

## Where caching was added

Caching was added to EventLoop, SocketConnection, SocketContext, WebSocket Receiver/Transmitter, MQTT Broker, RetainTree, SubscriptionTree, their hot nested topic levels, and Session.

## Where caching was deliberately not added and why

Free functions, semantic helper definitions, and sink-taking overloads were not converted because they either do not own stable semantic identity or intentionally support tests/custom capture. EventLoop constructor caching was deliberately avoided because initialization can happen before policy freeze.

## Tests added/updated

Added `SemanticStructuralLoggerCacheTest` and registered it in the log unit test CMake file. The test checks source structure, macro non-introduction, threshold/override behavior, and disabled/ enabled formatting behavior.

## Search commands run

- `rg -n "[A-Za-z0-9_:]*Log\\(\\)\\.(trace|debug|info|warn|error|critical|enabled)|\\.log\\(\\)\\.(trace|debug|info|warn|error|critical|enabled)|frameworkLog\\(\\)\\.(trace|debug|info|warn|error|critical|enabled)" src -g '*.h' -g '*.hpp' -g '*.cpp'`
- `rg -n "return .*LogScope.*\\.logger\\(logger::Logger::semanticSink\\(\\)\\)|return .*logScope\\.logger\\(logger::Logger::semanticSink\\(\\)\\)|return .*LogScopeOwner.*logger\\(logger::Logger::semanticSink\\(\\)\\)" src -g '*.h' -g '*.hpp' -g '*.cpp'`
- `rg -n "hexDump|toHexString|httputils::toString|dump\\(|payload|body|header|packet|buffer|retain|subscription|route" src -g '*.h' -g '*.hpp' -g '*.cpp'`
- `rg -n "logger::BoundaryLogger .*log_|std::optional<logger::BoundaryLogger>|cached.*BoundaryLogger|refresh.*Log|cache.*Log" src tests docs -g '*.h' -g '*.hpp' -g '*.cpp' -g '*.md'`
- `rg -n "auto log = .*Log\\(\\)|auto log = .*frameworkLog\\(\\)|auto log = .*\\.log\\(\\)" src/iot/mqtt src/web/http src/web/websocket src/core/socket src/core -g '*.h' -g '*.hpp' -g '*.cpp'`

## Tests run

The implementation was validated with configure, build, focused tests, source searches, formatting, and diff checks as recorded in the PR summary.

## Known follow-ups

1. PR G.2 — ConfigInstance instance-scoped --log-level.
2. Optional measurements/callgrind benchmark.
3. Optional async logging backend only if accepted-log sink I/O becomes the bottleneck.
4. Final macro removal/source gate.
5. Round 10 book integration.
