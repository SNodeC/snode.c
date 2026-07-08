# Broader syscall discipline report

## Problem summary

PR E audits the remaining concrete syscall diagnostics outside the PR #171 inner `DescriptorEventPublisher::EPollEvents` path. The main gaps were unchecked outer epoll setup syscalls and unchecked EventLoop signal syscalls. Poll/select outer wait behavior was inspected and documented rather than rewritten.

## Canonical recipe rule applied

System call failures must be logged at the boundary where they are consumed, `errno` must be captured immediately after the failing syscall, errno-bearing failures use `sysError`, duplicate/follow-on setup noise is avoided, and success-looking lifecycle diagnostics must not be emitted after setup failure.

## Files inspected

- `src/core/multiplexer/epoll/EventMultiplexer.cpp`
- `src/core/multiplexer/epoll/DescriptorEventPublisher.cpp`
- `src/core/multiplexer/epoll/DescriptorEventPublisher.h`
- `src/core/multiplexer/poll/EventMultiplexer.cpp`
- `src/core/multiplexer/poll/DescriptorEventPublisher.cpp`
- `src/core/multiplexer/poll/EventMultiplexer.h`
- `src/core/multiplexer/select/EventMultiplexer.cpp`
- `src/core/multiplexer/select/DescriptorEventPublisher.cpp`
- `src/core/multiplexer/select/EventMultiplexer.h`
- `src/core/EventMultiplexer.cpp`
- `src/core/EventLoop.cpp`
- `src/core/system/epoll.h` and `src/core/system/epoll.cpp`
- `src/core/system/poll.h` and `src/core/system/poll.cpp`
- `src/core/system/select.h` and `src/core/system/select.cpp`
- `utils/system/signal.h` as the available signal helper header; no `src/core/system/signal.h` exists in this tree.

## Outer epoll setup findings

The outer epoll multiplexer created its top-level epoll descriptor in the constructor initializer list and then unconditionally issued three `EPOLL_CTL_ADD` registrations. The `epoll_create1` result and the three `epoll_ctl` results were previously ignored, and the final `Core::multiplexer: epoll` debug line was emitted even if setup failed.

## Outer epoll implementation summary

The constructor now checks `epfd` as the first constructor-body action. If creation failed, it captures `errno` immediately, logs a critical `sysError` for `Core::multiplexer epoll_create1 failed`, leaves the invalid descriptor in place, and returns without issuing follow-on `epoll_ctl` calls.

Each outer publisher registration now goes through a small local helper that checks `core::system::epoll_ctl(epfd, EPOLL_CTL_ADD, ...)`, captures `errno` immediately on failure, and logs `Core::multiplexer epoll_ctl ADD failed: fd=... publisher=...` with `sysError`.

## Outer epoll success-log gating note

The success-looking `Core::multiplexer: epoll` debug line is emitted only when all three outer setup registrations succeed. It is not emitted after failed `epoll_create1` and is not emitted after a failed outer `EPOLL_CTL_ADD` registration.

## Signal syscall findings

`EventLoop::init()`, `EventLoop::_tick()`, `EventLoop::tick()`, and `EventLoop::start()` contained multiple unchecked `sigemptyset`, `sigaddset`, `sigaction`, and `sigprocmask` calls. These calls are bounded setup/restore or tick-boundary operations where failure matters for diagnostics.

## Signal syscall implementation summary

Small local helpers now check signal syscall return values, capture `errno` immediately on failure, and log EventLoop `sysError` diagnostics including the syscall, signal where applicable, and phase. The implementation intentionally continues after logging, matching the prior best-effort behavior while adding diagnostics.

## Signal ordering preservation note

Signal install and restore call order was preserved. In particular, the `start()` restore path still restores `SIGPIPE`, `SIGTERM`, `SIGALRM`, and `SIGHUP`, then calls `free()`, then restores `SIGINT` last.

## Poll/select outer wait assessment

Poll and select multiplexers were inspected and left unchanged. Poll still uses `core::system::ppoll(...)` directly in `poll::EventMultiplexer::monitorDescriptors()`. Select still uses `core::system::pselect(...)` directly in `select::EventMultiplexer::monitorDescriptors()`. Epoll still uses `core::system::epoll_pwait(...)` directly in `epoll::EventMultiplexer::monitorDescriptors()`.

## Poll/select errno preservation verification

The backend `monitorDescriptors()` implementations return the syscall result directly. The `core::system` wrappers set `errno = 0` and directly return the libc call. The generic `EventMultiplexer::waitForEvents()` assigns `activeDescriptorCount = monitorDescriptors(...)` and immediately checks `activeDescriptorCount < 0`; no intervening code runs between the syscall return and the `errno` classification. `EINTR` maps to `TickStatus::INTERRUPTED`; other negative results map to `TickStatus::TRACE`, which `EventLoop::start()` logs with `sysError`. No poll/select errno-preservation gap was found.

## Inner epoll PR #171 verification

The inner epoll path still checks/logs inner `epoll_create1`, `EPOLL_CTL_ADD`, `EPOLL_CTL_DEL`, `EPOLL_CTL_MOD`, and non-`EINTR` `epoll_wait` failures. The `EEXIST` and `EBADF` behavior remains present, invalid-epfd guards remain present, and `interestCount` decrement remains guarded.

## Descriptor-precondition notes for poll/select

`select::FdSet::set()`, `clr()`, and `isSet()` call `FD_SET`, `FD_CLR`, and `FD_ISSET` without local fd-range guards. `poll::PollFdsManager::muxDel()`, `muxOn()`, and `muxOff()` assume a descriptor exists in `pollFdIndices` before dereferencing lookup results. These are descriptor-container/precondition hardening concerns rather than errno-bearing syscall diagnostics, so they were left for a later focused PR.

## Invalid setup / log-storm behavior

If outer `epoll_create1` fails, PR E logs exactly that setup failure and skips the three outer `EPOLL_CTL_ADD` calls, avoiding three predictable follow-on `EBADF` logs. If an outer `EPOLL_CTL_ADD` fails after a valid outer epoll descriptor is created, only that concrete registration failure is logged, and the success-looking debug line is withheld.

## Tests added/updated

- Added `tests/unit/log/BroaderSyscallDisciplineTest.cpp` as a source-level regression test for outer epoll setup diagnostics, EventLoop signal syscall diagnostics, poll/select direct monitor paths, direct core system wrappers, and inner epoll PR #171 diagnostic evidence.
- Registered `BroaderSyscallDisciplineTest` in `tests/unit/log/CMakeLists.txt`.

## Search commands run

- `rg -n "epoll_create1|epoll_ctl|epoll_wait|epoll_pwait|ppoll|pselect|select\\(|poll\\(|sigaction|sigprocmask|sigemptyset|sigaddset|sysError|errno" src/core -g '*.h' -g '*.hpp' -g '*.cpp'`
- `rg -n "FD_SET|FD_CLR|FD_ISSET|FD_ZERO|pollFdIndices\\.find|pollFdIndices\\[|interestCount|epfd|TickStatus::TRACE" src/core/multiplexer src/core/EventMultiplexer.cpp src/core/EventLoop.cpp -g '*.h' -g '*.hpp' -g '*.cpp'`
- `rg -n "sigaction\\(|sigprocmask\\(|sigemptyset\\(|sigaddset\\(" src/core -g '*.h' -g '*.hpp' -g '*.cpp'`
- Post-change syscall, signal, sensitive logging, high-frequency logging, SocketContext detach, inner epoll, and legacy macro regression searches requested for PR E.

## Tests run

- `cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON`
- `cmake --build cmake-build --parallel 2`
- `ctest --test-dir cmake-build -R "BroaderSyscallDisciplineTest|InnerEpollDiagnosticsTest|CoreRuntimeMigration04Test|SocketContextDetachPolicyTest|HighFrequencyLoggingSeverityTest|SensitiveLoggingRedactionTest|SemanticEndToEndOutputTest|FinalCleanupMigration09Test" --output-on-failure`
- `ctest --test-dir cmake-build --output-on-failure`
- `cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON`
- `cmake --build cmake-build-asan --parallel 2 --target CoreRuntimeMigration04Test BroaderSyscallDisciplineTest InnerEpollDiagnosticsTest`
- `ASAN=$(gcc -print-file-name=libasan.so) LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan -R "CoreRuntimeMigration04Test|BroaderSyscallDisciplineTest|InnerEpollDiagnosticsTest" --output-on-failure`

## Known follow-ups

1. Final macro removal/source gate.
2. Round 10 — book integration.
3. Optional: descriptor-container/precondition hardening for poll/select.
