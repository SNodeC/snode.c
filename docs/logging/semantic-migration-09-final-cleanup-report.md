# Semantic logging migration 09 — final cleanup report

## Implementation summary

This is the complete final Migration 9 in one production PR. It is a production/application/example semantic logging migration, not inventory-only, and it does not split Migration 9. The local checkout starts at merge commit `4b7785d` for PR #162; `origin` and `master` refs were not present in this container, so the branch was created from the locally available post-PR #162 branch after verifying the PR #162 files.

The migration converts all remaining suitable `LOG` and `PLOG` call sites under `src` (there are no top-level `apps` or `examples` directories in this checkout) outside `src/log` to semantic logging helpers. Remaining `VLOG` call sites are intentionally deferred because preserving numeric verbose-level semantics would require changing verbose logging semantics or logging infrastructure.

This PR does not duplicate previous migrations; it only cleans up call sites that remained in the final inventory.

## Exact changed files

- `src/apps/echo/echoclient.cpp`
- `src/apps/echo/echoserver.cpp`
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
- `src/apps/tlslegacy/tlslegacyclient.cpp`
- `src/apps/tlslegacy/tlslegacyserver.cpp`
- `src/apps/warema-jalousien.cpp`
- `src/core/DynamicLoader.cpp`
- `src/core/socket/stream/SocketAcceptor.hpp`
- `src/core/socket/stream/SocketConnection.hpp`
- `src/core/socket/stream/SocketConnector.hpp`
- `src/core/socket/stream/SocketWriter.cpp`
- `src/database/mariadb/MariaDBConnection.cpp`
- `src/database/mariadb/MariaDBLibrary.cpp`
- `src/express/dispatcher/ApplicationDispatcher.cpp`
- `src/express/dispatcher/MiddlewareDispatcher.cpp`
- `src/express/dispatcher/RouterDispatcher.cpp`
- `src/express/middleware/StaticMiddleware.cpp`
- `src/express/middleware/VerboseRequest.cpp`
- `src/net/config/ConfigPhysicalSocket.cpp`
- `src/net/config/stream/tls/ConfigSocketServer.hpp`
- `src/web/http/MimeTypes.cpp`
- `src/web/http/SocketContextUpgradeFactorySelector.hpp`
- `tests/unit/log/CMakeLists.txt`
- `src/SemanticLog.h`
- `tests/unit/log/FinalCleanupMigration09Test.cpp`
- `docs/logging/semantic-migration-09-final-cleanup-report.md`


## Complete remaining inventory command/result

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" \
  src apps examples \
  -g '*.h' -g '*.hpp' -g '*.cpp' \
  -g '!src/log/**'
```

Result: 739 entries before migration. The command also reported that this checkout has no top-level `apps` or `examples` directories; application/example code lives under `src/apps`.

```text
src/express/legacy/rc/Server.cpp:73:                        VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/express/legacy/rc/Server.cpp:76:                        VLOG(1) << instanceName << ": disabled";
src/express/legacy/rc/Server.cpp:79:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/legacy/rc/Server.cpp:82:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/legacy/rc/Server.cpp:88:        VLOG(1) << "Instance: " << instanceName;
src/express/legacy/rc/Server.cpp:92:            VLOG(1) << "  " << route;
src/express/legacy/in6/Server.cpp:73:                        VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/express/legacy/in6/Server.cpp:76:                        VLOG(1) << instanceName << ": disabled";
src/express/legacy/in6/Server.cpp:79:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/legacy/in6/Server.cpp:82:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/legacy/in6/Server.cpp:88:        VLOG(1) << "Instance: " << instanceName;
src/express/legacy/in6/Server.cpp:92:            VLOG(1) << "  " << route;
src/express/legacy/in/Server.cpp:73:                        VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/express/legacy/in/Server.cpp:76:                        VLOG(1) << instanceName << ": disabled";
src/express/legacy/in/Server.cpp:79:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/legacy/in/Server.cpp:82:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/legacy/in/Server.cpp:88:        VLOG(1) << "Instance: " << instanceName;
src/express/legacy/in/Server.cpp:92:            VLOG(1) << "  " << route;
src/express/legacy/un/Server.cpp:73:                        VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/express/legacy/un/Server.cpp:76:                        VLOG(1) << instanceName << ": disabled";
src/express/legacy/un/Server.cpp:79:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/legacy/un/Server.cpp:82:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/legacy/un/Server.cpp:88:        VLOG(1) << "Instance: " << instanceName;
src/express/legacy/un/Server.cpp:92:            VLOG(1) << "  " << route;
src/express/tls/rc/Server.cpp:73:                        VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/express/tls/rc/Server.cpp:76:                        VLOG(1) << instanceName << ": disabled";
src/express/tls/rc/Server.cpp:79:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/tls/rc/Server.cpp:82:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/tls/rc/Server.cpp:88:        VLOG(1) << "Instance: " << instanceName;
src/express/tls/rc/Server.cpp:92:            VLOG(1) << "  " << route;
src/express/tls/in6/Server.cpp:73:                        VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/express/tls/in6/Server.cpp:76:                        VLOG(1) << instanceName << ": disabled";
src/express/tls/in6/Server.cpp:79:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/tls/in6/Server.cpp:82:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/tls/in6/Server.cpp:88:        VLOG(1) << "Instance: " << instanceName;
src/express/tls/in6/Server.cpp:92:            VLOG(1) << "  " << route;
src/express/tls/in/Server.cpp:73:                        VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/express/tls/in/Server.cpp:76:                        VLOG(1) << instanceName << ": disabled";
src/express/tls/in/Server.cpp:79:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/tls/in/Server.cpp:82:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/tls/in/Server.cpp:88:        VLOG(1) << "Instance: " << instanceName;
src/express/tls/in/Server.cpp:92:            VLOG(1) << "  " << route;
src/express/tls/un/Server.cpp:73:                        VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/express/tls/un/Server.cpp:76:                        VLOG(1) << instanceName << ": disabled";
src/express/tls/un/Server.cpp:79:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/tls/un/Server.cpp:82:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/tls/un/Server.cpp:88:        VLOG(1) << "Instance: " << instanceName;
src/express/tls/un/Server.cpp:92:            VLOG(1) << "  " << route;
src/express/middleware/VerboseRequest.cpp:60:            LOG(DEBUG) << res->getSocketContext()->getSocketConnection()->getConnectionName()
src/express/dispatcher/ApplicationDispatcher.cpp:73:        LOG(TRACE) << "======================= APPLICATION DISPATCH =======================";
src/express/dispatcher/ApplicationDispatcher.cpp:74:        LOG(TRACE) << controller.getResponse()->getSocketContext()->getSocketConnection()->getConnectionName();
src/express/dispatcher/ApplicationDispatcher.cpp:75:        LOG(TRACE) << "          Request Method: " << controller.getRequest()->method;
src/express/dispatcher/ApplicationDispatcher.cpp:76:        LOG(TRACE) << "             Request Url: " << controller.getRequest()->url;
src/express/dispatcher/ApplicationDispatcher.cpp:77:        LOG(TRACE) << "            Request Path: " << controller.getRequest()->path;
src/express/dispatcher/ApplicationDispatcher.cpp:78:        LOG(TRACE) << "       Mountpoint Method: " << mountPoint.method;
src/express/dispatcher/ApplicationDispatcher.cpp:79:        LOG(TRACE) << "         Mountpoint Path: " << mountPoint.relativeMountPath;
src/express/dispatcher/ApplicationDispatcher.cpp:80:        LOG(TRACE) << "           StrictRouting: " << strictRouting;
src/express/dispatcher/ApplicationDispatcher.cpp:81:        LOG(TRACE) << "  CaseInsensitiveRouting: " << caseInsensitiveRouting;
src/express/dispatcher/ApplicationDispatcher.cpp:82:        LOG(TRACE) << "             MergeParams: " << mergeParams;
src/express/dispatcher/ApplicationDispatcher.cpp:94:                LOG(TRACE) << "----------------------- APPLICATION    MATCH -----------------------";
src/express/dispatcher/ApplicationDispatcher.cpp:116:                LOG(TRACE) << "----------------------- APPLICATION  NOMATCH -----------------------";
src/express/dispatcher/ApplicationDispatcher.cpp:119:            LOG(TRACE) << "----------------------- APPLICATION  NOMATCH -----------------------";
src/express/middleware/StaticMiddleware.cpp:69:                LOG(DEBUG) << res->getSocketContext()->getSocketConnection()->getConnectionName() << " Express " << req->method;
src/express/middleware/StaticMiddleware.cpp:97:                        LOG(INFO) << res->getSocketContext()->getSocketConnection()->getConnectionName()
src/express/middleware/StaticMiddleware.cpp:116:                        LOG(INFO) << res->getSocketContext()->getSocketConnection()->getConnectionName()
src/express/middleware/StaticMiddleware.cpp:119:                        PLOG(ERROR) << res->getSocketContext()->getSocketConnection()->getConnectionName() << " Express StaticMiddleware "
src/express/dispatcher/MiddlewareDispatcher.cpp:73:        LOG(TRACE) << "======================= MIDDLEWARE  DISPATCH =======================";
src/express/dispatcher/MiddlewareDispatcher.cpp:74:        LOG(TRACE) << controller.getResponse()->getSocketContext()->getSocketConnection()->getConnectionName();
src/express/dispatcher/MiddlewareDispatcher.cpp:75:        LOG(TRACE) << "          Request Method: " << controller.getRequest()->method;
src/express/dispatcher/MiddlewareDispatcher.cpp:76:        LOG(TRACE) << "             Request Url: " << controller.getRequest()->url;
src/express/dispatcher/MiddlewareDispatcher.cpp:77:        LOG(TRACE) << "            Request Path: " << controller.getRequest()->path;
src/express/dispatcher/MiddlewareDispatcher.cpp:78:        LOG(TRACE) << "       Mountpoint Method: " << mountPoint.method;
src/express/dispatcher/MiddlewareDispatcher.cpp:79:        LOG(TRACE) << "         Mountpoint Path: " << mountPoint.relativeMountPath;
src/express/dispatcher/MiddlewareDispatcher.cpp:80:        LOG(TRACE) << "           StrictRouting: " << strictRouting;
src/express/dispatcher/MiddlewareDispatcher.cpp:81:        LOG(TRACE) << "  CaseInsensitiveRouting: " << caseInsensitiveRouting;
src/express/dispatcher/MiddlewareDispatcher.cpp:82:        LOG(TRACE) << "             MergeParams: " << mergeParams;
src/express/dispatcher/MiddlewareDispatcher.cpp:94:                LOG(TRACE) << "----------------------- MIDDLEWARE     MATCH -----------------------";
src/express/dispatcher/MiddlewareDispatcher.cpp:112:                            LOG(TRACE) << "Express: M - Next called - set to NO MATCH";
src/express/dispatcher/MiddlewareDispatcher.cpp:124:                LOG(TRACE) << "----------------------- MIDDLEWARE   NOMATCH -----------------------";
src/express/dispatcher/MiddlewareDispatcher.cpp:127:            LOG(TRACE) << "----------------------- MIDDLEWARE   NOMATCH -----------------------";
src/express/dispatcher/RouterDispatcher.cpp:72:        LOG(TRACE) << "======================= ROUTER      DISPATCH =======================";
src/express/dispatcher/RouterDispatcher.cpp:73:        LOG(TRACE) << controller.getResponse()->getSocketContext()->getSocketConnection()->getConnectionName();
src/express/dispatcher/RouterDispatcher.cpp:74:        LOG(TRACE) << "          Request Method: " << controller.getRequest()->method;
src/express/dispatcher/RouterDispatcher.cpp:75:        LOG(TRACE) << "             Request Url: " << controller.getRequest()->url;
src/express/dispatcher/RouterDispatcher.cpp:76:        LOG(TRACE) << "            Request Path: " << controller.getRequest()->path;
src/express/dispatcher/RouterDispatcher.cpp:77:        LOG(TRACE) << "       Mountpoint Method: " << mountPoint.method;
src/express/dispatcher/RouterDispatcher.cpp:78:        LOG(TRACE) << "         Mountpoint Path: " << mountPoint.relativeMountPath;
src/express/dispatcher/RouterDispatcher.cpp:79:        LOG(TRACE) << "           StrictRouting: " << this->strictRouting;
src/express/dispatcher/RouterDispatcher.cpp:80:        LOG(TRACE) << "  CaseInsensitiveRouting: " << this->caseInsensitiveRouting;
src/express/dispatcher/RouterDispatcher.cpp:81:        LOG(TRACE) << "             MergeParams: " << this->mergeParams;
src/express/dispatcher/RouterDispatcher.cpp:92:                LOG(TRACE) << "----------------------- ROUTER         MATCH -----------------------";
src/express/dispatcher/RouterDispatcher.cpp:115:                LOG(TRACE) << "----------------------- ROUTER       NOMATCH -----------------------";
src/express/dispatcher/RouterDispatcher.cpp:118:            LOG(TRACE) << "----------------------- ROUTER       NOMATCH -----------------------";
src/apps/testpost.cpp:110:                VLOG(1) << "legacyApp: listening on '" << socketAddress.toString() << "'";
src/apps/testpost.cpp:113:                VLOG(1) << "legacyApp: disabled";
src/apps/testpost.cpp:116:                VLOG(1) << "legacyApp: error occurred";
src/apps/testpost.cpp:119:                VLOG(1) << "legacyApp: fatal error occurred";
src/apps/testpost.cpp:140:                VLOG(1) << "tlsApp: listening on '" << socketAddress.toString() << "'";
src/apps/testpost.cpp:143:                VLOG(1) << "tlsApp: disabled";
src/apps/testpost.cpp:146:                VLOG(1) << "tlsApp: error occurred";
src/apps/testpost.cpp:149:                VLOG(1) << "tlsApp: fatal error occurred";
src/apps/testpipe.cpp:64:                VLOG(1) << "Pipe Data: " << string;
src/apps/testpipe.cpp:71:                VLOG(1) << "Pipe EOF";
src/apps/testpipe.cpp:75:                VLOG(1) << "PipeSink";
src/apps/testpipe.cpp:79:                VLOG(1) << "PipeSource";
src/apps/testpipe.cpp:85:            PLOG(ERROR) << "Pipe not created";
src/database/mariadb/MariaDBLibrary.cpp:62:                LOG(ERROR) << "MariaDB: mysql_library_init failed (rc=" << rc << ")";
src/database/mariadb/MariaDBConnection.cpp:101:                        LOG(ERROR) << this->connectionName << " MariaDB: Descriptor not registered in SNode.C eventloop";
src/database/mariadb/MariaDBConnection.cpp:106:                LOG(DEBUG) << this->connectionName << " MariaDB connect: success";
src/database/mariadb/MariaDBConnection.cpp:111:                LOG(WARNING) << this->connectionName << " MariaDB connect: error: " << errorString << " : " << errorNumber;
src/database/mariadb/MariaDBConnection.cpp:165:            LOG(DEBUG) << connectionName << " MariaDB start: " << currentCommand->commandInfo();
src/database/mariadb/MariaDBConnection.cpp:191:        LOG(DEBUG) << connectionName << " MariaDB completed: " << currentCommand->commandInfo();
src/database/mariadb/MariaDBConnection.cpp:302:            LOG(ERROR) << connectionName << " MariaDB: Lost connection";
src/web/http/MimeTypes.cpp:249:            LOG(DEBUG) << "HTTP: Cannot load magic database - " + std::string(magic_error(magic));
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:107:                                                VLOG(0) << "MySQL error: " << state.errorMessage << " [" << state.error << "]";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:109:                                                VLOG(0) << "MySQL connected";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:111:                                                VLOG(0) << "MySQL disconnected";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:128:                            VLOG(1) << "Valid client id '" << queryClientId << "'";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:129:                            VLOG(1) << "Next with " << req->httpVersion << " " << req->method << " " << req->url;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:132:                            VLOG(1) << "Invalid client id '" << queryClientId << "'";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:138:                    VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:156:        VLOG(1) << "Query params: "
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:163:            VLOG(1) << "Auth invalid, sending Bad Request";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:172:                    VLOG(1) << "Database: Set redirect_uri to " << paramRedirectUri;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:175:                    VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:183:                    VLOG(1) << "Database: Set scope to " << paramScope;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:186:                    VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:194:                    VLOG(1) << "Database: Set state to " << paramState;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:197:                    VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:201:        VLOG(1) << "Auth request valid, redirecting to login";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:211:                              PLOG(ERROR) << req->url;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:250:                                          VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:275:                                                        VLOG(1) << "Sending json reponse: " << responseJsonString;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:279:                                                        VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:285:                                            VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:292:                        VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:304:        VLOG(1) << "GrandType: " << queryGrantType;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:306:        VLOG(1) << "Code: " << queryCode;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:308:        VLOG(1) << "RedirectUri: " << queryRedirectUri;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:367:                                              VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:384:                                                            VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:390:                                                VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:403:                                                VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:426:                                                            VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:432:                                                VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:438:                                VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:445:                VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:453:        VLOG(1) << "ClientId: " << queryClientId;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:455:        VLOG(1) << "GrandType: " << queryGrantType;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:457:        VLOG(1) << "RefreshToken: " << queryRefreshToken;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:459:        VLOG(1) << "State: " << queryState;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:500:                              VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:520:                                            VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:526:                                VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:532:                VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:538:        VLOG(1) << "POST /token/validate";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:541:                VLOG(1) << "Missing 'access_token' in json";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:547:                VLOG(1) << "Missing 'client_id' in json";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:566:                            VLOG(1) << "Sending 401: Invalid access token '" << jsonAccessToken << "'";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:569:                            VLOG(1) << "Sending 200: Valid access token '" << jsonAccessToken << "";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:576:                    VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:589:                VLOG(1) << "OAuth2AuthorizationServer: listening on '" << socketAddress.toString() << "'";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:592:                VLOG(1) << "OAuth2AuthorizationServer: disabled";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:595:                VLOG(1) << "OAuth2AuthorizationServer: error occurred";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:598:                VLOG(1) << "OAuth2AuthorizationServer: fatal error occurred";
src/apps/oauth2/resource_server/ResourceServer.cpp:66:            VLOG(1) << "Missing access_token or client_id in body";
src/apps/oauth2/resource_server/ResourceServer.cpp:73:                VLOG(1) << "OnConnect";
src/apps/oauth2/resource_server/ResourceServer.cpp:75:                VLOG(1) << "\tServer: " + socketConnection->getRemoteAddress().toString();
src/apps/oauth2/resource_server/ResourceServer.cpp:76:                VLOG(1) << "\tClient: " + socketConnection->getLocalAddress().toString();
src/apps/oauth2/resource_server/ResourceServer.cpp:79:                VLOG(1) << "OnConnected";
src/apps/oauth2/resource_server/ResourceServer.cpp:82:                VLOG(1) << "OnDisconnect";
src/apps/oauth2/resource_server/ResourceServer.cpp:84:                VLOG(1) << "\tServer: " + socketConnection->getRemoteAddress().toString();
src/apps/oauth2/resource_server/ResourceServer.cpp:85:                VLOG(1) << "\tClient: " + socketConnection->getLocalAddress().toString();
src/apps/oauth2/resource_server/ResourceServer.cpp:88:                VLOG(1) << "OnRequestBegin";
src/apps/oauth2/resource_server/ResourceServer.cpp:91:                VLOG(1) << "ClientId: " << queryClientId;
src/apps/oauth2/resource_server/ResourceServer.cpp:92:                VLOG(1) << "AccessToken: " << queryAccessToken;
src/apps/oauth2/resource_server/ResourceServer.cpp:99:                        VLOG(1) << "OnResponse";
src/apps/oauth2/resource_server/ResourceServer.cpp:100:                        VLOG(1) << "Response: " << std::string(response->body.begin(), response->body.end());
src/apps/oauth2/resource_server/ResourceServer.cpp:110:                        VLOG(1) << "OAuth2ResourceServer: Request parse error: " << message;
src/apps/oauth2/resource_server/ResourceServer.cpp:114:                LOG(INFO) << " -- OnRequestEnd";
src/apps/oauth2/resource_server/ResourceServer.cpp:121:                        VLOG(1) << "OAuth2ResourceServer: connected to '" << socketAddress.toString() << "'";
src/apps/oauth2/resource_server/ResourceServer.cpp:124:                        VLOG(1) << "OAuth2ResourceServer: disabled";
src/apps/oauth2/resource_server/ResourceServer.cpp:127:                        VLOG(1) << "OAuth2ResourceServer: error occurred";
src/apps/oauth2/resource_server/ResourceServer.cpp:130:                        VLOG(1) << "OAuth2ResourceServer: fatal error occurred";
src/apps/oauth2/resource_server/ResourceServer.cpp:139:                VLOG(1) << "app: listening on '" << socketAddress.toString() << "'";
src/apps/oauth2/resource_server/ResourceServer.cpp:142:                VLOG(1) << "app: disabled";
src/apps/oauth2/resource_server/ResourceServer.cpp:145:                VLOG(1) << "app: error occurred";
src/apps/oauth2/resource_server/ResourceServer.cpp:148:                VLOG(1) << "app: fatal error occurred";
src/web/http/SocketContextUpgradeFactorySelector.hpp:100:                        LOG(TRACE) << "HTTP: SocketContextUpgradeFactory create success: " << socketContextUpgradeName;
src/web/http/SocketContextUpgradeFactorySelector.hpp:102:                        LOG(TRACE) << "HTTP: SocketContextUpgradeFactory already existing: " << socketContextUpgradeName;
src/web/http/SocketContextUpgradeFactorySelector.hpp:108:                    LOG(ERROR) << "HTTP: SocketContextUpgradeFactory create failed: " << socketContextUpgradeName;
src/web/http/SocketContextUpgradeFactorySelector.hpp:112:                LOG(ERROR) << "HTTP: Optaining function \"" << socketContextUpgradeFactoryFunctionName
src/web/http/SocketContextUpgradeFactorySelector.hpp:129:            LOG(DEBUG) << "HTTP upgrade: plugin '" << socketContextUpgradeName << "' selected from dynamic cache";
src/web/http/SocketContextUpgradeFactorySelector.hpp:133:            LOG(DEBUG) << "HTTP upgrade: plugin '" << socketContextUpgradeName << "' selected from static cache";
src/web/http/SocketContextUpgradeFactorySelector.hpp:137:            LOG(DEBUG) << "HTTP upgrade: plugin '" << socketContextUpgradeName << "' loaded and added to dynamic cache";
src/web/http/SocketContextUpgradeFactorySelector.hpp:139:            LOG(WARNING) << "HTTP upgrade: plugin '" << socketContextUpgradeName << "' not found";
src/apps/oauth2/client_app/ClientApp.cpp:56:                                  PLOG(ERROR) << req->url;
src/apps/oauth2/client_app/ClientApp.cpp:68:            VLOG(1) << "Recieving auth code from auth server: " << req.query("code") << ", requesting token from " << tokenRequestUri;
src/apps/oauth2/client_app/ClientApp.cpp:79:                VLOG(1) << "OAuth2Client: connected to '" << socketAddress.toString() << "'";
src/apps/oauth2/client_app/ClientApp.cpp:82:                VLOG(1) << "OAuth2Client: disabled";
src/apps/oauth2/client_app/ClientApp.cpp:85:                VLOG(1) << "OAuth2Client: error occurred";
src/apps/oauth2/client_app/ClientApp.cpp:88:                VLOG(1) << "OAuth2Client: fatal error occurred";
src/apps/database/testmariadb.cpp:114:            VLOG(0) << "MySQL error: " << state.errorMessage << " [" << state.error << "]";
src/apps/database/testmariadb.cpp:116:            VLOG(0) << "MySQL connected";
src/apps/database/testmariadb.cpp:118:            VLOG(0) << "MySQL disconnected";
src/apps/database/testmariadb.cpp:127:               VLOG(0) << "********** OnQuery 0;";
src/apps/database/testmariadb.cpp:130:                       VLOG(0) << "********** AffectedRows 1: " << affectedRows;
src/apps/database/testmariadb.cpp:133:                       VLOG(0) << "Error 1: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:137:               VLOG(0) << "********** Error 0: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:143:                VLOG(0) << "********** OnQuery 1: ";
src/apps/database/testmariadb.cpp:146:                        VLOG(0) << "********** AffectedRows 2: " << affectedRows;
src/apps/database/testmariadb.cpp:149:                        VLOG(0) << "********** Error 2: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:153:                VLOG(0) << "********** Error 1: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:159:                    VLOG(0) << "********** Row Result 2: " << row[0] << " : " << row[1];
src/apps/database/testmariadb.cpp:162:                    VLOG(0) << "********** Row Result 2: " << r;
src/apps/database/testmariadb.cpp:166:                VLOG(0) << "********** Error 2: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:172:                    VLOG(0) << "********** Row Result 2: " << row[0] << " : " << row[1];
src/apps/database/testmariadb.cpp:175:                    VLOG(0) << "********** Row Result 2: " << r;
src/apps/database/testmariadb.cpp:179:                VLOG(0) << "********** Error 2: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:184:            VLOG(0) << "MySQL error: " << state.errorMessage << " [" << state.error << "]";
src/apps/database/testmariadb.cpp:186:            VLOG(0) << "MySQL connected";
src/apps/database/testmariadb.cpp:188:            VLOG(0) << "MySQL disconnected";
src/apps/database/testmariadb.cpp:199:                    VLOG(0) << "Row Result 3: " << row[0] << " : " << row[1];
src/apps/database/testmariadb.cpp:201:                    VLOG(0) << "Row Result 3:";
src/apps/database/testmariadb.cpp:205:                VLOG(0) << "Error 3: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:212:                    VLOG(0) << "Row Result 4: " << row[0] << " : " << row[1];
src/apps/database/testmariadb.cpp:214:                    VLOG(0) << "Row Result 4:";
src/apps/database/testmariadb.cpp:220:                                VLOG(0) << "Row Result 5: " << row[0] << " : " << row[1];
src/apps/database/testmariadb.cpp:222:                                VLOG(0) << "Row Result 5:";
src/apps/database/testmariadb.cpp:227:                                        VLOG(0) << "Tick 2: " << i++;
src/apps/database/testmariadb.cpp:234:                                                    VLOG(0) << "Row Result 6: " << row[0] << " : " << row[1];
src/apps/database/testmariadb.cpp:237:                                                    VLOG(0) << "Row Result 6: " << r1;
src/apps/database/testmariadb.cpp:241:                                                VLOG(0) << "Error 6: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:250:                                        VLOG(0) << "Tick 0.7: " << i++;
src/apps/database/testmariadb.cpp:257:                                                       VLOG(0) << "Row Result 7: " << row[0] << " : " << row[1];
src/apps/database/testmariadb.cpp:260:                                                       VLOG(0) << "Row Result 7: " << r2;
src/apps/database/testmariadb.cpp:263:                                                               VLOG(0) << "************ FieldCount ************ = " << fieldCount;
src/apps/database/testmariadb.cpp:266:                                                               VLOG(0) << "Error 7: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:271:                                                   VLOG(0) << "Error 7: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:276:                                                    VLOG(0) << "************ FieldCount ************ = " << fieldCount;
src/apps/database/testmariadb.cpp:279:                                                    VLOG(0) << "Error 7: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:286:                            VLOG(0) << "Error 5: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:291:                VLOG(0) << "Error 4: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:297:                VLOG(0) << "Tick 0.1: " << i++;
src/apps/database/testmariadb.cpp:300:                    VLOG(0) << "Stop Stop";
src/apps/database/testmariadb.cpp:307:                           VLOG(0) << "Transactions activated 10:";
src/apps/database/testmariadb.cpp:310:                           VLOG(0) << "Error 8: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:315:                            VLOG(0) << "Inserted 10: " << j;
src/apps/database/testmariadb.cpp:318:                                    VLOG(0) << "AffectedRows 11: " << affectedRows;
src/apps/database/testmariadb.cpp:321:                                    VLOG(0) << "Error 11: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:325:                            VLOG(0) << "Error 10: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:330:                            VLOG(0) << "Rollback success 11";
src/apps/database/testmariadb.cpp:333:                            VLOG(0) << "Error 12: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:339:                            VLOG(0) << "Inserted 13: " << j;
src/apps/database/testmariadb.cpp:342:                                    VLOG(0) << "AffectedRows 14: " << affectedRows;
src/apps/database/testmariadb.cpp:345:                                    VLOG(0) << "Error 14: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:349:                            VLOG(0) << "Error 13: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:354:                            VLOG(0) << "Commit success 15";
src/apps/database/testmariadb.cpp:357:                            VLOG(0) << "Error 15: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:364:                                VLOG(0) << "Row Result count(*) 16: " << row[0];
src/apps/database/testmariadb.cpp:366:                                    VLOG(0) << "Wrong number of rows 16: " << std::atoi(row[0]) << " != " << j + 1; // NOLINT
src/apps/database/testmariadb.cpp:370:                                VLOG(0) << "Row Result count(*) 16: no result:";
src/apps/database/testmariadb.cpp:373:                                        VLOG(0) << "************ FieldCount ************ = " << fieldCount;
src/apps/database/testmariadb.cpp:376:                                        VLOG(0) << "Error 7: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:381:                            VLOG(0) << "Error 16: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:386:                            VLOG(0) << "Transactions deactivated 17";
src/apps/database/testmariadb.cpp:389:                            VLOG(0) << "Error 17: " << errorString << " : " << errorNumber;
src/apps/websocket/subprotocol/server/echo/Echo.cpp:63:        VLOG(1) << "Echo connected";
src/apps/websocket/subprotocol/server/echo/Echo.cpp:67:        VLOG(2) << "Message Start - OpCode: " << opCode;
src/apps/websocket/subprotocol/server/echo/Echo.cpp:73:        VLOG(2) << "Message Fragment: " << std::string(chunk, chunkLen);
src/apps/websocket/subprotocol/server/echo/Echo.cpp:77:        VLOG(1) << "Message Data: " << data;
src/apps/websocket/subprotocol/server/echo/Echo.cpp:90:        VLOG(1) << "Message error: " << errnum;
src/apps/websocket/subprotocol/server/echo/Echo.cpp:94:        VLOG(1) << "Echo disconnected:";
src/apps/websocket/subprotocol/server/echo/Echo.cpp:98:        VLOG(1) << "SubProtocol 'echo' exit due to '" << strsignal(sig) << "' (SIG" << utils::system::sigabbrev_np(sig) << " = " << sig
src/apps/websocket/subprotocol/client/echo/Echo.cpp:63:        VLOG(1) << "Echo connected";
src/apps/websocket/subprotocol/client/echo/Echo.cpp:70:        VLOG(2) << "Message Start - OpCode: " << opCode;
src/apps/websocket/subprotocol/client/echo/Echo.cpp:76:        VLOG(2) << "Message Fragment: " << std::string(chunk, chunkLen);
src/apps/websocket/subprotocol/client/echo/Echo.cpp:80:        VLOG(1) << "Message Data: " << data;
src/apps/websocket/subprotocol/client/echo/Echo.cpp:89:        VLOG(1) << "Message error: " << errnum;
src/apps/websocket/subprotocol/client/echo/Echo.cpp:93:        VLOG(1) << "Echo disconnected:";
src/apps/websocket/subprotocol/client/echo/Echo.cpp:97:        VLOG(1) << "SubProtocol 'echo' exit due to '" << strsignal(sig) << "' (SIG" << utils::system::sigabbrev_np(sig) << " = " << sig
src/apps/websocket/echoclient.cpp:67:                VLOG(1) << connectionName << ": OnRequestBegin";
src/apps/websocket/echoclient.cpp:75:                        VLOG(1) << connectionName << ": HTTP Upgrade (http -> websocket) start " << (success ? "success" : "failed");
src/apps/websocket/echoclient.cpp:80:                        VLOG(1) << connectionName << ": Upgrade success:";
src/apps/websocket/echoclient.cpp:82:                        VLOG(1) << connectionName << ":   Requested: " << req->header("upgrade");
src/apps/websocket/echoclient.cpp:83:                        VLOG(1) << connectionName << ":    Selected: " << res->get("upgrade");
src/apps/websocket/echoclient.cpp:86:                        VLOG(1) << connectionName << ": Request parse error: " << message;
src/apps/websocket/echoclient.cpp:92:                VLOG(1) << connectionName << ": OnRequestEnd";
src/apps/websocket/echoclient.cpp:99:                    VLOG(1) << instanceName << " connected to '" << socketAddress.toString() << "'";
src/apps/websocket/echoclient.cpp:102:                    VLOG(1) << instanceName << " disabled";
src/apps/websocket/echoclient.cpp:105:                    VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/websocket/echoclient.cpp:108:                    VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/websocket/echoclient.cpp:124:                VLOG(1) << connectionName << ": OnRequestBegin";
src/apps/websocket/echoclient.cpp:132:                        VLOG(1) << connectionName << ": HTTP Upgrade (http -> websocket) start " << (success ? "success" : "failed");
src/apps/websocket/echoclient.cpp:139:                        VLOG(1) << connectionName << ": Request parse error: " << message;
src/apps/websocket/echoclient.cpp:145:                VLOG(1) << connectionName << ": OnRequestEnd";
src/apps/websocket/echoclient.cpp:152:                    VLOG(1) << instanceName << " connected to '" << socketAddress.toString() << "'";
src/apps/websocket/echoclient.cpp:155:                    VLOG(1) << instanceName << " disabled";
src/apps/websocket/echoclient.cpp:158:                    VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/websocket/echoclient.cpp:161:                    VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/websocket/echoserver.cpp:75:                VLOG(1) << connectionName << ": Successful upgrade:";
src/apps/websocket/echoserver.cpp:76:                VLOG(1) << connectionName << ":   Requested: " << req->get("upgrade");
src/apps/websocket/echoserver.cpp:77:                VLOG(1) << connectionName << ":    Selected: " << name;
src/apps/websocket/echoserver.cpp:81:                VLOG(1) << connectionName << ": Can not upgrade to any of '" << req->get("upgrade") << "'";
src/apps/websocket/echoserver.cpp:89:        VLOG(1) << "HTTP GET on "
src/apps/websocket/echoserver.cpp:95:        VLOG(1) << CMAKE_CURRENT_SOURCE_DIR "/html" + req->url;
src/apps/websocket/echoserver.cpp:98:                VLOG(1) << req->url;
src/apps/websocket/echoserver.cpp:100:                VLOG(1) << "HTTP response send file failed: " << std::strerror(errnum);
src/apps/websocket/echoserver.cpp:111:                    VLOG(1) << instanceName << " listening on '" << socketAddress.toString() << "'";
src/apps/websocket/echoserver.cpp:114:                    VLOG(1) << instanceName << " disabled";
src/apps/websocket/echoserver.cpp:117:                    VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/websocket/echoserver.cpp:120:                    VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/websocket/echoserver.cpp:126:    VLOG(1) << "Legacy Routes:";
src/apps/websocket/echoserver.cpp:130:        VLOG(1) << "  " << route;
src/apps/websocket/echoserver.cpp:148:                    VLOG(1) << connectionName << ": Upgrade success:";
src/apps/websocket/echoserver.cpp:149:                    VLOG(1) << connectionName << ":   Requested: " << req->get("upgrade");
src/apps/websocket/echoserver.cpp:150:                    VLOG(1) << connectionName << ":    Selected: " << name;
src/apps/websocket/echoserver.cpp:154:                    VLOG(1) << connectionName << ": Can not upgrade to any of '" << req->get("upgrade") << "'";
src/apps/websocket/echoserver.cpp:166:            VLOG(1) << CMAKE_CURRENT_SOURCE_DIR "/html" + req->url;
src/apps/websocket/echoserver.cpp:169:                    VLOG(1) << req->url;
src/apps/websocket/echoserver.cpp:171:                    VLOG(1) << "HTTP response send file failed: " << std::strerror(errnum);
src/apps/websocket/echoserver.cpp:182:                        VLOG(1) << instanceName << " listening on '" << socketAddress.toString() << "'";
src/apps/websocket/echoserver.cpp:185:                        VLOG(1) << instanceName << " disabled";
src/apps/websocket/echoserver.cpp:188:                        VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/websocket/echoserver.cpp:191:                        VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/websocket/echoserver.cpp:197:        VLOG(1) << "Tls Routes:";
src/apps/websocket/echoserver.cpp:201:            VLOG(1) << "  " << route;
src/apps/echo/echoclient.cpp:62:                    VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
src/apps/echo/echoclient.cpp:65:                    VLOG(1) << instanceName << ": disabled";
src/apps/echo/echoclient.cpp:68:                    LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/echo/echoclient.cpp:71:                    LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/echo/echoclient.cpp:79:                    VLOG(1) << "echoclient: connected to '" << socketAddress.toString() << "'" << "'";
src/apps/echo/echoclient.cpp:82:                    VLOG(1) << "echoclient: disabled";
src/apps/echo/echoclient.cpp:85:                    VLOG(1) << "echoclientt: error occurred";
src/apps/echo/echoclient.cpp:88:                    VLOG(1) << "echoclient: fatal error occurred";
src/apps/echo/echoclient.cpp:124:        PLOG(ERROR) << "OnError";
src/apps/echo/echoclient.cpp:127:        PLOG(ERROR) << "OnError: " << socketAddress.toString();
src/apps/echo/echoclient.cpp:129:        VLOG(1) << "snode.c connecting to " << socketAddress.toString();
src/apps/echo/echoserver.cpp:86:                VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/apps/echo/echoserver.cpp:89:                VLOG(1) << instanceName << ": disabled";
src/apps/echo/echoserver.cpp:92:                LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/echo/echoserver.cpp:95:                LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/echo/echoserver.cpp:130:            PLOG(FATAL) << "listen";
src/apps/echo/echoserver.cpp:132:            VLOG(1) << "snode.c listening on " << socket.getBindAddress().toString();
src/apps/echo/model/clients.h:93:            VLOG(1) << "OnConnect " << client.getConfig()->getInstanceName();
src/apps/echo/model/clients.h:95:            VLOG(1) << "\tLocal: " << socketConnection->getLocalAddress().toString();
src/apps/echo/model/clients.h:96:            VLOG(1) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
src/apps/echo/model/clients.h:109:            VLOG(1) << "OnConnected " << client.getConfig()->getInstanceName();
src/apps/echo/model/clients.h:115:                VLOG(1) << "\tPeer certificate verifyErr = " + std::to_string(verifyErr) + ": " +
src/apps/echo/model/clients.h:119:                VLOG(1) << "\t   Subject: " + std::string(str);
src/apps/echo/model/clients.h:123:                VLOG(1) << "\t   Issuer: " + std::string(str);
src/apps/echo/model/clients.h:133:                VLOG(1) << "\t   Subject alternative name count: " << altNameCount;
src/apps/echo/model/clients.h:140:                        VLOG(1) << "\t      SAN (URI): '" + subjectAltName;
src/apps/echo/model/clients.h:145:                        VLOG(1) << "\t      SAN (DNS): '" + subjectAltName;
src/apps/echo/model/clients.h:147:                        VLOG(1) << "\t      SAN (Type): '" + std::to_string(generalName->type);
src/apps/echo/model/clients.h:155:                VLOG(1) << "\tPeer certificate: no certificate";
src/apps/echo/model/clients.h:160:            VLOG(1) << "OnDisconnect " << client.getConfig()->getInstanceName();
src/apps/echo/model/clients.h:162:            VLOG(1) << "\tLocal: " << socketConnection->getLocalAddress().toString();
src/apps/echo/model/clients.h:163:            VLOG(1) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
src/apps/echo/model/servers.h:93:            VLOG(1) << "OnConnect " << server.getConfig()->getInstanceName();
src/apps/echo/model/servers.h:95:            VLOG(1) << "\tLocal: " << socketConnection->getLocalAddress().toString();
src/apps/echo/model/servers.h:96:            VLOG(1) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
src/apps/echo/model/servers.h:109:            VLOG(1) << "OnConnected " << server.getConfig()->getInstanceName();
src/apps/echo/model/servers.h:115:                VLOG(1) << "\tPeer certificate verifyErr = " + std::to_string(verifyErr) + ": " +
src/apps/echo/model/servers.h:119:                VLOG(1) << "\t   Subject: " + std::string(str);
src/apps/echo/model/servers.h:123:                VLOG(1) << "\t   Issuer: " + std::string(str);
src/apps/echo/model/servers.h:133:                VLOG(1) << "\t   Subject alternative name count: " << altNameCount;
src/apps/echo/model/servers.h:140:                        VLOG(1) << "\t      SAN (URI): '" + subjectAltName;
src/apps/echo/model/servers.h:145:                        VLOG(1) << "\t      SAN (DNS): '" + subjectAltName;
src/apps/echo/model/servers.h:147:                        VLOG(1) << "\t      SAN (Type): '" + std::to_string(generalName->type);
src/apps/echo/model/servers.h:155:                VLOG(1) << "\tPeer certificate: no certificate";
src/apps/echo/model/servers.h:160:            VLOG(1) << "OnDisconnect " << server.getConfig()->getInstanceName();
src/apps/echo/model/servers.h:162:            VLOG(1) << "\tLocal: " << socketConnection->getLocalAddress().toString();
src/apps/echo/model/servers.h:163:            VLOG(1) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
src/apps/echo/model/EchoSocketContext.cpp:60:        VLOG(1) << "Echo connected";
src/apps/echo/model/EchoSocketContext.cpp:68:        VLOG(1) << "Echo disconnected";
src/apps/echo/model/EchoSocketContext.cpp:81:            VLOG(1) << "Data to reflect: " << std::string(chunk, chunklen);
src/apps/jsonserver.cpp:71:                    VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/apps/jsonserver.cpp:74:                    VLOG(1) << instanceName << ": disabled";
src/apps/jsonserver.cpp:77:                    LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/jsonserver.cpp:80:                    LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/jsonserver.cpp:91:                VLOG(1) << "Application received body: " << jsonString;
src/apps/jsonserver.cpp:94:                VLOG(1) << key << " attribute not found";
src/apps/jsonclient.cpp:63:            VLOG(1) << "-- OnRequest";
src/apps/jsonclient.cpp:71:                    VLOG(1) << "-- OnResponse";
src/apps/jsonclient.cpp:72:                    VLOG(1) << "     Status:";
src/apps/jsonclient.cpp:73:                    VLOG(1) << "       " << res->httpVersion;
src/apps/jsonclient.cpp:74:                    VLOG(1) << "       " << res->statusCode;
src/apps/jsonclient.cpp:75:                    VLOG(1) << "       " << res->reason;
src/apps/jsonclient.cpp:77:                    VLOG(1) << "     Headers:";
src/apps/jsonclient.cpp:79:                        VLOG(1) << "       " << field + " = " + value;
src/apps/jsonclient.cpp:82:                    VLOG(1) << "     Cookies:";
src/apps/jsonclient.cpp:84:                        VLOG(1) << "       " + name + " = " + cookie.getValue();
src/apps/jsonclient.cpp:86:                            VLOG(1) << "         " + option + " = " + value;
src/apps/jsonclient.cpp:91:                    VLOG(1) << "     Body:\n----------- start body -----------" << res->body.data() << "------------ end body ------------";
src/apps/jsonclient.cpp:94:                    VLOG(1) << "legacy: Request parse error: " << message;
src/apps/jsonclient.cpp:98:            LOG(INFO) << " -- OnRequestEnd";
src/apps/jsonclient.cpp:108:                                   VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
src/apps/jsonclient.cpp:111:                                   VLOG(1) << instanceName << ": disabled";
src/apps/jsonclient.cpp:114:                                   LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/jsonclient.cpp:117:                                   LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/jsonclient.cpp:129:                                       VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
src/apps/jsonclient.cpp:132:                                       VLOG(1) << instanceName << ": disabled";
src/apps/jsonclient.cpp:135:                                       LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/jsonclient.cpp:138:                                       LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/jsonclient.cpp:146:                PLOG(ERROR) << "OnError: " << err;
src/apps/warema-jalousien.cpp:74:        VLOG(1) << "Param: " << req->param("id");
src/apps/warema-jalousien.cpp:75:        VLOG(1) << "Qurey: " << req->query("action");
src/apps/warema-jalousien.cpp:106:                              VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/apps/warema-jalousien.cpp:109:                              VLOG(1) << instanceName << ": disabled";
src/apps/warema-jalousien.cpp:112:                              LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/warema-jalousien.cpp:115:                              LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httpserver.cpp:88:        VLOG(1) << "Routes:";
src/apps/http/httpserver.cpp:92:            VLOG(1) << "  " << route;
src/apps/http/httpserver.cpp:99:                    VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/apps/http/httpserver.cpp:102:                    VLOG(1) << instanceName << ": disabled";
src/apps/http/httpserver.cpp:105:                    VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httpserver.cpp:108:                    VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httpserver.cpp:141:core::socket::State& state) { // titan #endif if (errnum != 0) { PLOG(FATAL) << "listen"; } else { VLOG(1) << "snode.c listening on
src/apps/http/testbasicauthentication.cpp:100:                                    VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/apps/http/testbasicauthentication.cpp:103:                                    VLOG(1) << instanceName << ": disabled";
src/apps/http/testbasicauthentication.cpp:106:                                    LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/testbasicauthentication.cpp:109:                                    LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/testbasicauthentication.cpp:136:                VLOG(1) << "tls: listening on '" << socketAddress.toString() << "'"
src/apps/http/testbasicauthentication.cpp:140:                VLOG(1) << "tls: disabled";
src/apps/http/testbasicauthentication.cpp:143:                VLOG(1) << "tls: error occurred";
src/apps/http/testbasicauthentication.cpp:146:                VLOG(1) << "tls: fatal error occurred";
src/apps/http/vhostserver.cpp:124:                                     VLOG(1) << instanceName << " listening on '" << socketAddress.toString() << "'";
src/apps/http/vhostserver.cpp:127:                                     VLOG(1) << instanceName << " disabled";
src/apps/http/vhostserver.cpp:130:                                     LOG(ERROR) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/http/vhostserver.cpp:133:                                     LOG(FATAL) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/http/vhostserver.cpp:193:                                  VLOG(1) << instanceName << " listening on '" << socketAddress.toString() << "'";
src/apps/http/vhostserver.cpp:196:                                  VLOG(1) << instanceName << " disabled";
src/apps/http/vhostserver.cpp:199:                                  LOG(ERROR) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/http/vhostserver.cpp:202:                                  LOG(FATAL) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/http/testexpressnext.cpp:102:                    LOG(ERROR) << "FAIL: connect failed: " << state.what();
src/apps/http/testexpressnext.cpp:124:            LOG(INFO) << "All express next() tests executed. failures=" << failures;
src/apps/http/testexpressnext.cpp:138:            LOG(ERROR) << "FAIL: master request not connected";
src/apps/http/testexpressnext.cpp:161:                    LOG(INFO) << "PASS: " << current.name << " status=" << res->statusCode << " body='" << body << "'";
src/apps/http/testexpressnext.cpp:164:                    LOG(ERROR) << "FAIL: " << current.name << " expected status=" << current.expectedStatus << " expected body fragment='"
src/apps/http/testexpressnext.cpp:173:                LOG(ERROR) << "FAIL: " << current.name << " parse-error: " << reason;
src/apps/http/testexpressnext.cpp:304:                VLOG(1) << "testexpressnext listening on '" << socketAddress.toString() << "'";
src/apps/http/testexpressnext.cpp:307:                VLOG(1) << "testexpressnext disabled";
src/apps/http/testexpressnext.cpp:310:                LOG(ERROR) << "testexpressnext " << socketAddress.toString() << ": " << state.what();
src/apps/http/testexpressnext.cpp:313:                LOG(FATAL) << "testexpressnext " << socketAddress.toString() << ": " << state.what();
src/apps/http/testexpressnext.cpp:327:        LOG(ERROR) << "testexpressnext finished with failures=" << nextTester.getFailures();
src/apps/http/testexpressnext.cpp:331:    LOG(INFO) << "testexpressnext finished successfully";
src/apps/http/httpclient.cpp:63:                VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
src/apps/http/httpclient.cpp:66:                VLOG(1) << instanceName << ": disabled";
src/apps/http/httpclient.cpp:69:                VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httpclient.cpp:72:                VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httpclient.cpp:104:core::socket::State& state) { #endif if (errnum != 0) { PLOG(ERROR) << "OnError: " << errnum; } else { VLOG(1) << "snode.c
src/apps/http/model/clients.h:72:    VLOG(1) << req->getConnectionName() << " HTTP response: " << req->method << " " << req->url << " HTTP/" << req->httpMajor << "."
src/apps/http/model/clients.h:100:                VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName() << ": OnRequestStart";
src/apps/http/model/clients.h:124:                            VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName()
src/apps/http/model/clients.h:126:                            VLOG(1) << "  /home/voc/projects/snodec/snode.c/CMakeLists.tt";
src/apps/http/model/clients.h:128:                            LOG(ERROR) << req->getSocketContext()->getSocketConnection()->getConnectionName()
src/apps/http/model/clients.h:130:                            PLOG(ERROR) << "  /home/voc/projects/snodec/snode.c/CMakeLists.tt";
src/apps/http/model/clients.h:299:                        VLOG(0) << "OnOpen 1";
src/apps/http/model/clients.h:303:                        VLOG(0) << "OnError 1";
src/apps/http/model/clients.h:307:                        VLOG(0) << "OnMessage 1:1: " << message.data;
src/apps/http/model/clients.h:310:                        VLOG(0) << "OnMessage 1:2: " << message.data;
src/apps/http/model/clients.h:313:                        VLOG(0) << "EventListener for 'myevent' 1:1: " << message.lastEventId << " : " << message.data;
src/apps/http/model/clients.h:316:                        VLOG(0) << "EventListener for 'myevent' 1:2: " << message.lastEventId << " : " << message.data;
src/apps/http/model/clients.h:330:                        VLOG(0) << "OnOpen 2";
src/apps/http/model/clients.h:334:                        VLOG(0) << "OnError 2";
src/apps/http/model/clients.h:338:                        VLOG(0) << "OnMessage 2:1: " << message.data;
src/apps/http/model/clients.h:341:                        VLOG(0) << "OnMessage 2:2: " << message.data;
src/apps/http/model/clients.h:344:                        VLOG(0) << "EventListener for 'myevent' 2:1: " << message.lastEventId << " : " << message.data;
src/apps/http/model/clients.h:347:                        VLOG(0) << "EventListener for 'myevent' 2:2: " << message.lastEventId << " : " << message.data;
src/apps/http/model/clients.h:370:                        VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName()
src/apps/http/model/clients.h:372:                        VLOG(1) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";
src/apps/http/model/clients.h:374:                        LOG(ERROR) << req->getSocketContext()->getSocketConnection()->getConnectionName()
src/apps/http/model/clients.h:376:                        PLOG(ERROR) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";
src/apps/http/model/clients.h:390:                                VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName()
src/apps/http/model/clients.h:392:                                VLOG(1) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";
src/apps/http/model/clients.h:394:                                LOG(ERROR) << req->getSocketContext()->getSocketConnection()->getConnectionName()
src/apps/http/model/clients.h:396:                                PLOG(ERROR) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";
src/apps/http/model/clients.h:426:                            VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName()
src/apps/http/model/clients.h:428:                            VLOG(1) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";
src/apps/http/model/clients.h:430:                            LOG(ERROR) << req->getSocketContext()->getSocketConnection()->getConnectionName()
src/apps/http/model/clients.h:432:                            PLOG(ERROR) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";
src/apps/http/model/clients.h:451:                            VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName()
src/apps/http/model/clients.h:453:                            VLOG(1) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";
src/apps/http/model/clients.h:455:                            LOG(ERROR) << req->getSocketContext()->getSocketConnection()->getConnectionName()
src/apps/http/model/clients.h:457:                            PLOG(ERROR) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";
src/apps/http/model/clients.h:470:                VLOG(1) << req->getConnectionName() << ": OnRequestEnd";
src/apps/http/model/clients.h:474:            VLOG(1) << socketConnection->getConnectionName() << ": OnConnect";
src/apps/http/model/clients.h:476:            VLOG(1) << "\tLocal: " << socketConnection->getLocalAddress().toString();
src/apps/http/model/clients.h:477:            VLOG(1) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
src/apps/http/model/clients.h:481:            VLOG(1) << socketConnection->getConnectionName() << ": OnDisconnect";
src/apps/http/model/clients.h:483:            VLOG(1) << "\tLocal: " << socketConnection->getLocalAddress().toString();
src/apps/http/model/clients.h:484:            VLOG(1) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
src/apps/http/model/clients.h:508:                VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName() << ": OnRequestStart";
src/apps/http/model/clients.h:656:                VLOG(1) << req->getConnectionName() << ": OnRequestEnd";
src/apps/http/model/clients.h:660:            VLOG(1) << "OnConnect " << socketConnection->getConnectionName();
src/apps/http/model/clients.h:662:            VLOG(1) << "\tLocal: " << socketConnection->getLocalAddress().toString();
src/apps/http/model/clients.h:663:            VLOG(1) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
src/apps/http/model/clients.h:676:            VLOG(1) << socketConnection->getConnectionName() << ": OnConnected";
src/apps/http/model/clients.h:681:                VLOG(1) << "\tPeer certificate verifyErr = " + std::to_string(verifyErr) + ": " +
src/apps/http/model/clients.h:685:                VLOG(1) << "\t   Subject: " + std::string(str);
src/apps/http/model/clients.h:689:                VLOG(1) << "\t   Issuer: " + std::string(str);
src/apps/http/model/clients.h:699:                VLOG(1) << "\t   Subject alternative name count: " << altNameCount;
src/apps/http/model/clients.h:706:                        VLOG(1) << "\t      SAN (URI): '" + subjectAltName;
src/apps/http/model/clients.h:711:                        VLOG(1) << "\t      SAN (DNS): '" + subjectAltName;
src/apps/http/model/clients.h:713:                        VLOG(1) << "\t      SAN (Type): '" + std::to_string(generalName->type);
src/apps/http/model/clients.h:721:                VLOG(1) << "\tPeer certificate: no certificate";
src/apps/http/model/clients.h:726:            VLOG(1) << socketConnection->getConnectionName() << ": OnDisconnect";
src/apps/http/model/clients.h:728:            VLOG(1) << "\tLocal: " << socketConnection->getLocalAddress().toString();
src/apps/http/model/clients.h:729:            VLOG(1) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
src/apps/http/model/servers.h:102:            VLOG(1) << "OnConnect " << instanceName;
src/apps/http/model/servers.h:104:            VLOG(1) << "  Local: " << socketConnection->getLocalAddress().toString();
src/apps/http/model/servers.h:105:            VLOG(1) << "   Peer: " << socketConnection->getRemoteAddress().toString();
src/apps/http/model/servers.h:118:            VLOG(1) << "OnConnected " << instanceName;
src/apps/http/model/servers.h:124:                VLOG(1) << "\tPeer certificate verifyErr = " + std::to_string(verifyErr) + ": " +
src/apps/http/model/servers.h:128:                VLOG(1) << "\t   Subject: " + std::string(str);
src/apps/http/model/servers.h:132:                VLOG(1) << "\t   Issuer: " + std::string(str);
src/apps/http/model/servers.h:142:                VLOG(1) << "\t   Subject alternative name count: " << altNameCount;
src/apps/http/model/servers.h:150:                        VLOG(1) << "\t      SAN (URI): '" + subjectAltName;
src/apps/http/model/servers.h:155:                        VLOG(1) << "\t      SAN (DNS): '" + subjectAltName;
src/apps/http/model/servers.h:157:                        VLOG(1) << "\t      SAN (Type): '" + std::to_string(generalName->type);
src/apps/http/model/servers.h:165:                LOG(WARNING) << "\tPeer certificate: no certificate";
src/apps/http/model/servers.h:170:            VLOG(1) << "OnDisconnect " << instanceName;
src/apps/http/model/servers.h:172:            VLOG(2) << "            Local: " << socketConnection->getLocalAddress().toString(false);
src/apps/http/model/servers.h:173:            VLOG(2) << "             Peer: " << socketConnection->getRemoteAddress().toString(false);
src/apps/http/model/servers.h:175:            VLOG(2) << "     Online Since: " << socketConnection->getOnlineSince();
src/apps/http/model/servers.h:176:            VLOG(2) << "  Online Duration: " << socketConnection->getOnlineDuration();
src/apps/http/model/servers.h:178:            VLOG(2) << "     Total Queued: " << socketConnection->getTotalQueued();
src/apps/http/model/servers.h:179:            VLOG(2) << "       Total Sent: " << socketConnection->getTotalSent();
src/apps/http/model/servers.h:180:            VLOG(2) << "      Write Delta: " << socketConnection->getTotalQueued() - socketConnection->getTotalSent();
src/apps/http/model/servers.h:181:            VLOG(2) << "       Total Read: " << socketConnection->getTotalRead();
src/apps/http/model/servers.h:182:            VLOG(2) << "  Total Processed: " << socketConnection->getTotalProcessed();
src/apps/http/model/servers.h:183:            VLOG(2) << "       Read Delta: " << socketConnection->getTotalRead() - socketConnection->getTotalProcessed();
src/apps/http/httplowlevelclient.cpp:72:                VLOG(1) << "++   OnStarted";
src/apps/http/httplowlevelclient.cpp:75:                VLOG(1) << "++   OnParsed";
src/apps/http/httplowlevelclient.cpp:78:                VLOG(1) << "++   OnError: " + std::to_string(status) + " - " + reason;
src/apps/http/httplowlevelclient.cpp:94:            VLOG(1) << "SimpleSocketProtocol connected";
src/apps/http/httplowlevelclient.cpp:97:            VLOG(1) << "SimpleSocketProtocol disconnected";
src/apps/http/httplowlevelclient.cpp:151:                VLOG(1) << "OnConnect";
src/apps/http/httplowlevelclient.cpp:153:                VLOG(1) << "\tServer: " << socketConnection->getRemoteAddress().toString();
src/apps/http/httplowlevelclient.cpp:154:                VLOG(1) << "\tClient: " << socketConnection->getLocalAddress().toString();
src/apps/http/httplowlevelclient.cpp:166:                VLOG(1) << "OnConnected";
src/apps/http/httplowlevelclient.cpp:172:                    VLOG(1) << "     Server certificate: " + std::string(X509_verify_cert_error_string(verifyErr));
src/apps/http/httplowlevelclient.cpp:175:                    VLOG(1) << "        Subject: " + std::string(str);
src/apps/http/httplowlevelclient.cpp:179:                    VLOG(1) << "        Issuer: " + std::string(str);
src/apps/http/httplowlevelclient.cpp:189:                    VLOG(1) << "\t   Subject alternative name count: " << altNameCount;
src/apps/http/httplowlevelclient.cpp:196:                            VLOG(1) << "\t      SAN (URI): '" + subjectAltName;
src/apps/http/httplowlevelclient.cpp:201:                            VLOG(1) << "\t      SAN (DNS): '" + subjectAltName;
src/apps/http/httplowlevelclient.cpp:203:                            VLOG(1) << "\t      SAN (Type): '" + std::to_string(generalName->type);
src/apps/http/httplowlevelclient.cpp:211:                    VLOG(1) << "     Server certificate: no certificate";
src/apps/http/httplowlevelclient.cpp:217:                VLOG(1) << "OnDisconnect";
src/apps/http/httplowlevelclient.cpp:219:                VLOG(1) << "\tServer: " + socketConnection->getRemoteAddress().toString();
src/apps/http/httplowlevelclient.cpp:220:                VLOG(1) << "\tClient: " + socketConnection->getLocalAddress().toString();
src/apps/http/httplowlevelclient.cpp:232:                                      VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
src/apps/http/httplowlevelclient.cpp:235:                                      VLOG(1) << instanceName << ": disabled";
src/apps/http/httplowlevelclient.cpp:238:                                      LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httplowlevelclient.cpp:241:                                      LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httplowlevelclient.cpp:260:                VLOG(1) << "OnConnect";
src/apps/http/httplowlevelclient.cpp:262:                VLOG(1) << "\tServer: " << socketConnection->getRemoteAddress().toString();
src/apps/http/httplowlevelclient.cpp:263:                VLOG(1) << "\tClient: " << socketConnection->getLocalAddress().toString();
src/apps/http/httplowlevelclient.cpp:266:                VLOG(1) << "OnConnected";
src/apps/http/httplowlevelclient.cpp:271:                VLOG(1) << "OnDisconnect";
src/apps/http/httplowlevelclient.cpp:273:                VLOG(1) << "\tServer: " << socketConnection->getRemoteAddress().toString();
src/apps/http/httplowlevelclient.cpp:274:                VLOG(1) << "\tClient: " << socketConnection->getLocalAddress().toString();
src/apps/http/httplowlevelclient.cpp:281:        VLOG(1) << "###############': " << remoteAddress.getCanonName();
src/apps/http/httplowlevelclient.cpp:282:        VLOG(1) << "###############': " << remoteAddress.toString();
src/apps/http/httplowlevelclient.cpp:290:                                         VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
src/apps/http/httplowlevelclient.cpp:293:                                         VLOG(1) << instanceName << ": disabled";
src/apps/http/httplowlevelclient.cpp:296:                                         LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httplowlevelclient.cpp:299:                                         LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httplowlevelclient.cpp:323:                                         VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
src/apps/http/httplowlevelclient.cpp:326:                                         VLOG(1) << instanceName << ": disabled";
src/apps/http/httplowlevelclient.cpp:329:                                         LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httplowlevelclient.cpp:332:                                         LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httplowlevelclient.cpp:347:                                      VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
src/apps/http/httplowlevelclient.cpp:350:                                      VLOG(1) << instanceName << ": disabled";
src/apps/http/httplowlevelclient.cpp:353:                                      LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httplowlevelclient.cpp:356:                                      LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/verysimpleserver.cpp:72:                                 VLOG(1) << instanceName << " listening on '" << socketAddress.toString() << "'";
src/apps/http/verysimpleserver.cpp:75:                                 VLOG(1) << instanceName << " disabled";
src/apps/http/verysimpleserver.cpp:78:                                 LOG(ERROR) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/http/verysimpleserver.cpp:81:                                 LOG(FATAL) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/http/verysimpleserver.cpp:104:                    VLOG(1) << instanceName << " listening on '" << socketAddress.toString() << "'";
src/apps/http/verysimpleserver.cpp:107:                    VLOG(1) << instanceName << " disabled";
src/apps/http/verysimpleserver.cpp:110:                    LOG(ERROR) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/http/verysimpleserver.cpp:113:                    LOG(FATAL) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/configtest.cpp:85:    VLOG(1) << "Filename: " << filename;
src/apps/testregex.cpp:107:                VLOG(1) << "Move on to the next route to query database";
src/apps/testregex.cpp:111:                VLOG(1) << "UserId: " << req->params["userId"];
src/apps/testregex.cpp:162:                            VLOG(1) << "Move on to the next route to send result";
src/apps/testregex.cpp:167:                        VLOG(1) << "Error: " << errorString << " : " << errorNumber;
src/apps/testregex.cpp:172:                VLOG(1) << "And again 1: Move on to the next route to send result";
src/apps/testregex.cpp:176:                VLOG(1) << "And again 2: Move on to the next route to send result";
src/apps/testregex.cpp:180:            VLOG(1) << "And again 3: Move on to the next route to send result";
src/apps/testregex.cpp:184:            VLOG(1) << "SendResult";
src/apps/testregex.cpp:195:        VLOG(1) << "Show account of";
src/apps/testregex.cpp:196:        VLOG(1) << "UserId: " << req->params["userId"];
src/apps/testregex.cpp:197:        VLOG(1) << "UserName: " << req->params["userName"];
src/apps/testregex.cpp:222:                VLOG(1) << "Inserted: -> " << userId << " - " << userName;
src/apps/testregex.cpp:225:                VLOG(1) << "Error: " << errorString << " : " << errorNumber;
src/apps/testregex.cpp:231:        VLOG(1) << "Testing Regex";
src/apps/testregex.cpp:232:        VLOG(1) << "Regex1: " << req->params["testRegex1"];
src/apps/testregex.cpp:233:        VLOG(1) << "Regex2: " << req->params["testRegex2"];
src/apps/testregex.cpp:255:        VLOG(1) << "Show Search of";
src/apps/testregex.cpp:256:        VLOG(1) << "Search: " << req->params["search"];
src/apps/testregex.cpp:257:        VLOG(1) << "Queries: " << req->query("test");
src/apps/testregex.cpp:290:            VLOG(0) << "MySQL error: " << state.errorMessage << " [" << state.error << "]";
src/apps/testregex.cpp:292:            VLOG(0) << "MySQL connected";
src/apps/testregex.cpp:294:            VLOG(0) << "MySQL disconnected";
src/apps/testregex.cpp:306:                    VLOG(1) << "legacy-testregex: listening on '" << socketAddress.toString() << "'";
src/apps/testregex.cpp:309:                    VLOG(1) << "legacy-testregex: disabled";
src/apps/testregex.cpp:312:                    VLOG(1) << "legacy-testregex: error occurred";
src/apps/testregex.cpp:315:                    VLOG(1) << "legacy-testregex: fatal error occurred";
src/apps/testregex.cpp:321:            VLOG(1) << "OnConnect:";
src/apps/testregex.cpp:323:            VLOG(1) << "\tServer: " + socketConnection->getRemoteAddress().toString();
src/apps/testregex.cpp:324:            VLOG(1) << "\tClient: " + socketConnection->getLocalAddress().toString();
src/apps/testregex.cpp:328:            VLOG(1) << "OnDisconnect:";
src/apps/testregex.cpp:330:            VLOG(1) << "\tServer: " + socketConnection->getRemoteAddress().toString();
src/apps/testregex.cpp:331:            VLOG(1) << "\tClient: " + socketConnection->getLocalAddress().toString();
src/apps/testregex.cpp:341:                    VLOG(1) << "tls-testregex: listening on '" << socketAddress.toString() << "'";
src/apps/testregex.cpp:344:                    VLOG(1) << "tls-testregex: disabled";
src/apps/testregex.cpp:347:                    VLOG(1) << "tls-testregex: error occurred";
src/apps/testregex.cpp:350:                    VLOG(1) << "tls-testregex: fatal error occurred";
src/apps/testregex.cpp:356:            VLOG(1) << "OnConnect:";
src/apps/testregex.cpp:358:            VLOG(1) << "\tServer: " + socketConnection->getRemoteAddress().toString();
src/apps/testregex.cpp:359:            VLOG(1) << "\tClient: " + socketConnection->getLocalAddress().toString();
src/apps/testregex.cpp:363:            VLOG(1) << "OnConnected:";
src/apps/testregex.cpp:370:                VLOG(1) << "\tClient certificate: " + std::string(X509_verify_cert_error_string(verifyErr));
src/apps/testregex.cpp:373:                VLOG(1) << "\t   Subject: " + std::string(str);
src/apps/testregex.cpp:377:                VLOG(1) << "\t   Issuer: " + std::string(str);
src/apps/testregex.cpp:387:                VLOG(1) << "\t   Subject alternative name count: " << altNameCount;
src/apps/testregex.cpp:394:                        VLOG(1) << "\t      SAN (URI): '" + subjectAltName;
src/apps/testregex.cpp:399:                        VLOG(1) << "\t      SAN (DNS): '" + subjectAltName;
src/apps/testregex.cpp:401:                        VLOG(1) << "\t      SAN (Type): '" + std::to_string(generalName->type);
src/apps/testregex.cpp:409:                VLOG(1) << "\tClient certificate: no certificate";
src/apps/testregex.cpp:414:            VLOG(1) << "OnDisconnect:";
src/apps/testregex.cpp:416:            VLOG(1) << "\tServer: " + socketConnection->getRemoteAddress().toString();
src/apps/testregex.cpp:417:            VLOG(1) << "\tClient: " + socketConnection->getLocalAddress().toString();
src/apps/main.cpp:239:                           VLOG(1) << instanceName << " listening on '" << socketAddress.toString() << "'";
src/apps/main.cpp:242:                           VLOG(1) << instanceName << " disabled";
src/apps/main.cpp:245:                           LOG(ERROR) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/main.cpp:248:                           LOG(FATAL) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/tlslegacy/tlslegacyserver.cpp:30:            VLOG(1) << instanceName << ": listening on " << socketAddress.toString();
src/apps/tlslegacy/tlslegacyserver.cpp:32:            LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/tlslegacy/tlslegacyclient.cpp:30:            VLOG(1) << instanceName << ": connected to " << socketAddress.toString();
src/apps/tlslegacy/tlslegacyclient.cpp:32:            LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/tlslegacy/TlsLegacySocketContext.cpp:29:        VLOG(1) << getSocketConnection()->getConnectionName() << ": connected";
src/apps/tlslegacy/TlsLegacySocketContext.cpp:33:            VLOG(1) << getSocketConnection()->getConnectionName() << ": sent TLS greeting";
src/apps/tlslegacy/TlsLegacySocketContext.cpp:39:        VLOG(1) << getSocketConnection()->getConnectionName() << ": disconnected";
src/apps/tlslegacy/TlsLegacySocketContext.cpp:57:                VLOG(1) << getSocketConnection()->getConnectionName() << ": trying post-TLS legacy payload: " << payload;
src/apps/tlslegacy/TlsLegacySocketContext.cpp:65:            VLOG(1) << getSocketConnection()->getConnectionName() << ": got TLS ack, initiating TLS shutdown handshake (close_notify) "
src/apps/tlslegacy/TlsLegacySocketContext.cpp:72:            VLOG(1) << getSocketConnection()->getConnectionName() << ": got LEGACY ack -> post-TLS plaintext path works " << line;
src/apps/tlslegacy/TlsLegacySocketContext.cpp:81:            VLOG(1) << getSocketConnection()->getConnectionName() << ": TLS phase complete, waiting for peer close_notify " << line;
src/apps/tlslegacy/TlsLegacySocketContext.cpp:86:            VLOG(1) << getSocketConnection()->getConnectionName() << ": received LEGACY payload after TLS shutdown " << line;
src/apps/express_compat_server.cpp:240:                VLOG(1) << "express-compat listening on '" << socketAddress.toString() << "'";
src/apps/express_compat_server.cpp:243:                VLOG(1) << "express-compat disabled";
src/apps/express_compat_server.cpp:246:                LOG(ERROR) << "express-compat " << socketAddress.toString() << ": " << state.what();
src/apps/express_compat_server.cpp:249:                LOG(FATAL) << "express-compat " << socketAddress.toString() << ": " << state.what();
src/net/config/stream/tls/ConfigSocketServer.hpp:103:                LOG(TRACE) << getInstanceName() << " SSL/TLS: SSL_CTX (M) sni for '" << sni << "' from master certificate installed";
src/net/config/stream/tls/ConfigSocketServer.hpp:140:                        LOG(TRACE) << getInstanceName() << " SSL/TLS: SSL_CTX (E) sni for '" << domain << "' explicitly installed";
src/net/config/stream/tls/ConfigSocketServer.hpp:145:                            LOG(TRACE) << getInstanceName() << " SSL/TLS: SSL_CTX (S) sni for '" << san << "' from SAN installed";
src/net/config/stream/tls/ConfigSocketServer.hpp:148:                        LOG(WARNING) << getInstanceName() << " SSL/TLS: Can not create SNI_SSL_CTX for domain '" << domain << "'";
src/net/config/stream/tls/ConfigSocketServer.hpp:152:            LOG(TRACE) << getInstanceName() << " SSL/TLS: SNI list result:";
src/net/config/stream/tls/ConfigSocketServer.hpp:154:                LOG(TRACE) << "  " << sni;
src/net/config/stream/tls/ConfigSocketServer.hpp:163:        LOG(TRACE) << getInstanceName() << " SSL/TLS SNI: Lookup for sni='" << serverNameIndication << "' in sni certificates";
src/net/config/stream/tls/ConfigSocketServer.hpp:169:                LOG(TRACE) << getInstanceName() << " SSL/TLS SNI:  .. " << sniPair.first.c_str();
src/net/config/stream/tls/ConfigSocketServer.hpp:174:            LOG(TRACE) << getInstanceName() << " SSL/TLS SNI: found for " << serverNameIndication << " -> '" << sniPairIt->first << "'";
src/net/config/stream/tls/ConfigSocketServer.hpp:177:            LOG(WARNING) << getInstanceName() << " SSL/TL SNI: not found for " << serverNameIndication;
src/net/config/ConfigPhysicalSocket.cpp:86:                           LOG(ERROR) << err.what();
src/core/socket/stream/SocketAcceptor.hpp:107:                LOG(DEBUG) << config->getInstanceName() << " Listen: starting";
src/core/socket/stream/SocketAcceptor.hpp:112:                    PLOG(ERROR) << config->getInstanceName() << " open " << configuredAddress.toString();
src/core/socket/stream/SocketAcceptor.hpp:126:                    LOG(DEBUG) << config->getInstanceName() << " open " << configuredAddress.toString() << ": success";
src/core/socket/stream/SocketAcceptor.hpp:129:                        PLOG(ERROR) << config->getInstanceName() << " bind " << configuredAddress.toString();
src/core/socket/stream/SocketAcceptor.hpp:147:                        LOG(DEBUG) << config->getInstanceName() << " bind " << configuredAddressString
src/core/socket/stream/SocketAcceptor.hpp:154:                            PLOG(ERROR) << config->getInstanceName() << " listen " << physicalServerSocket.getBindAddress().toString();
src/core/socket/stream/SocketAcceptor.hpp:165:                            LOG(DEBUG) << config->getInstanceName() << " listen " << physicalServerSocket.getBindAddress().toString() << ": success";
src/core/socket/stream/SocketAcceptor.hpp:168:                                LOG(DEBUG) << config->getInstanceName() << " enable " << physicalServerSocket.getBindAddress().toString() << ": success";
src/core/socket/stream/SocketAcceptor.hpp:170:                                LOG(ERROR) << config->getInstanceName() << " enable " << physicalServerSocket.getBindAddress().toString()
src/core/socket/stream/SocketAcceptor.hpp:183:                    LOG(INFO) << config->getInstanceName()
src/core/socket/stream/SocketAcceptor.hpp:194:                LOG(ERROR) << state.what();
src/core/socket/stream/SocketAcceptor.hpp:199:            LOG(DEBUG) << config->getInstanceName() << ": disabled";
src/core/socket/stream/SocketAcceptor.hpp:226:                    LOG(DEBUG) << config->getInstanceName() << " accept " << physicalServerSocket.getBindAddress().toString() << ": success";
src/core/socket/stream/SocketAcceptor.hpp:227:                    LOG(DEBUG) << "  " << socketConnection->getRemoteAddress().toString() << " -> "
src/core/socket/stream/SocketAcceptor.hpp:233:                    PLOG(WARNING) << config->getInstanceName() << " accept " << physicalServerSocket.getBindAddress().toString();
src/core/socket/stream/SocketWriter.cpp:119:                LOG(TRACE) << getName() << ": Shutdown restart";
src/core/socket/stream/SocketWriter.cpp:145:                LOG(WARNING) << getName() << ": Send while not enabled";
src/core/socket/stream/SocketWriter.cpp:148:            LOG(WARNING) << getName() << ": Send while shutdown in progress: ignoring";
src/core/socket/stream/SocketWriter.cpp:160:                    LOG(TRACE) << getName() << ": Stream started";
src/core/socket/stream/SocketWriter.cpp:162:                    LOG(WARNING) << getName() << ": Stream source is nullptr";
src/core/socket/stream/SocketWriter.cpp:165:                LOG(WARNING) << getName() << ": Stream while not enabled";
src/core/socket/stream/SocketWriter.cpp:168:            LOG(WARNING) << getName() << ": Stream while shutdown in progress";
src/core/socket/stream/SocketWriter.cpp:177:        LOG(TRACE) << getName() << ": Stream EOF";
src/core/socket/stream/SocketWriter.cpp:187:                LOG(TRACE) << getName() << ": Shutdown start";
src/core/socket/stream/SocketWriter.cpp:191:                LOG(TRACE) << getName() << ": Shutdown delayed due to queued data";
src/core/socket/stream/SocketConnection.hpp:67:                LOG(TRACE) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
src/core/socket/stream/SocketConnection.hpp:70:                LOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
src/core/socket/stream/SocketConnection.hpp:74:            PLOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
src/core/socket/stream/SocketConnection.hpp:90:                LOG(TRACE) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
src/core/socket/stream/SocketConnection.hpp:93:                LOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
src/core/socket/stream/SocketConnection.hpp:97:            PLOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
src/core/socket/stream/SocketConnector.hpp:106:                LOG(DEBUG) << config->getInstanceName() << " Connect: starting";
src/core/socket/stream/SocketConnector.hpp:114:                        PLOG(DEBUG) << config->getInstanceName() << " open " << configuredLocalAddress.toString();
src/core/socket/stream/SocketConnector.hpp:130:                        LOG(TRACE) << config->getInstanceName() << " open " << configuredLocalAddress.toString() << ": success";
src/core/socket/stream/SocketConnector.hpp:133:                            PLOG(DEBUG) << config->getInstanceName() << " bind " << configuredLocalAddress.toString();
src/core/socket/stream/SocketConnector.hpp:149:                            LOG(TRACE) << config->getInstanceName() << " bind " << configuredLocalAddressString
src/core/socket/stream/SocketConnector.hpp:156:                                PLOG(DEBUG) << config->getInstanceName() << " connect " << remoteAddress.toString();
src/core/socket/stream/SocketConnector.hpp:175:                                    LOG(INFO) << config->getInstanceName() << ": Using next SocketAddress: " << remoteAddress.toString();
src/core/socket/stream/SocketConnector.hpp:182:                                LOG(TRACE) << config->getInstanceName() << " connect " << remoteAddress.toString() << ": success";
src/core/socket/stream/SocketConnector.hpp:186:                                        LOG(DEBUG)
src/core/socket/stream/SocketConnector.hpp:189:                                        LOG(ERROR) << config->getInstanceName() << " enable " << remoteAddress.toString()
src/core/socket/stream/SocketConnector.hpp:200:                                    LOG(DEBUG) << config->getInstanceName() << " connect " << remoteAddress.toString() << ": success";
src/core/socket/stream/SocketConnector.hpp:201:                                    LOG(DEBUG) << "  " << socketConnection->getLocalAddress().toString() << " -> "
src/core/socket/stream/SocketConnector.hpp:216:                    LOG(ERROR) << state.what();
src/core/socket/stream/SocketConnector.hpp:224:                LOG(ERROR) << state.what();
src/core/socket/stream/SocketConnector.hpp:229:            LOG(DEBUG) << config->getInstanceName() << ": disabled";
src/core/socket/stream/SocketConnector.hpp:254:                LOG(DEBUG) << config->getInstanceName() << " connect " << remoteAddress.toString() << ": success";
src/core/socket/stream/SocketConnector.hpp:255:                LOG(DEBUG) << "  " << socketConnection->getLocalAddress().toString() << " -> "
src/core/socket/stream/SocketConnector.hpp:265:                LOG(DEBUG) << config->getInstanceName() << " connect " << remoteAddress.toString() << ": in progress:";
src/core/socket/stream/SocketConnector.hpp:286:                    PLOG(DEBUG) << config->getInstanceName() << " connect '" << remoteAddress.toString();
src/core/socket/stream/SocketConnector.hpp:290:                    LOG(DEBUG) << config->getInstanceName()
src/core/socket/stream/SocketConnector.hpp:297:                    PLOG(DEBUG) << config->getInstanceName() << " connect " << remoteAddress.toString();
src/core/socket/stream/SocketConnector.hpp:305:            PLOG(DEBUG) << config->getInstanceName() << " getsockopt syscall error: '" << remoteAddress.toString() << "'";
src/core/socket/stream/SocketConnector.hpp:323:        LOG(TRACE) << config->getInstanceName() << " connect timeout " << remoteAddress.toString();
src/core/socket/stream/SocketConnector.hpp:327:            LOG(DEBUG) << config->getInstanceName() << " using next SocketAddress: '" << config->Remote::getSocketAddress().toString()
src/core/socket/stream/SocketConnector.hpp:332:            LOG(DEBUG) << config->getInstanceName() << " connect timeout '" << remoteAddress.toString() << "'";
src/core/DynamicLoader.cpp:87:            LOG(TRACE) << "DynLoader dlOpen: " << lib.fileName << ": already open (refCount=" << lib.refCount << ")";
src/core/DynamicLoader.cpp:105:                LOG(TRACE) << "DynLoader dlOpen: " << libFile << ": success";
src/core/DynamicLoader.cpp:107:                LOG(TRACE) << "DynLoader dlOpen: " << libFile << ": " << DynamicLoader::dlError();
src/core/DynamicLoader.cpp:116:            LOG(TRACE) << "DynLoader dlCloseDelayed: handle is nullptr";
src/core/DynamicLoader.cpp:120:                LOG(TRACE) << "DynLoader dlCloseDelayed: " << handle << ": not opened using dlOpen";
src/core/DynamicLoader.cpp:124:                    LOG(TRACE) << "DynLoader: dlCloseDelayed: internal error: handle known but library record missing";
src/core/DynamicLoader.cpp:135:                        LOG(TRACE) << "DynLoader dlCloseDelayed: " << lib.fileName;
src/core/DynamicLoader.cpp:137:                        LOG(TRACE) << "DynLoader dlCloseDelayed: " << lib.fileName << ": still referenced (refCount=" << lib.refCount
src/core/DynamicLoader.cpp:149:            LOG(TRACE) << "DynLoader dlClose: handle is nullptr";
src/core/DynamicLoader.cpp:153:                LOG(TRACE) << "DynLoader dlClose: " << handle << ": not opened using dlOpen";
src/core/DynamicLoader.cpp:157:                    LOG(TRACE) << "DynLoader dlClose: internal error: handle known but library record missing";
src/core/DynamicLoader.cpp:166:                        LOG(TRACE) << "DynLoader dlClose: " << lib.fileName << ": still referenced (refCount=" << lib.refCount << ")";
src/core/DynamicLoader.cpp:202:            LOG(TRACE) << "DynLoader dlClose: " << DynamicLoader::dlError();
src/core/DynamicLoader.cpp:204:            LOG(TRACE) << "DynLoader dlClose: " << library.fileName << ": success";

```

## src/log infrastructure inventory command/result

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" \
  src/log \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

Result: 6 logging infrastructure macro definitions; intentionally not migrated.

```text
src/log/Logger.h:144:#define LOG(level)                                                                                                                         \
src/log/Logger.h:148:#define PLOG(level)                                                                                                                        \
src/log/Logger.h:153:#define LOG(level)                                                                                                                         \
src/log/Logger.h:157:#define PLOG(level)                                                                                                                        \
src/log/Logger.h:164:#define VLOG(level)                                                                                                                        \
src/log/Logger.h:169:#define VLOG(level)                                                                                                                        \

```

## Focused inventory classification table

| Class | Result |
| --- | --- |
| A. DB / MariaDB framework code | `src/database/mariadb/MariaDBLibrary.cpp`, `src/database/mariadb/MariaDBConnection.cpp` LOG call sites migrated to `db.mariadb`. |
| B. TLS config / SNI / certificate configuration code | `src/net/config/stream/tls/ConfigSocketServer.hpp` inspected and migrated to `net.config.tls`; no TLS config LOG/PLOG deferral remains. |
| C. Application code under apps | No top-level `apps`; `src/apps/**` LOG/PLOG call sites migrated to `app`. |
| D. Example code under examples | No top-level `examples` directory in this checkout. |
| E. Leftovers in previously migrated framework areas | Core socket, express, web/http, dynamic loader and config leftovers migrated as final cleanup. |
| F. PLOG / errno-sensitive call sites | PLOG call sites migrated through `snode::semantic::sysError(...)`, whose constructor captures `errno` immediately at the migrated expression. |
| G. VLOG / verbose diagnostics | 531 VLOG call sites remain deferred to preserve numeric verbose-level semantics. |
| H. logging infrastructure only | `src/log/Logger.h` macro definitions excluded. |
| I. deferred/unclear call sites | Only VLOG call sites deferred; no suitable LOG/PLOG remains outside `src/log`. |

## Ownership discovery summary

Ownership discovery command was run:

```sh
rg -n "\blog\(\)|logger::|BoundaryLogger|LogScopeOwner|semantic|scope|getInstanceName|getConnectionName|MariaDb|Database|Storage|Config|TLS|SNI|Certificate|Application|App|Example|Server|Client" \
  src apps examples \
  -g '*.h' -g '*.hpp' -g '*.cpp' \
  -g '!src/log/**'
```

The existing migrations established `BoundaryLogger`, `LogScopeOwner`, `LogManager`, and helper-header patterns. Migration 9 uses a small `src/SemanticLog.h` final-cleanup helper with stable scopes instead of changing existing object constructors or public APIs.

## Migrated call-site table

All 210 pre-migration `LOG`/`PLOG` entries outside `src/log` were migrated. Complete source list:

```text
src/express/dispatcher/ApplicationDispatcher.cpp:73:        LOG(TRACE) << "======================= APPLICATION DISPATCH =======================";
src/express/dispatcher/ApplicationDispatcher.cpp:74:        LOG(TRACE) << controller.getResponse()->getSocketContext()->getSocketConnection()->getConnectionName();
src/express/dispatcher/ApplicationDispatcher.cpp:75:        LOG(TRACE) << "          Request Method: " << controller.getRequest()->method;
src/express/dispatcher/ApplicationDispatcher.cpp:76:        LOG(TRACE) << "             Request Url: " << controller.getRequest()->url;
src/express/dispatcher/ApplicationDispatcher.cpp:77:        LOG(TRACE) << "            Request Path: " << controller.getRequest()->path;
src/express/dispatcher/ApplicationDispatcher.cpp:78:        LOG(TRACE) << "       Mountpoint Method: " << mountPoint.method;
src/express/dispatcher/ApplicationDispatcher.cpp:79:        LOG(TRACE) << "         Mountpoint Path: " << mountPoint.relativeMountPath;
src/express/dispatcher/ApplicationDispatcher.cpp:80:        LOG(TRACE) << "           StrictRouting: " << strictRouting;
src/express/dispatcher/ApplicationDispatcher.cpp:81:        LOG(TRACE) << "  CaseInsensitiveRouting: " << caseInsensitiveRouting;
src/express/dispatcher/ApplicationDispatcher.cpp:82:        LOG(TRACE) << "             MergeParams: " << mergeParams;
src/express/dispatcher/ApplicationDispatcher.cpp:94:                LOG(TRACE) << "----------------------- APPLICATION    MATCH -----------------------";
src/express/dispatcher/ApplicationDispatcher.cpp:116:                LOG(TRACE) << "----------------------- APPLICATION  NOMATCH -----------------------";
src/express/dispatcher/ApplicationDispatcher.cpp:119:            LOG(TRACE) << "----------------------- APPLICATION  NOMATCH -----------------------";
src/express/dispatcher/MiddlewareDispatcher.cpp:73:        LOG(TRACE) << "======================= MIDDLEWARE  DISPATCH =======================";
src/express/dispatcher/MiddlewareDispatcher.cpp:74:        LOG(TRACE) << controller.getResponse()->getSocketContext()->getSocketConnection()->getConnectionName();
src/express/dispatcher/MiddlewareDispatcher.cpp:75:        LOG(TRACE) << "          Request Method: " << controller.getRequest()->method;
src/express/dispatcher/MiddlewareDispatcher.cpp:76:        LOG(TRACE) << "             Request Url: " << controller.getRequest()->url;
src/express/dispatcher/MiddlewareDispatcher.cpp:77:        LOG(TRACE) << "            Request Path: " << controller.getRequest()->path;
src/express/dispatcher/MiddlewareDispatcher.cpp:78:        LOG(TRACE) << "       Mountpoint Method: " << mountPoint.method;
src/express/dispatcher/MiddlewareDispatcher.cpp:79:        LOG(TRACE) << "         Mountpoint Path: " << mountPoint.relativeMountPath;
src/express/dispatcher/MiddlewareDispatcher.cpp:80:        LOG(TRACE) << "           StrictRouting: " << strictRouting;
src/express/dispatcher/MiddlewareDispatcher.cpp:81:        LOG(TRACE) << "  CaseInsensitiveRouting: " << caseInsensitiveRouting;
src/express/dispatcher/MiddlewareDispatcher.cpp:82:        LOG(TRACE) << "             MergeParams: " << mergeParams;
src/express/dispatcher/MiddlewareDispatcher.cpp:94:                LOG(TRACE) << "----------------------- MIDDLEWARE     MATCH -----------------------";
src/express/dispatcher/MiddlewareDispatcher.cpp:112:                            LOG(TRACE) << "Express: M - Next called - set to NO MATCH";
src/express/dispatcher/MiddlewareDispatcher.cpp:124:                LOG(TRACE) << "----------------------- MIDDLEWARE   NOMATCH -----------------------";
src/express/dispatcher/MiddlewareDispatcher.cpp:127:            LOG(TRACE) << "----------------------- MIDDLEWARE   NOMATCH -----------------------";
src/express/middleware/VerboseRequest.cpp:60:            LOG(DEBUG) << res->getSocketContext()->getSocketConnection()->getConnectionName()
src/express/dispatcher/RouterDispatcher.cpp:72:        LOG(TRACE) << "======================= ROUTER      DISPATCH =======================";
src/express/dispatcher/RouterDispatcher.cpp:73:        LOG(TRACE) << controller.getResponse()->getSocketContext()->getSocketConnection()->getConnectionName();
src/express/dispatcher/RouterDispatcher.cpp:74:        LOG(TRACE) << "          Request Method: " << controller.getRequest()->method;
src/express/dispatcher/RouterDispatcher.cpp:75:        LOG(TRACE) << "             Request Url: " << controller.getRequest()->url;
src/express/dispatcher/RouterDispatcher.cpp:76:        LOG(TRACE) << "            Request Path: " << controller.getRequest()->path;
src/express/dispatcher/RouterDispatcher.cpp:77:        LOG(TRACE) << "       Mountpoint Method: " << mountPoint.method;
src/express/dispatcher/RouterDispatcher.cpp:78:        LOG(TRACE) << "         Mountpoint Path: " << mountPoint.relativeMountPath;
src/express/dispatcher/RouterDispatcher.cpp:79:        LOG(TRACE) << "           StrictRouting: " << this->strictRouting;
src/express/dispatcher/RouterDispatcher.cpp:80:        LOG(TRACE) << "  CaseInsensitiveRouting: " << this->caseInsensitiveRouting;
src/express/dispatcher/RouterDispatcher.cpp:81:        LOG(TRACE) << "             MergeParams: " << this->mergeParams;
src/express/dispatcher/RouterDispatcher.cpp:92:                LOG(TRACE) << "----------------------- ROUTER         MATCH -----------------------";
src/express/dispatcher/RouterDispatcher.cpp:115:                LOG(TRACE) << "----------------------- ROUTER       NOMATCH -----------------------";
src/express/dispatcher/RouterDispatcher.cpp:118:            LOG(TRACE) << "----------------------- ROUTER       NOMATCH -----------------------";
src/express/middleware/StaticMiddleware.cpp:69:                LOG(DEBUG) << res->getSocketContext()->getSocketConnection()->getConnectionName() << " Express " << req->method;
src/express/middleware/StaticMiddleware.cpp:97:                        LOG(INFO) << res->getSocketContext()->getSocketConnection()->getConnectionName()
src/express/middleware/StaticMiddleware.cpp:116:                        LOG(INFO) << res->getSocketContext()->getSocketConnection()->getConnectionName()
src/express/middleware/StaticMiddleware.cpp:119:                        PLOG(ERROR) << res->getSocketContext()->getSocketConnection()->getConnectionName() << " Express StaticMiddleware "
src/apps/testpipe.cpp:85:            PLOG(ERROR) << "Pipe not created";
src/database/mariadb/MariaDBLibrary.cpp:62:                LOG(ERROR) << "MariaDB: mysql_library_init failed (rc=" << rc << ")";
src/database/mariadb/MariaDBConnection.cpp:101:                        LOG(ERROR) << this->connectionName << " MariaDB: Descriptor not registered in SNode.C eventloop";
src/database/mariadb/MariaDBConnection.cpp:106:                LOG(DEBUG) << this->connectionName << " MariaDB connect: success";
src/database/mariadb/MariaDBConnection.cpp:111:                LOG(WARNING) << this->connectionName << " MariaDB connect: error: " << errorString << " : " << errorNumber;
src/database/mariadb/MariaDBConnection.cpp:165:            LOG(DEBUG) << connectionName << " MariaDB start: " << currentCommand->commandInfo();
src/database/mariadb/MariaDBConnection.cpp:191:        LOG(DEBUG) << connectionName << " MariaDB completed: " << currentCommand->commandInfo();
src/database/mariadb/MariaDBConnection.cpp:302:            LOG(ERROR) << connectionName << " MariaDB: Lost connection";
src/web/http/MimeTypes.cpp:249:            LOG(DEBUG) << "HTTP: Cannot load magic database - " + std::string(magic_error(magic));
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:211:                              PLOG(ERROR) << req->url;
src/apps/oauth2/resource_server/ResourceServer.cpp:114:                LOG(INFO) << " -- OnRequestEnd";
src/web/http/SocketContextUpgradeFactorySelector.hpp:100:                        LOG(TRACE) << "HTTP: SocketContextUpgradeFactory create success: " << socketContextUpgradeName;
src/web/http/SocketContextUpgradeFactorySelector.hpp:102:                        LOG(TRACE) << "HTTP: SocketContextUpgradeFactory already existing: " << socketContextUpgradeName;
src/web/http/SocketContextUpgradeFactorySelector.hpp:108:                    LOG(ERROR) << "HTTP: SocketContextUpgradeFactory create failed: " << socketContextUpgradeName;
src/web/http/SocketContextUpgradeFactorySelector.hpp:112:                LOG(ERROR) << "HTTP: Optaining function \"" << socketContextUpgradeFactoryFunctionName
src/web/http/SocketContextUpgradeFactorySelector.hpp:129:            LOG(DEBUG) << "HTTP upgrade: plugin '" << socketContextUpgradeName << "' selected from dynamic cache";
src/web/http/SocketContextUpgradeFactorySelector.hpp:133:            LOG(DEBUG) << "HTTP upgrade: plugin '" << socketContextUpgradeName << "' selected from static cache";
src/web/http/SocketContextUpgradeFactorySelector.hpp:137:            LOG(DEBUG) << "HTTP upgrade: plugin '" << socketContextUpgradeName << "' loaded and added to dynamic cache";
src/web/http/SocketContextUpgradeFactorySelector.hpp:139:            LOG(WARNING) << "HTTP upgrade: plugin '" << socketContextUpgradeName << "' not found";
src/apps/oauth2/client_app/ClientApp.cpp:56:                                  PLOG(ERROR) << req->url;
src/apps/echo/echoclient.cpp:68:                    LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/echo/echoclient.cpp:71:                    LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/echo/echoclient.cpp:124:        PLOG(ERROR) << "OnError";
src/apps/echo/echoclient.cpp:127:        PLOG(ERROR) << "OnError: " << socketAddress.toString();
src/apps/echo/echoserver.cpp:92:                LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/echo/echoserver.cpp:95:                LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/echo/echoserver.cpp:130:            PLOG(FATAL) << "listen";
src/apps/jsonserver.cpp:77:                    LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/jsonserver.cpp:80:                    LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/jsonclient.cpp:98:            LOG(INFO) << " -- OnRequestEnd";
src/apps/jsonclient.cpp:114:                                   LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/jsonclient.cpp:117:                                   LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/jsonclient.cpp:135:                                       LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/jsonclient.cpp:138:                                       LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/jsonclient.cpp:146:                PLOG(ERROR) << "OnError: " << err;
src/apps/warema-jalousien.cpp:112:                              LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/warema-jalousien.cpp:115:                              LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httpserver.cpp:141:core::socket::State& state) { // titan #endif if (errnum != 0) { PLOG(FATAL) << "listen"; } else { VLOG(1) << "snode.c listening on
src/apps/http/testbasicauthentication.cpp:106:                                    LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/testbasicauthentication.cpp:109:                                    LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/vhostserver.cpp:130:                                     LOG(ERROR) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/http/vhostserver.cpp:133:                                     LOG(FATAL) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/http/vhostserver.cpp:199:                                  LOG(ERROR) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/http/vhostserver.cpp:202:                                  LOG(FATAL) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/http/testexpressnext.cpp:102:                    LOG(ERROR) << "FAIL: connect failed: " << state.what();
src/apps/http/testexpressnext.cpp:124:            LOG(INFO) << "All express next() tests executed. failures=" << failures;
src/apps/http/testexpressnext.cpp:138:            LOG(ERROR) << "FAIL: master request not connected";
src/apps/http/testexpressnext.cpp:161:                    LOG(INFO) << "PASS: " << current.name << " status=" << res->statusCode << " body='" << body << "'";
src/apps/http/testexpressnext.cpp:164:                    LOG(ERROR) << "FAIL: " << current.name << " expected status=" << current.expectedStatus << " expected body fragment='"
src/apps/http/testexpressnext.cpp:173:                LOG(ERROR) << "FAIL: " << current.name << " parse-error: " << reason;
src/apps/http/testexpressnext.cpp:310:                LOG(ERROR) << "testexpressnext " << socketAddress.toString() << ": " << state.what();
src/apps/http/testexpressnext.cpp:313:                LOG(FATAL) << "testexpressnext " << socketAddress.toString() << ": " << state.what();
src/apps/http/testexpressnext.cpp:327:        LOG(ERROR) << "testexpressnext finished with failures=" << nextTester.getFailures();
src/apps/http/testexpressnext.cpp:331:    LOG(INFO) << "testexpressnext finished successfully";
src/apps/http/httpclient.cpp:104:core::socket::State& state) { #endif if (errnum != 0) { PLOG(ERROR) << "OnError: " << errnum; } else { VLOG(1) << "snode.c
src/apps/http/model/clients.h:128:                            LOG(ERROR) << req->getSocketContext()->getSocketConnection()->getConnectionName()
src/apps/http/model/clients.h:130:                            PLOG(ERROR) << "  /home/voc/projects/snodec/snode.c/CMakeLists.tt";
src/apps/http/model/clients.h:374:                        LOG(ERROR) << req->getSocketContext()->getSocketConnection()->getConnectionName()
src/apps/http/model/clients.h:376:                        PLOG(ERROR) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";
src/apps/http/model/clients.h:394:                                LOG(ERROR) << req->getSocketContext()->getSocketConnection()->getConnectionName()
src/apps/http/model/clients.h:396:                                PLOG(ERROR) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";
src/apps/http/model/clients.h:430:                            LOG(ERROR) << req->getSocketContext()->getSocketConnection()->getConnectionName()
src/apps/http/model/clients.h:432:                            PLOG(ERROR) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";
src/apps/http/model/clients.h:455:                            LOG(ERROR) << req->getSocketContext()->getSocketConnection()->getConnectionName()
src/apps/http/model/clients.h:457:                            PLOG(ERROR) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";
src/apps/http/model/servers.h:165:                LOG(WARNING) << "\tPeer certificate: no certificate";
src/apps/http/httplowlevelclient.cpp:238:                                      LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httplowlevelclient.cpp:241:                                      LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httplowlevelclient.cpp:296:                                         LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httplowlevelclient.cpp:299:                                         LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httplowlevelclient.cpp:329:                                         LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httplowlevelclient.cpp:332:                                         LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httplowlevelclient.cpp:353:                                      LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httplowlevelclient.cpp:356:                                      LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/verysimpleserver.cpp:78:                                 LOG(ERROR) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/http/verysimpleserver.cpp:81:                                 LOG(FATAL) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/http/verysimpleserver.cpp:110:                    LOG(ERROR) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/http/verysimpleserver.cpp:113:                    LOG(FATAL) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/main.cpp:245:                           LOG(ERROR) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/main.cpp:248:                           LOG(FATAL) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/tlslegacy/tlslegacyserver.cpp:32:            LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/tlslegacy/tlslegacyclient.cpp:32:            LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/express_compat_server.cpp:246:                LOG(ERROR) << "express-compat " << socketAddress.toString() << ": " << state.what();
src/apps/express_compat_server.cpp:249:                LOG(FATAL) << "express-compat " << socketAddress.toString() << ": " << state.what();
src/net/config/stream/tls/ConfigSocketServer.hpp:103:                LOG(TRACE) << getInstanceName() << " SSL/TLS: SSL_CTX (M) sni for '" << sni << "' from master certificate installed";
src/net/config/stream/tls/ConfigSocketServer.hpp:140:                        LOG(TRACE) << getInstanceName() << " SSL/TLS: SSL_CTX (E) sni for '" << domain << "' explicitly installed";
src/net/config/stream/tls/ConfigSocketServer.hpp:145:                            LOG(TRACE) << getInstanceName() << " SSL/TLS: SSL_CTX (S) sni for '" << san << "' from SAN installed";
src/net/config/stream/tls/ConfigSocketServer.hpp:148:                        LOG(WARNING) << getInstanceName() << " SSL/TLS: Can not create SNI_SSL_CTX for domain '" << domain << "'";
src/net/config/stream/tls/ConfigSocketServer.hpp:152:            LOG(TRACE) << getInstanceName() << " SSL/TLS: SNI list result:";
src/net/config/stream/tls/ConfigSocketServer.hpp:154:                LOG(TRACE) << "  " << sni;
src/net/config/stream/tls/ConfigSocketServer.hpp:163:        LOG(TRACE) << getInstanceName() << " SSL/TLS SNI: Lookup for sni='" << serverNameIndication << "' in sni certificates";
src/net/config/stream/tls/ConfigSocketServer.hpp:169:                LOG(TRACE) << getInstanceName() << " SSL/TLS SNI:  .. " << sniPair.first.c_str();
src/net/config/stream/tls/ConfigSocketServer.hpp:174:            LOG(TRACE) << getInstanceName() << " SSL/TLS SNI: found for " << serverNameIndication << " -> '" << sniPairIt->first << "'";
src/net/config/stream/tls/ConfigSocketServer.hpp:177:            LOG(WARNING) << getInstanceName() << " SSL/TL SNI: not found for " << serverNameIndication;
src/net/config/ConfigPhysicalSocket.cpp:86:                           LOG(ERROR) << err.what();
src/core/socket/stream/SocketAcceptor.hpp:107:                LOG(DEBUG) << config->getInstanceName() << " Listen: starting";
src/core/socket/stream/SocketAcceptor.hpp:112:                    PLOG(ERROR) << config->getInstanceName() << " open " << configuredAddress.toString();
src/core/socket/stream/SocketAcceptor.hpp:126:                    LOG(DEBUG) << config->getInstanceName() << " open " << configuredAddress.toString() << ": success";
src/core/socket/stream/SocketAcceptor.hpp:129:                        PLOG(ERROR) << config->getInstanceName() << " bind " << configuredAddress.toString();
src/core/socket/stream/SocketAcceptor.hpp:147:                        LOG(DEBUG) << config->getInstanceName() << " bind " << configuredAddressString
src/core/socket/stream/SocketAcceptor.hpp:154:                            PLOG(ERROR) << config->getInstanceName() << " listen " << physicalServerSocket.getBindAddress().toString();
src/core/socket/stream/SocketAcceptor.hpp:165:                            LOG(DEBUG) << config->getInstanceName() << " listen " << physicalServerSocket.getBindAddress().toString() << ": success";
src/core/socket/stream/SocketAcceptor.hpp:168:                                LOG(DEBUG) << config->getInstanceName() << " enable " << physicalServerSocket.getBindAddress().toString() << ": success";
src/core/socket/stream/SocketAcceptor.hpp:170:                                LOG(ERROR) << config->getInstanceName() << " enable " << physicalServerSocket.getBindAddress().toString()
src/core/socket/stream/SocketAcceptor.hpp:183:                    LOG(INFO) << config->getInstanceName()
src/core/socket/stream/SocketAcceptor.hpp:194:                LOG(ERROR) << state.what();
src/core/socket/stream/SocketAcceptor.hpp:199:            LOG(DEBUG) << config->getInstanceName() << ": disabled";
src/core/socket/stream/SocketAcceptor.hpp:226:                    LOG(DEBUG) << config->getInstanceName() << " accept " << physicalServerSocket.getBindAddress().toString() << ": success";
src/core/socket/stream/SocketAcceptor.hpp:227:                    LOG(DEBUG) << "  " << socketConnection->getRemoteAddress().toString() << " -> "
src/core/socket/stream/SocketAcceptor.hpp:233:                    PLOG(WARNING) << config->getInstanceName() << " accept " << physicalServerSocket.getBindAddress().toString();
src/core/socket/stream/SocketWriter.cpp:119:                LOG(TRACE) << getName() << ": Shutdown restart";
src/core/socket/stream/SocketWriter.cpp:145:                LOG(WARNING) << getName() << ": Send while not enabled";
src/core/socket/stream/SocketWriter.cpp:148:            LOG(WARNING) << getName() << ": Send while shutdown in progress: ignoring";
src/core/socket/stream/SocketWriter.cpp:160:                    LOG(TRACE) << getName() << ": Stream started";
src/core/socket/stream/SocketWriter.cpp:162:                    LOG(WARNING) << getName() << ": Stream source is nullptr";
src/core/socket/stream/SocketWriter.cpp:165:                LOG(WARNING) << getName() << ": Stream while not enabled";
src/core/socket/stream/SocketWriter.cpp:168:            LOG(WARNING) << getName() << ": Stream while shutdown in progress";
src/core/socket/stream/SocketWriter.cpp:177:        LOG(TRACE) << getName() << ": Stream EOF";
src/core/socket/stream/SocketWriter.cpp:187:                LOG(TRACE) << getName() << ": Shutdown start";
src/core/socket/stream/SocketWriter.cpp:191:                LOG(TRACE) << getName() << ": Shutdown delayed due to queued data";
src/core/socket/stream/SocketConnection.hpp:67:                LOG(TRACE) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
src/core/socket/stream/SocketConnection.hpp:70:                LOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
src/core/socket/stream/SocketConnection.hpp:74:            PLOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
src/core/socket/stream/SocketConnection.hpp:90:                LOG(TRACE) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
src/core/socket/stream/SocketConnection.hpp:93:                LOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
src/core/socket/stream/SocketConnection.hpp:97:            PLOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
src/core/socket/stream/SocketConnector.hpp:106:                LOG(DEBUG) << config->getInstanceName() << " Connect: starting";
src/core/socket/stream/SocketConnector.hpp:114:                        PLOG(DEBUG) << config->getInstanceName() << " open " << configuredLocalAddress.toString();
src/core/socket/stream/SocketConnector.hpp:130:                        LOG(TRACE) << config->getInstanceName() << " open " << configuredLocalAddress.toString() << ": success";
src/core/socket/stream/SocketConnector.hpp:133:                            PLOG(DEBUG) << config->getInstanceName() << " bind " << configuredLocalAddress.toString();
src/core/socket/stream/SocketConnector.hpp:149:                            LOG(TRACE) << config->getInstanceName() << " bind " << configuredLocalAddressString
src/core/socket/stream/SocketConnector.hpp:156:                                PLOG(DEBUG) << config->getInstanceName() << " connect " << remoteAddress.toString();
src/core/socket/stream/SocketConnector.hpp:175:                                    LOG(INFO) << config->getInstanceName() << ": Using next SocketAddress: " << remoteAddress.toString();
src/core/socket/stream/SocketConnector.hpp:182:                                LOG(TRACE) << config->getInstanceName() << " connect " << remoteAddress.toString() << ": success";
src/core/socket/stream/SocketConnector.hpp:186:                                        LOG(DEBUG)
src/core/socket/stream/SocketConnector.hpp:189:                                        LOG(ERROR) << config->getInstanceName() << " enable " << remoteAddress.toString()
src/core/socket/stream/SocketConnector.hpp:200:                                    LOG(DEBUG) << config->getInstanceName() << " connect " << remoteAddress.toString() << ": success";
src/core/socket/stream/SocketConnector.hpp:201:                                    LOG(DEBUG) << "  " << socketConnection->getLocalAddress().toString() << " -> "
src/core/socket/stream/SocketConnector.hpp:216:                    LOG(ERROR) << state.what();
src/core/socket/stream/SocketConnector.hpp:224:                LOG(ERROR) << state.what();
src/core/socket/stream/SocketConnector.hpp:229:            LOG(DEBUG) << config->getInstanceName() << ": disabled";
src/core/socket/stream/SocketConnector.hpp:254:                LOG(DEBUG) << config->getInstanceName() << " connect " << remoteAddress.toString() << ": success";
src/core/socket/stream/SocketConnector.hpp:255:                LOG(DEBUG) << "  " << socketConnection->getLocalAddress().toString() << " -> "
src/core/socket/stream/SocketConnector.hpp:265:                LOG(DEBUG) << config->getInstanceName() << " connect " << remoteAddress.toString() << ": in progress:";
src/core/socket/stream/SocketConnector.hpp:286:                    PLOG(DEBUG) << config->getInstanceName() << " connect '" << remoteAddress.toString();
src/core/socket/stream/SocketConnector.hpp:290:                    LOG(DEBUG) << config->getInstanceName()
src/core/socket/stream/SocketConnector.hpp:297:                    PLOG(DEBUG) << config->getInstanceName() << " connect " << remoteAddress.toString();
src/core/socket/stream/SocketConnector.hpp:305:            PLOG(DEBUG) << config->getInstanceName() << " getsockopt syscall error: '" << remoteAddress.toString() << "'";
src/core/socket/stream/SocketConnector.hpp:323:        LOG(TRACE) << config->getInstanceName() << " connect timeout " << remoteAddress.toString();
src/core/socket/stream/SocketConnector.hpp:327:            LOG(DEBUG) << config->getInstanceName() << " using next SocketAddress: '" << config->Remote::getSocketAddress().toString()
src/core/socket/stream/SocketConnector.hpp:332:            LOG(DEBUG) << config->getInstanceName() << " connect timeout '" << remoteAddress.toString() << "'";
src/core/DynamicLoader.cpp:87:            LOG(TRACE) << "DynLoader dlOpen: " << lib.fileName << ": already open (refCount=" << lib.refCount << ")";
src/core/DynamicLoader.cpp:105:                LOG(TRACE) << "DynLoader dlOpen: " << libFile << ": success";
src/core/DynamicLoader.cpp:107:                LOG(TRACE) << "DynLoader dlOpen: " << libFile << ": " << DynamicLoader::dlError();
src/core/DynamicLoader.cpp:116:            LOG(TRACE) << "DynLoader dlCloseDelayed: handle is nullptr";
src/core/DynamicLoader.cpp:120:                LOG(TRACE) << "DynLoader dlCloseDelayed: " << handle << ": not opened using dlOpen";
src/core/DynamicLoader.cpp:124:                    LOG(TRACE) << "DynLoader: dlCloseDelayed: internal error: handle known but library record missing";
src/core/DynamicLoader.cpp:135:                        LOG(TRACE) << "DynLoader dlCloseDelayed: " << lib.fileName;
src/core/DynamicLoader.cpp:137:                        LOG(TRACE) << "DynLoader dlCloseDelayed: " << lib.fileName << ": still referenced (refCount=" << lib.refCount
src/core/DynamicLoader.cpp:149:            LOG(TRACE) << "DynLoader dlClose: handle is nullptr";
src/core/DynamicLoader.cpp:153:                LOG(TRACE) << "DynLoader dlClose: " << handle << ": not opened using dlOpen";
src/core/DynamicLoader.cpp:157:                    LOG(TRACE) << "DynLoader dlClose: internal error: handle known but library record missing";
src/core/DynamicLoader.cpp:166:                        LOG(TRACE) << "DynLoader dlClose: " << lib.fileName << ": still referenced (refCount=" << lib.refCount << ")";
src/core/DynamicLoader.cpp:202:            LOG(TRACE) << "DynLoader dlClose: " << DynamicLoader::dlError();
src/core/DynamicLoader.cpp:204:            LOG(TRACE) << "DynLoader dlClose: " << library.fileName << ": success";

```

## Deferred call-site table

No suitable `LOG`/`PLOG` call sites are deferred. Remaining VLOG call sites are deferred because migration would change VLOG numeric verbose-level semantics:

```text
src/express/legacy/rc/Server.cpp:73:                        VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/express/legacy/rc/Server.cpp:76:                        VLOG(1) << instanceName << ": disabled";
src/express/legacy/rc/Server.cpp:79:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/legacy/rc/Server.cpp:82:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/legacy/rc/Server.cpp:88:        VLOG(1) << "Instance: " << instanceName;
src/express/legacy/rc/Server.cpp:92:            VLOG(1) << "  " << route;
src/express/legacy/in6/Server.cpp:73:                        VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/express/legacy/in6/Server.cpp:76:                        VLOG(1) << instanceName << ": disabled";
src/express/legacy/in6/Server.cpp:79:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/legacy/in6/Server.cpp:82:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/legacy/in6/Server.cpp:88:        VLOG(1) << "Instance: " << instanceName;
src/express/legacy/in6/Server.cpp:92:            VLOG(1) << "  " << route;
src/express/legacy/in/Server.cpp:73:                        VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/express/legacy/in/Server.cpp:76:                        VLOG(1) << instanceName << ": disabled";
src/express/legacy/in/Server.cpp:79:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/legacy/in/Server.cpp:82:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/legacy/in/Server.cpp:88:        VLOG(1) << "Instance: " << instanceName;
src/express/legacy/in/Server.cpp:92:            VLOG(1) << "  " << route;
src/express/legacy/un/Server.cpp:73:                        VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/express/legacy/un/Server.cpp:76:                        VLOG(1) << instanceName << ": disabled";
src/express/legacy/un/Server.cpp:79:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/legacy/un/Server.cpp:82:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/legacy/un/Server.cpp:88:        VLOG(1) << "Instance: " << instanceName;
src/express/legacy/un/Server.cpp:92:            VLOG(1) << "  " << route;
src/express/tls/rc/Server.cpp:73:                        VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/express/tls/rc/Server.cpp:76:                        VLOG(1) << instanceName << ": disabled";
src/express/tls/rc/Server.cpp:79:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/tls/rc/Server.cpp:82:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/tls/rc/Server.cpp:88:        VLOG(1) << "Instance: " << instanceName;
src/express/tls/rc/Server.cpp:92:            VLOG(1) << "  " << route;
src/express/tls/in6/Server.cpp:73:                        VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/express/tls/in6/Server.cpp:76:                        VLOG(1) << instanceName << ": disabled";
src/express/tls/in6/Server.cpp:79:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/tls/in6/Server.cpp:82:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/tls/in6/Server.cpp:88:        VLOG(1) << "Instance: " << instanceName;
src/express/tls/in6/Server.cpp:92:            VLOG(1) << "  " << route;
src/express/tls/in/Server.cpp:73:                        VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/express/tls/in/Server.cpp:76:                        VLOG(1) << instanceName << ": disabled";
src/express/tls/in/Server.cpp:79:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/tls/in/Server.cpp:82:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/tls/in/Server.cpp:88:        VLOG(1) << "Instance: " << instanceName;
src/express/tls/in/Server.cpp:92:            VLOG(1) << "  " << route;
src/express/tls/un/Server.cpp:73:                        VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/express/tls/un/Server.cpp:76:                        VLOG(1) << instanceName << ": disabled";
src/express/tls/un/Server.cpp:79:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/tls/un/Server.cpp:82:                        VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/express/tls/un/Server.cpp:88:        VLOG(1) << "Instance: " << instanceName;
src/express/tls/un/Server.cpp:92:            VLOG(1) << "  " << route;
src/apps/testpost.cpp:110:                VLOG(1) << "legacyApp: listening on '" << socketAddress.toString() << "'";
src/apps/testpost.cpp:113:                VLOG(1) << "legacyApp: disabled";
src/apps/testpost.cpp:116:                VLOG(1) << "legacyApp: error occurred";
src/apps/testpost.cpp:119:                VLOG(1) << "legacyApp: fatal error occurred";
src/apps/testpost.cpp:140:                VLOG(1) << "tlsApp: listening on '" << socketAddress.toString() << "'";
src/apps/testpost.cpp:143:                VLOG(1) << "tlsApp: disabled";
src/apps/testpost.cpp:146:                VLOG(1) << "tlsApp: error occurred";
src/apps/testpost.cpp:149:                VLOG(1) << "tlsApp: fatal error occurred";
src/apps/testpipe.cpp:65:                VLOG(1) << "Pipe Data: " << string;
src/apps/testpipe.cpp:72:                VLOG(1) << "Pipe EOF";
src/apps/testpipe.cpp:76:                VLOG(1) << "PipeSink";
src/apps/testpipe.cpp:80:                VLOG(1) << "PipeSource";
src/apps/main.cpp:240:                           VLOG(1) << instanceName << " listening on '" << socketAddress.toString() << "'";
src/apps/main.cpp:243:                           VLOG(1) << instanceName << " disabled";
src/apps/tlslegacy/tlslegacyserver.cpp:31:            VLOG(1) << instanceName << ": listening on " << socketAddress.toString();
src/apps/tlslegacy/tlslegacyclient.cpp:31:            VLOG(1) << instanceName << ": connected to " << socketAddress.toString();
src/apps/tlslegacy/TlsLegacySocketContext.cpp:29:        VLOG(1) << getSocketConnection()->getConnectionName() << ": connected";
src/apps/tlslegacy/TlsLegacySocketContext.cpp:33:            VLOG(1) << getSocketConnection()->getConnectionName() << ": sent TLS greeting";
src/apps/tlslegacy/TlsLegacySocketContext.cpp:39:        VLOG(1) << getSocketConnection()->getConnectionName() << ": disconnected";
src/apps/tlslegacy/TlsLegacySocketContext.cpp:57:                VLOG(1) << getSocketConnection()->getConnectionName() << ": trying post-TLS legacy payload: " << payload;
src/apps/tlslegacy/TlsLegacySocketContext.cpp:65:            VLOG(1) << getSocketConnection()->getConnectionName() << ": got TLS ack, initiating TLS shutdown handshake (close_notify) "
src/apps/tlslegacy/TlsLegacySocketContext.cpp:72:            VLOG(1) << getSocketConnection()->getConnectionName() << ": got LEGACY ack -> post-TLS plaintext path works " << line;
src/apps/tlslegacy/TlsLegacySocketContext.cpp:81:            VLOG(1) << getSocketConnection()->getConnectionName() << ": TLS phase complete, waiting for peer close_notify " << line;
src/apps/tlslegacy/TlsLegacySocketContext.cpp:86:            VLOG(1) << getSocketConnection()->getConnectionName() << ": received LEGACY payload after TLS shutdown " << line;
src/apps/express_compat_server.cpp:241:                VLOG(1) << "express-compat listening on '" << socketAddress.toString() << "'";
src/apps/express_compat_server.cpp:244:                VLOG(1) << "express-compat disabled";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:108:                                                VLOG(0) << "MySQL error: " << state.errorMessage << " [" << state.error << "]";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:110:                                                VLOG(0) << "MySQL connected";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:112:                                                VLOG(0) << "MySQL disconnected";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:129:                            VLOG(1) << "Valid client id '" << queryClientId << "'";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:130:                            VLOG(1) << "Next with " << req->httpVersion << " " << req->method << " " << req->url;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:133:                            VLOG(1) << "Invalid client id '" << queryClientId << "'";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:139:                    VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:157:        VLOG(1) << "Query params: "
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:164:            VLOG(1) << "Auth invalid, sending Bad Request";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:173:                    VLOG(1) << "Database: Set redirect_uri to " << paramRedirectUri;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:176:                    VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:184:                    VLOG(1) << "Database: Set scope to " << paramScope;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:187:                    VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:195:                    VLOG(1) << "Database: Set state to " << paramState;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:198:                    VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:202:        VLOG(1) << "Auth request valid, redirecting to login";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:251:                                          VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:276:                                                        VLOG(1) << "Sending json reponse: " << responseJsonString;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:280:                                                        VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:286:                                            VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:293:                        VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:305:        VLOG(1) << "GrandType: " << queryGrantType;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:307:        VLOG(1) << "Code: " << queryCode;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:309:        VLOG(1) << "RedirectUri: " << queryRedirectUri;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:368:                                              VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:385:                                                            VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:391:                                                VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:404:                                                VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:427:                                                            VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:433:                                                VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:439:                                VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:446:                VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:454:        VLOG(1) << "ClientId: " << queryClientId;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:456:        VLOG(1) << "GrandType: " << queryGrantType;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:458:        VLOG(1) << "RefreshToken: " << queryRefreshToken;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:460:        VLOG(1) << "State: " << queryState;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:501:                              VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:521:                                            VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:527:                                VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:533:                VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:539:        VLOG(1) << "POST /token/validate";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:542:                VLOG(1) << "Missing 'access_token' in json";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:548:                VLOG(1) << "Missing 'client_id' in json";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:567:                            VLOG(1) << "Sending 401: Invalid access token '" << jsonAccessToken << "'";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:570:                            VLOG(1) << "Sending 200: Valid access token '" << jsonAccessToken << "";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:577:                    VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:590:                VLOG(1) << "OAuth2AuthorizationServer: listening on '" << socketAddress.toString() << "'";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:593:                VLOG(1) << "OAuth2AuthorizationServer: disabled";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:596:                VLOG(1) << "OAuth2AuthorizationServer: error occurred";
src/apps/oauth2/authorization_server/AuthorizationServer.cpp:599:                VLOG(1) << "OAuth2AuthorizationServer: fatal error occurred";
src/apps/oauth2/resource_server/ResourceServer.cpp:67:            VLOG(1) << "Missing access_token or client_id in body";
src/apps/oauth2/resource_server/ResourceServer.cpp:74:                VLOG(1) << "OnConnect";
src/apps/oauth2/resource_server/ResourceServer.cpp:76:                VLOG(1) << "\tServer: " + socketConnection->getRemoteAddress().toString();
src/apps/oauth2/resource_server/ResourceServer.cpp:77:                VLOG(1) << "\tClient: " + socketConnection->getLocalAddress().toString();
src/apps/oauth2/resource_server/ResourceServer.cpp:80:                VLOG(1) << "OnConnected";
src/apps/oauth2/resource_server/ResourceServer.cpp:83:                VLOG(1) << "OnDisconnect";
src/apps/oauth2/resource_server/ResourceServer.cpp:85:                VLOG(1) << "\tServer: " + socketConnection->getRemoteAddress().toString();
src/apps/oauth2/resource_server/ResourceServer.cpp:86:                VLOG(1) << "\tClient: " + socketConnection->getLocalAddress().toString();
src/apps/oauth2/resource_server/ResourceServer.cpp:89:                VLOG(1) << "OnRequestBegin";
src/apps/oauth2/resource_server/ResourceServer.cpp:92:                VLOG(1) << "ClientId: " << queryClientId;
src/apps/oauth2/resource_server/ResourceServer.cpp:93:                VLOG(1) << "AccessToken: " << queryAccessToken;
src/apps/oauth2/resource_server/ResourceServer.cpp:100:                        VLOG(1) << "OnResponse";
src/apps/oauth2/resource_server/ResourceServer.cpp:101:                        VLOG(1) << "Response: " << std::string(response->body.begin(), response->body.end());
src/apps/oauth2/resource_server/ResourceServer.cpp:111:                        VLOG(1) << "OAuth2ResourceServer: Request parse error: " << message;
src/apps/oauth2/resource_server/ResourceServer.cpp:122:                        VLOG(1) << "OAuth2ResourceServer: connected to '" << socketAddress.toString() << "'";
src/apps/oauth2/resource_server/ResourceServer.cpp:125:                        VLOG(1) << "OAuth2ResourceServer: disabled";
src/apps/oauth2/resource_server/ResourceServer.cpp:128:                        VLOG(1) << "OAuth2ResourceServer: error occurred";
src/apps/oauth2/resource_server/ResourceServer.cpp:131:                        VLOG(1) << "OAuth2ResourceServer: fatal error occurred";
src/apps/oauth2/resource_server/ResourceServer.cpp:140:                VLOG(1) << "app: listening on '" << socketAddress.toString() << "'";
src/apps/oauth2/resource_server/ResourceServer.cpp:143:                VLOG(1) << "app: disabled";
src/apps/oauth2/resource_server/ResourceServer.cpp:146:                VLOG(1) << "app: error occurred";
src/apps/oauth2/resource_server/ResourceServer.cpp:149:                VLOG(1) << "app: fatal error occurred";
src/apps/oauth2/client_app/ClientApp.cpp:69:            VLOG(1) << "Recieving auth code from auth server: " << req.query("code") << ", requesting token from " << tokenRequestUri;
src/apps/oauth2/client_app/ClientApp.cpp:80:                VLOG(1) << "OAuth2Client: connected to '" << socketAddress.toString() << "'";
src/apps/oauth2/client_app/ClientApp.cpp:83:                VLOG(1) << "OAuth2Client: disabled";
src/apps/oauth2/client_app/ClientApp.cpp:86:                VLOG(1) << "OAuth2Client: error occurred";
src/apps/oauth2/client_app/ClientApp.cpp:89:                VLOG(1) << "OAuth2Client: fatal error occurred";
src/apps/database/testmariadb.cpp:114:            VLOG(0) << "MySQL error: " << state.errorMessage << " [" << state.error << "]";
src/apps/database/testmariadb.cpp:116:            VLOG(0) << "MySQL connected";
src/apps/database/testmariadb.cpp:118:            VLOG(0) << "MySQL disconnected";
src/apps/database/testmariadb.cpp:127:               VLOG(0) << "********** OnQuery 0;";
src/apps/database/testmariadb.cpp:130:                       VLOG(0) << "********** AffectedRows 1: " << affectedRows;
src/apps/database/testmariadb.cpp:133:                       VLOG(0) << "Error 1: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:137:               VLOG(0) << "********** Error 0: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:143:                VLOG(0) << "********** OnQuery 1: ";
src/apps/database/testmariadb.cpp:146:                        VLOG(0) << "********** AffectedRows 2: " << affectedRows;
src/apps/database/testmariadb.cpp:149:                        VLOG(0) << "********** Error 2: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:153:                VLOG(0) << "********** Error 1: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:159:                    VLOG(0) << "********** Row Result 2: " << row[0] << " : " << row[1];
src/apps/database/testmariadb.cpp:162:                    VLOG(0) << "********** Row Result 2: " << r;
src/apps/database/testmariadb.cpp:166:                VLOG(0) << "********** Error 2: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:172:                    VLOG(0) << "********** Row Result 2: " << row[0] << " : " << row[1];
src/apps/database/testmariadb.cpp:175:                    VLOG(0) << "********** Row Result 2: " << r;
src/apps/database/testmariadb.cpp:179:                VLOG(0) << "********** Error 2: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:184:            VLOG(0) << "MySQL error: " << state.errorMessage << " [" << state.error << "]";
src/apps/database/testmariadb.cpp:186:            VLOG(0) << "MySQL connected";
src/apps/database/testmariadb.cpp:188:            VLOG(0) << "MySQL disconnected";
src/apps/database/testmariadb.cpp:199:                    VLOG(0) << "Row Result 3: " << row[0] << " : " << row[1];
src/apps/database/testmariadb.cpp:201:                    VLOG(0) << "Row Result 3:";
src/apps/database/testmariadb.cpp:205:                VLOG(0) << "Error 3: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:212:                    VLOG(0) << "Row Result 4: " << row[0] << " : " << row[1];
src/apps/database/testmariadb.cpp:214:                    VLOG(0) << "Row Result 4:";
src/apps/database/testmariadb.cpp:220:                                VLOG(0) << "Row Result 5: " << row[0] << " : " << row[1];
src/apps/database/testmariadb.cpp:222:                                VLOG(0) << "Row Result 5:";
src/apps/database/testmariadb.cpp:227:                                        VLOG(0) << "Tick 2: " << i++;
src/apps/database/testmariadb.cpp:234:                                                    VLOG(0) << "Row Result 6: " << row[0] << " : " << row[1];
src/apps/database/testmariadb.cpp:237:                                                    VLOG(0) << "Row Result 6: " << r1;
src/apps/database/testmariadb.cpp:241:                                                VLOG(0) << "Error 6: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:250:                                        VLOG(0) << "Tick 0.7: " << i++;
src/apps/database/testmariadb.cpp:257:                                                       VLOG(0) << "Row Result 7: " << row[0] << " : " << row[1];
src/apps/database/testmariadb.cpp:260:                                                       VLOG(0) << "Row Result 7: " << r2;
src/apps/database/testmariadb.cpp:263:                                                               VLOG(0) << "************ FieldCount ************ = " << fieldCount;
src/apps/database/testmariadb.cpp:266:                                                               VLOG(0) << "Error 7: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:271:                                                   VLOG(0) << "Error 7: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:276:                                                    VLOG(0) << "************ FieldCount ************ = " << fieldCount;
src/apps/database/testmariadb.cpp:279:                                                    VLOG(0) << "Error 7: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:286:                            VLOG(0) << "Error 5: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:291:                VLOG(0) << "Error 4: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:297:                VLOG(0) << "Tick 0.1: " << i++;
src/apps/database/testmariadb.cpp:300:                    VLOG(0) << "Stop Stop";
src/apps/database/testmariadb.cpp:307:                           VLOG(0) << "Transactions activated 10:";
src/apps/database/testmariadb.cpp:310:                           VLOG(0) << "Error 8: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:315:                            VLOG(0) << "Inserted 10: " << j;
src/apps/database/testmariadb.cpp:318:                                    VLOG(0) << "AffectedRows 11: " << affectedRows;
src/apps/database/testmariadb.cpp:321:                                    VLOG(0) << "Error 11: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:325:                            VLOG(0) << "Error 10: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:330:                            VLOG(0) << "Rollback success 11";
src/apps/database/testmariadb.cpp:333:                            VLOG(0) << "Error 12: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:339:                            VLOG(0) << "Inserted 13: " << j;
src/apps/database/testmariadb.cpp:342:                                    VLOG(0) << "AffectedRows 14: " << affectedRows;
src/apps/database/testmariadb.cpp:345:                                    VLOG(0) << "Error 14: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:349:                            VLOG(0) << "Error 13: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:354:                            VLOG(0) << "Commit success 15";
src/apps/database/testmariadb.cpp:357:                            VLOG(0) << "Error 15: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:364:                                VLOG(0) << "Row Result count(*) 16: " << row[0];
src/apps/database/testmariadb.cpp:366:                                    VLOG(0) << "Wrong number of rows 16: " << std::atoi(row[0]) << " != " << j + 1; // NOLINT
src/apps/database/testmariadb.cpp:370:                                VLOG(0) << "Row Result count(*) 16: no result:";
src/apps/database/testmariadb.cpp:373:                                        VLOG(0) << "************ FieldCount ************ = " << fieldCount;
src/apps/database/testmariadb.cpp:376:                                        VLOG(0) << "Error 7: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:381:                            VLOG(0) << "Error 16: " << errorString << " : " << errorNumber;
src/apps/database/testmariadb.cpp:386:                            VLOG(0) << "Transactions deactivated 17";
src/apps/database/testmariadb.cpp:389:                            VLOG(0) << "Error 17: " << errorString << " : " << errorNumber;
src/apps/websocket/subprotocol/server/echo/Echo.cpp:63:        VLOG(1) << "Echo connected";
src/apps/websocket/subprotocol/server/echo/Echo.cpp:67:        VLOG(2) << "Message Start - OpCode: " << opCode;
src/apps/websocket/subprotocol/server/echo/Echo.cpp:73:        VLOG(2) << "Message Fragment: " << std::string(chunk, chunkLen);
src/apps/websocket/subprotocol/server/echo/Echo.cpp:77:        VLOG(1) << "Message Data: " << data;
src/apps/websocket/subprotocol/server/echo/Echo.cpp:90:        VLOG(1) << "Message error: " << errnum;
src/apps/websocket/subprotocol/server/echo/Echo.cpp:94:        VLOG(1) << "Echo disconnected:";
src/apps/websocket/subprotocol/server/echo/Echo.cpp:98:        VLOG(1) << "SubProtocol 'echo' exit due to '" << strsignal(sig) << "' (SIG" << utils::system::sigabbrev_np(sig) << " = " << sig
src/apps/websocket/subprotocol/client/echo/Echo.cpp:63:        VLOG(1) << "Echo connected";
src/apps/websocket/subprotocol/client/echo/Echo.cpp:70:        VLOG(2) << "Message Start - OpCode: " << opCode;
src/apps/websocket/subprotocol/client/echo/Echo.cpp:76:        VLOG(2) << "Message Fragment: " << std::string(chunk, chunkLen);
src/apps/websocket/subprotocol/client/echo/Echo.cpp:80:        VLOG(1) << "Message Data: " << data;
src/apps/websocket/subprotocol/client/echo/Echo.cpp:89:        VLOG(1) << "Message error: " << errnum;
src/apps/websocket/subprotocol/client/echo/Echo.cpp:93:        VLOG(1) << "Echo disconnected:";
src/apps/websocket/subprotocol/client/echo/Echo.cpp:97:        VLOG(1) << "SubProtocol 'echo' exit due to '" << strsignal(sig) << "' (SIG" << utils::system::sigabbrev_np(sig) << " = " << sig
src/apps/websocket/echoclient.cpp:67:                VLOG(1) << connectionName << ": OnRequestBegin";
src/apps/websocket/echoclient.cpp:75:                        VLOG(1) << connectionName << ": HTTP Upgrade (http -> websocket) start " << (success ? "success" : "failed");
src/apps/websocket/echoclient.cpp:80:                        VLOG(1) << connectionName << ": Upgrade success:";
src/apps/websocket/echoclient.cpp:82:                        VLOG(1) << connectionName << ":   Requested: " << req->header("upgrade");
src/apps/websocket/echoclient.cpp:83:                        VLOG(1) << connectionName << ":    Selected: " << res->get("upgrade");
src/apps/websocket/echoclient.cpp:86:                        VLOG(1) << connectionName << ": Request parse error: " << message;
src/apps/websocket/echoclient.cpp:92:                VLOG(1) << connectionName << ": OnRequestEnd";
src/apps/websocket/echoclient.cpp:99:                    VLOG(1) << instanceName << " connected to '" << socketAddress.toString() << "'";
src/apps/websocket/echoclient.cpp:102:                    VLOG(1) << instanceName << " disabled";
src/apps/websocket/echoclient.cpp:105:                    VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/websocket/echoclient.cpp:108:                    VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/websocket/echoclient.cpp:124:                VLOG(1) << connectionName << ": OnRequestBegin";
src/apps/websocket/echoclient.cpp:132:                        VLOG(1) << connectionName << ": HTTP Upgrade (http -> websocket) start " << (success ? "success" : "failed");
src/apps/websocket/echoclient.cpp:139:                        VLOG(1) << connectionName << ": Request parse error: " << message;
src/apps/websocket/echoclient.cpp:145:                VLOG(1) << connectionName << ": OnRequestEnd";
src/apps/websocket/echoclient.cpp:152:                    VLOG(1) << instanceName << " connected to '" << socketAddress.toString() << "'";
src/apps/websocket/echoclient.cpp:155:                    VLOG(1) << instanceName << " disabled";
src/apps/websocket/echoclient.cpp:158:                    VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/websocket/echoclient.cpp:161:                    VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/websocket/echoserver.cpp:75:                VLOG(1) << connectionName << ": Successful upgrade:";
src/apps/websocket/echoserver.cpp:76:                VLOG(1) << connectionName << ":   Requested: " << req->get("upgrade");
src/apps/websocket/echoserver.cpp:77:                VLOG(1) << connectionName << ":    Selected: " << name;
src/apps/websocket/echoserver.cpp:81:                VLOG(1) << connectionName << ": Can not upgrade to any of '" << req->get("upgrade") << "'";
src/apps/websocket/echoserver.cpp:89:        VLOG(1) << "HTTP GET on "
src/apps/websocket/echoserver.cpp:95:        VLOG(1) << CMAKE_CURRENT_SOURCE_DIR "/html" + req->url;
src/apps/websocket/echoserver.cpp:98:                VLOG(1) << req->url;
src/apps/websocket/echoserver.cpp:100:                VLOG(1) << "HTTP response send file failed: " << std::strerror(errnum);
src/apps/websocket/echoserver.cpp:111:                    VLOG(1) << instanceName << " listening on '" << socketAddress.toString() << "'";
src/apps/websocket/echoserver.cpp:114:                    VLOG(1) << instanceName << " disabled";
src/apps/websocket/echoserver.cpp:117:                    VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/websocket/echoserver.cpp:120:                    VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/websocket/echoserver.cpp:126:    VLOG(1) << "Legacy Routes:";
src/apps/websocket/echoserver.cpp:130:        VLOG(1) << "  " << route;
src/apps/websocket/echoserver.cpp:148:                    VLOG(1) << connectionName << ": Upgrade success:";
src/apps/websocket/echoserver.cpp:149:                    VLOG(1) << connectionName << ":   Requested: " << req->get("upgrade");
src/apps/websocket/echoserver.cpp:150:                    VLOG(1) << connectionName << ":    Selected: " << name;
src/apps/websocket/echoserver.cpp:154:                    VLOG(1) << connectionName << ": Can not upgrade to any of '" << req->get("upgrade") << "'";
src/apps/websocket/echoserver.cpp:166:            VLOG(1) << CMAKE_CURRENT_SOURCE_DIR "/html" + req->url;
src/apps/websocket/echoserver.cpp:169:                    VLOG(1) << req->url;
src/apps/websocket/echoserver.cpp:171:                    VLOG(1) << "HTTP response send file failed: " << std::strerror(errnum);
src/apps/websocket/echoserver.cpp:182:                        VLOG(1) << instanceName << " listening on '" << socketAddress.toString() << "'";
src/apps/websocket/echoserver.cpp:185:                        VLOG(1) << instanceName << " disabled";
src/apps/websocket/echoserver.cpp:188:                        VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/websocket/echoserver.cpp:191:                        VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
src/apps/websocket/echoserver.cpp:197:        VLOG(1) << "Tls Routes:";
src/apps/websocket/echoserver.cpp:201:            VLOG(1) << "  " << route;
src/apps/echo/echoclient.cpp:63:                    VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
src/apps/echo/echoclient.cpp:66:                    VLOG(1) << instanceName << ": disabled";
src/apps/echo/echoclient.cpp:80:                    VLOG(1) << "echoclient: connected to '" << socketAddress.toString() << "'" << "'";
src/apps/echo/echoclient.cpp:83:                    VLOG(1) << "echoclient: disabled";
src/apps/echo/echoclient.cpp:86:                    VLOG(1) << "echoclientt: error occurred";
src/apps/echo/echoclient.cpp:89:                    VLOG(1) << "echoclient: fatal error occurred";
src/apps/echo/echoclient.cpp:130:        VLOG(1) << "snode.c connecting to " << socketAddress.toString();
src/apps/echo/echoserver.cpp:87:                VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/apps/echo/echoserver.cpp:90:                VLOG(1) << instanceName << ": disabled";
src/apps/echo/echoserver.cpp:133:            VLOG(1) << "snode.c listening on " << socket.getBindAddress().toString();
src/apps/echo/model/clients.h:93:            VLOG(1) << "OnConnect " << client.getConfig()->getInstanceName();
src/apps/echo/model/clients.h:95:            VLOG(1) << "\tLocal: " << socketConnection->getLocalAddress().toString();
src/apps/echo/model/clients.h:96:            VLOG(1) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
src/apps/echo/model/clients.h:109:            VLOG(1) << "OnConnected " << client.getConfig()->getInstanceName();
src/apps/echo/model/clients.h:115:                VLOG(1) << "\tPeer certificate verifyErr = " + std::to_string(verifyErr) + ": " +
src/apps/echo/model/clients.h:119:                VLOG(1) << "\t   Subject: " + std::string(str);
src/apps/echo/model/clients.h:123:                VLOG(1) << "\t   Issuer: " + std::string(str);
src/apps/echo/model/clients.h:133:                VLOG(1) << "\t   Subject alternative name count: " << altNameCount;
src/apps/echo/model/clients.h:140:                        VLOG(1) << "\t      SAN (URI): '" + subjectAltName;
src/apps/echo/model/clients.h:145:                        VLOG(1) << "\t      SAN (DNS): '" + subjectAltName;
src/apps/echo/model/clients.h:147:                        VLOG(1) << "\t      SAN (Type): '" + std::to_string(generalName->type);
src/apps/echo/model/clients.h:155:                VLOG(1) << "\tPeer certificate: no certificate";
src/apps/echo/model/clients.h:160:            VLOG(1) << "OnDisconnect " << client.getConfig()->getInstanceName();
src/apps/echo/model/clients.h:162:            VLOG(1) << "\tLocal: " << socketConnection->getLocalAddress().toString();
src/apps/echo/model/clients.h:163:            VLOG(1) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
src/apps/echo/model/servers.h:93:            VLOG(1) << "OnConnect " << server.getConfig()->getInstanceName();
src/apps/echo/model/servers.h:95:            VLOG(1) << "\tLocal: " << socketConnection->getLocalAddress().toString();
src/apps/echo/model/servers.h:96:            VLOG(1) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
src/apps/echo/model/servers.h:109:            VLOG(1) << "OnConnected " << server.getConfig()->getInstanceName();
src/apps/echo/model/servers.h:115:                VLOG(1) << "\tPeer certificate verifyErr = " + std::to_string(verifyErr) + ": " +
src/apps/echo/model/servers.h:119:                VLOG(1) << "\t   Subject: " + std::string(str);
src/apps/echo/model/servers.h:123:                VLOG(1) << "\t   Issuer: " + std::string(str);
src/apps/echo/model/servers.h:133:                VLOG(1) << "\t   Subject alternative name count: " << altNameCount;
src/apps/echo/model/servers.h:140:                        VLOG(1) << "\t      SAN (URI): '" + subjectAltName;
src/apps/echo/model/servers.h:145:                        VLOG(1) << "\t      SAN (DNS): '" + subjectAltName;
src/apps/echo/model/servers.h:147:                        VLOG(1) << "\t      SAN (Type): '" + std::to_string(generalName->type);
src/apps/echo/model/servers.h:155:                VLOG(1) << "\tPeer certificate: no certificate";
src/apps/echo/model/servers.h:160:            VLOG(1) << "OnDisconnect " << server.getConfig()->getInstanceName();
src/apps/echo/model/servers.h:162:            VLOG(1) << "\tLocal: " << socketConnection->getLocalAddress().toString();
src/apps/echo/model/servers.h:163:            VLOG(1) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
src/apps/echo/model/EchoSocketContext.cpp:60:        VLOG(1) << "Echo connected";
src/apps/echo/model/EchoSocketContext.cpp:68:        VLOG(1) << "Echo disconnected";
src/apps/echo/model/EchoSocketContext.cpp:81:            VLOG(1) << "Data to reflect: " << std::string(chunk, chunklen);
src/apps/jsonserver.cpp:72:                    VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/apps/jsonserver.cpp:75:                    VLOG(1) << instanceName << ": disabled";
src/apps/jsonserver.cpp:92:                VLOG(1) << "Application received body: " << jsonString;
src/apps/jsonserver.cpp:95:                VLOG(1) << key << " attribute not found";
src/apps/jsonclient.cpp:64:            VLOG(1) << "-- OnRequest";
src/apps/jsonclient.cpp:72:                    VLOG(1) << "-- OnResponse";
src/apps/jsonclient.cpp:73:                    VLOG(1) << "     Status:";
src/apps/jsonclient.cpp:74:                    VLOG(1) << "       " << res->httpVersion;
src/apps/jsonclient.cpp:75:                    VLOG(1) << "       " << res->statusCode;
src/apps/jsonclient.cpp:76:                    VLOG(1) << "       " << res->reason;
src/apps/jsonclient.cpp:78:                    VLOG(1) << "     Headers:";
src/apps/jsonclient.cpp:80:                        VLOG(1) << "       " << field + " = " + value;
src/apps/jsonclient.cpp:83:                    VLOG(1) << "     Cookies:";
src/apps/jsonclient.cpp:85:                        VLOG(1) << "       " + name + " = " + cookie.getValue();
src/apps/jsonclient.cpp:87:                            VLOG(1) << "         " + option + " = " + value;
src/apps/jsonclient.cpp:92:                    VLOG(1) << "     Body:\n----------- start body -----------" << res->body.data() << "------------ end body ------------";
src/apps/jsonclient.cpp:95:                    VLOG(1) << "legacy: Request parse error: " << message;
src/apps/jsonclient.cpp:110:                    VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
src/apps/jsonclient.cpp:113:                    VLOG(1) << instanceName << ": disabled";
src/apps/jsonclient.cpp:131:                                       VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
src/apps/jsonclient.cpp:134:                                       VLOG(1) << instanceName << ": disabled";
src/apps/warema-jalousien.cpp:75:        VLOG(1) << "Param: " << req->param("id");
src/apps/warema-jalousien.cpp:76:        VLOG(1) << "Qurey: " << req->query("action");
src/apps/warema-jalousien.cpp:107:                              VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/apps/warema-jalousien.cpp:110:                              VLOG(1) << instanceName << ": disabled";
src/apps/http/httpserver.cpp:89:        VLOG(1) << "Routes:";
src/apps/http/httpserver.cpp:93:            VLOG(1) << "  " << route;
src/apps/http/httpserver.cpp:100:                    VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/apps/http/httpserver.cpp:103:                    VLOG(1) << instanceName << ": disabled";
src/apps/http/httpserver.cpp:106:                    VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httpserver.cpp:109:                    VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httpserver.cpp:143:logger::LogLevel::Critical) << "listen"; } else { VLOG(1) << "snode.c listening on " << socketAddress.toString();
src/apps/http/testbasicauthentication.cpp:102:                    VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
src/apps/http/testbasicauthentication.cpp:105:                    VLOG(1) << instanceName << ": disabled";
src/apps/http/testbasicauthentication.cpp:138:                VLOG(1) << "tls: listening on '" << socketAddress.toString() << "'"
src/apps/http/testbasicauthentication.cpp:142:                VLOG(1) << "tls: disabled";
src/apps/http/testbasicauthentication.cpp:145:                VLOG(1) << "tls: error occurred";
src/apps/http/testbasicauthentication.cpp:148:                VLOG(1) << "tls: fatal error occurred";
src/apps/http/vhostserver.cpp:125:                                     VLOG(1) << instanceName << " listening on '" << socketAddress.toString() << "'";
src/apps/http/vhostserver.cpp:128:                                     VLOG(1) << instanceName << " disabled";
src/apps/http/vhostserver.cpp:196:                                  VLOG(1) << instanceName << " listening on '" << socketAddress.toString() << "'";
src/apps/http/vhostserver.cpp:199:                                  VLOG(1) << instanceName << " disabled";
src/apps/http/testexpressnext.cpp:306:                VLOG(1) << "testexpressnext listening on '" << socketAddress.toString() << "'";
src/apps/http/testexpressnext.cpp:309:                VLOG(1) << "testexpressnext disabled";
src/apps/http/httpclient.cpp:64:                VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
src/apps/http/httpclient.cpp:67:                VLOG(1) << instanceName << ": disabled";
src/apps/http/httpclient.cpp:70:                VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httpclient.cpp:73:                VLOG(1) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
src/apps/http/httpclient.cpp:106:"OnError: " << errnum; } else { VLOG(1) << "snode.c connecting to " << socketAddress.toString();
src/apps/http/model/clients.h:73:    VLOG(1) << req->getConnectionName() << " HTTP response: " << req->method << " " << req->url << " HTTP/" << req->httpMajor << "."
src/apps/http/model/clients.h:101:                VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName() << ": OnRequestStart";
src/apps/http/model/clients.h:125:                            VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName()
src/apps/http/model/clients.h:127:                            VLOG(1) << "  /home/voc/projects/snodec/snode.c/CMakeLists.tt";
src/apps/http/model/clients.h:302:                        VLOG(0) << "OnOpen 1";
src/apps/http/model/clients.h:306:                        VLOG(0) << "OnError 1";
src/apps/http/model/clients.h:310:                        VLOG(0) << "OnMessage 1:1: " << message.data;
src/apps/http/model/clients.h:313:                        VLOG(0) << "OnMessage 1:2: " << message.data;
src/apps/http/model/clients.h:316:                        VLOG(0) << "EventListener for 'myevent' 1:1: " << message.lastEventId << " : " << message.data;
src/apps/http/model/clients.h:319:                        VLOG(0) << "EventListener for 'myevent' 1:2: " << message.lastEventId << " : " << message.data;
src/apps/http/model/clients.h:333:                        VLOG(0) << "OnOpen 2";
src/apps/http/model/clients.h:337:                        VLOG(0) << "OnError 2";
src/apps/http/model/clients.h:341:                        VLOG(0) << "OnMessage 2:1: " << message.data;
src/apps/http/model/clients.h:344:                        VLOG(0) << "OnMessage 2:2: " << message.data;
src/apps/http/model/clients.h:347:                        VLOG(0) << "EventListener for 'myevent' 2:1: " << message.lastEventId << " : " << message.data;
src/apps/http/model/clients.h:350:                        VLOG(0) << "EventListener for 'myevent' 2:2: " << message.lastEventId << " : " << message.data;
src/apps/http/model/clients.h:373:                        VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName()
src/apps/http/model/clients.h:375:                        VLOG(1) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";
src/apps/http/model/clients.h:394:                                VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName()
src/apps/http/model/clients.h:396:                                VLOG(1) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";
src/apps/http/model/clients.h:431:                            VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName()
src/apps/http/model/clients.h:433:                            VLOG(1) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";
src/apps/http/model/clients.h:457:                            VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName()
src/apps/http/model/clients.h:459:                            VLOG(1) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";
src/apps/http/model/clients.h:477:                VLOG(1) << req->getConnectionName() << ": OnRequestEnd";
src/apps/http/model/clients.h:481:            VLOG(1) << socketConnection->getConnectionName() << ": OnConnect";
src/apps/http/model/clients.h:483:            VLOG(1) << "\tLocal: " << socketConnection->getLocalAddress().toString();
src/apps/http/model/clients.h:484:            VLOG(1) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
src/apps/http/model/clients.h:488:            VLOG(1) << socketConnection->getConnectionName() << ": OnDisconnect";
src/apps/http/model/clients.h:490:            VLOG(1) << "\tLocal: " << socketConnection->getLocalAddress().toString();
src/apps/http/model/clients.h:491:            VLOG(1) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
src/apps/http/model/clients.h:515:                VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName() << ": OnRequestStart";
src/apps/http/model/clients.h:663:                VLOG(1) << req->getConnectionName() << ": OnRequestEnd";
src/apps/http/model/clients.h:667:            VLOG(1) << "OnConnect " << socketConnection->getConnectionName();
src/apps/http/model/clients.h:669:            VLOG(1) << "\tLocal: " << socketConnection->getLocalAddress().toString();
src/apps/http/model/clients.h:670:            VLOG(1) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
src/apps/http/model/clients.h:683:            VLOG(1) << socketConnection->getConnectionName() << ": OnConnected";
src/apps/http/model/clients.h:688:                VLOG(1) << "\tPeer certificate verifyErr = " + std::to_string(verifyErr) + ": " +
src/apps/http/model/clients.h:692:                VLOG(1) << "\t   Subject: " + std::string(str);
src/apps/http/model/clients.h:696:                VLOG(1) << "\t   Issuer: " + std::string(str);
src/apps/http/model/clients.h:706:                VLOG(1) << "\t   Subject alternative name count: " << altNameCount;
src/apps/http/model/clients.h:713:                        VLOG(1) << "\t      SAN (URI): '" + subjectAltName;
src/apps/http/model/clients.h:718:                        VLOG(1) << "\t      SAN (DNS): '" + subjectAltName;
src/apps/http/model/clients.h:720:                        VLOG(1) << "\t      SAN (Type): '" + std::to_string(generalName->type);
src/apps/http/model/clients.h:728:                VLOG(1) << "\tPeer certificate: no certificate";
src/apps/http/model/clients.h:733:            VLOG(1) << socketConnection->getConnectionName() << ": OnDisconnect";
src/apps/http/model/clients.h:735:            VLOG(1) << "\tLocal: " << socketConnection->getLocalAddress().toString();
src/apps/http/model/clients.h:736:            VLOG(1) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
src/apps/http/model/servers.h:103:            VLOG(1) << "OnConnect " << instanceName;
src/apps/http/model/servers.h:105:            VLOG(1) << "  Local: " << socketConnection->getLocalAddress().toString();
src/apps/http/model/servers.h:106:            VLOG(1) << "   Peer: " << socketConnection->getRemoteAddress().toString();
src/apps/http/model/servers.h:119:            VLOG(1) << "OnConnected " << instanceName;
src/apps/http/model/servers.h:125:                VLOG(1) << "\tPeer certificate verifyErr = " + std::to_string(verifyErr) + ": " +
src/apps/http/model/servers.h:129:                VLOG(1) << "\t   Subject: " + std::string(str);
src/apps/http/model/servers.h:133:                VLOG(1) << "\t   Issuer: " + std::string(str);
src/apps/http/model/servers.h:143:                VLOG(1) << "\t   Subject alternative name count: " << altNameCount;
src/apps/http/model/servers.h:151:                        VLOG(1) << "\t      SAN (URI): '" + subjectAltName;
src/apps/http/model/servers.h:156:                        VLOG(1) << "\t      SAN (DNS): '" + subjectAltName;
src/apps/http/model/servers.h:158:                        VLOG(1) << "\t      SAN (Type): '" + std::to_string(generalName->type);
src/apps/http/model/servers.h:171:            VLOG(1) << "OnDisconnect " << instanceName;
src/apps/http/model/servers.h:173:            VLOG(2) << "            Local: " << socketConnection->getLocalAddress().toString(false);
src/apps/http/model/servers.h:174:            VLOG(2) << "             Peer: " << socketConnection->getRemoteAddress().toString(false);
src/apps/http/model/servers.h:176:            VLOG(2) << "     Online Since: " << socketConnection->getOnlineSince();
src/apps/http/model/servers.h:177:            VLOG(2) << "  Online Duration: " << socketConnection->getOnlineDuration();
src/apps/http/model/servers.h:179:            VLOG(2) << "     Total Queued: " << socketConnection->getTotalQueued();
src/apps/http/model/servers.h:180:            VLOG(2) << "       Total Sent: " << socketConnection->getTotalSent();
src/apps/http/model/servers.h:181:            VLOG(2) << "      Write Delta: " << socketConnection->getTotalQueued() - socketConnection->getTotalSent();
src/apps/http/model/servers.h:182:            VLOG(2) << "       Total Read: " << socketConnection->getTotalRead();
src/apps/http/model/servers.h:183:            VLOG(2) << "  Total Processed: " << socketConnection->getTotalProcessed();
src/apps/http/model/servers.h:184:            VLOG(2) << "       Read Delta: " << socketConnection->getTotalRead() - socketConnection->getTotalProcessed();
src/apps/http/httplowlevelclient.cpp:73:                VLOG(1) << "++   OnStarted";
src/apps/http/httplowlevelclient.cpp:76:                VLOG(1) << "++   OnParsed";
src/apps/http/httplowlevelclient.cpp:79:                VLOG(1) << "++   OnError: " + std::to_string(status) + " - " + reason;
src/apps/http/httplowlevelclient.cpp:95:            VLOG(1) << "SimpleSocketProtocol connected";
src/apps/http/httplowlevelclient.cpp:98:            VLOG(1) << "SimpleSocketProtocol disconnected";
src/apps/http/httplowlevelclient.cpp:152:                VLOG(1) << "OnConnect";
src/apps/http/httplowlevelclient.cpp:154:                VLOG(1) << "\tServer: " << socketConnection->getRemoteAddress().toString();
src/apps/http/httplowlevelclient.cpp:155:                VLOG(1) << "\tClient: " << socketConnection->getLocalAddress().toString();
src/apps/http/httplowlevelclient.cpp:167:                VLOG(1) << "OnConnected";
src/apps/http/httplowlevelclient.cpp:173:                    VLOG(1) << "     Server certificate: " + std::string(X509_verify_cert_error_string(verifyErr));
src/apps/http/httplowlevelclient.cpp:176:                    VLOG(1) << "        Subject: " + std::string(str);
src/apps/http/httplowlevelclient.cpp:180:                    VLOG(1) << "        Issuer: " + std::string(str);
src/apps/http/httplowlevelclient.cpp:190:                    VLOG(1) << "\t   Subject alternative name count: " << altNameCount;
src/apps/http/httplowlevelclient.cpp:197:                            VLOG(1) << "\t      SAN (URI): '" + subjectAltName;
src/apps/http/httplowlevelclient.cpp:202:                            VLOG(1) << "\t      SAN (DNS): '" + subjectAltName;
src/apps/http/httplowlevelclient.cpp:204:                            VLOG(1) << "\t      SAN (Type): '" + std::to_string(generalName->type);
src/apps/http/httplowlevelclient.cpp:212:                    VLOG(1) << "     Server certificate: no certificate";
src/apps/http/httplowlevelclient.cpp:218:                VLOG(1) << "OnDisconnect";
src/apps/http/httplowlevelclient.cpp:220:                VLOG(1) << "\tServer: " + socketConnection->getRemoteAddress().toString();
src/apps/http/httplowlevelclient.cpp:221:                VLOG(1) << "\tClient: " + socketConnection->getLocalAddress().toString();
src/apps/http/httplowlevelclient.cpp:234:                        VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
src/apps/http/httplowlevelclient.cpp:237:                        VLOG(1) << instanceName << ": disabled";
src/apps/http/httplowlevelclient.cpp:262:                VLOG(1) << "OnConnect";
src/apps/http/httplowlevelclient.cpp:264:                VLOG(1) << "\tServer: " << socketConnection->getRemoteAddress().toString();
src/apps/http/httplowlevelclient.cpp:265:                VLOG(1) << "\tClient: " << socketConnection->getLocalAddress().toString();
src/apps/http/httplowlevelclient.cpp:268:                VLOG(1) << "OnConnected";
src/apps/http/httplowlevelclient.cpp:273:                VLOG(1) << "OnDisconnect";
src/apps/http/httplowlevelclient.cpp:275:                VLOG(1) << "\tServer: " << socketConnection->getRemoteAddress().toString();
src/apps/http/httplowlevelclient.cpp:276:                VLOG(1) << "\tClient: " << socketConnection->getLocalAddress().toString();
src/apps/http/httplowlevelclient.cpp:283:        VLOG(1) << "###############': " << remoteAddress.getCanonName();
src/apps/http/httplowlevelclient.cpp:284:        VLOG(1) << "###############': " << remoteAddress.toString();
src/apps/http/httplowlevelclient.cpp:292:                                         VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
src/apps/http/httplowlevelclient.cpp:295:                                         VLOG(1) << instanceName << ": disabled";
src/apps/http/httplowlevelclient.cpp:327:                                         VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
src/apps/http/httplowlevelclient.cpp:330:                                         VLOG(1) << instanceName << ": disabled";
src/apps/http/httplowlevelclient.cpp:354:                        VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
src/apps/http/httplowlevelclient.cpp:357:                        VLOG(1) << instanceName << ": disabled";
src/apps/http/verysimpleserver.cpp:73:                                 VLOG(1) << instanceName << " listening on '" << socketAddress.toString() << "'";
src/apps/http/verysimpleserver.cpp:76:                                 VLOG(1) << instanceName << " disabled";
src/apps/http/verysimpleserver.cpp:107:                    VLOG(1) << instanceName << " listening on '" << socketAddress.toString() << "'";
src/apps/http/verysimpleserver.cpp:110:                    VLOG(1) << instanceName << " disabled";
src/apps/configtest.cpp:85:    VLOG(1) << "Filename: " << filename;
src/apps/testregex.cpp:107:                VLOG(1) << "Move on to the next route to query database";
src/apps/testregex.cpp:111:                VLOG(1) << "UserId: " << req->params["userId"];
src/apps/testregex.cpp:162:                            VLOG(1) << "Move on to the next route to send result";
src/apps/testregex.cpp:167:                        VLOG(1) << "Error: " << errorString << " : " << errorNumber;
src/apps/testregex.cpp:172:                VLOG(1) << "And again 1: Move on to the next route to send result";
src/apps/testregex.cpp:176:                VLOG(1) << "And again 2: Move on to the next route to send result";
src/apps/testregex.cpp:180:            VLOG(1) << "And again 3: Move on to the next route to send result";
src/apps/testregex.cpp:184:            VLOG(1) << "SendResult";
src/apps/testregex.cpp:195:        VLOG(1) << "Show account of";
src/apps/testregex.cpp:196:        VLOG(1) << "UserId: " << req->params["userId"];
src/apps/testregex.cpp:197:        VLOG(1) << "UserName: " << req->params["userName"];
src/apps/testregex.cpp:222:                VLOG(1) << "Inserted: -> " << userId << " - " << userName;
src/apps/testregex.cpp:225:                VLOG(1) << "Error: " << errorString << " : " << errorNumber;
src/apps/testregex.cpp:231:        VLOG(1) << "Testing Regex";
src/apps/testregex.cpp:232:        VLOG(1) << "Regex1: " << req->params["testRegex1"];
src/apps/testregex.cpp:233:        VLOG(1) << "Regex2: " << req->params["testRegex2"];
src/apps/testregex.cpp:255:        VLOG(1) << "Show Search of";
src/apps/testregex.cpp:256:        VLOG(1) << "Search: " << req->params["search"];
src/apps/testregex.cpp:257:        VLOG(1) << "Queries: " << req->query("test");
src/apps/testregex.cpp:290:            VLOG(0) << "MySQL error: " << state.errorMessage << " [" << state.error << "]";
src/apps/testregex.cpp:292:            VLOG(0) << "MySQL connected";
src/apps/testregex.cpp:294:            VLOG(0) << "MySQL disconnected";
src/apps/testregex.cpp:306:                    VLOG(1) << "legacy-testregex: listening on '" << socketAddress.toString() << "'";
src/apps/testregex.cpp:309:                    VLOG(1) << "legacy-testregex: disabled";
src/apps/testregex.cpp:312:                    VLOG(1) << "legacy-testregex: error occurred";
src/apps/testregex.cpp:315:                    VLOG(1) << "legacy-testregex: fatal error occurred";
src/apps/testregex.cpp:321:            VLOG(1) << "OnConnect:";
src/apps/testregex.cpp:323:            VLOG(1) << "\tServer: " + socketConnection->getRemoteAddress().toString();
src/apps/testregex.cpp:324:            VLOG(1) << "\tClient: " + socketConnection->getLocalAddress().toString();
src/apps/testregex.cpp:328:            VLOG(1) << "OnDisconnect:";
src/apps/testregex.cpp:330:            VLOG(1) << "\tServer: " + socketConnection->getRemoteAddress().toString();
src/apps/testregex.cpp:331:            VLOG(1) << "\tClient: " + socketConnection->getLocalAddress().toString();
src/apps/testregex.cpp:341:                    VLOG(1) << "tls-testregex: listening on '" << socketAddress.toString() << "'";
src/apps/testregex.cpp:344:                    VLOG(1) << "tls-testregex: disabled";
src/apps/testregex.cpp:347:                    VLOG(1) << "tls-testregex: error occurred";
src/apps/testregex.cpp:350:                    VLOG(1) << "tls-testregex: fatal error occurred";
src/apps/testregex.cpp:356:            VLOG(1) << "OnConnect:";
src/apps/testregex.cpp:358:            VLOG(1) << "\tServer: " + socketConnection->getRemoteAddress().toString();
src/apps/testregex.cpp:359:            VLOG(1) << "\tClient: " + socketConnection->getLocalAddress().toString();
src/apps/testregex.cpp:363:            VLOG(1) << "OnConnected:";
src/apps/testregex.cpp:370:                VLOG(1) << "\tClient certificate: " + std::string(X509_verify_cert_error_string(verifyErr));
src/apps/testregex.cpp:373:                VLOG(1) << "\t   Subject: " + std::string(str);
src/apps/testregex.cpp:377:                VLOG(1) << "\t   Issuer: " + std::string(str);
src/apps/testregex.cpp:387:                VLOG(1) << "\t   Subject alternative name count: " << altNameCount;
src/apps/testregex.cpp:394:                        VLOG(1) << "\t      SAN (URI): '" + subjectAltName;
src/apps/testregex.cpp:399:                        VLOG(1) << "\t      SAN (DNS): '" + subjectAltName;
src/apps/testregex.cpp:401:                        VLOG(1) << "\t      SAN (Type): '" + std::to_string(generalName->type);
src/apps/testregex.cpp:409:                VLOG(1) << "\tClient certificate: no certificate";
src/apps/testregex.cpp:414:            VLOG(1) << "OnDisconnect:";
src/apps/testregex.cpp:416:            VLOG(1) << "\tServer: " + socketConnection->getRemoteAddress().toString();
src/apps/testregex.cpp:417:            VLOG(1) << "\tClient: " + socketConnection->getLocalAddress().toString();

```

## Semantic owner/helper used

- `snode::semantic::mariaDbLog()` for DB/MariaDB.
- `snode::semantic::tlsConfigLog()` for TLS config/SNI.
- `snode::semantic::appLog()` for application/example style code under `src/apps`.
- `snode::semantic::expressLog()` for Express framework dispatch/middleware leftovers.
- `snode::semantic::webHttpLog()` for web/http leftovers.
- `snode::semantic::coreSocketLog()` and `coreSystemLog()` for core socket/dynamic-loader leftovers.
- `snode::semantic::netConfigLog()` for net config leftovers.
- `snode::semantic::sysError(...)` for PLOG/errno-sensitive sites.

## Component names used

- `db.mariadb`
- `net.config.tls`
- `app`
- `express`
- `web.http`
- `core.socket`
- `core.system`
- `net.config`
- `framework`

All component names are stable, lowercase, dot-separated where hierarchical, and are not generated from user data.

## Severity mapping

- `LOG(TRACE)` -> `.trace()`
- `LOG(DEBUG)` -> `.debug()`
- `LOG(INFO)` -> `.info()`
- `LOG(WARNING)` -> `.warn()`
- `LOG(ERROR)` -> `.error()`
- `LOG(FATAL)` -> `.critical()`
- `PLOG(level)` -> `snode::semantic::sysError(..., logger::LogLevel::<level>)`

## PLOG/sysError errno handling

PLOG migrations use `snode::semantic::sysError(...)`. Its constructor default argument captures `errno` at the replacement expression before stream formatting, preserving errno code/text. `FinalCleanupMigration09Test` verifies `ENOENT` code and text preservation through the sink path.

## VLOG result

No VLOG was migrated. 531 VLOG entries remain after Migration 9 because VLOG has numeric verbose-level semantics that are not one-to-one with semantic log levels.

## Payload preservation notes

- DB/MariaDB messages retain connection name, connect/start/completed command info, error strings, error numbers, lost connection state, and library-init return code payloads.
- TLS config/SNI messages retain instance name, SNI hostname/domain/SAN values, explicit/master/SAN installation details, lookup/found/not-found state, and SNI list output.
- Application/example messages retain application names, instance names, socket addresses, URLs, request paths, state messages, protocol/application prefixes, and errno-derived messages.

## Expensive payload / dump guard notes

The migration does not introduce SQL dumps, JSON dumps, packet dumps, certificate dumps, or hex dumps. Stream-style semantic logging only appends values when the helper stream is enabled. The required test also covers a threshold-suppressed expensive value through the semantic helper and an explicit expensive dump guard.

## Post-migration inventory command/result

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" \
  src apps examples \
  -g '*.h' -g '*.hpp' -g '*.cpp' \
  -g '!src/log/**'
```

Result: 531 entries, all VLOG. There are 0 remaining LOG/PLOG call sites outside `src/log`.

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" \
  src/log \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

Result: 6 logging-infrastructure macro definitions; intentionally not migrated.

## Filter/backend/format compatibility

`FinalCleanupMigration09Test` covers enabled emission, LogManager filtering, backend `Logger::setLogLevel` filtering, JSON format selection, sink-taking overloads, sysError errno preservation, representative DB/TLS/app payload preservation, and suppressed expensive formatting/guard behavior.

## Production-scope confirmations

- This PR changes only allowed Migration 9 files.
- This PR is not inventory-only.
- This PR completes the remaining production/application/example semantic logging migration.
- This PR does not split Migration 9.
- This PR includes `src/net/config/stream/tls` inspection and migration.
- This PR does not modify `SemanticLogger.*`.
- This PR does not modify `LogScopeOwner.*`.
- This PR does not modify `Logger.*`.
- This PR does not modify `ConfigInstance.cpp`.
- This PR does not modify LOG/PLOG/VLOG macro definitions.
- This PR does not change LogManager precedence.
- This PR does not change LogManager freeze behavior.
- This PR does not change JSON v1 schema.
- This PR does not modify external mqttsuite repository.
- This PR does not modify previous migration tests except `tests/unit/log/CMakeLists.txt` registration.
- This PR does not start Round 10.

## Build commands run

- `git diff --check`
- `cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON`
- `cmake --build cmake-build --parallel 2`
- `cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON`
- `cmake --build cmake-build-asan --parallel 2 --target FinalCleanupMigration09Test`

## Test commands run

- `ctest --test-dir cmake-build -R "SemanticLoggerRound2Test|LogScopeOwnerRound3Test|ProductionLogApiRound4Test|SocketEndpointLogApiRound5Test|SemanticBackendRound6Test|SemanticFilterRound7Test|ControlledMigrationRound8Test|SemanticCompatibilityRound9Test|SemanticOverheadRound9Test|SemanticProductionThresholdRepairTest|SocketConnectionMigration01Test|SocketConnectorAcceptorMigration02Test|SocketServerClientMigration03Test|CoreRuntimeMigration04Test|NetPhysicalSocketMigration05Test|TlsSocketStreamMigration06Test|HttpClientMigration07aTest|HttpServerMigration07bTest|WebSocketMigration07cTest|MqttMigration08Test|FinalCleanupMigration09Test" --output-on-failure`
- `ctest --test-dir cmake-build --output-on-failure`
- `ASAN=$(gcc -print-file-name=libasan.so) LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan -R FinalCleanupMigration09Test --output-on-failure`

## ASan result

Focused `FinalCleanupMigration09Test` passed under ASan. Full ASan was started but not completed because the full ASan build was still compiling the entire application/test graph after the focused ASan requirement had passed; the exact command was interrupted before ctest execution.

## Final migration status

Complete final production/application/example Migration 9 is implemented in one PR. No suitable LOG/PLOG call sites remain in `src` outside `src/log`. Remaining VLOG sites are explicitly deferred due numeric verbose-level semantics.

## Known follow-ups

- Round 10 — book integration
