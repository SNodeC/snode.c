# Codex stdio lifecycle and descriptor ownership

This document records the ownership and shutdown contract of the local Codex
App Server transport. It is an implementation note for maintainers, not an
extension of the public API.

## Event-loop model

The transport uses no worker threads and performs no blocking I/O or blocking
process wait. Process creation uses `posix_spawnp()`. All parent descriptors are
registered through SNode.C descriptor event receivers, and child reaping uses
only `waitpid(..., WNOHANG)`.

The implementation relies on the existing `DescriptorEventReceiver` shutdown
notification path and the bounded drain already performed by
`core::EventLoop::free()`. It does not register a public `EventLoop::onShutdown`
callback and does not introduce a lifecycle pipe.

## Pipe construction and spawn mapping

Three anonymous POSIX kernel pipes are created with `pipe2(..., O_CLOEXEC)`:

| Pipe | Parent endpoint | Child endpoint |
| --- | --- | --- |
| stdin | write endpoint | read endpoint mapped to FD 0 |
| stdout | read endpoint | write endpoint mapped to FD 1 |
| stderr | read endpoint | write endpoint mapped to FD 2 |

Immediately after creation, every endpoint is relocated above FD 2 with
`F_DUPFD_CLOEXEC` when necessary. This makes the `posix_spawn_file_actions`
mapping safe even when the parent entered startup with one or more standard
descriptors closed. No temporary unreserved descriptor range is used.

The required setup order is:

1. construct all three `core::pipe::Pipe` owners;
2. verify that both endpoints of every pipe are present and above FD 2;
3. add direct `dup2` mappings for child FDs 0, 1, and 2;
4. add child-side close actions for all six original pipe endpoints;
5. register the timerfd rollback reserve;
6. create a new child process group with `posix_spawnp()`;
7. close the three child-facing endpoints in the parent;
8. prefer pidfd observation while retaining the reserve on failure;
9. transfer the parent endpoints through `Pipe`, which makes them nonblocking
   and registers their receivers transactionally; and
10. complete parent callback setup.

The child's standard descriptors remain normal blocking descriptors. Only the
parent endpoints used by the event loop have `O_NONBLOCK`.

## Ownership chain

Before transfer, each `core::pipe::Pipe` is the sole RAII owner of its remaining
endpoints. Releasing an endpoint transfers ownership exactly once:

```text
stdin write  -> PipeSource
stdout read  -> PipeSink
stderr read  -> PipeSink
```

`PipeSource` and `PipeSink` are self-owned event receivers. Callers keep
non-owning pointers only to coordinate shutdown; they do not delete receivers.
Closing a receiver removes it asynchronously and invokes its closure callback.
Closure callbacks clear the matching non-owning pointer and never delete the
receiver.

Callbacks, including the pipe receivers' shutdown callbacks, capture a weak
session reference. The child-exit receiver is the exception: it retains the
session until the child has been reaped. The session also uses `selfKeepAlive`
after a successful spawn. Final exit handling detaches the observer, closes
remaining endpoints, and releases `selfKeepAlive`.

If setup fails before spawn, `Pipe` destructors close every endpoint still
owned. If setup fails after spawn, rollback closes parent endpoints, sends
`SIGKILL` to the child process group, retains exit observation, and completes a
nonblocking reap before releasing the session.

## Child-exit observation

Immediately before `posix_spawnp()`, the transport registers a nonblocking
timerfd polling receiver as a rollback reserve. Failure to register this
receiver aborts before a child exists. After a successful spawn, the transport
prefers a nonblocking close-on-exec pidfd returned by `pidfd_open()`. When pidfd
registration succeeds, it becomes the child-exit receiver and the reserve is
detached. When pidfd creation or registration fails, the already registered
reserve remains the polling fallback.

If pidfd creation is unavailable, or the test-only forced fallback is active,
the nonblocking periodic Linux `timerfd` reserve remains registered. Each
expiration performs one nonblocking lifecycle tick and
`waitpid(childPid, ..., WNOHANG)`. The timer interval is 20 ms. The timerfd is
not a pipe and carries no process data; it only provides event-loop readiness
for polling and shutdown deadlines.

The fallback deliberately does not duplicate stdout. A duplicate stdout reader
can receive EOF/HUP, consume protocol bytes, or disappear before the child is
reaped. That failure mode caused shutdown to lose its only polling trigger after
unexpected stdout closure.

If pidfd opening or registration fails, the pidfd is closed and polling is used.
Both observer types retain the session until the child has been reaped. There is
never a blocking `waitpid()` loop.

Descriptor registration is observable across epoll, poll, and select. Publisher
bookkeeping is rolled back on failure. Parent pipe transfer does not release an
endpoint until nonblocking setup and receiver registration both succeed, so any
partial parent setup failure closes cleanly while the registered child observer
continues nonblocking reaping.

## Shutdown sequence

Shutdown distinguishes transport flush, EOF delivery, and process signals:

```text
PipeSource::eof()
  -> drain accepted stdin output
  -> physically close child stdin
  -> SIGTERM grace period
  -> SIGTERM
  -> SIGKILL grace period
  -> SIGKILL if still running
  -> nonblocking reap
  -> release session retention
```

If stdin cannot drain before its flush deadline, queued data is discarded and
stdin is closed immediately before the signal sequence continues. All deadlines
are driven by the registered pidfd timeout or polling timerfd; there is no busy
loop.

During global SNode.C shutdown, descriptor receivers receive `onShutdown`, which
starts the same sequence. The existing bounded `EventLoop::free()` drain keeps
registered descriptors, timeouts, and queued events alive long enough for EOF,
SIGTERM, SIGKILL, and nonblocking reaping. Destruction of the public client does
not invalidate this sequence because the session remains retained until reap.

## Stream closure rules

- stdin failure is fatal outside expected shutdown;
- stdout EOF or read failure is fatal outside expected shutdown or child exit;
- stderr EOF emits a trailing partial diagnostic line and detaches cleanly;
- stderr read failure emits the trailing line plus a system-error diagnostic and
  detaches cleanly;
- stderr closure never changes the client to `Failed` by itself.

Fatal reporting is guarded so that a transport failure and the subsequent child
exit cannot report failure twice.

## Current platform boundary

The production pidfd path and the current timerfd fallback are Linux-specific.
The event-loop behavior is verified with epoll, poll, and select on Linux. A
non-Linux port still needs a native event-loop-compatible child-exit trigger,
such as integration with the platform's existing signal facilities, while
preserving `waitpid(..., WNOHANG)` semantics.
