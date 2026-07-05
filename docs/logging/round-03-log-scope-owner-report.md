# Logging Roadmap Round 03 Report: `LogScopeOwner` Composition Helper

Branch/report: current work branch / Round 03 `LogScopeOwner` composition helper.

## Implementation summary

Round 03 adds the reusable object-owned semantic identity holder for later composition into framework objects. The implementation stays limited to `src/log`, focused unit-test registration, and this report.

Implemented:

* `logger::LogScopeOwner` in `src/log/LogScopeOwner.h` and `src/log/LogScopeOwner.cpp`.
* Immediate copying from borrowed `LogScope` into owned `std::string` storage for `component`, optional `instance`, and optional `connection`.
* Owner-held `origin`, `boundary`, and optional known `role`; unknown roles are represented as absent in owned storage and exposed as `LogRole::Unknown` in borrowed views.
* `LogScopeOwner::scope() const noexcept`, returning a temporary borrowed `LogScope` view that remains valid only while the owner remains alive.
* `LogScopeOwner::logger(...)`, returning a `BoundaryLogger` created from the owner's owned identity, so construction does not retain caller-owned `string_view` data.
* A small Round 2 refactor that exposes one shared owned-scope copy/view model (`OwnedLogScope`, `copyLogScope`, `viewLogScope`, and `knownLogRole`) instead of duplicating ownership logic between `BoundaryLogger` and `LogScopeOwner`.
* Focused Round 3 unit tests covering borrowed string mutation/destruction, owner views, logger emission, unknown-role JSON omission, and copy/move behavior.

## Exact files changed

* `src/log/CMakeLists.txt` — adds `LogScopeOwner.cpp` and `LogScopeOwner.h` to the `logger` library source/header lists.
* `src/log/SemanticLogger.h` — exposes the shared owned-scope helper declarations and grants `LogScopeOwner` access to the `BoundaryLogger` owned-scope constructor.
* `src/log/SemanticLogger.cpp` — refactors Round 2's internal owned-scope copy/view helpers into shared functions used by both Round 2 `BoundaryLogger` construction and Round 3 `LogScopeOwner`.
* `src/log/LogScopeOwner.h` — declares the Round 3 composition helper.
* `src/log/LogScopeOwner.cpp` — implements owned construction, borrowed-scope copying, temporary borrowed views, and owner-based `BoundaryLogger` creation.
* `tests/unit/log/CMakeLists.txt` — registers `LogScopeOwnerRound3Test` and labels it `unit;log;round3`.
* `tests/unit/log/LogScopeOwnerRound3Test.cpp` — adds the focused Round 3 lifetime, JSON, logger, and copy/move tests.
* `docs/logging/round-03-log-scope-owner-report.md` — this report.

No socket, HTTP, WebSocket, MQTT, DB, application, `ConfigInstance`, `SocketConnection`, `SocketContext`, `SocketServer`, `SocketClient`, `SocketAcceptor`, or `SocketConnector` production files were changed.

## Exact build commands run

Dependency repair command run because the initial normal configure reported missing `nlohmann_json>=3.11`:

```sh
apt-get update && apt-get install -y nlohmann-json3-dev
```

Normal configure/build:

```sh
git status --short
git diff --check
cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON
cmake --build cmake-build --parallel 2
```

ASan configure/build:

```sh
cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON
cmake --build cmake-build-asan --parallel 2
```

## Exact test commands run

Normal tests:

```sh
ctest --test-dir cmake-build -R "SemanticLoggerRound2Test|LogScopeOwnerRound3Test" --output-on-failure
ctest --test-dir cmake-build --output-on-failure
```

ASan tests:

```sh
ASAN=$(gcc -print-file-name=libasan.so) LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan --output-on-failure
```

## Sanitizer result

ASan configure, build, and full `ctest` completed successfully. The ASan full test command reported `100% tests passed, 0 tests failed out of 109`; many component tests were skipped by the existing test harness, matching the normal build behavior. The ASan link emitted platform warnings from `libasan.so.8` about `tmpnam_r`, `tmpnam`, and `tempnam`; these warnings are not from Round 3 code and did not fail the build or tests.

## `LogScopeOwner` ownership/lifetime result

`LogScopeOwner` owns the semantic identity bytes needed for later object composition. `component`, `instance`, and `connection` are stored as owned `std::string` values inside the shared `OwnedLogScope` model. Construction from a borrowed `LogScope` copies all borrowed `std::string_view` fields immediately. Tests verify that mutating the original source strings and destroying local source strings after construction do not change `owner.scope()`.

`LogScopeOwner::scope() const noexcept` returns a borrowed `LogScope` view over the owner's storage. This view is intentionally temporary and must not be stored in sinks, async structures, queues, or owned records. Existing `materialize(...)` remains the handoff point that copies borrowed views into owned `LogRecord` bytes.

## BoundaryLogger-from-owner result

`LogScopeOwner::logger(...)` creates a `BoundaryLogger` from the owner's `OwnedLogScope`. `BoundaryLogger` already owns an `OwnedLogScope`, so the returned logger remains independent of caller-owned strings and safe after the original borrowed inputs are modified or destroyed. The Round 3 test verifies that logger records contain the owner's original component, instance, role, and connection values after the source strings have been changed.

## Copy/move behavior result

Copy and move are intentionally enabled with the compiler-generated operations. A copied owner owns independent string storage through `std::string` and `std::optional<std::string>` value semantics. A moved-to owner remains valid. `LogScopeOwnerRound3Test` contains `static_assert` checks for copy/move support and runtime checks for copied and moved-to owner views.

## Macro compatibility result

Round 3 did not change `src/log/Logger.h`, `src/log/Logger.cpp`, or any legacy macro implementation. Round 2's macro compatibility smoke coverage remains in `SemanticLoggerRound2Test`, and the focused Round 2 test was rerun successfully alongside the new Round 3 test.

## Confirmation that `LOG`/`PLOG`/`VLOG` were not changed

Confirmed. Round 3 does not alter the `LOG`, `PLOG`, or `VLOG` macro definitions or behavior.

## Confirmation that no production object integration was added

Confirmed. Round 3 adds only the reusable composition helper in `src/log`; it does not add `SocketContext::log()`, `SocketContext::frameworkLog()`, `SocketConnection::log()`, `ConfigInstance` logging ownership, or SocketServer/SocketClient/SocketAcceptor/SocketConnector logging APIs.

## Confirmation that no production call sites were migrated

Confirmed. No production logging call sites were migrated, and no production framework object now contains `LogScopeOwner`.

## Extended SNode.C and MQTT/mqttsuite review note

For this round, the extended SNode.C/MQTT context was reviewed through the merged Round 1 report and current Round 2 semantic core files before coding. The repo still has MQTT implementation under `src/iot/mqtt`; no separate top-level `mqttsuite` tree was present in the checkout during the earlier roadmap review. Round 3 deliberately leaves MQTT and broader production integration untouched so the helper can be validated in isolation before later roadmap rounds.

## Deliberate deferrals

* No Round 4 object integration.
* No `SocketContext::log()` or `SocketContext::frameworkLog()`.
* No `SocketConnection::log()`.
* No `ConfigInstance` logging ownership.
* No SocketServer, SocketClient, SocketAcceptor, or SocketConnector logging APIs.
* No HTTP, WebSocket, MQTT, DB, or application integration.
* No production call-site migration.
* No `LogSuperClass`, logging base class, mixin, or inheritance-based logging ownership.
* No `LOG`/`PLOG`/`VLOG` behavior changes.
* No backend unification.
* No startup filter configuration, precedence implementation, `freeze()`, CLI integration, runtime reload, signals, admin endpoints, or mutable atomic thresholds.

## Known non-blocking follow-ups

* Later rounds should integrate `LogScopeOwner` into framework objects by composition only, following the semantic logging contract.
* Later integration should continue to materialize borrowed views before formatter/sink/async handoff.
* Future macro compatibility work can add a stronger byte-for-byte or normalized harness if any later round touches legacy macro output paths.
