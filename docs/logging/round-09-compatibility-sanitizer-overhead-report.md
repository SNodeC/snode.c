# Logging roadmap round 09 — compatibility, sanitizer, and overhead gates

Round 9 is a gate-hardening round.

Round 9 does not perform additional production call-site migration.

Round 9 does not change LOG/PLOG/VLOG macro definitions.

Round 9 does not change startup semantic policy.

Round 9 does not change JSON v1.

Round 9 does not add runtime reload, signals, admin endpoints, or mutable atomic runtime thresholds.

Round 10 remains the book integration round.

## Base verification

The Codex environment had no usable `origin` remote: `git fetch origin` failed with `fatal: 'origin' does not appear to be a git repository`. The local checkout had no `master` branch; it started from the `work` branch at `a5f4478 Merge pull request #149 from SNodeC/codex/implement-logging-roadmap-round-8-migration`. The working tree was clean before Round 9 edits, and Round 8 was confirmed present before work continued by checking:

- `docs/logging/round-08-controlled-subsystem-migration-report.md`
- `tests/unit/log/ControlledMigrationRound8Test.cpp`
- `git log --oneline -n 10`, which showed the Round 8 merge commit.

A fresh branch named `codex/implement-logging-roadmap-round-9` was created from that checked-out Round 8 commit.

## Implementation summary

Round 9 adds deterministic compatibility and overhead gates around the semantic logging foundation. It does not add logging architecture, does not change production logging code, and does not migrate additional production call sites. The new tests cover legacy macro behavior, semantic API behavior, startup/freeze policy compatibility, Text/JSON formatter compatibility, `sysError`/errno preservation, borrowed `string_view` lifetime safety, Round 8 migrated SocketConnection/SocketContext call-site compatibility, and suppression/overhead invariants.

## Exact files changed

- `tests/unit/log/SemanticCompatibilityRound9Test.cpp` — new focused Round 9 compatibility test.
- `tests/unit/log/SemanticOverheadRound9Test.cpp` — new focused Round 9 overhead/suppression test.
- `tests/unit/log/CMakeLists.txt` — registers the two Round 9 tests.
- `docs/logging/round-09-compatibility-sanitizer-overhead-report.md` — this report.

No production source files were changed.

## Exact build commands run

```sh
git fetch origin
# failed because there is no usable origin remote in this Codex environment

git checkout master
# failed because this checkout has no local master branch

git status --short
test -f docs/logging/round-08-controlled-subsystem-migration-report.md
test -f tests/unit/log/ControlledMigrationRound8Test.cpp
git log --oneline -n 10
git checkout -b codex/implement-logging-roadmap-round-9

cmake -S . -B cmake-build \
  -DSNODEC_BUILD_TESTS=ON \
  -DSNODEC_BUILD_APPS=ON
cmake --build cmake-build --parallel 2

cmake -S . -B cmake-build-asan \
  -DSNODEC_BUILD_TESTS=ON \
  -DSNODEC_BUILD_APPS=ON \
  -DSNODEC_ENABLE_ASAN=ON
cmake --build cmake-build-asan --parallel 2
```

The first normal configure initially failed because `nlohmann-json3-dev` was not installed while apps were enabled. The missing package was installed with:

```sh
apt-get update && apt-get install -y nlohmann-json3-dev
```

The normal build and ASan build then completed successfully. Configure emitted non-blocking environment warnings for missing Doxygen, include-what-you-use, BlueZ/libbluetooth, libmagic, and libmariadb.

## Exact test and check commands run

```sh
git diff --check

ctest --test-dir cmake-build -R "SemanticLoggerRound2Test|LogScopeOwnerRound3Test|ProductionLogApiRound4Test|SocketEndpointLogApiRound5Test|SemanticBackendRound6Test|SemanticFilterRound7Test|ControlledMigrationRound8Test|SemanticCompatibilityRound9Test|SemanticOverheadRound9Test" --output-on-failure

ctest --test-dir cmake-build --output-on-failure

ASAN=$(gcc -print-file-name=libasan.so)
LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan --output-on-failure
```

Results:

- `git diff --check` passed.
- Focused Round 2–9 logging ctest passed: 9/9 tests passed.
- Full normal ctest passed: 116/116 tests passed, with 0 failures; the suite reports many network/component tests as skipped in this environment.
- ASan full ctest passed: 116/116 tests passed, with 0 failures; the suite reports the same environment skips.

## Sanitizer commands and results

ASan was run with the project-supported `SNODEC_ENABLE_ASAN` option:

```sh
cmake -S . -B cmake-build-asan \
  -DSNODEC_BUILD_TESTS=ON \
  -DSNODEC_BUILD_APPS=ON \
  -DSNODEC_ENABLE_ASAN=ON
cmake --build cmake-build-asan --parallel 2
ASAN=$(gcc -print-file-name=libasan.so)
LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan --output-on-failure
```

ASan result: build passed and full ctest passed with no ASan-reported failures. The ASan validation includes `ControlledMigrationRound8Test`, `SemanticCompatibilityRound9Test`, and `SemanticOverheadRound9Test`.

UBSan result: UBSan was not run because the project exposes `SNODEC_ENABLE_ASAN` but no existing UBSan CMake option was found by `rg -n "UBSAN|ASAN|SANIT" CMakeLists.txt cmake src tests`. Round 9 intentionally did not invent a new global sanitizer flag or otherwise disrupt normal build behavior.

## Legacy macro compatibility audit result

The required audit command was run:

```sh
rg -n "\\b(LOG|PLOG|VLOG)\\s*\\(" src tests | tee /tmp/round9-legacy-macro-audit.txt
```

The audit found 1242 legacy macro call-site matches across `src` and `tests`. Legacy macros remain defined, legacy macro call sites still compile, Round 9 did not migrate additional production call sites, and remaining `LOG`/`PLOG`/`VLOG` call sites are intentionally left for future controlled migrations.

## Compatibility results

- Legacy macro compatibility: `SemanticCompatibilityRound9Test` verifies `LOG(INFO)`, `PLOG(ERROR)`, `VLOG(level)`, and numeric `Logger::setLogLevel(...)` behavior.
- Semantic API compatibility: default backend-backed semantic sinks and sink-taking semantic overloads are covered.
- LogManager compatibility: semantic filters remain independent from legacy macros; freeze behavior remains immutable; repeated frozen `effectiveLevel(scope)` lookups are stable.
- Text/JSON compatibility: Text format selection still uses `formatText(record)`, Json format selection still uses `formatJsonV1(record)`, JSON v1 mandatory fields are present, and absent optional fields are omitted rather than emitted as `null`.
- `sysError`/errno compatibility: semantic `sysError` preserves errno code and text; Round 8 migrated write-error logging also preserves errno code/text.
- Borrowed `string_view` lifetime result: tests cover copied scope fields when original component/instance strings are mutated or destroyed before logger emission.
- Round 8 migrated call-site compatibility: tests cover the migrated `SocketConnection` switch log, migrated `SocketContext` framework EOF log, and migrated `SocketContext` sysError log.

## Overhead/suppression gate result

`SemanticOverheadRound9Test` adds deterministic, non-timing gates for:

- BoundaryLogger threshold suppression not invoking the supplied sink.
- `LogLevel::Off` not invoking the supplied sink.
- LogManager semantic suppression preventing backend output.
- `Logger::setLogLevel` backend suppression preventing backend output.
- Suppressed stream emission not evaluating an expensive insertion operand.
- Suppressed default backend path emitting no formatted output.
- Repeated frozen `effectiveLevel(scope)` lookups remaining stable.
- No heap-use-after-free when original scope strings are mutated/destroyed before logger emission.

No timing-based hard CI gate was introduced. No optional microbenchmark was added or run.

## General SNode.C and mqttsuite review note

Round 9 validation built the broad SNode.C tree with apps enabled, including MQTT-related targets present in this repository, and full ctest was run in both normal and ASan configurations. No MQTT-specific source changes or migrations were made. A deeper mqttsuite-specific review is deliberately deferred because Round 9's hard non-goals prohibit touching protocol/application/MQTT layers.

## Confirmations

- No production migration was done.
- Macro definitions were not changed.
- CLI behavior was not changed.
- LogManager policy semantics were not changed.
- JSON v1 schema was not changed.
- No protocol/application/MQTT/HTTP/WebSocket/DB/MiniGateway migration was done.
- No runtime reload, signals, admin endpoints, mutable atomic runtime thresholds, logging inheritance, logging mixin base, or `LogSuperClass` was added.
- Round 10 remains the book integration round.

## Deliberate deferrals

- No book/manuscript material was edited.
- No new production logging call sites were migrated.
- No runtime reconfiguration or reload feature was added.
- No new public hooks were added solely to count formatter calls.
- UBSan support remains deferred until the project has an agreed, non-disruptive sanitizer option.

## Known non-blocking follow-ups

- Add a project-supported UBSan or ASan+UBSan CMake option in a dedicated build-system round if desired.
- Add non-gating logging microbenchmarks if reviewers want repeatable local performance observations.
- Continue controlled production call-site migrations only in future roadmap rounds.
