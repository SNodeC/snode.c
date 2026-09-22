# Public logging API

`<Log.h>` is the single application-facing logging header. New code belongs in
the `snode::log` namespace and does not need the internal `logger` model or the
backend implementation.

```cpp
#include <Log.h>

auto log = snode::log::application("gateway.mqtt");
log.info("Connected to {}", broker);
log.systemError(snode::log::Level::Error, errno, "Publish failed");
```

Use `application()` for process or component diagnostics, `framework()` for
framework-owned diagnostics, `forConnection()` when a live connection is
available, and `makeLogger()` for an explicitly constructed `Scope`. `Scope`
and `Identity` own their strings, so loggers cannot retain dangling views.

The six severity methods support both stream and `{}`-formatted forms. Escaped
braces are written as `{{` and `}}`; malformed formats and argument-count
mismatches throw `std::invalid_argument`. Formatting is skipped when the level
is disabled. `event()` adds a stable event name, `systemError()` adds a typed
error, and `emit()` accepts separate plain and terminal presentations.

Configure logging once during startup with `configure(Settings)`. Settings
cover the global threshold, text or JSON output, color policy, quiet mode,
rotating-file output, and component or instance overrides. Configuration is
also overridable by origin and boundary and is frozen after it is applied.

`SemanticLog.h` and the lower-level headers under `log/` remain compatibility
surfaces for existing consumers. They are implementation and migration aids;
new application code should include only `<Log.h>`.

## Binary diagnostics

Use the same scoped logger for binary data:

```cpp
log.hexDump(snode::log::Level::Trace, "MQTT payload", message);
log.hexDump(snode::log::Level::Trace, "Received frame",
            std::as_bytes(std::span(buffer, length)));
```

The overloads accept `std::string_view` or `std::span<const std::byte>`. They
borrow bytes only for the synchronous call and preserve embedded NULs. Disabled
levels return before formatting. Expensive argument construction, such as packet
serialization, still needs an `enabled()` guard at the caller.

One log record contains the label, total byte count and 16-byte rows, grouped
into two blocks of eight, with offsets and a printable-ASCII column. Empty input
produces a `0 bytes` heading without a data row. There is no implicit truncation
or terminal-width detection. Files and JSON contain only the plain form; terminal
colors follow the existing output policy, with one color span per row section.
Large enabled dumps still incur synchronous formatting and output costs.

Framework protocol diagnostics use this same operation on their existing internal
scoped loggers. The legacy `utils::hexDump*` and MQTT `toHexString()` formatting
functions remain source-compatible and share the single renderer; they do not
define a separate logging layout. Its implementation is compiled once into the
logging library, which the utility library already depends on.
