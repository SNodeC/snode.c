# Semantic Logging API Contract

## 1. Purpose

This document freezes the semantic logging API after the completed `LOG`/`PLOG`
migration and before the final legacy macro removal pass. It is the
authoritative contract for future logging work.

The final goal is:

```text
no LOG(...)
no PLOG(...)
no VLOG(...)
no compatibility macros
no semantic bridge for VLOG
no numeric verbose API
explicit semantic logging everywhere
```

## 2. Current status

`LOG`/`PLOG` migration is complete for suitable production/application/example
call sites outside `src/log`. `src/log` still contains legacy macro definitions
until the removal pass. `VLOG` call sites remain temporarily and are
intentionally not migrated mechanically. Historical migration reports may
mention `LOG`/`PLOG`/`VLOG` as pre-migration artifacts.

## 3. Approved logging API

The approved normal stream-style logging forms are:

```cpp
auto log = someSemanticLog();

log.trace() << "...";
log.debug() << "...";
log.info() << "...";
log.warn() << "...";
log.error() << "...";
log.critical() << "...";
```

The approved format-style logging forms are:

```cpp
log.trace("message {}", value);
log.debug("message {}", value);
log.info("message {}", value);
log.warn("message {}", value);
log.error("message {}", value);
log.critical("message {}", value);
```

Both stream-style and format-style logging are allowed. New code must prefer
local semantic owner/helper functions over global ad-hoc scope construction.

## 4. Severity policy

| Severity | Meaning |
| --- | --- |
| `trace` | Very detailed protocol/runtime diagnostics; route matching; frame/body dumps; internal state transitions. |
| `debug` | Normal developer diagnostics; lifecycle detail; configuration choices; non-error fallback detail. |
| `info` | Externally useful lifecycle events; listening, connected, started, stopped, selected mode. |
| `warn` | Recoverable anomalies or degraded behavior. |
| `error` | Failed operation that is handled or reported but not process-fatal. |
| `critical` | Fatal/unrecoverable condition, old `LOG(FATAL)` equivalent. |

Former `VLOG(n)` sites must be classified by meaning, not by numeric level.

## 5. Former VLOG policy

`VLOG` is legacy. `VLOG(n)` must not be preserved as a numeric semantic API.
There will be no semantic `verbose(n)` API. There will be no `VLOG`
compatibility macro. There will be no `VLOG`-to-`trace` mechanical rule. Each
remaining `VLOG` site must be classified semantically or deleted.

Conversion categories:

| Former verbose use | Required treatment |
| --- | --- |
| startup/listen/connected/disabled state | `info` or `debug` |
| request/response lifecycle | `debug` |
| route table / dispatcher match diagnostics | `trace` |
| protocol/frame/body/payload dump | guarded `trace` |
| TLS certificate inspection | guarded `debug` or `trace` |
| demo/example progress output | `info` if useful, otherwise delete |
| old noisy developer chatter | delete |
| error-like verbose line | `warn` or `error` |

Deleting obsolete chatter is acceptable and sometimes preferred.

## 6. Errno / system error policy

The only approved errno format-style pattern is:

```cpp
const int errnum = errno;
log.sysError(logger::LogLevel::Error, errnum, "operation {} failed", name);
```

When stream-style is needed, the only approved pattern is:

```cpp
const int errnum = errno;
snode::semantic::sysError(log, logger::LogLevel::Error, errnum)
    << "operation " << name << " failed";
```

`errno` must be captured immediately after the failing syscall/library call. No
helper construction, string conversion, semantic logging, formatting,
allocation, callback, or branching may occur before capture. If later logic
branches on the error, branch on the captured `errnum`, not `errno`. If a
callback/API already supplies an error code, pass that explicit code directly
and do not rewrite `errno`. No default-errno `sysError` helper is allowed.

## 7. Expensive diagnostics policy

Expensive diagnostics require explicit `enabled(...)` guards.

Format-style example:

```cpp
auto log = someSemanticLog();

if (log.enabled(logger::LogLevel::Trace)) {
    log.trace("Payload:\n{}", expensiveDump(payload));
}
```

Stream-style example:

```cpp
auto log = someSemanticLog();

if (log.enabled(logger::LogLevel::Trace)) {
    log.trace() << "Payload:\n" << expensiveDump(payload);
}
```

This applies to:

- hex dumps
- JSON/body dumps
- SQL dumps
- certificate dumps
- route table dumps
- large payload summaries
- filesystem tree/listing dumps
- formatted strings requiring non-trivial construction

## 8. Component naming policy

Component names are frozen by these rules:

- components are lowercase
- components are dot-separated
- components are stable string literals
- components are not generated from user data
- components are not generated from instance names, hostnames, topics, routes,
  paths, client IDs, database names, or file names
- runtime details belong in message text or scope fields, not component names

Current approved component registry:

- `framework`
- `core.system`
- `core.socket`
- `net.config`
- `net.config.tls`
- `db.mariadb`
- `express`
- `web.http`
- `web.http.client`
- `web.http.client.upgrade`
- `web.http.client.eventsource`
- `web.http.server`
- `web.http.server.upgrade`
- `web.websocket`
- `web.websocket.frame`
- `web.websocket.subprotocol`
- `web.websocket.factory`
- `iot.mqtt`
- `iot.mqtt.client`
- `iot.mqtt.server`
- `iot.mqtt.broker`
- `iot.mqtt.session`
- `iot.mqtt.packet`
- `iot.mqtt.topic`
- `iot.mqtt.websocket`
- `app`

## 9. Helper placement policy

Subsystem-local semantic helpers are preferred. Broad top-level helpers may
remain temporarily only as migration scaffolding or final-cleanup support. New
subsystem work should not add unrelated responsibilities to a broad helper.

Preferred helper locations:

- `src/core/SemanticLog.h`
- `src/net/config/SemanticLog.h`
- `src/net/config/stream/tls/SemanticLog.h`
- `src/database/mariadb/SemanticLog.h`
- `src/express/SemanticLog.h`
- `src/web/http/client/SemanticLog.h`
- `src/web/http/server/SemanticLog.h`
- `src/web/websocket/SemanticLog.h`
- `src/iot/mqtt/SemanticLog.h`
- `src/apps/SemanticLog.h`

The existing `src/SemanticLog.h` is accepted as a final-cleanup helper for the
current state but should not become a dumping ground for unrelated future
components.

## 10. Final zero-macro target

The future final hard target is:

```text
No source/test C++ file may contain LOG(...), PLOG(...), or VLOG(...).
src/log must no longer define LOG, PLOG, or VLOG.
No replacement compatibility macros may be introduced.
Historical docs may keep mentions as historical record only.
```

Future check commands:

```sh
rg -n '\b(LOG|PLOG|VLOG)\s*\(' \
  src tests \
  -g '*.h' -g '*.hpp' -g '*.cpp'

rg -n '#define\s+(LOG|PLOG|VLOG)\b' src/log
```

Expected final result:

```text
no matches
```

## 11. Non-goals of this phase

This phase does not migrate `VLOG`. This phase does not remove macros. This
phase does not change runtime logging behavior. This phase does not change
`LogManager` filtering. This phase does not change JSON schema. This phase does
not add semantic verbose support. This phase does not start Round 10 book
integration.
