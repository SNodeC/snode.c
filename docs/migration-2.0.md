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

Recompile all application objects, shared plugins, dynamically loaded HTTP/WebSocket extensions, and libraries that include or derive from these public C++ types.

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
