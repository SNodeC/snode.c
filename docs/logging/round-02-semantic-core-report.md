# Logging Roadmap Round 02 Report: Semantic `src/log` Core

Branch/report: `codex/draft-pr-for-logging-roadmap-round-2` / Round 02 semantic core

## Implementation diff summary

Round 02 adds the semantic logging engine on a test stand without integrating it into the framework object model. The implementation is intentionally limited to `src/log` production code, unit-test registration, a focused Round 02 unit harness, and this report.

Implemented:

* Semantic enums: `LogLevel`, `LogOrigin`, `LogBoundary`, and `LogRole`.
* Borrowed `LogScope` with non-owning `std::string_view` fields.
* Owned `LogRecord` plus optional `LogError` and `LogSource` payloads.
* `materialize(...)` conversion that copies borrowed scope strings into owned record storage before formatter/sink handoff.
* JSON v1 formatter with mandatory `v`, `ts`, `level`, `origin`, `boundary`, `component`, and `message` fields.
* JSON optional fields `instance`, `role`, `connection`, `event`, `error`, and `source`, emitted only when present; absent optionals are omitted and never emitted as `null`.
* Minimal text formatter generated from the same owned semantic record.
* `BoundaryLogger` test/local API with format-style calls, stream-style calls, local threshold checks, and `sysError` overloads for `int` and `std::error_code`.
* Owned internal `OwnedLogScope` storage so `BoundaryLogger` does not retain borrowed `LogScope` `std::string_view` fields after construction.
* Complete JSON escaping for all control characters below `0x20`, including `\b`, `\f`, and `\u00XX` fallback escapes.
* Focused macro compatibility smoke harness proving `LOG`, `PLOG`, and `VLOG` still compile/link/run through the existing macro path.

## Exact files changed

Repair-only files changed in this follow-up:

* `src/log/SemanticLogger.h`
* `src/log/SemanticLogger.cpp`
* `tests/unit/log/SemanticLoggerRound2Test.cpp`
* `docs/logging/round-02-semantic-core-report.md`

No other files were changed by this repair. The full Round 02 PR file set remains:

* `src/log/CMakeLists.txt` — adds the Round 02 semantic logger source/header to the existing `logger` library.
* `src/log/SemanticLogger.h` — new semantic types, owned internal `OwnedLogScope`, materialization/formatter declarations, `BoundaryLogger`, and `LogStream` API.
* `src/log/SemanticLogger.cpp` — new materialization, owned-scope copying/viewing for `BoundaryLogger`, complete JSON control-character escaping, string rendering, timestamp rendering, JSON v1/text formatting, stream flush, threshold, and sink emission implementation.
* `tests/unit/CMakeLists.txt` — registers the new `tests/unit/log` subdirectory.
* `tests/unit/log/CMakeLists.txt` — declares `SemanticLoggerRound2Test` and labels it `unit;log;round2`.
* `tests/unit/log/SemanticLoggerRound2Test.cpp` — focused Round 02 semantic core and macro compatibility harness.
* `docs/logging/round-02-semantic-core-report.md` — this report.

No socket, HTTP, WebSocket, MQTT, DB, application, `ConfigInstance`, `SocketConnection`, `SocketContext`, `SocketServer`, `SocketClient`, `SocketAcceptor`, or `SocketConnector` production files were changed.

## Exact build commands run

Dependency gate:

```sh
pkg-config --modversion nlohmann_json || true
```

Initial configure before installation failed because `nlohmann_json>=3.11` was missing. The dependency was then installed successfully:

```sh
sudo apt-get update
sudo apt-get install -y nlohmann-json3-dev
```

After installation, `pkg-config --modversion nlohmann_json || true` reported:

```text
3.11.3
```

Normal configure/build:

```sh
cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON
cmake --build cmake-build --parallel 2
```

Focused build:

```sh
cmake --build cmake-build --parallel 2 --target SemanticLoggerRound2Test
```

## Exact test commands run

Focused Round 02 test:

```sh
ctest --test-dir cmake-build -R SemanticLoggerRound2Test --output-on-failure
```

Result: passed, 1/1 tests. The focused test now includes the BoundaryLogger construction-time scope-copy check and the JSON control-character escaping check.

Full normal test suite:

```sh
ctest --test-dir cmake-build --output-on-failure
```

Result: passed, 108/108 tests, with many existing component tests reported as skipped by the existing harness/environment skip behavior.

Source-level/status checks:

```sh
git status --short
git diff --check
```

Result: `git status --short` showed the four expected modified repair files plus local untracked build directories while validation was running; `git diff --check` passed with no whitespace errors. The build directories were not committed.

## Sanitizer commands/results

AddressSanitizer configure/build/test:

```sh
cmake -S . -B cmake-build-asan \
  -DSNODEC_BUILD_TESTS=ON \
  -DSNODEC_BUILD_APPS=ON \
  -DSNODEC_ENABLE_ASAN=ON
cmake --build cmake-build-asan --parallel 2
ASAN=$(gcc -print-file-name=libasan.so)
LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan --output-on-failure
```

Result: passed, 108/108 tests, with many existing component tests reported as skipped by the existing harness/environment skip behavior.

The ASan run used `LD_PRELOAD=$(gcc -print-file-name=libasan.so)` so the ASan runtime loaded first.

An earlier Round 02 run without `LD_PRELOAD` failed with:

```text
ASan runtime does not come first in initial library list; you should either link runtime to your application or manually preload it with LD_PRELOAD.
```

The corrected `LD_PRELOAD=$(gcc -print-file-name=libasan.so)` invocation passed.

## Macro compatibility result

`SemanticLoggerRound2Test` includes `log/Logger.h` and executes representative `LOG`, `PLOG`, and `VLOG` statements after using existing logger hooks (`Logger::init`, `Logger::setDisableColor`, `Logger::setTickResolver`, `Logger::setLogLevel`, and `Logger::setVerboseLevel`). This proves the legacy macros still compile/link/run through the existing macro definitions.

Covered in the harness:

* enabled `LOG(INFO)` smoke path,
* disabled `LOG(TRACE)` smoke path under the configured level,
* enabled `PLOG(ERROR)` smoke path with fixed `errno = EACCES`,
* enabled `VLOG(1)` smoke path,
* disabled `VLOG(2)` smoke path.

No legacy macro definition was removed or rewritten. No production macro call site was migrated. `VLOG` remains compatibility-only and is not modeled as a third semantic axis.

Byte-for-byte output compatibility capture remains deliberately deferred because Round 02 must not change macro internals, sink plumbing, or backend routing to make capture easier. The Round 02 harness therefore stays at compile/link/run smoke coverage for macro compatibility.

## JSON schema result

The JSON v1 formatter emits the frozen mandatory fields:

* `v`
* `ts`
* `level`
* `origin`
* `boundary`
* `component`
* `message`

The formatter escapes all JSON string control characters below `0x20`: common controls use short escapes where applicable and other controls use `\u00XX`. The focused test verifies `\b` and `\u0001` output and checks that the corresponding raw control bytes are absent.

The formatter emits optional fields only when present:

* `instance`
* `role`
* `connection`
* `event`
* `error`
* `source`

Absent optional fields are omitted rather than emitted as `null`. Unknown role is omitted; Round 02 does not emit `"unknown"`.

## string_view materialization/lifetime result

The direct materialization test constructs `LogScope` from mutable `std::string` storage, materializes a `LogRecord`, clears the original strings, and verifies that `record.component`, `record.instance`, and `record.connection` still contain the original values. This proves formatter/sink handoff receives owned bytes, not borrowed `LogScope` references.

The `BoundaryLogger` lifetime repair adds owned internal scope storage. The test constructs a logger from borrowed strings, mutates those original strings before logging, emits a record, and verifies that the emitted record still contains the original `mqtt.server`, `mqtt-broker`, and `#7` values. This proves logger construction copies the borrowed scope data and later log calls do not read stale caller-owned `std::string_view` storage.

## sysError result

`BoundaryLogger::sysError` accepts both `int` errno-like values and `std::error_code`. The test verifies that:

* `error.code` is stored,
* `error.text` is populated,
* the human message is formatted separately from the error object,
* JSON emits `error` when present.

Existing `PLOG` behavior was not changed.

## Stream-style API result

`logger.debug() << "received " << 42 << " bytes";` emits one complete record at stream object destruction. The focused test verifies exactly one stream-style record for that expression and no partial fragments.

## Format-style API result

`logger.debug("received {} bytes", 42);` and `logger.info("echo session started");` both work in the focused test. The Round 02 formatter is intentionally minimal and local to the semantic logger test stand.

## CI result

GitHub PR #143 checks were inspected on GitHub. The visible `CI on: pull_request` / `gcc-debug` job for the PR head shown on GitHub succeeded on July 5, 2026 in 7m 33s, with one Node.js deprecation warning annotation. This local repair commit has been pushed by this environment, so no newer remote CI result exists for these local changes yet.

## Scope/non-goal confirmations

Confirmed:

* No production object `.log()` APIs were added.
* No `SocketContext::log()` or `SocketContext::frameworkLog()` API was added.
* No `SocketConnection::log()` API was added.
* No `LogScopeOwner` was added.
* `SocketConnection`, `SocketContext`, `ConfigInstance`, `SocketServer`, `SocketClient`, `SocketAcceptor`, and `SocketConnector` integration was not started.
* No production call sites were migrated.
* `LOG`, `PLOG`, and `VLOG` runtime behavior was not intentionally changed.
* Existing `Logger.cpp` CLI/backend behavior was not unified with the semantic logger.
* Startup filter configuration, filter precedence, `freeze()`, runtime reload, signals, admin endpoints, and mutable atomic thresholds were not implemented.

## Repair-specific confirmations

Confirmed for this repair:

* `BoundaryLogger` no longer stores a borrowed `LogScope` member.
* `BoundaryLogger` stores owned internal scope data in `OwnedLogScope`.
* A focused test proves mutating original scope strings after logger construction does not affect emitted records.
* The direct `materialize(...)` borrowed-to-owned test still passes.
* JSON escaping handles all control characters below `0x20`.
* A focused test covers previously unescaped control characters (`\b` and `\u0001`).
* The report branch name is `codex/draft-pr-for-logging-roadmap-round-2`.
* No files outside the expected repair set were changed.

## Extended SNode.C and MQTT/mqttsuite review note

For this Round 02 implementation I re-read the corrected Round 01 contract/report from the current checkout before coding and kept the implementation constrained to the semantic `src/log` core. The broader SNode.C/MQTT observations remain the same as the corrected Round 01 baseline: no top-level `mqttsuite` source tree is present in this checkout, MQTT code lives under `src/iot/mqtt`, and MQTT call-site migration is explicitly deferred until later roadmap rounds. Round 02 does not modify MQTT production code.

## Deliberate deferrals

* Production framework object integration (`SocketContext`, `SocketConnection`, `ConfigInstance`, server/client/acceptor/connector APIs).
* `LogScopeOwner` ownership model.
* Production call-site migration.
* Backend unification with current `Logger.cpp`, CLI behavior, or spdlog output format.
* Startup-only logging manager, immutable filtering configuration, and precedence implementation.
* Runtime log-level reload, signals, admin endpoints, and mutable atomic thresholds.
* Byte-for-byte legacy macro output capture.
* A full formatting library implementation; Round 02 uses a small `{}` replacement helper sufficient for the test-stand API proof.

## Known non-blocking follow-ups

* Add a proper output-capture/golden compatibility harness in the later backend unification work.
* Replace or extend the minimal `{}` formatter if/when the project standardizes on a formatting dependency for semantic logger messages.
* Decide later whether JSON output should use a shared JSON library or continue using a no-extra-dependency formatter.
* Wire object-owned scopes and cached thresholds only in the designated future rounds.
