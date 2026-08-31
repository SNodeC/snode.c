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
