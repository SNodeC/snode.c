<!-- snodec:begin page-header -->
<a id="page-overview"></a>
<p><picture><source media="(max-width: 640px)" srcset="../media/nav/banner-logging-compact.svg"><img src="../media/nav/banner-logging.svg" alt="SNode.C — Logging" width="100%"></picture></p>
<p><a href="README.md#page-overview" title="Up to Logging"><picture><source media="(prefers-color-scheme: dark)" srcset="../media/nav/up-logging-dark.svg"><img src="../media/nav/up-logging-light.svg" alt="Up to Logging" title="Up to Logging" width="160" height="28"></picture></a><a href="https://github.com/SNodeC" title="SNodeC organization"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../media/nav/organization-icon-dark.svg"><source media="(max-width: 640px)" srcset="../media/nav/organization-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../media/nav/organization-dark.svg"><img align="right" src="../media/nav/organization-light.svg" alt="SNodeC organization" title="SNodeC organization"></picture></a><a href="../../README.md#project-overview" title="SNode.C repository"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../media/nav/repository-icon-dark.svg"><source media="(max-width: 640px)" srcset="../media/nav/repository-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../media/nav/repository-dark.svg"><img align="right" src="../media/nav/repository-light.svg" alt="SNode.C repository" title="SNode.C repository"></picture></a><br clear="all"></p>
<!-- snodec:end page-header -->

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
