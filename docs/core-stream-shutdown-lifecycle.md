# Established stream shutdown lifecycle

This document describes framework shutdown for established plain and TLS stream
connections. It covers the existing `EventLoop` shutdown graph and the
connection-level coordination policy; it does not define a second shutdown or
ownership subsystem.

## Coordinated connection shutdown

`EventLoop::free()` sets the EventLoop state to `STOPPING` and sends a
`ShutdownContext` through the existing graph:

```text
EventLoop
  -> EventMultiplexer
    -> DescriptorEventPublisher
      -> DescriptorEventReceiver
        -> SocketConnectionT
```

An established `SocketConnectionT` has separate reader and writer
`DescriptorEventReceiver` subobjects. Either subobject can therefore deliver the
framework notification, but the complete connection processes it once. A later
notification through the other subobject joins the shutdown already in progress
and does not repeat the connection notification, signal callback, or transport
shutdown request.

`ShutdownReason::Requested`, `ShutdownReason::Signal`, and
`ShutdownReason::NoObserver` all initiate or join the same existing bounded
`shutdownWrite()`/`doWriteShutdown()` transport path. `ShutdownContext` still
records why shutdown began, including the signal number, and trigger-specific
logs and `SNodeC::start()` return values remain distinct. The trigger no longer
selects abrupt descriptor disablement versus graceful transport handling.

Plain streams retain their physical-socket write-shutdown implementation. TLS
streams retain their TLS-aware `doWriteShutdown()` implementation, including
`close_notify`, peer progress, and the existing failure and timeout behavior.
The read side remains observable when it is needed for peer EOF or TLS
`close_notify`; it is not disabled merely because the framework notification
arrived.

### Signal callback compatibility note

Signal-triggered framework shutdown still calls the existing `SocketContext`
signal callback exactly once and leaves its `bool` signature unchanged. Before
this policy, `false` prevented `SocketWriter` from starting graceful shutdown.
While the EventLoop is `STOPPING`, `false` is now retained as an observable
callback result but cannot veto framework transport cleanup: the common bounded
shutdown path proceeds.

This change is deliberately limited to framework shutdown. The existing
`SocketWriter::signalEvent()` behavior remains unchanged for any independent
signal dispatch outside EventLoop shutdown. The current source has no such
ordinary non-`STOPPING` dispatch path: `signalEvent()` is reached through
`DescriptorEventReceiver::onShutdown()`, after `EventLoop::free()` has entered
`STOPPING`. No synthetic signal path is introduced by this change.

## TLS helper coordination

If protocol-initiated TLS shutdown is already active when framework shutdown
begins, the owning connection records the framework notification but joins the
existing operation. It neither creates a second `TLSShutdown` helper nor
restarts the active helper. A signal shutdown still delivers the context signal
callback exactly once in this case.

`TLSShutdown` itself has read and write `DescriptorEventReceiver` subobjects, so
it can receive the same framework shutdown through either or both halves. Its
framework notification response is idempotent and continuation-safe: an active
helper continues the cleanup it already represents instead of being disabled
or replaced. This holds for Requested, Signal, and NoObserver shutdown. The
response is harmless if the helper is encountered twice or is encountered later
in the same multiplexer shutdown pass. The forced `EventMultiplexer::terminate()`
path remains able to disable and release a helper that does not finish during
the graceful drain.

Shutdown of a connection during an active TLS handshake continues to use the
existing TLS handshake ownership and deferred-shutdown mechanisms. This policy
does not change the TLS state machine, helper ownership, application-data
handoff, or fail-closed rules.

## Scheduling while STOPPING

Descriptor receivers remain admissible during `STOPPING`.
`DescriptorEventReceiver::enable()` and `DescriptorEventPublisher::enable()` do
not reject this state, allowing a TLS shutdown helper to register read or write
observation while the notification walk is active. Descriptor inactivity
timeouts also remain usable and drive the existing plain-stream and TLS helper
timeout paths.

New core timers are different. `TimerEventReceiver::enable()` rejects and
deletes a newly enabled timer while `STOPPING`, and the central shutdown also
stops `TimerEventPublisher`. Consequently, framework connection shutdown,
`shutdownWrite()`, TLS shutdown, helper release, context detach, and connection
finalization do not rely on a newly armed `core::timer::Timer`, singleshot timer,
`TimerEventReceiver`, or next-tick callback. TLS progress uses descriptor
observation and descriptor inactivity timeouts instead.

## Live descriptor-publisher iteration

`DescriptorEventPublisher` stores observed receivers as:

```cpp
std::map<int, std::list<DescriptorEventReceiver*>>
```

Registration uses `push_front()` on the descriptor's list, while shutdown uses
live range iteration over the map and each list. `std::list` insertion does not
invalidate existing iterators. When a connection's shutdown notification
registers a TLS helper on the same descriptor list currently being traversed,
`push_front()` places the new receiver before the current element. The active
forward iteration does not move backward to that new front element, and every
pre-existing element remains valid and is still visited.

That same-list property is not the whole correctness argument. Read, write, and
exception receivers live in separate publishers, and the multiplexer visits
those publishers in sequence. A helper added to a publisher whose traversal has
not begun can be visited later in the same overall shutdown call. The helper's
continuation-safe notification behavior therefore supports both outcomes: it
is safe whether the new receiver is skipped by the current list walk or is
notified later through another publisher. Correctness does not depend on read
versus write publisher order.

The container type and the runtime insertion behavior are locked by focused
compile-time and runtime regression tests. No snapshot, secondary receiver
registry, or alternate ownership graph is required.

## Bounded cleanup and lifetime ordering

Graceful shutdown is best effort and bounded. TLS has a configurable
per-connection descriptor/helper `sslShutdownTimeout`, while EventLoop has a
hard two-second outer drain budget shared by all resources and connections. A
per-connection timeout does not extend the EventLoop deadline. If
`sslShutdownTimeout` is longer than the remaining outer budget, or several
connections share that budget, forced termination can occur before TLS
shutdown completes gracefully.

The required ordering is:

```text
EventLoop enters STOPPING
  -> EventMultiplexer::shutdown(context)
  -> coordinated connection shutdown and TLS helper progress
  -> bounded EventLoop drain (two seconds total)
  -> EventMultiplexer::terminate()
  -> disabled receivers/helpers are released
  -> active and pending SocketContext ownership is resolved
  -> connections and their physical sockets are destroyed
  -> EventMultiplexer::clearEventQueue()
  -> "Core: Shutdown config system"
  -> utils::Config::terminate()
```

Completion and forced termination both preserve exactly-once helper release,
context detach, disconnect notification, connection destruction, and physical
descriptor closure. An active `SocketContext` is detached with
`ConnectionClose`; a pending `newSocketContext` is either installed during
normal operation or destroyed during shutdown.

Connection and context `LogScopeOwner` objects remain alive for their final
semantic logs. Those logs are emitted before the existing
`"Core: Shutdown config system"` phase and before `utils::Config::terminate()`
removes configuration and subcommand state, so component, instance,
connection, role, context, origin, and boundary identity remain valid where
applicable. This also respects the semantic logging contract's rule that
borrowed scope strings are materialized synchronously during a log call.
`Config::terminate()` is not destruction of `Logger` or `LogManager`, and this
policy introduces no logging-lifecycle facility.

The implementation stays within the existing shutdown graph and its existing
forced local fallback. It adds no shutdown registry, participant list,
lifecycle coordinator, public shutdown API, or general admission-control
change.
