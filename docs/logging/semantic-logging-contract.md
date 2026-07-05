# Semantic Logging Contract

Status: frozen roadmap contract for Logging Roadmap Round 1. This document defines the contract later logging rounds must follow. Round 1 does not implement the semantic logger and does not change `LOG`, `PLOG`, or `VLOG` behavior.

## 1. Core goal

The new logging system moves from manually prefixed text:

```cpp
LOG(DEBUG) << getConnectionName() << ": received " << n << " bytes";
PLOG(ERROR) << "bind failed";
```

toward object-bound semantic logging:

```cpp
log().debug("received {} bytes", n);
log().debug() << "received " << n << " bytes";
log().sysError(logger::LogLevel::Error, errno, "bind failed on {}", endpoint);
frameworkLog().trace("dispatching read event to application context");
```

Architectural identity must become structured metadata, not hand-written message prefixes.

## 2. Semantic fields

Intended semantic record fields:

```text
ts           event timestamp; canonical JSON name is `ts`, text format may render it as `timestamp`
level        trace | debug | info | warn | error | critical
origin       framework | application
boundary     application | configuration | instance | connection | context | system
component    dotted component name, e.g. core.socket.stream, net.in.stream, http.server, websocket, mqtt, db.mariadb
instance     configured instance name, when available
role         server | client | unknown
connection   connection identity, when available
event        optional stable event name
message      human text
error        optional { code, text }
source       optional { file, line, func }, only if explicitly enabled
```

Protocols and modules are components, not boundaries.

## 3. Enums

Intended enums:

```cpp
enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Critical,
    Off
};

enum class LogOrigin {
    Framework,
    Application
};

enum class LogBoundary {
    Application,
    Configuration,
    Instance,
    Connection,
    Context,
    System
};

enum class LogRole {
    Unknown,
    Server,
    Client
};
```

## 4. Object-bound logging model

```text
boundary-owning classes:
  use LogScopeOwner via composition

helper classes:
  do not own their own log scope
  do not receive/store BoundaryLogger
  log through the owning parent, e.g. parent.log()

SocketContext:
  has two LogScopeOwner members:
    applicationLogScope -> log()
    frameworkLogScope   -> frameworkLog()
```

No logging superclass inheritance. No universal `LogSuperClass`.

## 5. Intended user-facing API

Intended API shape:

```cpp
class BoundaryLogger {
public:
    bool enabled(LogLevel level) const noexcept;

    LogStream trace() const;
    LogStream debug() const;
    LogStream info() const;
    LogStream warn() const;
    LogStream error() const;
    LogStream critical() const;

    template <class... Args>
    void trace(fmt::format_string<Args...>, Args&&...) const;

    template <class... Args>
    void debug(fmt::format_string<Args...>, Args&&...) const;

    template <class... Args>
    void info(fmt::format_string<Args...>, Args&&...) const;

    template <class... Args>
    void warn(fmt::format_string<Args...>, Args&&...) const;

    template <class... Args>
    void error(fmt::format_string<Args...>, Args&&...) const;

    template <class... Args>
    void critical(fmt::format_string<Args...>, Args&&...) const;

    template <class... Args>
    void sysError(LogLevel, std::error_code, fmt::format_string<Args...>, Args&&...) const;

    template <class... Args>
    void sysError(LogLevel, int errnum, fmt::format_string<Args...>, Args&&...) const;
};
```

## 6. Startup-only configuration

Intended immutable startup sequence:

```cpp
logger::LogManager::init();

logger::LogManager::setGlobalLevel(logger::LogLevel::Info);
logger::LogManager::setOriginLevel(logger::LogOrigin::Framework,
                                   logger::LogLevel::Warn);
logger::LogManager::setBoundaryLevel(logger::LogBoundary::Connection,
                                     logger::LogLevel::Debug);
logger::LogManager::setComponentLevel("mqtt", logger::LogLevel::Trace);
logger::LogManager::setInstanceLevel("mqtt-broker", logger::LogLevel::Trace);
logger::LogManager::setFormat(logger::LogManager::Format::Text);

logger::LogManager::freeze();
```

No runtime reload.
No signal-triggered log-level mutation.
No admin endpoint for changing logging.
No atomics for mutable log thresholds.
Changing logging policy requires process restart.

## 7. Filtering precedence

```text
instance > component > boundary > origin > global
```

Objects cache their effective threshold after startup configuration is frozen.

## 8. SocketContext origin split

Mandatory API:

```cpp
class SocketContext {
public:
    logger::BoundaryLogger log() const;          // application origin
    logger::BoundaryLogger frameworkLog() const; // framework origin
};
```

Application or derived context code uses:

```cpp
log().info("echo session started");
```

Framework base-class behavior uses:

```cpp
frameworkLog().trace("dispatching read event to application context");
```

## 9. string_view lifetime rule

LogScope string_view fields are non-owning borrows from the owning object.
They may be used only synchronously during the log call.
Before anything is handed to a sink, formatter, async worker, or queue, all borrowed fields must be materialized into owned bytes.
No sink or async worker may dereference LogScope string_views after the log call returns.

## 10. Text and JSON formats

Both text and JSON must be generated from the same semantic record.

JSON schema v1 must include:

```json
{
  "v": 1,
  "ts": "2026-07-05T12:34:56.789Z",
  "level": "info",
  "origin": "application",
  "boundary": "context",
  "component": "EchoServerContext",
  "instance": "echo-server",
  "connection": "#7",
  "role": "server",
  "message": "echo session started"
}
```

Mandatory JSON fields:

```text
v
ts
level
origin
boundary
component
role
message
```

Optional JSON fields:

```text
instance
connection
event
error
source
```

Optional fields are omitted when absent, not emitted as null.

## 11. Legacy macro compatibility

LOG, PLOG, and VLOG remain supported during migration.
LOG remains compatibility.
PLOG maps conceptually to sysError where possible.
VLOG remains compatibility only and is not a third semantic axis.

Later rounds must preserve current macro behavior unless a deliberate compatibility change is explicitly reviewed.

### Baseline gate for later rounds

The current implementation emits through `spdlog` with a date/time prefix, a steady-clock elapsed-tick field, optional ANSI color, and `errno` text for `PLOG`. That means wall-clock and elapsed fields are nondeterministic, and raw byte-for-byte comparison across separate processes is not a stable compatibility gate.

Later rounds must use one of these equivalent strategies before changing macro internals:

1. Same-process golden capture: initialize the legacy logger and candidate compatibility path in one process, set a deterministic tick resolver, disable color, set identical log and verbose levels, redirect output to comparable sinks, and compare normalized records for `LOG`, `PLOG`, and `VLOG` cases.
2. Normalized process capture: run a small harness twice, normalize the leading timestamp and elapsed tick fields, strip ANSI escape sequences if present, and compare the remaining level label, message text, `PLOG` strerror suffix, and verbose behavior.

The gate must cover enabled and disabled levels, `PLOG` errno capture at macro construction time, and `VLOG` threshold behavior. Any change in the normalized compatibility output must be called out explicitly in the PR.

## 12. PR workflow agreement

One draft pull request per roadmap round.
Do not create one PR per tiny internal fix.
Do not continue into the next round before this PR is reviewed and accepted.
Do not stack several dependent PRs unless explicitly approved.
Each PR must contain:

```text
implementation diff
short round report
exact build commands run
test/sanitizer results
macro compatibility result
deliberate deferrals
known non-blocking follow-ups
```

For this round, the branch and draft PR are:

```text
logging/round-01-baseline-contract
```
