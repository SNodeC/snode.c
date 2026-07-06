# Semantic logging migration 06 — TLS socket-stream internals inventory report

## Implementation summary

This is a clean inventory-only Migration 6 PR from the current local base that already contains PR #151, #154, #155, #156, #157, and #158. It does not duplicate any previous semantic logging PR.

The initial TLS socket-stream macro inventory found **67 production LOG/PLOG/VLOG call sites** under `src/core/socket/stream/tls`, which exceeds the stop/split threshold of 35 call sites. Per the Migration 6 instructions, this PR stops before production edits and recommends splitting the work into:

- Migration 6a — TLS reader/writer I/O paths
- Migration 6b — TLS handshake/shutdown/error helpers

No TLS socket-stream production logging call sites were migrated in this inventory-only PR.

## Exact changed files

- `docs/logging/semantic-migration-06-tls-socket-stream-report.md`

## Base verification result

The required prerequisite files for PR #151 and migrations #154, #155, #156, #157, and #158 are present, as are the Round 8 and Round 9 report/test files. The container did not have an `origin` remote, so `git fetch origin`, `git checkout master`, and `git pull --ff-only origin master` could not be completed against a remote. The local commit history includes the merge commits through PR #158.

## Initial TLS socket-stream inventory command/result

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" \
  src/core/socket/stream/tls \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

Result: **67 call sites**.

```text
src/core/socket/stream/tls/SocketConnection.hpp:169:                    LOG(DEBUG) << Super::getConnectionName() << " SSL/TLS: Passive close_notify received and sent";
src/core/socket/stream/tls/SocketConnection.hpp:171:                    LOG(DEBUG) << Super::getConnectionName() << " SSL/TLS: Active close_notify sent";
src/core/socket/stream/tls/SocketConnection.hpp:181:                LOG(ERROR) << Super::getConnectionName() << " SSL/TLS: Shutdown handshake timed out";
src/core/socket/stream/tls/SocketConnection.hpp:205:                LOG(DEBUG) << Super::getConnectionName() << " SSL/TLS: Active close_notify sent and received";
src/core/socket/stream/tls/SocketConnection.hpp:212:                LOG(DEBUG) << Super::getConnectionName() << " SSL/TLS: Passive close_notify received, answering with close_notify";
src/core/socket/stream/tls/SocketConnection.hpp:217:            LOG(ERROR) << Super::getConnectionName() << " SSL/TLS: Unexpected EOF error";
src/core/socket/stream/tls/SocketConnection.hpp:227:            LOG(DEBUG) << Super::getConnectionName() << " SSL/TLS: Active send close_notify";
src/core/socket/stream/tls/ssl_utils.cpp:90:            LOG(DEBUG) << connectionName << ": SSL/TLS verify success at depth=" << depth;
src/core/socket/stream/tls/ssl_utils.cpp:91:            LOG(DEBUG) << "   Issuer: " << issuerName;
src/core/socket/stream/tls/ssl_utils.cpp:92:            LOG(DEBUG) << "  Subject: " << subjectName;
src/core/socket/stream/tls/ssl_utils.cpp:96:            LOG(DEBUG) << connectionName << ": SSL/TLS verify error at depth=" << depth << ": " << X509_verify_cert_error_string(err);
src/core/socket/stream/tls/ssl_utils.cpp:97:            LOG(DEBUG) << "   Issuer: " << issuerName;
src/core/socket/stream/tls/ssl_utils.cpp:98:            LOG(DEBUG) << "  Subject: " << subjectName;
src/core/socket/stream/tls/ssl_utils.cpp:140:                        LOG(TRACE) << sslConfig.instanceName << " SSL/TLS: CA certificate loaded";
src/core/socket/stream/tls/ssl_utils.cpp:141:                        LOG(TRACE) << "  " << sslConfig.caCert;
src/core/socket/stream/tls/ssl_utils.cpp:143:                        LOG(TRACE) << sslConfig.instanceName << " SSL/TLS: CA certificate not loaded from a file";
src/core/socket/stream/tls/ssl_utils.cpp:146:                        LOG(TRACE) << sslConfig.instanceName << " SSL/TLS: CA certificates load from";
src/core/socket/stream/tls/ssl_utils.cpp:147:                        LOG(TRACE) << "  " << sslConfig.caCertDir;
src/core/socket/stream/tls/ssl_utils.cpp:149:                        LOG(TRACE) << sslConfig.instanceName << " SSL/TLS: CA certificates not loaded from a directory";
src/core/socket/stream/tls/ssl_utils.cpp:153:                LOG(TRACE) << sslConfig.instanceName << " SSL/TLS: CA certificate not loaded from a file";
src/core/socket/stream/tls/ssl_utils.cpp:154:                LOG(TRACE) << sslConfig.instanceName << " SSL/TLS: CA certificates not loaded from a directory";
src/core/socket/stream/tls/ssl_utils.cpp:161:                    LOG(TRACE) << sslConfig.instanceName << " SSL/TLS: CA certificates enabled load from default openssl CA directory";
src/core/socket/stream/tls/ssl_utils.cpp:164:                LOG(TRACE) << sslConfig.instanceName << " SSL/TLS: CA certificates not loaded from default openssl CA directory";
src/core/socket/stream/tls/ssl_utils.cpp:175:                    LOG(TRACE) << sslConfig.instanceName << " SSL/TLS: CA requested verify";
src/core/socket/stream/tls/ssl_utils.cpp:193:                            LOG(TRACE) << "  " << sslConfig.certKey;
src/core/socket/stream/tls/ssl_utils.cpp:196:                            LOG(TRACE) << sslConfig.instanceName << " SSL/TLS: Cert chain key loaded";
src/core/socket/stream/tls/ssl_utils.cpp:197:                            LOG(TRACE) << "  " << sslConfig.certKey;
src/core/socket/stream/tls/ssl_utils.cpp:199:                            LOG(TRACE) << sslConfig.instanceName << " SSL/TLS: Cert chain loaded";
src/core/socket/stream/tls/ssl_utils.cpp:200:                            LOG(TRACE) << "  " << sslConfig.cert;
src/core/socket/stream/tls/ssl_utils.cpp:342:        LOG(ERROR) << message;
src/core/socket/stream/tls/ssl_utils.cpp:343:        LOG(ERROR) << "  " << ERR_error_string(ERR_get_error(), nullptr);
src/core/socket/stream/tls/ssl_utils.cpp:347:            LOG(ERROR) << "  " << ERR_error_string(errorCode, nullptr);
src/core/socket/stream/tls/ssl_utils.cpp:352:        LOG(WARNING) << message;
src/core/socket/stream/tls/ssl_utils.cpp:353:        LOG(WARNING) << "  " << ERR_error_string(ERR_get_error(), nullptr);
src/core/socket/stream/tls/ssl_utils.cpp:357:            LOG(WARNING) << "  " << ERR_error_string(errorCode, nullptr);
src/core/socket/stream/tls/ssl_utils.cpp:362:        LOG(INFO) << message;
src/core/socket/stream/tls/ssl_utils.cpp:363:        LOG(INFO) << "  " << ERR_error_string(ERR_get_error(), nullptr);
src/core/socket/stream/tls/ssl_utils.cpp:367:            LOG(INFO) << "  " << ERR_error_string(errorCode, nullptr);
src/core/socket/stream/tls/SocketReader.cpp:77:                        LOG(TRACE) << getName() << " SSL/TLS: Start renegotiation on read";
src/core/socket/stream/tls/SocketReader.cpp:80:                                LOG(DEBUG) << getName() << " SSL/TLS: Renegotiation on read success";
src/core/socket/stream/tls/SocketReader.cpp:83:                                LOG(WARNING) << getName() << " SSL/TLS: Renegotiation on read timed out";
src/core/socket/stream/tls/SocketReader.cpp:107:                                PLOG(DEBUG) << getName() << " SSL/TLS: EOF detected: Connection closed by peer.";
src/core/socket/stream/tls/SocketReader.cpp:109:                                PLOG(WARNING) << getName() + " SSL/TLS: Syscall error on read";
src/core/socket/stream/tls/SocketConnector.hpp:81:                  LOG(TRACE) << socketConnection->getConnectionName() << " SSL/TLS: Start handshake";
src/core/socket/stream/tls/SocketConnector.hpp:86:                              LOG(DEBUG) << socketConnection->getConnectionName() << " SSL/TLS: Handshake success";
src/core/socket/stream/tls/SocketConnector.hpp:93:                              LOG(ERROR) << socketConnection->getConnectionName() << " SSL/TLS: Handshake timed out";
src/core/socket/stream/tls/SocketConnector.hpp:102:                      LOG(ERROR) << socketConnection->getConnectionName() + " SSL/TLS: Handshake failed";
src/core/socket/stream/tls/SocketConnector.hpp:139:            LOG(TRACE) << config->getInstanceName() << " SSL/TLS: SSL_CTX creating ...";
src/core/socket/stream/tls/SocketConnector.hpp:142:                LOG(DEBUG) << config->getInstanceName() << " SSL/TLS: SSL_CTX created";
src/core/socket/stream/tls/SocketConnector.hpp:146:                LOG(ERROR) << config->getInstanceName() << " SSL/TLS: SSL_CTX creation failed";
src/core/socket/stream/tls/SocketAcceptor.hpp:78:                  LOG(TRACE) << socketConnection->getConnectionName() << " SSL/TLS: Start handshake";
src/core/socket/stream/tls/SocketAcceptor.hpp:83:                              LOG(DEBUG) << socketConnection->getConnectionName() << " SSL/TLS: Handshake success";
src/core/socket/stream/tls/SocketAcceptor.hpp:90:                              LOG(ERROR) << socketConnection->getConnectionName() << "SSL/TLS: Handshake timed out";
src/core/socket/stream/tls/SocketAcceptor.hpp:99:                      LOG(ERROR) << socketConnection->getConnectionName() + " SSL/TLS: Handshake failed";
src/core/socket/stream/tls/SocketAcceptor.hpp:136:            LOG(TRACE) << config->getInstanceName() << " SSL/TLS: SSL_CTX creating ...";
src/core/socket/stream/tls/SocketAcceptor.hpp:140:                LOG(DEBUG) << config->getInstanceName() << " SSL/TLS: SSL_CTX created";
src/core/socket/stream/tls/SocketAcceptor.hpp:146:                LOG(ERROR) << config->getInstanceName() << " SSL/TLS: SSL/TLS creation failed";
src/core/socket/stream/tls/SocketAcceptor.hpp:169:                LOG(DEBUG) << connectionName << " SSL/TLS: Setting sni certificate for '" << serverNameIndication << "'";
src/core/socket/stream/tls/SocketAcceptor.hpp:172:                LOG(ERROR) << connectionName << " SSL/TLS: No sni certificate found for '" << serverNameIndication
src/core/socket/stream/tls/SocketAcceptor.hpp:177:                LOG(WARNING) << connectionName << " SSL/TLS: No sni certificate found for '" << serverNameIndication
src/core/socket/stream/tls/SocketAcceptor.hpp:181:            LOG(DEBUG) << connectionName << " SSL/TLS: No sni certificate requested from client. Still using master certificate";
src/core/socket/stream/tls/SocketWriter.cpp:71:                        LOG(TRACE) << getName() << " SSL/TLS: Start renegotiation on read";
src/core/socket/stream/tls/SocketWriter.cpp:74:                                LOG(DEBUG) << getName() << " SSL/TLS: Renegotiation on read success";
src/core/socket/stream/tls/SocketWriter.cpp:77:                                LOG(WARNING) << getName() << " SSL/TLS: Renegotiation on read timed out";
src/core/socket/stream/tls/SocketWriter.cpp:99:                                PLOG(WARNING) << getName() << " SSL/TLS: Syscal error (SIGPIPE detected) on write.";
src/core/socket/stream/tls/SocketWriter.cpp:101:                                PLOG(WARNING) << getName() << " SSL/TLS: Connection reset by peer (ECONNRESET).";
src/core/socket/stream/tls/SocketWriter.cpp:103:                                PLOG(WARNING) << getName() << " SSL/TLS: Syscall error on write";
```

## Deferred TLS configuration/SNI inventory command/result

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" \
  src/net/config/stream/tls \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

Result: **10 call sites**, intentionally not modified because TLS configuration/SNI logging is outside Migration 6.

```text
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
```

## Inventory classification table

| Class | Area | Call sites | Notes |
| --- | --- | ---: | --- |
| A | TLS reader | 5 | `SocketReader.cpp` renegotiation and read syscall/EOF paths. |
| B | TLS writer | 6 | `SocketWriter.cpp` renegotiation and write syscall error paths. |
| C | TLS handshake/connect/accept paths | 14 | `SocketConnector.hpp` and `SocketAcceptor.hpp` handshake and SSL_CTX lifecycle paths. |
| D | TLS shutdown/close paths | 7 | `SocketConnection.hpp` close_notify, shutdown timeout, EOF paths. |
| E | OpenSSL error-drain/error-reporting helpers | 9 | `ssl_utils.cpp` `ssl_log*` helpers draining OpenSSL error queue. |
| F | Generic TLS socket-stream lifecycle | 16 | `ssl_utils.cpp` SSL_CTX setup and certificate/CA lifecycle logs. |
| G | VLOG or verbose diagnostics | 0 | No `VLOG` call sites were found in scope. |
| H | Unclear/out-of-scope TLS matches | 10 | `SocketAcceptor.hpp` SNI/client-hello paths are physically in scope but semantically overlap deferred TLS configuration/SNI work and should be handled carefully in a split. |

## Ownership discovery summary

Command:

```sh
rg -n "\blog\(\)|logger::|BoundaryLogger|LogScopeOwner|semantic|scope|SSL|TLS|connectionName|getConnectionName|getInstanceName" \
  src/core/socket/stream/tls \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

Summary: TLS socket-stream files currently rely on connection and instance names (`getConnectionName`, `getInstanceName`) plus OpenSSL callback `ex_data` for identity. No existing `logger::LogScopeOwner` or `logger::BoundaryLogger` owner was found directly inside `src/core/socket/stream/tls`. A later split should prefer existing socket-connection semantic ownership where available and add only minimal local TLS stream owners if no safe owner exists.

## Stop/split decision and reason

The stop/split rule applies because the initial production macro inventory found 67 call sites, which is greater than the 35-call-site threshold. This PR therefore stops before editing production code and recommends splitting Migration 6 into smaller PRs:

1. Migration 6a — TLS reader/writer I/O paths
2. Migration 6b — TLS handshake/shutdown/error helpers

## Post-migration TLS socket-stream inventory command/result

No production migration was performed. The post-migration inventory is therefore expected to match the initial inventory: 67 call sites remain and are deferred to the split migrations.

## Migrated call-site table

| Call site | Result |
| --- | --- |
| None | No call sites migrated because the stop/split threshold was exceeded. |

## Deferred call-site table

All 67 initial `src/core/socket/stream/tls` call sites are deferred because the stop/split threshold was exceeded. The 10 `src/net/config/stream/tls` call sites are also deferred because TLS configuration/SNI logging is outside Migration 6.

## Semantic owner(s) used or added

None. No production files were modified and no semantic owner was added.

## Severity mapping

No severity mapping was applied in this inventory-only PR. Future split migrations should apply the requested `LOG(TRACE|DEBUG|INFO|WARNING|ERROR|FATAL)` to semantic `trace|debug|info|warn|error|critical` mapping, and `PLOG` to `sysError` only for errno-based call sites.

## PLOG/sysError errno handling

No `PLOG` call sites were migrated. Future reader/writer migration should capture `errno` immediately after failed TLS read/write syscall conditions before formatting or helper calls.

## OpenSSL error handling preservation

No OpenSSL error-drain helpers were modified. Future migration must preserve `ERR_get_error()`, `ERR_error_string()`, `SSL_get_error()`, and existing OpenSSL error queue timing.

## VLOG result

No `VLOG` call sites were found under `src/core/socket/stream/tls`.

## Message/identity preservation notes

No messages were changed. Future migrations should preserve connection names, instance names, SSL state/error codes, OpenSSL reason strings, errno code/text, byte counts, handshake phase, shutdown phase, retry wants, and SNI/client-hello identity payloads where applicable.

## Filter/backend/format compatibility

No production semantic calls were added, so no new filter/backend/format compatibility behavior was introduced in this inventory-only PR. Split migrations should add focused tests for LogManager filtering, backend `Logger::setLogLevel` filtering, JSON format selection, sink overload compatibility, and suppressed expensive formatting.

## Production-scope confirmations

- This PR changes only allowed Migration 6 files.
- This PR does not modify `SemanticLogger.*`.
- This PR does not modify `LogScopeOwner.*`.
- This PR does not modify `Logger.*`.
- This PR does not modify `ConfigInstance.cpp`.
- This PR does not modify `src/net/config/stream/tls`.
- This PR does not modify previous socket-stream migration files outside `src/core/socket/stream/tls`.
- This PR does not modify previous core-runtime migration files.
- This PR does not modify previous net physical-socket migration files.
- This PR does not modify previous migration tests.
- This PR does not change `LOG`/`PLOG`/`VLOG` macro definitions.
- This PR does not change LogManager precedence.
- This PR does not change LogManager freeze behavior.
- This PR does not change JSON v1 schema.
- This PR does not start Round 10.

## Build commands run

No build was required for this inventory-only report. `git diff --check` was run.

## Test commands run

No tests were required for this inventory-only report because no production or test code was changed.

## ASan result or exact reason not run

ASan was not run because this PR is report-only and makes no compiled code changes.

## Known follow-ups

- Start Migration 6a for TLS reader/writer I/O paths.
- Start Migration 6b for TLS handshake/shutdown/OpenSSL error helpers.
- Keep TLS configuration/SNI under `src/net/config/stream/tls` deferred unless a future dedicated migration includes it.
