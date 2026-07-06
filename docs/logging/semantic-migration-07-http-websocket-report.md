# Semantic logging migration 07 — HTTP and WebSocket report

## Implementation summary

This is an inventory-only split-planning PR. The initial HTTP/WebSocket logging macro inventory found **134 production macro call sites**, which exceeds the stop/split threshold of 35. Per the Migration 07 instructions, no production files were modified, no test file was added, and no compiled tests were required because no compiled code changed.

This is a clean PR from the current local base that contains PR #151 and migrations #154, #155, #156, #157, #158, and #159. This PR does not duplicate any previous PR.

## Exact changed files

- `docs/logging/semantic-migration-07-http-websocket-report.md`

## Base verification result

- `git fetch origin` could not be completed because this checkout has no `origin` remote configured.
- The current local base is `f28df06 Merge pull request #159 from SNodeC/codex/create-semantic-logging-migration-06`.
- Required PR #151, migration #154-#159, Round 8, and Round 9 report/test files were present.
- A new branch was created from that local base: `codex/semantic-logging-migration-07-http-websocket`.

## Initial HTTP/WebSocket inventory command/result

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\("   src/web/http src/web/websocket   -g '*.h' -g '*.hpp' -g '*.cpp'
```

Result: **134** production macro call sites.

### Complete inventory

| Classification | Call site |
| --- | --- |
| J. WebSocket close/error/control-frame lifecycle | `src/web/websocket/SocketContextUpgrade.hpp:118:            LOG(DEBUG) << this->getSocketConnection()->getConnectionName() << " WebSocketContext: Subprotocol '" << subProtocol->name` |
| J. WebSocket close/error/control-frame lifecycle | `src/web/websocket/SocketContextUpgrade.hpp:189:                    LOG(DEBUG) << getSocketConnection()->getConnectionName() << " WebSocketContext: Subprotocol '" << subProtocol->name` |
| J. WebSocket close/error/control-frame lifecycle | `src/web/websocket/SocketContextUpgrade.hpp:194:                    LOG(DEBUG) << getSocketConnection()->getConnectionName() << " WebSocketContext: Subprotocol '" << subProtocol->name` |
| J. WebSocket close/error/control-frame lifecycle | `src/web/websocket/SocketContextUpgrade.hpp:222:        LOG(INFO) << getSocketConnection()->getConnectionName() << " WebSocketContext: Subprotocol '" << subProtocol->name << "' connect";` |
| J. WebSocket close/error/control-frame lifecycle | `src/web/websocket/SocketContextUpgrade.hpp:229:        LOG(INFO) << getSocketConnection()->getConnectionName() << " WebSocketContext:  Subprotocol '" << subProtocol->name` |
| C. HTTP server | `src/web/http/server/Response.cpp:255:                LOG(DEBUG) << connectionName << " HTTP: Initiating upgrade: " << request->method << " " << request->url` |
| C. HTTP server | `src/web/http/server/Response.cpp:272:                        LOG(DEBUG) << connectionName` |
| C. HTTP server | `src/web/http/server/Response.cpp:279:                            LOG(DEBUG) << connectionName` |
| C. HTTP server | `src/web/http/server/Response.cpp:282:                            LOG(DEBUG) << connectionName << " HTTP upgrade: Response to upgrade request: " << request->method << " "` |
| C. HTTP server | `src/web/http/server/Response.cpp:293:                            LOG(DEBUG) << connectionName` |
| C. HTTP server | `src/web/http/server/Response.cpp:299:                        LOG(DEBUG) << connectionName` |
| C. HTTP server | `src/web/http/server/Response.cpp:305:                    LOG(DEBUG) << connectionName << " HTTP upgrade: No upgrade requested";` |
| C. HTTP server | `src/web/http/server/Response.cpp:310:                LOG(ERROR) << connectionName << " HTTP upgrade: Request has gone away";` |
| C. HTTP server | `src/web/http/server/Response.cpp:315:            LOG(DEBUG) << connectionName << " HTTP: Upgrade bootstrap " << (!socketContextUpgradeName.empty() ? "success" : "failed");` |
| C. HTTP server | `src/web/http/server/Response.cpp:316:            LOG(DEBUG) << "      Protocol selected: " << socketContextUpgradeName;` |
| C. HTTP server | `src/web/http/server/Response.cpp:317:            LOG(DEBUG) << "              requested: " << request->get("upgrade");` |
| C. HTTP server | `src/web/http/server/Response.cpp:318:            LOG(DEBUG) << "  Subprotocol  selected: " << header("Sec-WebSocket-Protocol");` |
| C. HTTP server | `src/web/http/server/Response.cpp:319:            LOG(DEBUG) << "              requested: " << request->get("Sec-WebSocket-Protocol");` |
| C. HTTP server | `src/web/http/server/Response.cpp:323:            LOG(ERROR) << "HTTP upgrade: Unexpected disconnect";` |
| C. HTTP server | `src/web/http/server/SocketContextUpgradeFactorySelector.cpp:69:            LOG(WARNING) << "HTTP upgrade: Overriding http upgrade library dir";` |
| C. HTTP server | `src/web/http/server/SocketContext.cpp:70:                  LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Request start";` |
| C. HTTP server | `src/web/http/server/SocketContext.cpp:73:                  LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Request parse success: " << request.method << " "` |
| C. HTTP server | `src/web/http/server/SocketContext.cpp:83:                  LOG(ERROR) << getSocketConnection()->getConnectionName() << " HTTP: Request parse error: " << reason << " (" << status` |
| C. HTTP server | `src/web/http/server/SocketContext.cpp:112:                LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Request deliver: " << pendingRequest->method << " "` |
| C. HTTP server | `src/web/http/server/SocketContext.cpp:137:            LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: No more pending request";` |
| C. HTTP server | `src/web/http/server/SocketContext.cpp:151:            LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Response start for request: " << pendingRequest->method` |
| C. HTTP server | `src/web/http/server/SocketContext.cpp:153:            LOG(INFO) << getSocketConnection()->getConnectionName() << "   "` |
| C. HTTP server | `src/web/http/server/SocketContext.cpp:168:            LOG(WARNING) << getSocketConnection()->getConnectionName() << " HTTP: Response completed with error: " << response.statusCode` |
| C. HTTP server | `src/web/http/server/SocketContext.cpp:179:        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Response completed for request: " << request->method << " "` |
| C. HTTP server | `src/web/http/server/SocketContext.cpp:181:        LOG(INFO) << getSocketConnection()->getConnectionName() << "   "` |
| C. HTTP server | `src/web/http/server/SocketContext.cpp:195:            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Connection = Close";` |
| C. HTTP server | `src/web/http/server/SocketContext.cpp:199:            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Connection = Keep-Alive";` |
| C. HTTP server | `src/web/http/server/SocketContext.cpp:216:        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Connected";` |
| C. HTTP server | `src/web/http/server/SocketContext.cpp:236:        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Received disconnect";` |
| C. HTTP server | `src/web/http/server/SocketContext.cpp:244:        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Received signal " << signum;` |
| I. WebSocket server | `src/web/websocket/server/SubProtocolFactorySelector.cpp:83:            LOG(WARNING) << "WebSocket: Overriding websocket subprotocol library dir";` |
| G. WebSocket common/frame/protocol utilities | `src/web/websocket/Transmitter.cpp:142:            LOG(TRACE) << "WebSocket send: Frame data\n" << utils::hexDump(payload, payloadLength, 32, true);` |
| G. WebSocket common/frame/protocol utilities | `src/web/websocket/SubProtocolFactorySelector.hpp:70:                    LOG(DEBUG) << "WebSocket: SubProtocolFactory create success: " << subProtocolName;` |
| G. WebSocket common/frame/protocol utilities | `src/web/websocket/SubProtocolFactorySelector.hpp:72:                    LOG(DEBUG) << "WebSocket: SubProtocolFactory create failed: " << subProtocolName;` |
| G. WebSocket common/frame/protocol utilities | `src/web/websocket/SubProtocolFactorySelector.hpp:76:                LOG(DEBUG) << "WebSocket: Optaining function \"" << subProtocolFactoryFunctionName` |
| G. WebSocket common/frame/protocol utilities | `src/web/websocket/SubProtocolFactorySelector.hpp:93:            LOG(DEBUG) << "WebSocket subprotocol: plugin '" << subProtocolName << "' selected from dynamic cache";` |
| G. WebSocket common/frame/protocol utilities | `src/web/websocket/SubProtocolFactorySelector.hpp:98:            LOG(DEBUG) << "WebSocket subprotocol: plugin '" << subProtocolName << "' selected from static cache";` |
| G. WebSocket common/frame/protocol utilities | `src/web/websocket/SubProtocolFactorySelector.hpp:103:            LOG(DEBUG) << "WebSocket subprotocol: plugin '" << subProtocolName << "' loaded and added to dynamic cache";` |
| G. WebSocket common/frame/protocol utilities | `src/web/websocket/SubProtocolFactorySelector.hpp:105:            LOG(WARNING) << "WebSocket subprotocol: plugin '" << subProtocolName << "' not found";` |
| A. HTTP common/message/parser utilities | `src/web/http/tls/rc/EventSource.h:109:            LOG(ERROR) << "EventSource RFCOMM url not accepted: " << url;` |
| G. WebSocket common/frame/protocol utilities | `src/web/websocket/SubProtocol.hpp:72:                        LOG(WARNING) << this->subProtocolContext->getSocketConnection()->getConnectionName() << " Subprotocol '"` |
| G. WebSocket common/frame/protocol utilities | `src/web/websocket/SubProtocol.hpp:132:        LOG(DEBUG) << subProtocolContext->getSocketConnection()->getConnectionName() << " Subprotocol '" << name << "': Ping sent";` |
| G. WebSocket common/frame/protocol utilities | `src/web/websocket/SubProtocol.hpp:144:        LOG(DEBUG) << subProtocolContext->getSocketConnection()->getConnectionName() << " Subprotocol '" << name << "': start";` |
| G. WebSocket common/frame/protocol utilities | `src/web/websocket/SubProtocol.hpp:153:        LOG(DEBUG) << subProtocolContext->getSocketConnection()->getConnectionName() << " Subprotocol '" << name << "': stopped";` |
| G. WebSocket common/frame/protocol utilities | `src/web/websocket/SubProtocol.hpp:155:        LOG(DEBUG) << "       Total Payload sent: " << getPayloadTotalSent();` |
| G. WebSocket common/frame/protocol utilities | `src/web/websocket/SubProtocol.hpp:156:        LOG(DEBUG) << "  Total Payload processed: " << getPayloadTotalRead();` |
| G. WebSocket common/frame/protocol utilities | `src/web/websocket/SubProtocol.hpp:161:        LOG(DEBUG) << subProtocolContext->getSocketConnection()->getConnectionName() << " Subprotocol '" << name << "': Pong received";` |
| A. HTTP common/message/parser utilities | `src/web/http/tls/in/EventSource.h:108:                LOG(ERROR) << "Scheme not valid: " << scheme;` |
| A. HTTP common/message/parser utilities | `src/web/http/tls/in/EventSource.h:111:            LOG(ERROR) << "EventSource url not accepted: " << url;` |
| A. HTTP common/message/parser utilities | `src/web/http/tls/un/EventSource.h:135:                LOG(ERROR) << "UNIX socket must decode to absolute ('/..') or abstract ('@name'): " << sockToken;` |
| A. HTTP common/message/parser utilities | `src/web/http/tls/un/EventSource.h:138:            LOG(ERROR) << "EventSource unix-domain url not accepted: " << url;` |
| H. WebSocket client | `src/web/websocket/client/SubProtocolFactorySelector.cpp:83:            LOG(WARNING) << "WebSocket: Overriding websocket subprotocol library dir";` |
| A. HTTP common/message/parser utilities | `src/web/http/tls/in6/EventSource.h:108:                LOG(ERROR) << "Scheme not valid: " << scheme;` |
| A. HTTP common/message/parser utilities | `src/web/http/tls/in6/EventSource.h:111:            LOG(ERROR) << "EventSource url not accepted: " << url;` |
| G. WebSocket common/frame/protocol utilities | `src/web/websocket/Receiver.cpp:130:                LOG(ERROR) << "WebSocket: Error opcode in continuation frame";` |
| G. WebSocket common/frame/protocol utilities | `src/web/websocket/Receiver.cpp:284:            LOG(TRACE) << "WebSocket receive: Frame data\n" << utils::hexDump(payloadChunk, payloadChunkLen, 32, true);` |
| A. HTTP common/message/parser utilities | `src/web/http/MimeTypes.cpp:249:            LOG(DEBUG) << "HTTP: Cannot load magic database - " + std::string(magic_error(magic));` |
| A. HTTP common/message/parser utilities | `src/web/http/legacy/rc/EventSource.h:109:            LOG(ERROR) << "EventSource RFCOMM url not accepted: " << url;` |
| A. HTTP common/message/parser utilities | `src/web/http/legacy/in/EventSource.h:110:                LOG(ERROR) << "Scheme not valid: " << scheme;` |
| A. HTTP common/message/parser utilities | `src/web/http/legacy/in/EventSource.h:113:            LOG(ERROR) << "EventSource url not accepted: " << url;` |
| B. HTTP client | `src/web/http/client/SocketContextUpgradeFactorySelector.cpp:69:            LOG(WARNING) << "HTTP upgrade: Overriding http upgrade library dir";` |
| A. HTTP common/message/parser utilities | `src/web/http/legacy/un/EventSource.h:135:                LOG(ERROR) << "UNIX socket must decode to absolute ('/..') or abstract ('@name'): " << sockToken;` |
| A. HTTP common/message/parser utilities | `src/web/http/legacy/un/EventSource.h:138:            LOG(ERROR) << "EventSource unix-domain url not accepted: " << url;` |
| B. HTTP client | `src/web/http/client/Request.cpp:367:                        LOG(DEBUG) << connectionName << " HTTP upgrade: Response to upgrade request: " << request->method << " "` |
| B. HTTP client | `src/web/http/client/Request.cpp:386:                                LOG(DEBUG) << connectionName` |
| B. HTTP client | `src/web/http/client/Request.cpp:393:                                    LOG(DEBUG) << connectionName` |
| B. HTTP client | `src/web/http/client/Request.cpp:397:                                    LOG(DEBUG) << connectionName` |
| B. HTTP client | `src/web/http/client/Request.cpp:403:                                LOG(DEBUG) << connectionName << " HTTP upgrade: SocketContextUpgradeFactory not supported by server: "` |
| B. HTTP client | `src/web/http/client/Request.cpp:409:                            LOG(DEBUG) << connectionName << " HTTP upgrade: No upgrade requested";` |
| B. HTTP client | `src/web/http/client/Request.cpp:414:                        LOG(DEBUG) << connectionName << " HTTP upgrade: bootstrap "` |
| B. HTTP client | `src/web/http/client/Request.cpp:416:                        LOG(DEBUG) << "      Protocol selected: " << socketContextUpgradeName;` |
| B. HTTP client | `src/web/http/client/Request.cpp:417:                        LOG(DEBUG) << "              requested: " << request->header("upgrade");` |
| B. HTTP client | `src/web/http/client/Request.cpp:418:                        LOG(DEBUG) << "  Subprotocol  selected: " << response->get("Sec-WebSocket-Protocol");` |
| B. HTTP client | `src/web/http/client/Request.cpp:419:                        LOG(DEBUG) << "              requested: " << request->header("Sec-WebSocket-Protocol");` |
| B. HTTP client | `src/web/http/client/Request.cpp:470:                        LOG(DEBUG) << connectionName << " error in response: " << status;` |
| B. HTTP client | `src/web/http/client/Request.cpp:621:            LOG(DEBUG) << connectionName << " HTTP: "` |
| B. HTTP client | `src/web/http/client/Request.cpp:623:            LOG(DEBUG) << connectionName << " HTTP: Initiating upgrade: " << method << " " << url` |
| B. HTTP client | `src/web/http/client/Request.cpp:627:            LOG(DEBUG) << connectionName << " HTTP: "` |
| B. HTTP client | `src/web/http/client/Request.cpp:629:            LOG(DEBUG) << connectionName << " HTTP: Not initiating upgrade " << method << " " << url` |
| B. HTTP client | `src/web/http/client/Request.cpp:633:        LOG(DEBUG) << connectionName << " HTTP: Upgrade request:\n"` |
| A. HTTP common/message/parser utilities | `src/web/http/legacy/in6/EventSource.h:108:                LOG(ERROR) << "Scheme not valid: " << scheme;` |
| A. HTTP common/message/parser utilities | `src/web/http/legacy/in6/EventSource.h:111:            LOG(ERROR) << "EventSource url not accepted: " << url;` |
| B. HTTP client | `src/web/http/client/tools/EventSource.h:291:            LOG(TRACE) << "Origin: " << sharedState->origin;` |
| B. HTTP client | `src/web/http/client/tools/EventSource.h:292:            LOG(TRACE) << "  Path: " << sharedState->path;` |
| B. HTTP client | `src/web/http/client/tools/EventSource.h:298:                    LOG(DEBUG) << socketConnection->getConnectionName() << ": OnConnect";` |
| B. HTTP client | `src/web/http/client/tools/EventSource.h:305:                    LOG(DEBUG) << socketConnection->getConnectionName() << ": OnConnected";` |
| B. HTTP client | `src/web/http/client/tools/EventSource.h:308:                    LOG(DEBUG) << socketConnection->getConnectionName() << " : OnDisconnect";` |
| B. HTTP client | `src/web/http/client/tools/EventSource.h:341:                    LOG(DEBUG) << connectionName << ": OnRequestStart";` |
| B. HTTP client | `src/web/http/client/tools/EventSource.h:363:                                    LOG(DEBUG) << connectionName << ": server-sent event: server disconnect";` |
| B. HTTP client | `src/web/http/client/tools/EventSource.h:369:                                LOG(DEBUG) << connectionName << ": server-sent event stream start";` |
| B. HTTP client | `src/web/http/client/tools/EventSource.h:389:                                LOG(DEBUG) << connectionName` |
| B. HTTP client | `src/web/http/client/tools/EventSource.h:409:                    LOG(DEBUG) << req->getConnectionName() << ": OnRequestEnd";` |
| B. HTTP client | `src/web/http/client/tools/EventSource.h:425:                        LOG(DEBUG) << instanceName << ": connected to '" << socketAddress.toString() << "'";` |
| B. HTTP client | `src/web/http/client/tools/EventSource.h:428:                        LOG(DEBUG) << instanceName << ": disabled";` |
| B. HTTP client | `src/web/http/client/tools/EventSource.h:431:                        LOG(DEBUG) << instanceName << ": " << socketAddress.toString() << ": " << state.what();` |
| B. HTTP client | `src/web/http/client/tools/EventSource.h:434:                        LOG(DEBUG) << instanceName << ": " << socketAddress.toString() << ": " << state.what();` |
| F. HTTP upgrade path | `src/web/http/SocketContextUpgradeFactorySelector.hpp:100:                        LOG(TRACE) << "HTTP: SocketContextUpgradeFactory create success: " << socketContextUpgradeName;` |
| F. HTTP upgrade path | `src/web/http/SocketContextUpgradeFactorySelector.hpp:102:                        LOG(TRACE) << "HTTP: SocketContextUpgradeFactory already existing: " << socketContextUpgradeName;` |
| F. HTTP upgrade path | `src/web/http/SocketContextUpgradeFactorySelector.hpp:108:                    LOG(ERROR) << "HTTP: SocketContextUpgradeFactory create failed: " << socketContextUpgradeName;` |
| F. HTTP upgrade path | `src/web/http/SocketContextUpgradeFactorySelector.hpp:112:                LOG(ERROR) << "HTTP: Optaining function \"" << socketContextUpgradeFactoryFunctionName` |
| F. HTTP upgrade path | `src/web/http/SocketContextUpgradeFactorySelector.hpp:129:            LOG(DEBUG) << "HTTP upgrade: plugin '" << socketContextUpgradeName << "' selected from dynamic cache";` |
| F. HTTP upgrade path | `src/web/http/SocketContextUpgradeFactorySelector.hpp:133:            LOG(DEBUG) << "HTTP upgrade: plugin '" << socketContextUpgradeName << "' selected from static cache";` |
| F. HTTP upgrade path | `src/web/http/SocketContextUpgradeFactorySelector.hpp:137:            LOG(DEBUG) << "HTTP upgrade: plugin '" << socketContextUpgradeName << "' loaded and added to dynamic cache";` |
| F. HTTP upgrade path | `src/web/http/SocketContextUpgradeFactorySelector.hpp:139:            LOG(WARNING) << "HTTP upgrade: plugin '" << socketContextUpgradeName << "' not found";` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:90:            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Responses missed";` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:92:                LOG(DEBUG) << "  " << request->method << " " << request->url << " HTTP/" << request->httpMajor << "." << request->httpMinor;` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:97:            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Requests ignored";` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:99:                LOG(DEBUG) << "  " << request->method << " " << request->url << " HTTP/" << request->httpMajor << "." << request->httpMinor;` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:115:            LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Request (" << request->count` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:125:            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Request (" << request->count << ") queued: " << requestLine` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:133:            LOG(WARNING) << getSocketConnection()->getConnectionName() << " HTTP: Request (" << request->count` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:136:            LOG(WARNING) << httputils::toString(request->method,` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:159:            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Request (" << request->count << ") start: " << requestLine;` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:162:                LOG(WARNING) << getSocketConnection()->getConnectionName() << " HTTP: Request (" << request->count` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:174:                                LOG(DEBUG) << socketContext->getSocketConnection()->getConnectionName() << " HTTP: Request ("` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:200:            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Request (" << currentRequest->count` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:213:                            LOG(DEBUG) << socketContext->getSocketConnection()->getConnectionName() << " HTTP: Request (" << request->count` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:223:            LOG(WARNING) << getSocketConnection()->getConnectionName() << " HTTP: Request (" << currentRequest->count` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:232:            LOG(ERROR) << getSocketConnection()->getConnectionName() << " HTTP: Response without delivered request";` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:250:        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Response received for request (" << request->count` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:253:        LOG(INFO) << getSocketConnection()->getConnectionName() << "   HTTP/" << response->httpMajor << "." << response->httpMinor << " "` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:258:        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Request (" << request->count << ") completed: " << requestLine;` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:275:        LOG(WARNING) << getSocketConnection()->getConnectionName() << " HTTP: Response parse error: " << reason << " (" << status` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:296:            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Connection = Close";` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:300:            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " HTTP: Connection = Keep-Alive";` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:310:                            LOG(DEBUG) << socketContext->getSocketConnection()->getConnectionName() << " HTTP: Initiating request ("` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:329:        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Connected";` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:363:        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Received disconnect";` |
| B. HTTP client | `src/web/http/client/SocketContext.cpp:367:        LOG(INFO) << getSocketConnection()->getConnectionName() << " HTTP: Received signal " << signum;` |

## Inventory classification table

| Classification | Count |
| --- | ---: |
| A. HTTP common/message/parser utilities | 15 |
| B. HTTP client | 57 |
| C. HTTP server | 30 |
| D. HTTP response/request/body/header lifecycle | 0 |
| E. HTTP express/router/middleware layer | 0 |
| F. HTTP upgrade path | 8 |
| G. WebSocket common/frame/protocol utilities | 17 |
| H. WebSocket client | 1 |
| I. WebSocket server | 1 |
| J. WebSocket close/error/control-frame lifecycle | 5 |
| K. VLOG or verbose diagnostics | 0 |
| L. unclear/out-of-scope matches | 0 |

## Ownership discovery summary

Command:

```sh
rg -n "\blog\(\)|logger::|BoundaryLogger|LogScopeOwner|semantic|scope|getInstanceName|getConnectionName|Request|Response|SocketContext|WebSocket|Upgrade"   src/web/http src/web/websocket   -g '*.h' -g '*.hpp' -g '*.cpp'
```

Summary:

- The discovery output is large because HTTP and WebSocket framework types are heavily represented in this tree.
- Many call sites already have runtime identity payloads such as `getSocketConnection()->getConnectionName()`, request method/URL, response status, upgrade protocol, subprotocol name, and EventSource instance/socket address values.
- The inventory indicates future migrations should prefer existing socket connection / socket context ownership where available rather than adding broad API plumbing.
- No logging infrastructure edits are needed for this inventory-only PR.

## Stop/split decision and reason

The stop/split rule applies because the initial inventory found **134** production macro call sites, which is greater than 35. This PR therefore stops before production edits and records split-planning only.

Recommended split:

1. Migration 7a — HTTP common/client/server core
2. Migration 7b — HTTP express/router/middleware
3. Migration 7c — WebSocket common/client/server

## Deferred call-site table

All **134** initial inventory call sites are deferred by the stop/split rule. Reason: migration would exceed the requested Migration 07 one-PR threshold and should be split before production edits. The complete deferred list is the complete inventory table above.

## Semantic owner(s) used or added

None. This is inventory-only. No production files were modified.

## Severity mapping

No severity mappings were applied. Future split PRs should use the requested mapping: `LOG(TRACE)` to semantic trace, `LOG(DEBUG)` to semantic debug, `LOG(INFO)` to semantic info, `LOG(WARNING)` to semantic warn, `LOG(ERROR)` to semantic error, `LOG(FATAL)` to semantic critical, and `PLOG(level)` to semantic `sysError`.

## PLOG/sysError errno handling

No `PLOG` call sites were migrated in this inventory-only PR. Future split PRs must capture `errno` immediately after failing syscalls before formatting or helper calls.

## VLOG result

No `VLOG` call sites were found or migrated. If future split inventories find `VLOG`, they should remain deferred unless a one-to-one semantic verbose equivalent is explicitly justified.

## HTTP payload preservation notes

No production HTTP logs were changed. Future split PRs should preserve method, URL/path/target, status code, header values where already logged, content length/body details, parser state/reason, route/middleware identity, upgrade target/protocol, connection name, socket address, and errno details where present.

## WebSocket payload preservation notes

No production WebSocket logs were changed. Future split PRs should preserve opcode, close code/reason, frame length, masking/protocol diagnostics, subprotocol name, connection name, and payload totals where present.

## Filter/backend/format compatibility

No compiled code changed. No semantic owners or migrated calls were added in this inventory-only PR, so LogManager filtering, backend filtering, text/JSON formatting, and suppressed formatting behavior are unchanged.

## Production-scope confirmations

- This PR changes only allowed Migration 7 files.
- This PR does not modify SemanticLogger.*.
- This PR does not modify LogScopeOwner.*.
- This PR does not modify Logger.*.
- This PR does not modify ConfigInstance.cpp.
- This PR does not modify src/net/config/stream/tls.
- This PR does not modify src/iot or MQTT-over-WebSocket code.
- This PR does not modify previous migration tests.
- This PR does not change LOG/PLOG/VLOG macro definitions.
- This PR does not change LogManager precedence.
- This PR does not change LogManager freeze behavior.
- This PR does not change JSON v1 schema.
- This PR does not start Round 10.
- This is an inventory-only split-planning PR.
- No production files were modified.
- No test file was added.
- No compiled tests were required because no compiled code changed.

## Build commands run

No build was required because this is inventory-only and no compiled code changed.

## Test commands run

- `git status --short`
- `git diff --name-only master...HEAD`
- `git diff --check`

## ASan result or exact reason not run

ASan was not run because this is inventory-only and no compiled code changed.

## Known follow-ups

- Start Migration 7a from current master for HTTP common/client/server core.
- Start Migration 7b from current master for HTTP express/router/middleware if matching call sites are present after focused inventory.
- Start Migration 7c from current master for WebSocket common/client/server.
