# SocketContext Detach Logging Report

## Problem summary

`SocketContext::detach()` served two lifecycle meanings: protocol/context switches where the underlying connection remains open, and final connection cleanup where the connection is closing. Before this change, both cases emitted the same context statistics block. On true close, that block overlapped with the richer `SocketServer` and `SocketClient` disconnect statistics for the same lifecycle event.

## Canonical recipe rule applied

The semantic logging recipe says not to log the same event twice unless the second log adds new information, such as a new decision, consequence, domain interpretation, recovery action, failure boundary, or identity. Adjacent framework layers alone do not justify duplicate close-stat logging.

## Files inspected

- `src/core/socket/stream/SocketConnection.hpp`
- `src/core/socket/stream/SocketContext.h`
- `src/core/socket/stream/SocketContext.cpp`
- `src/core/socket/stream/SocketServer.h`
- `src/core/socket/stream/SocketClient.h`
- `tests/unit/log/SocketServerClientMigration03Test.cpp`
- `tests/unit/log/CoreRuntimeMigration04Test.cpp`
- `tests/unit/log/CMakeLists.txt`
- `src/web/http/client/Request.cpp`
- `src/web/http/server/Response.cpp`
- `src/web/websocket/SocketContextUpgrade.hpp`
- `src/web/websocket/SubProtocol.h`
- `src/web/websocket/SubProtocol.hpp`

## Two detach paths found

### Context-switch path summary

`SocketConnectionT::onReceivedFromPeer(...)` performs a pending `SocketContext` switch when `newSocketContext` is set. The old context is detached, the connection adopts the new context, and the new context is attached. This is a protocol/context switch path, such as an HTTP-to-WebSocket upgrade, and the connection remains open.

### Connection-close path summary

`SocketConnectionT::unobservedEvent()` performs final cleanup when the connection is no longer observed. It detaches the current context, invokes `onDisconnect()`, logs the connection as disconnected, and deletes the connection object. This is the true close path.

## Context-relative counter explanation

`SocketContext::attach()` captures the connection's current queued and processed totals as baselines. `SocketContext::getTotalQueued()` and `SocketContext::getTotalProcessed()` subtract those baselines from the connection totals. Therefore, these values describe the current context tenure, not necessarily the whole connection lifetime.

## Implementation summary

- Added `SocketContext::DetachReason` with `ContextSwitch` and `ConnectionClose` values.
- Changed `SocketContext::detach()` to accept the explicit detach reason.
- Marked the pending context switch call as `ContextSwitch`.
- Marked the final cleanup call as `ConnectionClose`.
- Kept per-context statistics only in the context-switch branch.
- Reduced true-close `SocketContext` logging to a detach reason line so `SocketServer` and `SocketClient` remain the close-stat owners.

## Why context-switch stats are preserved

During a context switch, the old context's tenure is ending while the connection remains open. `SocketServer` and `SocketClient` disconnect callbacks do not run at that boundary, so `SocketContext` is the natural owner for context-relative accounting such as context online duration, context queued bytes, and context processed bytes.

## Why true-close SocketContext stats are suppressed/reduced

On final connection close, the `SocketServer` and `SocketClient` disconnect callbacks log the richer connection-lifetime statistics block. Repeating `Online Since`, `Online Duration`, `Total Queued`, `Total Sent`, and `Total Processed` from `SocketContext` at the same close boundary would violate the duplicate-log recipe unless it added distinct close information. The true-close `SocketContext` branch now logs only that the context detached for connection close.

## Why SocketServer/SocketClient remain authoritative for connection-lifetime close stats

`SocketServer.h` and `SocketClient.h` still contain the connection-lifetime close-stat labels: `Online Since`, `Online Duration`, `Total Queued`, `Total Sent`, `Write Delta`, `Total Read`, `Total Processed`, and `Read Delta`. They have access to the connection-lifetime totals and remain the authoritative close-stat logging boundary.

## Mislabeled Total Sent/getTotalQueued note

The old `SocketContext::detach()` log labeled `getTotalQueued()` as `Total Sent`. That label was misleading because queued bytes are not sent bytes. The replacement context-switch diagnostics now label the value as `Context Total Queued`, and true-close `SocketContext` logging no longer emits that statistic.

## Tests added/updated

- Added `tests/unit/log/SocketContextDetachPolicyTest.cpp` to verify the source-level logging policy and ownership boundary.
- Registered `SocketContextDetachPolicyTest` in `tests/unit/log/CMakeLists.txt`.

## Search commands run

Before changes:

```sh
rg -n "detach\\(|setSocketContext\\(|SocketContextSwitch|Online Since|Online Duration|Total Sent|Total Processed|Total Queued|Total Read|Write Delta|Read Delta|OnDisconnect|SocketContext: detached" \
  src/core/socket/stream src/web/http src/web/websocket \
  -g '*.h' -g '*.hpp' -g '*.cpp'

rg -n "SocketContext: detached|Online Duration|Total Sent|Total Processed|getTotalQueued\\(\\)" \
  tests src \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

After changes:

```sh
rg -n "detach\\(|setSocketContext\\(|SocketContextSwitch|Online Since|Online Duration|Total Sent|Total Processed|Total Queued|Total Read|Write Delta|Read Delta|OnDisconnect|SocketContext: detached" \
  src/core/socket/stream src/web/http src/web/websocket \
  -g '*.h' -g '*.hpp' -g '*.cpp'

rg -n "Total Sent:.*getTotalQueued|getTotalQueued\\(\\).*Total Sent" \
  src/core/socket/stream tests \
  -g '*.h' -g '*.hpp' -g '*.cpp'

rg -n "getPassword\\(|getWillMessage\\(|RefreshToken:|AccessToken:|Invalid access token '|Valid access token '|Password:|WillMessage:|Recieving auth code|Receiving auth code|req\\.query\\(\"code\"\\)|authorization code.*<<" \
  src \
  -g '*.h' -g '*.hpp' -g '*.cpp'

rg -n "mqttLog\\(\\)\\.info\\(\\)|frameworkLog\\(\\)\\.info\\(\\).*HTTP: Request|frameworkLog\\(\\)\\.info\\(\\).*HTTP: Response|frameworkLog\\(\\)\\.warn\\(\\) << httputils::toString" \
  src/iot/mqtt src/web/http \
  -g '*.h' -g '*.hpp' -g '*.cpp'

rg -n "\\b(LOG|PLOG|VLOG)\\s*\\(" \
  src \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

## Tests run

See the PR validation notes for the exact commands run on this branch.

## Known follow-ups

1. PR D — inner epoll robustness and syscall diagnostics.
2. PR E — broader syscall discipline audit.
3. Final macro removal/source gate.
4. Round 10 — book integration.
