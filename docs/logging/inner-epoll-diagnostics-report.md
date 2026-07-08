# Inner epoll diagnostics report

## Problem summary

`DescriptorEventPublisher::EPollEvents` owns the inner descriptor-publisher epoll dispatch path and previously consumed several epoll syscall failures without stable diagnostics. The missing checks covered `epoll_create1`, `epoll_ctl` ADD/DEL/MOD, and the non-blocking inner `epoll_wait(..., timeout=0)` drain.

## Outer vs inner epoll distinction

This change targets only the inner `DescriptorEventPublisher::EPollEvents` path. The outer event-loop monitor path still flows through `EventMultiplexer::monitorDescriptors()`, `waitForEvents()`, `TickStatus::TRACE`, and `EventLoop::start()` logging. No outer `EventMultiplexer`/`TickStatus` logic was rewritten.

## Canonical recipe rule applied

The implementation applies the semantic logging recipe at the boundary where each syscall failure is consumed: capture `errno` immediately after the failing syscall and log with `sysError` when the error is relevant.

## Files inspected

- `src/core/multiplexer/epoll/DescriptorEventPublisher.cpp`
- `src/core/multiplexer/epoll/DescriptorEventPublisher.h`
- `src/core/multiplexer/epoll/EventMultiplexer.cpp`
- `src/core/EventMultiplexer.cpp`
- `src/core/EventLoop.cpp`
- `src/core/system/epoll.h`

## epfd ownership/reference note

`EPollEvents` keeps the existing caller-owned `int& epfd` model. The constructor still writes the `epoll_create1(EPOLL_CLOEXEC)` result through that reference; this report and implementation do not convert `epfd` into an owned value member.

## interestCount unsigned/underflow note

`interestCount` remains `std::vector<epoll_event>::size_type`, so it is unsigned. The deletion path now decrements it only after a successful DEL or tolerated `EBADF`, and only when it is greater than zero. Real DEL failures log and return without decrementing.

## epoll_create1 handling

The constructor checks the `epoll_create1` result. On failure, it immediately captures `errno`, logs one `sysError` diagnostic, leaves `epfd` invalid, and relies on follow-on invalid-epfd guards to avoid repeated syscall/log noise.

## epoll_ctl ADD handling

ADD success still increments `interestCount` and resizes the event vector as before. `EEXIST` still falls back to MOD. Other ADD failures now capture `errno` immediately and log a concise `sysError` with the fd.

## epoll_ctl DEL handling

DEL success and tolerated `EBADF` still complete cleanup accounting. Other DEL failures now capture `errno` immediately and log a concise `sysError` with the fd. The failure path returns before resizing or decrementing `interestCount`.

## epoll_ctl MOD handling

MOD now checks the return value, captures `errno` immediately on failure, and logs a concise `sysError` with the fd.

## inner epoll_wait handling

The inner non-blocking drain checks `count < 0`. It captures `errno` immediately, ignores `EINTR` at this layer, logs non-`EINTR` failures with `sysError`, and returns before dispatch.

## Invalid epfd / log-storm behavior

The create failure logs once in the constructor. Later ADD/DEL/MOD and drain calls return early when `epfd` is invalid, avoiding repeated invalid-fd syscall failures and per-tick log storms.

## Implementation summary

- Added a local framework/system `core.mux` `epoll` semantic logger helper to `DescriptorEventPublisher.cpp`.
- Added `epoll_create1` return checking and diagnostics.
- Added invalid-epfd guards for inner follow-on paths.
- Added ADD/DEL/MOD failure diagnostics while preserving `EEXIST` and `EBADF` behavior.
- Added non-`EINTR` inner wait failure diagnostics.
- Preserved event dispatch ordering, timeout behavior, descriptor ownership, and outer epoll monitor behavior.

## Tests added/updated

- Added `tests/unit/log/InnerEpollDiagnosticsTest.cpp`, a source-level regression test for the inner epoll diagnostics contract.
- Registered `InnerEpollDiagnosticsTest` in `tests/unit/log/CMakeLists.txt`.

## Search commands run

Before changes:

```sh
rg -n "epoll_create1|epoll_ctl|epoll_wait|epoll_pwait|TickStatus::TRACE|Core::EventLoop: _tick|sysError|errno" \
  src/core/multiplexer/epoll src/core/EventMultiplexer.cpp src/core/EventLoop.cpp src/core/system \
  -g '*.h' -g '*.hpp' -g '*.cpp'
rg -n "EPOLL_CTL_ADD|EPOLL_CTL_DEL|EPOLL_CTL_MOD|EEXIST|EBADF|EINTR|interestCount|epfd" \
  src/core/multiplexer/epoll \
  -g '*.h' -g '*.hpp' -g '*.cpp'
rg -n "epfd|interestCount|EPollEvents" \
  src/core/multiplexer/epoll/DescriptorEventPublisher.cpp \
  src/core/multiplexer/epoll/DescriptorEventPublisher.h
```

After changes, the same inner/outer epoll searches plus the required syscall, `interestCount`, sensitive logging, high-frequency logging, detach-policy, and legacy macro searches were run during validation.

## Tests run

- `git diff --check` passed.
- `cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON` passed after installing the missing `nlohmann-json3-dev` package required by the existing app configuration.
- `cmake --build cmake-build --parallel 2` passed.
- Focused logging/runtime `ctest` passed.
- Full `ctest --test-dir cmake-build --output-on-failure` passed with the repository's skip-gated component tests reported as skipped.
- Focused ASan configure/build/ctest passed for `CoreRuntimeMigration04Test` and `InnerEpollDiagnosticsTest`.

## Known follow-ups

1. PR E — broader syscall discipline audit.
2. Final macro removal/source gate.
3. Round 10 — book integration.
