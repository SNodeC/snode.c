# Migrating to SNode.C 2.0

SNode.C 2.0 intentionally starts a new C++ API/ABI epoch. The project version is `2.0.0`, and the existing CMake release model gives all SNode.C shared libraries `SOVERSION 2`. Applications and plugins built against SNode.C 1.x must be rebuilt. Do not load or link SNode.C 1.x and 2.0 C++ libraries into the same process.

Default resource limits and transport behavior remain compatible after rebuilding; no configuration changes are required merely to retain the previous defaults.

## Affected areas

- `SocketConnection` has new result-returning queue APIs and a changed virtual interface.
- `SocketWriter` carries immutable per-connection queue limits and watermarks; its installed class layout changed.
- `FileReader` adds path, `openat()`-style, and descriptor-adoption entry points. Failed opens can now return `nullptr`.
- HTTP server and client connections snapshot the new shared parser configuration. The affected parser and connection class layouts changed.
- WebSocket upgrades snapshot receiver frame, message, and fragment limits. The receiver and upgrade layouts changed.
- Connection configuration classes contain the new write-queue options, changing their installed layouts.
- Explicit stream `connect()` and `listen()` calls return independent flow handles;
  endpoint-wide `getFlowController()` and controller `restartFlow()` are removed.

Recompile all application objects, shared plugins, dynamically loaded HTTP/WebSocket extensions, and libraries that include or derive from these public C++ types.

## Per-call stream flow handles

Each explicit call creates a `std::shared_ptr<ClientFlowController>` or
`std::shared_ptr<ServerFlowController>`. Existing retry/reconnect callbacks keep
that controller alive and reuse it for automatic attempts. A new explicit call
creates a new flow, even if an earlier flow terminated:

```cpp
auto first = client.connect(onStatus);
auto second = client.connect(onStatus);
first->terminateFlow(); // Only first's attempts/retry/reconnect stop.
client.connect(onStatus); // Ignoring the handle does not cancel the operation.

auto listener = server.listen(onListenStatus);
server.listen(onListenStatus); // Independent listener; configure port reuse as needed.
listener->terminateFlow(); // Stops this listener, not its sibling.
```

Store the returned handle instead of calling `endpoint.getFlowController()`.
Chaining endpoint methods after `connect()`/`listen()` must be split into separate
statements. Register flow callbacks on the returned handle before entering the
event loop. `setOnFlowTerminated()` reports this flow's termination;
`setOnFlowCompleted()` now reports final controller release, not destruction of
the shared endpoint configuration. Retaining a handle therefore delays completion
notification; releasing a handle does not terminate active runtime work. Avoid
capturing a strong reference to the same controller in its own observer callback.

The existing event loop, connector/acceptor ownership, timer receivers and
per-connection `SocketContext` factories are unchanged. Runtime callbacks retain
the flow; there is no endpoint registry. Termination cancels pending attempts and
recovery, not already established connections: close those through the existing
connection API. Accepted server connections do not retain the listening flow.

Configuration and endpoint callbacks remain shared, as do connection-ID counters.
This change isolates flow control, not configuration: address overloads still
update the endpoint configuration and do not capture independent configuration
snapshots. Use separate endpoints for independently configured destinations.
Controllers retain the existing event-loop-thread usage contract.

Use `client.setOnDestroy(callback)` or `server.setOnDestroy(callback)` when work
must wait for the shared instance name to be unregistered. This facade forwards
to the existing configuration destruction callback; it does not fire when a
wrapper copy or an individual flow is destroyed. The callback takes no arguments
and may outlive the wrapper, so capture required identifiers by value, not `this`.
Multiple registrations are invoked in registration order. Do not retain the
endpoint or its configuration inside its own destruction callback.

## `FileReader::open()` failure migration

Previously, callers commonly relied only on the callback-based open result and then used the returned pointer:

```cpp
auto* reader = core::file::FileReader::open(path, [&](int fd) {
    if (fd < 0) {
        callback(errno);
    }
});

reader->pipe(sink);
```

An opening failure may now return `nullptr`. Treat that immediate return as the condition that controls whether the source can be used:

```cpp
auto* reader = core::file::FileReader::open(path);
if (reader == nullptr) {
    callback(errno);
    return;
}

reader->pipe(sink);
```

The legacy `open(path, callback)` overload remains available and still reports the open result through its callback, but its return value must also be checked before dereferencing or piping. Errors that occur later while reading or streaming continue through the existing `Source`/`Sink` asynchronous lifecycle, including the sink's source-error callback.

The same immediate check applies to `open(directoryFd, path, flags)` and `adopt(fd)`. `openat()` semantics do not provide directory confinement.

## Configuration migration

The new options live in the existing SNode.C CLI11/subcommand and configuration-file hierarchy. Applications should configure them through `ConfigConnection`, `ConfigHttpParser`, `ConfigHttpServer`, `ConfigHTTP`/`ConfigHttpClient`, and `ConfigWebSocket`; there is no runtime setter or parallel configuration mechanism. Runtime connections consume immutable snapshots.

See [Framework resource policies and descriptor streaming](resource-policy-and-streaming.md) for option names, configuration-file keys, defaults, and validation rules.
