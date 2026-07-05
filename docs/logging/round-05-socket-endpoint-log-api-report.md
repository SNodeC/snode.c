# Round 05 socket endpoint log API report

## Implementation summary

Round 05 extends the composition-owned semantic logging API to the common socket endpoint layer:

- `core::socket::stream::SocketServer` owns a `logger::LogScopeOwner` by composition and exposes the Round 5 `log()` overload pair.
- `core::socket::stream::SocketClient` owns a `logger::LogScopeOwner` by composition and exposes the Round 5 `log()` overload pair.
- `core::socket::stream::SocketConnector` exists in the current tree and is a client-side endpoint object that owns the active physical client socket/connect event receiver. It now owns a `logger::LogScopeOwner` by composition and exposes the Round 5 `log()` overload pair.
- `core::socket::stream::SocketAcceptor` exists in the current tree and is a server-side endpoint object that owns the active physical server socket/accept event receiver. It now owns a `logger::LogScopeOwner` by composition and exposes the Round 5 `log()` overload pair.
- Focused Round 5 tests validate the common server and client endpoint APIs and record metadata, and statically validate common connector and acceptor API availability.

The no-argument semantic logger overloads added in this round are transitional Round 5 API-shape scaffolding only. Backend-backed default semantic sinks are introduced in Round 6; until then these overloads return no-op loggers, tests and early validation must use sink-taking overloads, and production call sites must not be migrated to them yet.

## Exact files changed

- `src/core/socket/stream/SocketServer.h`
- `src/core/socket/stream/SocketClient.h`
- `src/core/socket/stream/SocketConnector.h`
- `src/core/socket/stream/SocketConnector.hpp`
- `src/core/socket/stream/SocketAcceptor.h`
- `src/core/socket/stream/SocketAcceptor.hpp`
- `tests/unit/log/SocketEndpointLogApiRound5Test.cpp`
- `tests/unit/log/CMakeLists.txt`
- `docs/logging/round-05-socket-endpoint-log-api-report.md`

## Branch and baseline result

The environment has no usable `origin` remote: `git fetch origin` failed with `fatal: 'origin' does not appear to be a git repository`. The local checkout did not have a `master` branch (`git checkout master` failed), but the checked-out commit was `6a93c46 Merge pull request #145 from SNodeC/codex/implement-logging-roadmap-round-4`, so it already contains merged Round 4. Work continued only after confirming the working tree was clean and creating `codex/implement-logging-roadmap-round-5` from that commit.

## API results

### SocketServer log API result

`SocketServer` now composes `logger::LogScopeOwner` and exposes:

```cpp
logger::BoundaryLogger log() const;
logger::BoundaryLogger log(logger::BoundaryLogger::Sink sink,
                           logger::LogLevel threshold = logger::LogLevel::Trace,
                           logger::BoundaryLogger::Clock clock = {}) const;
```

Server endpoint records use framework origin, instance boundary, `core.socket.stream` component, the configured constructor instance name when provided, server role, and no connection identity.

### SocketClient log API result

`SocketClient` now composes `logger::LogScopeOwner` and exposes the same Round 5 `log()` overload pair. Client endpoint records use framework origin, instance boundary, `core.socket.stream` component, the configured constructor instance name when provided, client role, and no connection identity.

### SocketConnector log API result

`SocketConnector` exists in the current tree. It owns the active physical client socket and connect event receiver for a connection attempt, so Round 5 treats it as a boundary-owning client-side endpoint object rather than a pure helper. It now composes `logger::LogScopeOwner` and exposes the Round 5 `log()` overload pair. Its semantic scope uses framework origin, instance boundary, `core.socket.stream` component, configured instance name when available, client role, and no connection identity because no stable connection identity exists before a connection is created.

### SocketAcceptor result

`SocketAcceptor` exists in the current tree. It owns the active physical server socket and accept event receiver for a listen attempt, so Round 5 treats it as a boundary-owning server-side endpoint object. It now composes `logger::LogScopeOwner` and exposes the Round 5 `log()` overload pair. Its semantic scope uses framework origin, instance boundary, `core.socket.stream` component, configured instance name when available, server role, and no connection identity because accepted connections already receive their own connection identity after creation.

## Semantic choices

| Class | Origin | Boundary | Component | Instance | Role | Connection |
| --- | --- | --- | --- | --- | --- | --- |
| `SocketServer` | framework | instance | `core.socket.stream` | configured constructor instance name when provided | server | absent |
| `SocketClient` | framework | instance | `core.socket.stream` | configured constructor instance name when provided | client | absent |
| `SocketConnector` | framework | instance | `core.socket.stream` | configured config instance name when available | client | absent |
| `SocketAcceptor` | framework | instance | `core.socket.stream` | configured config instance name when available | server | absent |

The endpoint layer does not currently expose a more precise established endpoint component, so `core.socket.stream` is used consistently with Round 4 connection logging.

## Role propagation result for SocketConnection/SocketContext

Round 5 did not propagate endpoint role into `SocketConnection` or `SocketContext`. The current legacy/TLS connector and acceptor construction paths allocate protocol-specific connection objects from shared connector/acceptor templates; safely threading role through those paths would create broader constructor churn than this round allows. This is deliberately deferred to a localized follow-up once the construction path can pass role without broad protocol-family rewrites.

## Helper parent.log() policy result

Policy established for the touched endpoint area:

- Boundary-owning classes own `logger::LogScopeOwner` by composition.
- Helper classes do not own their own `LogScopeOwner` unless they are true boundary owners.
- Helper classes do not store `logger::BoundaryLogger`.
- Helper classes should use the owning parent and log through `parent.log()` or `parent.frameworkLog()` when semantic logging call-site migration is later performed.

Touched helper classification:

- `SocketServer`: boundary owner.
- `SocketClient`: boundary owner.
- `SocketConnector`: boundary owner in this tree because it owns the active connect event receiver and physical client socket for an endpoint attempt.
- `SocketAcceptor`: boundary owner in this tree because it owns the active accept event receiver and physical server socket for an endpoint attempt.
- `ServerFlowController`: helper left unchanged because migration is deferred; it does not own a `LogScopeOwner` and does not store `BoundaryLogger`.
- `ClientFlowController`: helper left unchanged because migration is deferred; it does not own a `LogScopeOwner` and does not store `BoundaryLogger`.

No helper class gained accidental independent semantic identity.

## Lifetime and ownership result

All new endpoint APIs return `logger::BoundaryLogger` objects created from owned `logger::LogScopeOwner` state. Identity strings are copied into owned scopes; no endpoint stores caller-owned `std::string_view` identity. Returned loggers are lifetime-safe for use after construction because `BoundaryLogger` materializes from owned semantic scope data.

## Macro compatibility result

Existing `LOG`, `PLOG`, and `VLOG` behavior is unchanged. Round 5 did not alter the macro logger implementation, did not migrate production macro call sites to semantic logging, and did not start backend unification.

## Non-goal confirmations

- `LOG`/`PLOG`/`VLOG` call sites were not broadly migrated.
- No broad production call-site migration was done.
- Backend unification was not started.
- Startup filter configuration was not started.
- No CLI logging behavior changes were made.
- No global/origin/boundary/component/instance precedence work was started.
- No runtime reload, signals, admin endpoints, mutable atomic thresholds, or `freeze()` work was started.
- No `LogSuperClass`, logging inheritance, or logging mixin base was introduced.

## Exact build commands run

- `cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON`
- `cmake --build cmake-build --parallel 2`
- `cmake -S . -B cmake-build-dev -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=OFF`
- `cmake --build cmake-build-dev --parallel 2 --target SocketEndpointLogApiRound5Test`

## Exact test and check commands run

- `git status --short`
- `git diff --check`
- `./cmake-build-dev/tests/unit/log/SocketEndpointLogApiRound5Test`
- `ctest --test-dir cmake-build -R "SemanticLoggerRound2Test|LogScopeOwnerRound3Test|ProductionLogApiRound4Test|SocketEndpointLogApiRound5Test" --output-on-failure`
- `ctest --test-dir cmake-build --output-on-failure`

## Sanitizer result

ASan was attempted with:

- `cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON`
- `cmake --build cmake-build-asan --parallel 2`
- `ASAN=$(gcc -print-file-name=libasan.so) LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan --output-on-failure`

ASan configure, build, and full `ctest` completed successfully: 100% tests passed, 0 tests failed out of 111, with the same intentionally skipped integration/component tests reported by CTest.

## Deliberate deferrals

- Default semantic sink/backend integration remains Round 6 work.
- Production `LOG`/`PLOG`/`VLOG` migration remains future roadmap work.
- Endpoint role propagation into `SocketConnection` and `SocketContext` is deferred to avoid broad constructor churn.
- Helper call-site migration through `parent.log()`/`parent.frameworkLog()` is documented but deferred.

## Known non-blocking follow-ups

- Round 6 should replace no-op default semantic sinks with backend-backed default behavior.
- A later localized role propagation change can pass server/client role into connection/context construction once the protocol-specific connection constructors can accept it without broad rewrites.
- Future semantic call-site migration can update selected helper logs to use parent-owned log APIs once backend unification exists.
