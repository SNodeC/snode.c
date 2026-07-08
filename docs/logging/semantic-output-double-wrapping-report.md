# Semantic output double-wrapping report

## Problem summary

Semantic records emitted through `logger::Logger::semanticSink()` were formatted by `logger::LogManager::formatRecord(record)` and then passed through the legacy `emitLine(...)` path. That caused semantic output to receive legacy severity labeling and logger framing after it had already been formatted as semantic text or JSON.

## Root cause

`Logger::emitSemantic(...)` correctly applied semantic filtering and the legacy numeric threshold mapping, but then called `emitLine(*mappedLevel, LogManager::formatRecord(record), false, 0)`. `emitLine(...)` is the legacy LOG/PLOG/VLOG backend; it adds errno text for PLOG and prepends the legacy severity label before writing through the configured loggers. JSON records therefore could be prefixed before the `{`, and text records could be wrapped in a second legacy format.

## Implementation summary

- Kept `emitLine(...)` as the legacy-only backend for `LOG`, `PLOG`, and `VLOG`.
- Added an internal `emitFormattedLine(...)` semantic backend that writes an already formatted semantic line without adding legacy severity, color, or errno decoration.
- Added semantic stdout/file logger instances using a `%v` pattern so semantic records are written exactly as selected by `LogManager::formatRecord(record)`.
- Preserved `LogManager::shouldEmit(record)` and the existing mapped legacy numeric threshold check in `Logger::emitSemantic(...)`.
- Added backward-compatible identity-capable helper overloads in `src/SemanticLog.h`.

## Changed files

- `src/log/Logger.cpp`
- `src/SemanticLog.h`
- `tests/unit/log/CMakeLists.txt`
- `tests/unit/log/SemanticEndToEndOutputTest.cpp`
- `docs/logging/semantic-output-double-wrapping-report.md`

## End-to-end test coverage

`SemanticEndToEndOutputTest` exercises the production semantic output path:

1. `snode::semantic::*Log(...)` helper creates a `BoundaryLogger`.
2. The helper uses `logger::Logger::semanticSink()`.
3. The sink calls `Logger::emitSemantic(...)`.
4. The semantic backend writes to the configured file logger.
5. The test reads the emitted file line back and checks the real output.

The identity helper assertion intentionally uses a capture sink because it validates helper materialization, not the production output path.

## Proof that semantic text output is not legacy-wrapped

The text-output test configures semantic text format, emits one `core.socket` info record through `Logger::semanticSink()`, reads the file, and asserts:

- exactly one non-empty line was emitted;
- the line contains the formatter-selected semantic fields and message;
- the line does not start with a legacy severity label;
- the line does not contain the legacy logger tick wrapper.

## Proof that semantic JSON output is one clean JSON object per line

The JSON-output test configures semantic JSON format, emits multiple records through `Logger::semanticSink()`, reads every non-empty line, and asserts:

- each line starts with `{`;
- no line starts with legacy severity text;
- no line contains the legacy logger tick wrapper;
- each line contains `"v":1`, the expected component, a message field, and an expected level;
- each line ends with `}`.

This catches the previous broken case where JSON was prefixed by legacy text before the JSON object.

## Proof that legacy LOG/PLOG behavior remains unchanged

The legacy regression section emits:

- `LOG(INFO) << "legacy info still works";`
- `PLOG(ERROR) << "legacy plog still works";` with `errno = EACCES`.

It verifies that the configured file output still contains the legacy severity labels, the legacy messages, and the PLOG errno text (`Permission denied`). Legacy output is intentionally not asserted to be semantic JSON.

## Identity helper overload summary

`src/SemanticLog.h` now exposes `snode::semantic::LogIdentity` with optional `instance`, `role`, and `connection` fields. A new `scopedLog(...)` overload forwards those values into `logger::LogScopeOwner`, and identity-capable overloads were added for the public helper layer:

- `frameworkLog(LogIdentity, ...)`
- `coreSystemLog(LogIdentity, ...)`
- `coreSocketLog(LogIdentity, ...)`
- `netConfigLog(LogIdentity, ...)`
- `tlsConfigLog(LogIdentity, ...)`
- `mariaDbLog(LogIdentity, ...)`
- `webHttpLog(LogIdentity, ...)`
- `expressLog(LogIdentity, ...)`
- `appLog(LogIdentity, ...)`

Existing overloads remain intact, so current call sites stay source-compatible.

## Logger caching style note

When multiple semantic lines are emitted in one local scope, prefer `auto log = ...;` and reuse it. Existing app/example call sites may be polished later; this is not a correctness blocker.

## SNODEC_VLOG compat-suite note

Non-compiled compat-suite artifact still contains `SNODEC_VLOG`. It is outside the built app target and will be handled or explicitly excluded in the final macro-removal/source-gate PR.

## Commands run

- `git fetch origin` (failed: repository has no `origin` remote in this workspace)
- `git checkout master` / `git pull --ff-only origin master` were not possible because this workspace has no `master` branch or `origin` remote; the starting `work` branch was already at merge commit `4eef54d` for PR #166.
- `git status --short`
- `git log --oneline -n 30`
- `test -f docs/logging/semantic-api-contract.md`
- `test -f docs/logging/semantic-vlog-elimination-report.md`
- `test -f src/SemanticLog.h`
- `git checkout -b codex/fix-semantic-output-double-wrapping`
- `rg -n "emitSemantic|formatRecord\(|emitLine\(" src/log -g '*.cpp' -g '*.h' -g '*.hpp'`
- `rg -n "SNODEC_VLOG|\bVLOG\s*\(" src tests -g '*.h' -g '*.hpp' -g '*.cpp'`
- `clang-format -i src/log/Logger.cpp src/log/Logger.h src/SemanticLog.h tests/unit/log/SemanticEndToEndOutputTest.cpp`
- `git diff --check`
- `cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON`
- `cmake --build cmake-build --parallel 2`
- `ctest --test-dir cmake-build -R SemanticEndToEndOutputTest --output-on-failure`
- `ctest --test-dir cmake-build -R "SemanticLoggerRound2Test|SemanticCompatibilityRound9Test|SemanticProductionThresholdRepairTest|FinalCleanupMigration09Test|SemanticEndToEndOutputTest" --output-on-failure`
- `ctest --test-dir cmake-build --output-on-failure`
- `cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON`
- `cmake --build cmake-build-asan --parallel 2 --target SemanticEndToEndOutputTest`
- `ASAN=$(gcc -print-file-name=libasan.so); LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan -R SemanticEndToEndOutputTest --output-on-failure`

## Known follow-ups

1. Remove legacy LOG/PLOG/VLOG macro definitions and add the final hard source gate.
2. Decide whether the non-compiled compat-suite SNODEC_VLOG artifact is removed, rewritten, or explicitly excluded by the final gate.
3. Optional polish: use identity-capable helpers in selected app/framework call sites where instance/connection identity matters.
4. Round 10 — book integration.
