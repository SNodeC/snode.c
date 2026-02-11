# Core runtime module (`src/core/*`, `src/utils/*`, `src/log/*`)

This document covers the **core runtime** of SNode.C:

- `src/core/*` (excluding `src/core/socket/*`)
- `src/utils/*`
- `src/log/*`

It explains the *event loop*, *multiplexer*, *timer and descriptor observation*, *dynamic loader*, and the *configuration + logging* facilities that almost every other module relies on.

---

## 1. Overview and goals

The core runtime is designed around a few constraints that show up throughout the project:

- **Single-threaded, non-blocking I/O** as the default execution model (with explicit points where you may add threads in application code).
- **Unified “event receiver” abstraction**: sockets, pipes, files and timers all “plug into” the same loop.
- **Portability**: multiple I/O multiplexers exist (`select`, `poll`, `epoll`) and are wrapped in a consistent interface.
- **Composable subsystems**: modules may register FDs or timers, and the loop drives them without knowing the higher-level protocol.

The result is a compact core that other modules (HTTP, Express, WebSocket, MQTT, DB) can build upon without bringing in heavyweight frameworks.

---

## 2. Public API entry points

If you only want to “use SNode.C as a runtime”, these are the most common entry points:

### Core lifecycle
- `#include "core/SNodeC.h"` — global runtime lifecycle  
  - `core::SNodeC::init(argc, argv)`
  - `core::SNodeC::start(timeout)`
  - `core::SNodeC::tick(timeout)` for embedding into an external loop
  - `core::SNodeC::stop()`, `core::SNodeC::free()`
  - `core::SNodeC::state()` returning `core::State`

### Event loop / multiplexer internals (advanced)
- `#include "core/EventLoop.h"`
- `#include "core/EventMultiplexer.h"`

### Timer primitives
- `#include "core/TimerEventPublisher.h"`
- `#include "core/timer/Timer.h"` (and `Timer.cpp`)

### Event receivers
Located under `src/core/eventreceiver/`:
- `AcceptEventReceiver`, `ReadEventReceiver`, `WriteEventReceiver`, `ExceptionalConditionEventReceiver`
- “descriptor” and “timer” oriented receivers
- base `core::EventReceiver`

### Utilities & configuration
- `#include "utils/Config.h"` — CLI + config file + persistent options
- `#include "utils/Timeval.h"` — time handling used across the project
- `#include "utils/CLI11.hpp"` — vendored CLI11 (wrapped and used by `utils::Config`)
- `#include "log/Logger.h"` — logging macros + logger initialization

---

## 3. Core lifecycle and the event loop

### 3.1 `core::SNodeC`: the global runtime facade

`core::SNodeC` is a static-only facade (`operator new` deleted) that centralizes:

- argument parsing and global config setup
- initialization of logging
- construction/activation of the event loop
- entering the loop (`start`) or stepping it (`tick`)
- shutdown (`stop`, `free`)

See: `src/core/SNodeC.h`, `src/core/SNodeC.cpp`.

From a user perspective, this is the primary “runtime API”.

### 3.2 `core::EventLoop` and `core::EventMultiplexer`

At the heart is an `EventLoop` which delegates I/O waiting to an `EventMultiplexer`.

- The **EventLoop** manages the high-level state machine:
  - initialization → running → stopping
  - running the “tick” iterations
  - coordinating deferred cleanup (notably the delayed dynamic loader closes)
- The **EventMultiplexer** is the abstraction over platform-specific I/O wait mechanisms.

See: `src/core/EventLoop.h`, `src/core/EventMultiplexer.h`, and `src/core/multiplexer/*`.

### 3.3 Multiplexer backends

SNode.C provides multiple backends (depending on platform/build):

- `src/core/multiplexer/select`
- `src/core/multiplexer/poll`
- `src/core/multiplexer/epoll`

Each backend provides a concrete `EventMultiplexer` implementation but exposes the same conceptual features:

- register/unregister interest for:
  - readable events
  - writable events
  - exceptional/out-of-band events (when supported)
- compute next timeout (combining timer deadlines and user timeouts)
- dispatch ready FDs to their event receivers

**Practical note**: `epoll` is usually the preferred backend on Linux for scalability, while `select` exists as a portability fallback.

---

## 4. Event receivers: how work gets scheduled

A central pattern in SNode.C is: *modules don’t poll; they register an event receiver.*

### 4.1 The `core::EventReceiver` base class

`core::EventReceiver` represents “something the loop can call back”.

It provides the “span” operation (used by the loop to run active receivers), and is subclassed into:

- descriptor-based event receivers (attached to file descriptors)
- timer-based event receivers (attached to deadlines)

### 4.2 Descriptor event receivers (`src/core/eventreceiver/*`)

The split by event type mirrors common multiplexer semantics:

- `ReadEventReceiver` — called when the FD is readable
- `WriteEventReceiver` — called when the FD is writable
- `ExceptionalConditionEventReceiver` — called on exceptional conditions (OOB, error flags)
- `AcceptEventReceiver` — specialized for server accept loops

A typical connection or server will own one or more of these receivers and implement the relevant callbacks (e.g. `readEvent()`, `writeEvent()`, …).

### 4.3 Timer events and the timer publisher

Timers are managed through `core::TimerEventPublisher` (`src/core/TimerEventPublisher.h`):

- Maintains a `std::set` of timer receivers ordered by their next deadline.
- Computes the next overall timeout (`getNextTimeout`).
- “Spans” active timers on each tick and can remove/disable timers safely.

This design keeps timer handling deterministic and integrates with the multiplexer wait timeout.

---

## 5. Dynamic loading (`core::DynamicLoader`)

SNode.C includes a wrapper around `dlopen/dlsym/dlclose` (`src/core/DynamicLoader.h`):

- Keeps a registry of opened shared libraries (canonicalized path, refCount).
- Supports **delayed close** (`dlCloseDelayed`) rather than immediately unloading.
- The close queue is processed by the event loop / multiplexer friends.

The delayed close mechanism is a pragmatic solution for real-world plugin systems:
unloading a shared object while callbacks are still on the stack (or when other objects still hold function pointers) is a frequent source of crashes.
Deferring the close to a safe point in the loop reduces that risk.

**Pros**
- Centralized reference counting.
- Safer shutdown semantics for dynamically loaded modules.

**Cons / gotchas**
- Delayed closes mean “unload” is not immediate; if you rely on strict unload timing, you must understand when the loop drains the close queue.
- As with any `dlopen`-based plugin system, ABI stability becomes a key responsibility for module authors.

---

## 6. File utilities (`core/file/*`)

The `core/file` folder provides lightweight helpers for file I/O that integrate with the overall error handling style:

- `File` (`File.h/.cpp`) — open, size, basic management
- `FileReader` (`FileReader.h/.cpp`) — read file contents

These utilities are used by higher-level modules such as HTTP “sendFile” functionality and configuration tooling.

---

## 7. System wrappers (`core/system/*`)

The `core/system` folder wraps platform APIs (`unistd`, `socket`, `select`, `poll`, `epoll`, `netdb`, `dlfcn`) behind project-local headers.

Reasons this is useful:

- consistent include points for platform-dependent compilation
- ability to introduce compatibility shims or test hooks
- reducing scattered `#ifdef` logic across the project

---

## 8. Utilities (`src/utils/*`)

The `utils` module contains general-purpose infrastructure used across the stack. The most impactful parts for users are **time handling** and **configuration**.

### 8.1 `utils::Timeval`

`utils::Timeval` is the project-wide time type. It appears in:

- `core::SNodeC::start(timeOut)`
- timer scheduling
- socket timeouts
- retry/backoff logic in clients

The intention is to provide a consistent “seconds + microseconds”-style representation that maps well to POSIX time APIs while remaining convenient in C++.

### 8.2 `utils::Config`: CLI + config file + persistent options

`utils::Config` wraps the vendored `CLI11` (`src/utils/CLI11.hpp`) and provides:

- application-global CLI parsing
- per-“instance” subcommands (SNode.C uses “INSTANCE” where CLI11 says “SUBCOMMAND”)
- config file integration via `--config-file` (defaulting into `~/.config/snode.c/<app>.conf` for non-root users)
- a model of **persistent** vs **nonpersistent** options (as seen in the custom help labels)

Important behaviors visible in `src/utils/Config.cpp`:

- The config parser allows “extras” and “config extras” (useful when multiple modules contribute options).
- The default config/log/pid directories depend on whether the process runs as root.
- The config file default path is computed from the derived application name (`argv[0]` basename).
- Logger initialization is triggered early (`logger::Logger::init();`).

In other words: SNode.C assumes long-running services that can be configured in a sysadmin-friendly manner.

### 8.3 Other notable utilities

Depending on what modules you use, you will also encounter:

- `utils::Daemon` — daemonization and PID/log directory conventions
- `utils::Formatter` — formatting helpers used by config/help
- attribute injection helpers used by Express (`utils/AttributeInjector.h`)

---

## 9. Logging (`src/log/*`)

Logging is centralized in `log/Logger.h`:

- SNode.C uses a logger facade that provides macros like `LOG(level)` and `VLOG(n)`.
- The logger is initialized during configuration setup.
- The intent is to make logging *cheap to call* and consistent across modules.

Because the framework is event-driven, good logging is also critical for diagnosing:

- lifecycle transitions (connect/listen/close)
- protocol state machines (HTTP parsing, WebSocket frames, MQTT packets, DB commands)
- subtle edge cases (timeouts, partial reads, backpressure)

---

## 10. Pros / cons summary (core runtime)

### Pros
- Clean event-driven foundation (timers + FD observation) that scales to many connections.
- Multiple multiplexer backends for portability.
- Unified configuration and logging.
- Dynamic loader supports safe-ish plugin patterns via delayed closes.

### Cons / tradeoffs
- “Global singleton-ish” style (`core::SNodeC`, `utils::Config`) is convenient for services but less ideal for strict library embedding scenarios.
- The architecture assumes non-blocking designs; blocking calls in callbacks will harm throughput and latency.
- Correctness depends heavily on carefully managed lifetimes (shared_ptr usage patterns in higher modules help, but users can still create cycles).

### Gotchas
- Understand the `tick()` vs `start()` distinction: `tick()` enables embedding but requires the embedder to call it often enough.
- Delayed dynamic loader closes are not immediate; don’t assume an unload is synchronous.
- Option naming and persistence are standardized; when adding new options, follow the conventions to keep config files stable.

---

## 11. Where to go next

If you want to understand how SNode.C actually does networking, continue with:

- [`core_socket.md`](core_socket.md) — the socket abstraction (stream + dgram) and TLS integration  
- [`net.md`](net.md) — addresses and configuration objects used by servers and clients
