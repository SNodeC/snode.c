# Semantic identity message cleanup report

## Scope

This PR completed Phase 1 only. Phase 2 text formatting and terminal coloring were intentionally deferred because the identity audit found enough call-site cleanup to keep this PR focused and reviewable.

## Searches performed

The audit was started before editing with these repository-wide searches:

```sh
rg -n "getConnectionName|getInstanceName" src tests/unit/log
rg -n "<<.*get(Connection|Instance)Name\(" src
rg -n "get(Connection|Instance)Name\(\).*<<" src
rg -n "(frameworkLog|\blog\(|appLog|expressLog|httpLog|httpClientLog|webSocketLog|websocketLog|mqttLog|mqttBrokerLog|mqttSessionLog|mqttWebSocketLog|coreSocketLog|tlsConfigLog).*get(Connection|Instance)Name" src
rg -n "(getSocketConnection\(\)->getConnectionName|getSocketConnection\(\)->getInstanceName|socketConnection->getConnectionName|socketConnection->getInstanceName|config->getInstanceName|Super::getConnectionName|Super::getInstanceName)" src
```

The first pass found 268 broad identity-name occurrences in `src` and `tests/unit/log`, with the concentrated duplicated semantic-message prefixes in HTTP socket contexts and TLS stream wrappers.

## Files audited

The audit covered the requested production and logging-test areas:

- `src/core/socket`
- `src/core/socket/stream`
- `src/core/socket/stream/tls`
- `src/web/http`
- `src/web/http/client/tools`
- `src/web/websocket`
- `src/iot/mqtt`
- `src/express`
- `src/apps`
- `tests/unit/log`

## Files changed

- `src/web/http/client/SocketContext.cpp`
- `src/web/http/server/SocketContext.cpp`
- `src/core/socket/stream/tls/SocketAcceptor.hpp`
- `src/core/socket/stream/tls/SocketConnector.hpp`
- `src/core/socket/stream/tls/SocketConnection.hpp`
- `tests/unit/log/SemanticIdentityMessagePolicyTest.cpp`
- `tests/unit/log/CMakeLists.txt`
- `docs/logging/semantic-identity-message-cleanup-report.md`

## Removed duplicated identity prefixes

Removed current connection prefixes from `frameworkLog()` message text in the HTTP client/server socket contexts. Those loggers are created by `core::socket::stream::SocketContext` with concrete `inst=` and `conn=` fields from the owning socket connection, so the free-text message no longer needs `getSocketConnection()->getConnectionName()`.

Removed current instance prefixes from TLS socket acceptor/connector `this->log()` messages. Those object loggers inherit the stream connector/acceptor instance scope, so `config->getInstanceName()` was duplicated when used as a leading `{}` argument.

Removed current connection prefixes from TLS socket connection shutdown messages emitted through the scoped stream socket connection logger. That logger carries the concrete socket `conn=` field, so leading `Super::getConnectionName()` format arguments were duplicated.

## Remaining identity-name uses intentionally kept

Remaining `getConnectionName()` / `getInstanceName()` uses were kept by class:

- Log scope construction in socket connections, contexts, clients, servers, acceptors, connectors, and configuration objects. These are the structured identity source and must remain.
- Non-logging logic, including event receiver names, flow controller names, request construction, OpenSSL ex-data, TLS handshake/shutdown helper names, SNI lookup messages, and user-facing request/response objects.
- Static helper semantic logs such as `coreSocketLog()`, `httpClientLog()`, `webSocketLog()`, `mqttWebSocketLog()`, `expressLog()`, and `tlsConfigLog()` where the helper itself does not carry the concrete current `conn=`/`inst=` value. These may still need a name in the message until those helpers are redesigned or wrapped with object scopes.
- Application/demo code under `src/apps`, where output is intentionally user-facing and may name the connection or instance explicitly.
- Tests and source-policy checks under `tests/unit/log`, where identity names are used to verify scope construction and expected propagation.

## Static-helper follow-up

Static semantic helper logs still produce component/boundary records without a concrete connection or instance in many call sites. This PR does not redesign those helpers, convert them to object-bound loggers, or strip identity in the formatter. A future PR can decide whether specific static-helper surfaces should grow object-owned scopes; until then, keeping explicit names in those messages is intentional.

## Phase 2 status

Semantic text format and terminal coloring were not changed in this PR. JSON output was not changed.
