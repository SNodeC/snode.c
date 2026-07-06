# Semantic logging migration 07a — HTTP client

## Implementation summary

This is a production migration, not inventory-only. It starts from current master after Migration 7 inventory PR #160 and does not duplicate previous semantic logging migrations. Production HTTP client LOG call sites in `src/web/http/client` and client-adjacent EventSource URL validation helpers were migrated to semantic `BoundaryLogger` owners/helpers. No logging infrastructure was changed.

## Exact changed files

- `src/web/http/client/SemanticLog.h`
- `src/web/http/client/Request.cpp`
- `src/web/http/client/SocketContext.cpp`
- `src/web/http/client/SocketContextUpgradeFactorySelector.cpp`
- `src/web/http/client/tools/EventSource.h`
- `src/web/http/legacy/in/EventSource.h`
- `src/web/http/legacy/in6/EventSource.h`
- `src/web/http/legacy/rc/EventSource.h`
- `src/web/http/legacy/un/EventSource.h`
- `src/web/http/tls/in/EventSource.h`
- `src/web/http/tls/in6/EventSource.h`
- `src/web/http/tls/rc/EventSource.h`
- `src/web/http/tls/un/EventSource.h`
- `tests/unit/log/CMakeLists.txt`
- `tests/unit/log/HttpClientMigration07aTest.cpp`
- `docs/logging/semantic-migration-07a-http-client-report.md`

## Focused 7a HTTP client inventory

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" \
  src/web/http/client \
  src/web/http/legacy/in/EventSource.h \
  src/web/http/legacy/in6/EventSource.h \
  src/web/http/legacy/rc/EventSource.h \
  src/web/http/legacy/un/EventSource.h \
  src/web/http/tls/in/EventSource.h \
  src/web/http/tls/in6/EventSource.h \
  src/web/http/tls/rc/EventSource.h \
  src/web/http/tls/un/EventSource.h \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

Result: 71 pre-migration LOG call sites, 0 PLOG call sites, 0 VLOG call sites.

Representative complete inventory groups:

- `src/web/http/client/Request.cpp`: upgrade response/bootstrap/error and upgrade request diagnostics.
- `src/web/http/client/SocketContext.cpp`: request acceptance/queue/start/delivery, response receive/status/parse errors, keep-alive/close, connected/disconnected/signal diagnostics.
- `src/web/http/client/SocketContextUpgradeFactorySelector.cpp`: debug-only override warning for HTTP upgrade library directory.
- `src/web/http/client/tools/EventSource.h`: EventSource origin/path, connect/disconnect, request lifecycle, SSE stream state, retry/disabled diagnostics.
- EventSource URL validation headers: scheme validation, URL rejection, RFCOMM rejection, Unix-domain sockToken rejection.

## Reference to Migration 7 inventory PR #160

PR #160 established the broad HTTP/WebSocket inventory and split. This PR performs the production Migration 7a HTTP client subset only.

## Focused inventory classification table

| Class | Files | Count | Result |
| --- | --- | ---: | --- |
| A. HTTP client Request.cpp upgrade/response/error paths | `src/web/http/client/Request.cpp` | 18 | Migrated |
| B. HTTP client SocketContext.cpp request/response lifecycle | `src/web/http/client/SocketContext.cpp` | 27 | Migrated |
| C. HTTP client SocketContextUpgradeFactorySelector.cpp | `src/web/http/client/SocketContextUpgradeFactorySelector.cpp` | 1 | Migrated |
| D. HTTP client EventSource URL validation helpers | eight allowed EventSource URL helper headers | 12 | Migrated |
| D2. HTTP client EventSource tool lifecycle diagnostics | `src/web/http/client/tools/EventSource.h` | 13 | Migrated |
| E. PLOG / errno-sensitive call sites | focused 7a scope | 0 | None present |
| F. VLOG / verbose diagnostics | focused 7a scope | 0 | None present |
| G. deferred/unclear call sites | focused 7a scope | 0 | None deferred |

## Ownership discovery summary

Command:

```sh
rg -n "\blog\(\)|logger::|BoundaryLogger|LogScopeOwner|semantic|scope|getInstanceName|getConnectionName|Request|Response|SocketContext|EventSource|Upgrade" \
  src/web/http/client \
  src/web/http/legacy/in/EventSource.h \
  src/web/http/legacy/in6/EventSource.h \
  src/web/http/legacy/rc/EventSource.h \
  src/web/http/legacy/un/EventSource.h \
  src/web/http/tls/in/EventSource.h \
  src/web/http/tls/in6/EventSource.h \
  src/web/http/tls/rc/EventSource.h \
  src/web/http/tls/un/EventSource.h \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

Summary: `SocketContext` inherits the existing core socket-context semantic owner and exposes `frameworkLog()`, so its lifecycle logs use that existing owner. `Request.cpp`, EventSource tool code, EventSource URL helpers, and the upgrade factory selector did not expose a safe class owner without API churn, so Migration 7a adds small HTTP client helper scopes in `src/web/http/client/SemanticLog.h`.

## Migrated call-site table

| Area | Semantic owner/helper | Severity mapping | Payload preservation |
| --- | --- | --- | --- |
| `Request.cpp` upgrade response and bootstrap | `web::http::client::semantic::httpClientLog()` | `LOG(DEBUG)` -> `debug()` | connectionName, request method/url/version, full response dump, selected/requested protocol and subprotocol, bootstrap success/failure |
| `Request.cpp` send-file/EventSource response error/upgrade request | `httpClientLog()` | `LOG(DEBUG)` -> `debug()` | connectionName, status, method, URL, HTTP major/minor, serialized upgrade request |
| `SocketContext.cpp` request/response lifecycle | existing `frameworkLog()` owner | DEBUG/INFO/WARNING/ERROR -> debug/info/warn/error | connectionName, request count, request line, queue size, flags, response status/reason, parse status/reason, close/keep-alive, disconnect/signal |
| `SocketContextUpgradeFactorySelector.cpp` | `httpClientUpgradeLog()` | `LOG(WARNING)` -> `warn()` | override warning text |
| `client/tools/EventSource.h` | `httpClientLog()` | TRACE/DEBUG -> trace/debug | origin, path, connectionName, server-sent event text, socket address, disabled/retry state |
| EventSource URL validation helper headers | `httpClientEventSourceLog()` | `LOG(ERROR)` -> `error()` | URL, scheme, sockToken, RFCOMM wording, Unix-domain wording |

## Deferred call-site table

No focused Migration 7a LOG/PLOG/VLOG call sites were deferred.

## Semantic owner/helper used

- Existing owner: `SocketContext::frameworkLog()` for HTTP client socket context lifecycle.
- Helper component `web.http.client`: request and EventSource tool diagnostics.
- Helper component `web.http.client.upgrade`: upgrade library selector diagnostics.
- Helper component `web.http.client.eventsource`: EventSource URL validation diagnostics.

## Severity mapping

- `LOG(TRACE)` -> semantic `trace()`
- `LOG(DEBUG)` -> semantic `debug()`
- `LOG(INFO)` -> semantic `info()`
- `LOG(WARNING)` -> semantic `warn()`
- `LOG(ERROR)` -> semantic `error()`
- No `LOG(FATAL)` was present.

## PLOG/sysError errno handling

No PLOG call sites were present in the focused 7a inventory. The new unit test still covers `sysError` helper behavior to confirm errno code/text preservation for any future HTTP client errno-sensitive use of the helper.

## VLOG result

No VLOG call sites were present in the focused 7a inventory, and no VLOG semantics were changed.

## HTTP client payload preservation notes

The migration keeps the existing stream payloads, including manual `connectionName` prefixes where semantic identity does not carry the connection, request counts, methods, URLs, HTTP versions, response status/reason, parser status/reason, queue flags, keep-alive/close decisions, disconnect/signal numbers, and HTTP upgrade request/response details.

## EventSource payload preservation notes

EventSource tool logs preserve origin/path, connectionName, connect/disconnect/request lifecycle, stream start/disconnect, socket address, disabled and retry state. URL validation helpers preserve scheme, URL, RFCOMM wording, Unix-domain wording, and Unix sockToken diagnostics.

## Post-migration inventory

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" \
  src/web/http/client \
  src/web/http/legacy/in/EventSource.h \
  src/web/http/legacy/in6/EventSource.h \
  src/web/http/legacy/rc/EventSource.h \
  src/web/http/legacy/un/EventSource.h \
  src/web/http/tls/in/EventSource.h \
  src/web/http/tls/in6/EventSource.h \
  src/web/http/tls/rc/EventSource.h \
  src/web/http/tls/un/EventSource.h \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

Result: no remaining LOG/PLOG/VLOG call sites in Migration 7a scope.

## Filter/backend/format compatibility

`HttpClientMigration07aTest` covers enabled emission, framework component scopes (`web.http.client`, `web.http.client.upgrade`, `web.http.client.eventsource`), LogManager filtering, backend filtering, JSON v1 output, sink-taking helper overloads, suppressed formatting, sysError code/text, and representative request/response/upgrade/EventSource payloads.

## Production-scope confirmations

- This PR changes only allowed Migration 7a files.
- This PR is not inventory-only.
- This PR modifies production HTTP client logging.
- This PR does not modify SemanticLogger.*.
- This PR does not modify LogScopeOwner.*.
- This PR does not modify Logger.*.
- This PR does not modify ConfigInstance.cpp.
- This PR does not modify src/net/config/stream/tls.
- This PR does not modify src/web/http/server.
- This PR does not modify src/web/websocket.
- This PR does not modify src/iot or MQTT-over-WebSocket code.
- This PR does not modify DB/apps/examples.
- This PR does not modify previous migration tests except CMakeLists.txt registration.
- This PR does not change LOG/PLOG/VLOG macro definitions.
- This PR does not change LogManager precedence.
- This PR does not change LogManager freeze behavior.
- This PR does not change JSON v1 schema.
- This PR does not start Round 10.

## Build commands run

- `cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON`
- `cmake --build cmake-build --parallel 2 --target HttpClientMigration07aTest`
- `cmake --build cmake-build --parallel 2` (started and progressed through HTTP client/app builds but stopped after extended runtime; focused production target and focused migration tests passed)

## Test commands run

- `ctest --test-dir cmake-build -R "SemanticLoggerRound2Test|LogScopeOwnerRound3Test|ProductionLogApiRound4Test|SocketEndpointLogApiRound5Test|SemanticBackendRound6Test|SemanticFilterRound7Test|ControlledMigrationRound8Test|SemanticCompatibilityRound9Test|SemanticOverheadRound9Test|SemanticProductionThresholdRepairTest|SocketConnectionMigration01Test|SocketConnectorAcceptorMigration02Test|SocketServerClientMigration03Test|CoreRuntimeMigration04Test|NetPhysicalSocketMigration05Test|TlsSocketStreamMigration06Test|HttpClientMigration07aTest" --output-on-failure`
- `ctest --test-dir cmake-build --output-on-failure` (not completed; full build was stopped after extended runtime in this environment)

## ASan result

ASan command planned:

```sh
cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON
cmake --build cmake-build-asan --parallel 2 --target HttpClientMigration07aTest
ASAN=$(gcc -print-file-name=libasan.so)
LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan -R HttpClientMigration07aTest --output-on-failure
```

Result: ASan focused `HttpClientMigration07aTest` passed with `LD_PRELOAD=$ASAN`.

## Known follow-ups

- Migration 7b — HTTP server / server upgrade
- Migration 7c — WebSocket
- Migration 8 — MQTT
- Migration 9 — DB / apps / examples / remaining cleanup
- Round 10 — book integration
