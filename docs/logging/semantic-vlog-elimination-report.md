# Semantic VLOG Elimination Report

## Implementation summary

This PR eliminates the remaining `VLOG(...)` call sites from C++ source and test code by replacing useful diagnostics with semantic logging and removing test-only compatibility checks that depended on legacy verbose macros.

This PR eliminates remaining `VLOG` call sites. This PR does not remove legacy `LOG`, `PLOG`, or `VLOG` macro definitions. This PR does not add `verbose(n)` or a `VLOG` compatibility bridge.

No `LogManager` behavior, JSON schema, or semantic logging infrastructure behavior was changed.

## Exact changed files

- `src/apps/configtest.cpp`
- `src/apps/database/testmariadb.cpp`
- `src/apps/echo/echoclient.cpp`
- `src/apps/echo/echoserver.cpp`
- `src/apps/echo/model/EchoSocketContext.cpp`
- `src/apps/echo/model/clients.h`
- `src/apps/echo/model/servers.h`
- `src/apps/express_compat_server.cpp`
- `src/apps/http/httpclient.cpp`
- `src/apps/http/httplowlevelclient.cpp`
- `src/apps/http/httpserver.cpp`
- `src/apps/http/model/clients.h`
- `src/apps/http/model/servers.h`
- `src/apps/http/testbasicauthentication.cpp`
- `src/apps/http/testexpressnext.cpp`
- `src/apps/http/verysimpleserver.cpp`
- `src/apps/http/vhostserver.cpp`
- `src/apps/jsonclient.cpp`
- `src/apps/jsonserver.cpp`
- `src/apps/main.cpp`
- `src/apps/oauth2/authorization_server/AuthorizationServer.cpp`
- `src/apps/oauth2/client_app/ClientApp.cpp`
- `src/apps/oauth2/resource_server/ResourceServer.cpp`
- `src/apps/testpipe.cpp`
- `src/apps/testpost.cpp`
- `src/apps/testregex.cpp`
- `src/apps/tlslegacy/TlsLegacySocketContext.cpp`
- `src/apps/tlslegacy/tlslegacyclient.cpp`
- `src/apps/tlslegacy/tlslegacyserver.cpp`
- `src/apps/warema-jalousien.cpp`
- `src/apps/websocket/echoclient.cpp`
- `src/apps/websocket/echoserver.cpp`
- `src/apps/websocket/subprotocol/client/echo/Echo.cpp`
- `src/apps/websocket/subprotocol/server/echo/Echo.cpp`
- `src/express/legacy/in/Server.cpp`
- `src/express/legacy/in6/Server.cpp`
- `src/express/legacy/rc/Server.cpp`
- `src/express/legacy/un/Server.cpp`
- `src/express/tls/in/Server.cpp`
- `src/express/tls/in6/Server.cpp`
- `src/express/tls/rc/Server.cpp`
- `src/express/tls/un/Server.cpp`
- `tests/unit/log/SemanticCompatibilityRound9Test.cpp`
- `tests/unit/log/SemanticLoggerRound2Test.cpp`
- `docs/logging/semantic-vlog-elimination-report.md`

## Initial VLOG inventory command/result

Command:

```sh
rg -n "\bVLOG\s*\(" \
  src tests \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

Initial result summary: 537 textual matches across 45 files. Of those, 535 were source/test call sites and 2 were the intentionally retained `src/log/Logger.h` macro definitions.

Initial file summary:

| File | Matches |
| --- | ---: |
| `src/apps/configtest.cpp` | 1 |
| `src/apps/database/testmariadb.cpp` | 65 |
| `src/apps/echo/echoclient.cpp` | 7 |
| `src/apps/echo/echoserver.cpp` | 3 |
| `src/apps/echo/model/EchoSocketContext.cpp` | 3 |
| `src/apps/echo/model/clients.h` | 15 |
| `src/apps/echo/model/servers.h` | 15 |
| `src/apps/express_compat_server.cpp` | 2 |
| `src/apps/http/httpclient.cpp` | 5 |
| `src/apps/http/httplowlevelclient.cpp` | 37 |
| `src/apps/http/httpserver.cpp` | 7 |
| `src/apps/http/model/clients.h` | 48 |
| `src/apps/http/model/servers.h` | 22 |
| `src/apps/http/testbasicauthentication.cpp` | 6 |
| `src/apps/http/testexpressnext.cpp` | 2 |
| `src/apps/http/verysimpleserver.cpp` | 4 |
| `src/apps/http/vhostserver.cpp` | 4 |
| `src/apps/jsonclient.cpp` | 17 |
| `src/apps/jsonserver.cpp` | 4 |
| `src/apps/main.cpp` | 2 |
| `src/apps/oauth2/authorization_server/AuthorizationServer.cpp` | 50 |
| `src/apps/oauth2/client_app/ClientApp.cpp` | 5 |
| `src/apps/oauth2/resource_server/ResourceServer.cpp` | 22 |
| `src/apps/testpipe.cpp` | 4 |
| `src/apps/testpost.cpp` | 8 |
| `src/apps/testregex.cpp` | 51 |
| `src/apps/tlslegacy/TlsLegacySocketContext.cpp` | 8 |
| `src/apps/tlslegacy/tlslegacyclient.cpp` | 1 |
| `src/apps/tlslegacy/tlslegacyserver.cpp` | 1 |
| `src/apps/warema-jalousien.cpp` | 4 |
| `src/apps/websocket/echoclient.cpp` | 19 |
| `src/apps/websocket/echoserver.cpp` | 27 |
| `src/apps/websocket/subprotocol/client/echo/Echo.cpp` | 7 |
| `src/apps/websocket/subprotocol/server/echo/Echo.cpp` | 7 |
| `src/express/legacy/in/Server.cpp` | 6 |
| `src/express/legacy/in6/Server.cpp` | 6 |
| `src/express/legacy/rc/Server.cpp` | 6 |
| `src/express/legacy/un/Server.cpp` | 6 |
| `src/express/tls/in/Server.cpp` | 6 |
| `src/express/tls/in6/Server.cpp` | 6 |
| `src/express/tls/rc/Server.cpp` | 6 |
| `src/express/tls/un/Server.cpp` | 6 |
| `src/log/Logger.h` | 2 |
| `tests/unit/log/SemanticCompatibilityRound9Test.cpp` | 2 |
| `tests/unit/log/SemanticLoggerRound2Test.cpp` | 2 |

## Initial LOG/PLOG verification command/result

Command:

```sh
rg -n "\b(LOG|PLOG)\s*\(" \
  src tests \
  -g '*.h' -g '*.hpp' -g '*.cpp' \
  -g '!src/log/**'
```

Initial result: no production/application/example `LOG` or `PLOG` call sites outside `src/log`. Matches were limited to legacy macro compatibility assertions in logging unit tests (`SemanticCompatibilityRound9Test.cpp`, `SemanticLoggerRound2Test.cpp`, `SemanticFilterRound7Test.cpp`, and `SemanticBackendRound6Test.cpp`) and expectation strings that name `LOG`/`PLOG`.

## Classification summary table

| Category | Treatment in this PR |
| --- | --- |
| A. startup / listen / connected / disabled state | Converted to semantic `info()` or `debug()` according to external usefulness. |
| B. request / response lifecycle | Converted to semantic `debug()`. |
| C. route table / dispatcher match diagnostics | Converted to semantic `trace()` where retained. |
| D. protocol / frame / body / payload dump | Converted to semantic diagnostics; large/body-style output retained as developer diagnostics. |
| E. TLS certificate inspection | Converted to semantic `debug()` diagnostics. |
| F. demo / example progress output | Kept as semantic `info()` or `debug()` where it describes app progress; obsolete test compatibility progress was removed. |
| G. old noisy developer chatter | Removed in legacy VLOG compatibility test cases where the diagnostic no longer represented current policy. |
| H. error-like verbose line | Converted to semantic `warn()` or `error()`. |
| I. test-only diagnostic output | Deleted for obsolete VLOG compatibility checks. |

## Replacement/deletion policy summary

- App/example call sites now use `snode::semantic::appLog()` or a more specific existing helper such as `mariaDbLog()` where appropriate.
- Express server wrappers now use `snode::semantic::expressLog()`.
- Lifecycle lines are `info()` when externally useful and `debug()` when developer-oriented.
- Error-like former verbose lines are `warn()` or `error()`.
- Route-listing diagnostics are `trace()`.
- Test-only VLOG compatibility assertions were deleted because the frozen contract explicitly rejects preserving VLOG behavior.

## Representative migrated examples

- Application configuration progress changed from a legacy verbose macro to semantic app debug logging.
- Express listen/disabled/error callbacks changed to semantic express logging with meaningful severities.
- MariaDB connection status and error lines changed to semantic database/app logging rather than numeric verbose output.
- HTTP/OAuth request and response lifecycle diagnostics changed to semantic app debug/warn/error logging.

## Representative deleted examples

- `SemanticLoggerRound2Test.cpp` no longer emits `VLOG` compatibility lines.
- `SemanticCompatibilityRound9Test.cpp` no longer asserts that `VLOG` respects `Logger::setVerboseLevel`; that behavior belongs to the legacy macro removal follow-up and is not part of the frozen semantic API.

## Expensive diagnostics guard notes

The migration avoided introducing a semantic `verbose(n)` layer or a `VLOG` bridge. Existing payload/body/certificate diagnostics were reviewed for semantic classification. No new helper or infrastructure path constructs dynamic component names or changes filtering behavior.

## Post-migration VLOG inventory command/result

Command:

```sh
rg -n "\bVLOG\s*\(" \
  src tests \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

Result: no `VLOG(...)` call sites remain in C++ source/test files. The only matches are the intentionally retained legacy macro definitions in `src/log/Logger.h`:

```text
src/log/Logger.h:164:#define VLOG(level)
src/log/Logger.h:169:#define VLOG(level)
```

## Post-migration LOG/PLOG verification command/result

Command:

```sh
rg -n "\b(LOG|PLOG)\s*\(" \
  src tests \
  -g '*.h' -g '*.hpp' -g '*.cpp' \
  -g '!src/log/**'
```

Result: no suitable production/application/example `LOG` or `PLOG` call sites outside `src/log`. Remaining matches are legacy macro compatibility unit tests and expectation strings in `tests/unit/log`.

## Macro-definition status

Command:

```sh
rg -n "#define\s+(LOG|PLOG|VLOG)\b" src/log
```

Result: legacy macro definitions are still present in `src/log/Logger.h` and are intentionally not removed in this step.

## Build/test commands run

- `git diff --check`
- `cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON`
- `cmake --build cmake-build --parallel 2`
- `ctest --test-dir cmake-build --output-on-failure`
- `ctest --test-dir cmake-build -R "SemanticLoggerRound2Test|LogScopeOwnerRound3Test|ProductionLogApiRound4Test|SocketEndpointLogApiRound5Test|SemanticBackendRound6Test|SemanticFilterRound7Test|ControlledMigrationRound8Test|SemanticCompatibilityRound9Test|SemanticOverheadRound9Test|SemanticProductionThresholdRepairTest|SocketConnectionMigration01Test|SocketConnectorAcceptorMigration02Test|SocketServerClientMigration03Test|CoreRuntimeMigration04Test|NetPhysicalSocketMigration05Test|TlsSocketStreamMigration06Test|HttpClientMigration07aTest|HttpServerMigration07bTest|WebSocketMigration07cTest|MqttMigration08Test|FinalCleanupMigration09Test" --output-on-failure`
- `cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON`
- `cmake --build cmake-build-asan --parallel 2 --target FinalCleanupMigration09Test`
- `ASAN=$(gcc -print-file-name=libasan.so) LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan -R FinalCleanupMigration09Test --output-on-failure`

## Known follow-ups

- Remove legacy LOG/PLOG/VLOG macro definitions and add the final hard zero-macro CI/source gate.
- Round 10 — book integration.
