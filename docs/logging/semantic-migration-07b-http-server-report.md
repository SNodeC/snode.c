# Semantic logging migration 07b — HTTP server and upgrade

## Implementation summary

This is a production follow-up inside PR #161, not inventory-only. PR #161 now contains Migration 7a — HTTP client and Migration 7b — HTTP server and upgrade. This continues from the existing PR #161 branch/worktree with the 7a files already present.

Migration 7b migrates HTTP server-side and HTTP server-upgrade production logging under `src/web/http/server` to semantic `BoundaryLogger` owners/helpers without changing logging infrastructure.

## Exact changed files for the 7b follow-up

- `docs/logging/semantic-migration-07b-http-server-report.md`
- `src/web/http/server/SemanticLog.h`
- `src/web/http/server/Response.cpp`
- `src/web/http/server/SocketContext.cpp`
- `src/web/http/server/SocketContextUpgradeFactorySelector.cpp`
- `tests/unit/log/CMakeLists.txt`
- `tests/unit/log/HttpServerMigration07bTest.cpp`

## Focused 7b HTTP server inventory

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" \
  src/web/http/server \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

Result: 30 pre-migration LOG call sites, 0 PLOG call sites, 0 VLOG call sites.

Complete focused inventory:

```text
src/web/http/server/Response.cpp:255:                LOG(DEBUG) << connectionName << " HTTP: Initiating upgrade: " << request->method << " " << request->url
src/web/http/server/Response.cpp:272:                        LOG(DEBUG) << connectionName
src/web/http/server/Response.cpp:279:                            LOG(DEBUG) << connectionName
src/web/http/server/Response.cpp:282:                            LOG(DEBUG) << connectionName << " HTTP upgrade: Response to upgrade request: " << request->method << " "
src/web/http/server/Response.cpp:293:                            LOG(DEBUG) << connectionName
src/web/http/server/Response.cpp:299:                        LOG(DEBUG) << connectionName
src/web/http/server/Response.cpp:305:                    LOG(DEBUG) << connectionName << " HTTP upgrade: No upgrade requested";
src/web/http/server/Response.cpp:310:                LOG(ERROR) << connectionName << " HTTP upgrade: Request has gone away";
src/web/http/server/Response.cpp:315:            LOG(DEBUG) << connectionName << " HTTP: Upgrade bootstrap " << (!socketContextUpgradeName.empty() ? "success" : "failed");
src/web/http/server/Response.cpp:316:            LOG(DEBUG) << "      Protocol selected: " << socketContextUpgradeName;
src/web/http/server/Response.cpp:317:            LOG(DEBUG) << "              requested: " << request->get("upgrade");
src/web/http/server/Response.cpp:318:            LOG(DEBUG) << "  Subprotocol  selected: " << header("Sec-WebSocket-Protocol");
src/web/http/server/Response.cpp:319:            LOG(DEBUG) << "              requested: " << request->get("Sec-WebSocket-Protocol");
src/web/http/server/Response.cpp:323:            LOG(ERROR) << "HTTP upgrade: Unexpected disconnect";
src/web/http/server/SocketContextUpgradeFactorySelector.cpp:69:            LOG(WARNING) << "HTTP upgrade: Overriding http upgrade library dir";
src/web/http/server/SocketContext.cpp:70:                  LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Request start";
src/web/http/server/SocketContext.cpp:73:                  LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Request parse success: " << request.method << " "
src/web/http/server/SocketContext.cpp:83:                  LOG(ERROR) << getSocketConnection()->getConnectionName() << " HTTP: Request parse error: " << reason << " (" << status
src/web/http/server/SocketContext.cpp:112:                LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Request deliver: " << pendingRequest->method << " "
src/web/http/server/SocketContext.cpp:137:            LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: No more pending request";
src/web/http/server/SocketContext.cpp:151:            LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Response start for request: " << pendingRequest->method
src/web/http/server/SocketContext.cpp:153:            LOG(INFO) << getSocketConnection()->getConnectionName() << "   "
src/web/http/server/SocketContext.cpp:168:            LOG(WARNING) << getSocketConnection()->getConnectionName() << " HTTP: Response completed with error: " << response.statusCode
src/web/http/server/SocketContext.cpp:179:        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Response completed for request: " << request->method << " "
src/web/http/server/SocketContext.cpp:181:        LOG(INFO) << getSocketConnection()->getConnectionName() << "   "
src/web/http/server/SocketContext.cpp:195:            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Connection = Close";
src/web/http/server/SocketContext.cpp:199:            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Connection = Keep-Alive";
src/web/http/server/SocketContext.cpp:216:        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Connected";
src/web/http/server/SocketContext.cpp:236:        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Received disconnect";
src/web/http/server/SocketContext.cpp:244:        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Received signal " << signum;
```

## References

- Migration 7 inventory PR #160 established HTTP/WebSocket split planning.
- Migration 7a work is already present in PR #161 and covers HTTP client and client EventSource logging.

## Focused inventory classification table

| Class | Files | Count | Result |
| --- | --- | ---: | --- |
| A. HTTP server SocketContext.cpp request/response lifecycle | `src/web/http/server/SocketContext.cpp` | 15 | Migrated |
| B. HTTP server Response.cpp upgrade response/bootstrap/error paths | `src/web/http/server/Response.cpp` | 14 | Migrated |
| C. HTTP server SocketContextUpgradeFactorySelector.cpp | `src/web/http/server/SocketContextUpgradeFactorySelector.cpp` | 1 | Migrated |
| D. PLOG / errno-sensitive call sites | focused 7b scope | 0 | None present |
| E. VLOG / verbose diagnostics | focused 7b scope | 0 | None present |
| F. deferred/unclear call sites | focused 7b scope | 0 | None deferred |

## Ownership discovery summary

Command:

```sh
rg -n "\blog\(\)|logger::|BoundaryLogger|LogScopeOwner|semantic|scope|getInstanceName|getConnectionName|Request|Response|SocketContext|Upgrade" \
  src/web/http/server \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

Summary: server `SocketContext` inherits the existing core socket context semantic owner and can safely use `frameworkLog()`. `Response.cpp` and `SocketContextUpgradeFactorySelector.cpp` do not expose a safe response/upgrade semantic object owner without API churn, so 7b adds small HTTP server helper scopes in `src/web/http/server/SemanticLog.h`.

## Migrated call-site table

| Area | Semantic owner/helper | Severity mapping | Payload preservation |
| --- | --- | --- | --- |
| `SocketContext.cpp` request parse/delivery/response lifecycle | existing `frameworkLog()` owner | INFO/WARNING/ERROR/DEBUG -> info/warn/error/debug | connectionName, request start, method, URL, HTTP major/minor, parse error reason/status, response status/reason, close/keep-alive, disconnect/signal |
| `Response.cpp` server upgrade path | `web::http::server::semantic::httpServerLog()` | DEBUG/ERROR -> debug/error | connectionName, method, URL, selected/requested upgrade protocol, selected/requested subprotocol, bootstrap success/failure, request gone away, unexpected disconnect |
| `SocketContextUpgradeFactorySelector.cpp` upgrade selector | `httpServerUpgradeLog()` | WARNING -> warn | override warning text |

## Deferred call-site table

No focused Migration 7b LOG/PLOG/VLOG call sites were deferred.

## Semantic owner/helper used

- Existing owner: `SocketContext::frameworkLog()` for HTTP server socket-context lifecycle.
- Helper component `web.http.server`: server response/upgrade flow diagnostics where no safe class owner exists.
- Helper component `web.http.server.upgrade`: server upgrade factory selector diagnostics.

## Severity mapping

- `LOG(DEBUG)` -> semantic `debug()`
- `LOG(INFO)` -> semantic `info()`
- `LOG(WARNING)` -> semantic `warn()`
- `LOG(ERROR)` -> semantic `error()`
- No `LOG(TRACE)` or `LOG(FATAL)` was present.

## PLOG/sysError errno handling

No PLOG call sites were present in the focused 7b inventory. The new unit test still covers `sysError` helper behavior to confirm errno code/text preservation for future HTTP server errno-sensitive use.

## VLOG result

No VLOG call sites were present in the focused 7b inventory, and no VLOG semantics were changed.

## HTTP server payload preservation notes

The migration keeps existing technical payloads: connectionName, request method, URL, request line, HTTP major/minor, response status/reason, parse error reason/status, close/keep-alive decisions, disconnect, and signal number.

## HTTP server-upgrade payload preservation notes

The migration preserves server upgrade method/URL/version, selected upgrade protocol, requested upgrade protocol, selected WebSocket subprotocol, requested WebSocket subprotocol, bootstrap success/failure, request gone away, and unexpected disconnect diagnostics.

## Post-migration inventory

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" \
  src/web/http/server \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

Result: no remaining LOG/PLOG/VLOG call sites in Migration 7b scope.

## Filter/backend/format compatibility

`HttpServerMigration07bTest` covers enabled emission, framework component scopes (`web.http.server`, `web.http.server.upgrade`), LogManager filtering, backend filtering, JSON v1 output, sink-taking helper overloads, suppressed formatting, sysError code/text, and representative request/response/upgrade/error payloads.

## Production-scope confirmations

- This follow-up changes only allowed Migration 7b files plus tests/unit/log/CMakeLists.txt.
- This follow-up is not inventory-only.
- This follow-up modifies production HTTP server logging.
- This follow-up does not modify SemanticLogger.*.
- This follow-up does not modify LogScopeOwner.*.
- This follow-up does not modify Logger.*.
- This follow-up does not modify ConfigInstance.cpp.
- This follow-up does not modify src/net/config/stream/tls.
- This follow-up does not modify src/web/websocket.
- This follow-up does not modify src/iot or MQTT-over-WebSocket code.
- This follow-up does not modify DB/apps/examples.
- This follow-up does not change LOG/PLOG/VLOG macro definitions.
- This follow-up does not change LogManager precedence.
- This follow-up does not change LogManager freeze behavior.
- This follow-up does not change JSON v1 schema.
- This follow-up does not start Round 10.

## Changed-file check

- `git diff --name-only master...HEAD` was attempted, but the local worktree does not have a `master` ref in this Codex environment, so Git reported `fatal: ambiguous argument 'master...HEAD'`.
- Equivalent local changed-file inspection used `git diff --name-only HEAD -- src/web/http/server tests/unit/log/CMakeLists.txt docs/logging/semantic-migration-07b-http-server-report.md tests/unit/log/HttpServerMigration07bTest.cpp` plus `git ls-files --others --exclude-standard ...` and showed only the 7b files listed above.

## Build commands run

- `cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON`
- `cmake --build cmake-build --parallel 2 --target HttpServerMigration07bTest http-server`
- `cmake --build cmake-build --parallel 2 --target SemanticLoggerRound2Test LogScopeOwnerRound3Test ProductionLogApiRound4Test SocketEndpointLogApiRound5Test SemanticBackendRound6Test SemanticFilterRound7Test ControlledMigrationRound8Test SemanticCompatibilityRound9Test SemanticOverheadRound9Test SemanticProductionThresholdRepairTest SocketConnectionMigration01Test SocketConnectorAcceptorMigration02Test SocketServerClientMigration03Test CoreRuntimeMigration04Test NetPhysicalSocketMigration05Test TlsSocketStreamMigration06Test HttpClientMigration07aTest HttpServerMigration07bTest`
- `cmake --build cmake-build --parallel 2`
- `cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON`
- `cmake --build cmake-build-asan --parallel 2 --target HttpServerMigration07bTest`

## Test commands run

- `ctest --test-dir cmake-build -R HttpServerMigration07bTest --output-on-failure` passed.
- An early focused-suite `ctest` invocation was run before the reused build tree had all focused test executables and reported those missing executables as `Not Run`; after building the focused targets, the same focused-suite command passed.
- `ctest --test-dir cmake-build -R "SemanticLoggerRound2Test|LogScopeOwnerRound3Test|ProductionLogApiRound4Test|SocketEndpointLogApiRound5Test|SemanticBackendRound6Test|SemanticFilterRound7Test|ControlledMigrationRound8Test|SemanticCompatibilityRound9Test|SemanticOverheadRound9Test|SemanticProductionThresholdRepairTest|SocketConnectionMigration01Test|SocketConnectorAcceptorMigration02Test|SocketServerClientMigration03Test|CoreRuntimeMigration04Test|NetPhysicalSocketMigration05Test|TlsSocketStreamMigration06Test|HttpClientMigration07aTest|HttpServerMigration07bTest" --output-on-failure` passed.
- `ctest --test-dir cmake-build --output-on-failure` passed with 125/125 tests successful; skipped component tests were reported as skipped by the test harness, not failed.
- `ASAN=$(gcc -print-file-name=libasan.so) && LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan -R HttpServerMigration07bTest --output-on-failure` passed.

## ASan result

ASan focused `HttpServerMigration07bTest` passed in `cmake-build-asan` with `LD_PRELOAD` set to `gcc -print-file-name=libasan.so`.

## Known follow-ups

- Migration 7c — WebSocket
- Migration 8 — MQTT
- Migration 9 — DB / apps / examples / remaining cleanup
- Round 10 — book integration
