# Core Runtime Module (`src/core/*`, `src/utils/*`, `src/log/*`)

> Scope intentionally excludes `src/core/socket/*`.

## What this module is

The non-socket core is the runtime heart of SNode.C: event loop lifecycle, multiplexing strategy, timer/event primitives, low-level system wrappers, process utilities, and logging.

At a high level, this module answers:

- **How does the framework start/stop?** (`core::SNodeC`, `core::EventLoop`)
- **How are descriptors/timers scheduled?** (`EventMultiplexer`, `DescriptorEventReceiver`, `TimerEventReceiver`)
- **How is portability handled?** (`core/system/*`, `utils/system/*`)
- **How is observability handled?** (`log::Logger`)

## Runtime lifecycle

- `core::SNodeC` provides static control entry points (`init`, `start`, `tick`, `stop`, `free`, `state`).
- `core::EventLoop` is the singleton runtime orchestrator and delegates actual I/O waiting to `EventMultiplexer`.
- `core::State` (`LOADED`, `INITIALIZED`, `RUNNING`, `STOPPING`) gives coarse lifecycle semantics.
- `TickStatus` gives per-iteration status and enables controlled integration in external loops.

### Design implications

- Centralized lifecycle lowers accidental complexity for apps.
- `tick()` support makes SNode.C embeddable in hybrid runtimes.
- Singleton loop simplifies global ownership but enforces one primary event domain per process.

## Eventing architecture

### Descriptor side

- `DescriptorEventReceiver` models callbacks for read/write/exception/signal/unobserved and timeout behavior.
- Specialized receivers in `src/core/eventreceiver/*` provide concrete read/write/accept/connect/exception handlers.
- `DescriptorEventPublisher` tracks observed receivers, activation, timeout checks, and cleanup of disabled watchers.

### Multiplexer side

- `src/core/multiplexer/*` implements backend variants: `epoll`, `poll`, and `select`.
- Backend selection is abstracted so users consume one API while platform choice stays internal.

### Timer side

- `TimerEventPublisher`, `TimerEventReceiver`, and `core/timer/*` (`SingleshotTimer`, `IntervalTimer`, stopable interval timers) provide scheduling APIs.
- `core::timer::Timer` wraps timer receiver ownership and cancellation/restart semantics.

## Utility subsystem (`src/utils`)

Key utility domains:

- **Configuration plumbing**: `utils::Config` and embedded `CLI11.hpp` integration.
- **Process control**: `Daemon` support for long-running services.
- **Time and randomness**: `Timeval`, `Random`, `Uuid`.
- **Encoding and diagnostics**: `base64`, `sha1`, `hexdump`, formatting and exception helpers.
- **System wrappers**: signal/time wrappers to normalize platform specifics.

### Why it matters

SNode.C deliberately keeps many runtime dependencies in-tree (or wrapped), reducing hidden coupling and improving portability and reproducibility.

## Logging subsystem (`src/log`)

- `logger::Logger` wraps easylogging++ configuration.
- Provides global log-level and verbose-level controls.
- Supports colored output, optional file logging, and quiet mode.
- CMake options can compile out log categories (`SNODEC_DISABLE_*_LOGGING`).

## Functionality summary

- Event-loop lifecycle API
- Descriptor and timer event scheduling
- Multi-backend polling abstraction
- Daemon/process and OS wrapper helpers
- Runtime logging and verbosity controls

## Pros

1. **Strong modularity**: clear split between loop, receivers, and multiplexers.
2. **Portability focus**: select/poll/epoll and wrapper-based system calls.
3. **Embeddability**: `tick()` can integrate with external orchestration loops.
4. **Operational readiness**: daemon support + structured logging controls.
5. **Template-light at the runtime core**: easier debugging around lifecycle code.

## Cons / tradeoffs

1. **Singleton runtime model** can be restrictive for multi-reactor scenarios.
2. **Complex internal state machine** around receiver lists/timeouts is powerful but harder to reason about for new contributors.
3. **Verbose source footprint** (many adapter/wrapper classes) raises onboarding cost.
4. **Heavy compile flags** and strict warning policy may challenge downstream integration in mixed toolchains.

## Contributor notes

- If changing event semantics, validate all multiplexer backends, not only epoll.
- Keep timeout and disabled-receiver cleanup behavior consistent across publishers.
- Preserve deterministic logging defaults in runtime initialization code.
