# Source-policy test root discovery report

## Problem summary

Several source-level logging and syscall policy tests locate the repository root before scanning production source files. Their previous fallback discovery logic depended on walking parent directories from the current working directory when `SNODEC_SOURCE_DIR` was not provided.

## Local failure summary

The affected local CTest failures were timeouts in source-scanning tests rather than runtime networking failures:

- `SocketContextDetachPolicyTest`
- `SensitiveLoggingRedactionTest`
- `HighFrequencyLoggingSeverityTest`
- `InnerEpollDiagnosticsTest`
- `BroaderSyscallDisciplineTest`

## Root cause

The fallback root discovery loop used `std::filesystem::parent_path()` until the current path became empty. For an absolute filesystem root such as `/`, `parent_path()` can return the same path again. If the marker file was not found from the CTest working directory, the loop could stay at the filesystem root forever.

## Files inspected

- `tests/unit/log/CMakeLists.txt`
- `tests/unit/log/SensitiveLoggingRedactionTest.cpp`
- `tests/unit/log/HighFrequencyLoggingSeverityTest.cpp`
- `tests/unit/log/SocketContextDetachPolicyTest.cpp`
- `tests/unit/log/InnerEpollDiagnosticsTest.cpp`
- `tests/unit/log/BroaderSyscallDisciplineTest.cpp`

## Tests fixed

- `SensitiveLoggingRedactionTest`
- `HighFrequencyLoggingSeverityTest`
- `SocketContextDetachPolicyTest`
- `InnerEpollDiagnosticsTest`
- `BroaderSyscallDisciplineTest`

## CMake environment fix

`tests/unit/log/CMakeLists.txt` now assigns `SNODEC_SOURCE_DIR=${CMAKE_SOURCE_DIR}` through CTest for each source-policy test while preserving its existing labels. This gives the tests a deterministic source root during normal CTest execution.

## Fallback loop fix

The duplicated root discovery logic was replaced by a small header-only helper in `tests/unit/log/SourcePolicyTestRoot.h`. The fallback walk now compares each path with its parent and breaks when `parent == current`, so it cannot loop forever at the filesystem root.

## Common root marker decision

All affected tests now use the same stable project-root marker, `src/SemanticLog.h`. This marker is used only to locate the repository root; each policy test still reads and validates its own target files after the root is found.

## Fail-fast behavior

If `SNODEC_SOURCE_DIR` is set but does not point at the project root, the helper prints a clear diagnostic and returns an empty path. If the fallback walk cannot locate the marker, the helper prints the starting directory and returns an empty path. Each fixed test exits with status 1 when the root is unavailable.

## Validation run

Validation performed for this PR:

- Required pre-change root-discovery searches.
- Required post-change root-discovery searches.
- Timeout/skip/disable guard search.
- Legacy macro call-site regression search.
- `clang-format` on the touched C++ tests and shared helper.
- `git diff --check`.
- CMake configure and build with tests and apps enabled.
- Focused CTest run for the five affected source-policy tests.
- Focused CTest run from `/tmp` using the build directory.
- Full CTest run.

## Known follow-ups

1. Final macro removal/source gate.
2. Round 10 — book integration.
