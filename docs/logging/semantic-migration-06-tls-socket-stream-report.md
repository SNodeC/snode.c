# Semantic logging migration 06 — TLS socket-stream internals inventory report

## Implementation summary

This is the accumulated Migration 6 PR from the current local base that already contains PR #151, #154, #155, #156, #157, and #158. It does not duplicate any previous semantic logging PR.

The initial TLS socket-stream macro inventory found **67 production LOG/PLOG/VLOG call sites** under `src/core/socket/stream/tls`, which exceeds the stop/split threshold of 35 call sites. Per the Migration 6 instructions, the work was split and is being completed in staged follow-ups:

- Migration 6 inventory/split: done.
- Migration 6a — TLS reader/writer I/O paths: done.
- Migration 6b — TLS handshake/shutdown/OpenSSL helpers: done.
- Migration 6c — TLS VLOG/unclear/SNI-overlap cleanup and final closure: still open.

This PR remains open until 6c is complete.

## Exact changed files

- `docs/logging/semantic-migration-06-tls-socket-stream-report.md`
- `src/core/socket/stream/tls/SocketReader.h`
- `src/core/socket/stream/tls/SocketReader.cpp`
- `src/core/socket/stream/tls/SocketWriter.h`
- `src/core/socket/stream/tls/SocketWriter.cpp`
- `tests/unit/log/CMakeLists.txt`
- `tests/unit/log/TlsSocketStreamMigration06Test.cpp`
- `src/core/socket/stream/tls/SocketConnection.hpp`
- `src/core/socket/stream/tls/SocketConnector.hpp`
- `src/core/socket/stream/tls/SocketAcceptor.hpp`
- `src/core/socket/stream/tls/ssl_utils.cpp`

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
src/core/socket/stream/tls/SocketAcceptor.hpp:189:            LOG(DEBUG) << connectionName << " SSL/TLS: No sni certificate requested from client. Still using master certificate";
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

The stop/split rule applies because the initial production macro inventory found 67 call sites, which is greater than the 35-call-site threshold. The initial checkpoint therefore stopped before editing production code and split Migration 6 into staged follow-ups. Migration 6a and 6b are now implemented in this accumulated PR, while 6c remains open:

1. Migration 6a — TLS reader/writer I/O paths
2. Migration 6b — TLS handshake/shutdown/error helpers

## Post-migration TLS socket-stream inventory command/result

The original inventory/split checkpoint left all 67 call sites for the split migrations. Migration 6a reduced the reader/writer I/O subset, and Migration 6b reduced the handshake/shutdown/OpenSSL-helper subset. See the 6a and 6b sections below for current post-migration inventories.

## Migrated call-site table

The initial checkpoint migrated no call sites because the stop/split threshold was exceeded. Migration 6a has since migrated the reader/writer I/O subset, and Migration 6b has migrated the handshake/shutdown/OpenSSL-helper subset; see the dedicated sections below.

## Deferred call-site table

The initial checkpoint deferred all 67 `src/core/socket/stream/tls` call sites because the stop/split threshold was exceeded. Migration 6a and 6b have since migrated their scoped subsets. The 10 `src/net/config/stream/tls` call sites remain deferred because TLS configuration/SNI logging is outside Migration 6.

## Semantic owner(s) used or added

The initial checkpoint added none. Migration 6a added TLS reader/writer owners, and this 6b follow-up uses existing socket connection/connector/acceptor owners plus a file-local TLS helper owner for `ssl_utils.cpp`.

## Severity mapping

The initial checkpoint applied no severity mapping. Migration 6a and 6b apply the requested `LOG(TRACE|DEBUG|INFO|WARNING|ERROR|FATAL)` to semantic `trace|debug|info|warn|error|critical` mapping, and `PLOG` to `sysError` only for errno-based call sites in their scoped subsets.

## PLOG/sysError errno handling

Migration 6a migrated errno-based reader/writer `PLOG` call sites and captured `errno` immediately after failed TLS read/write syscall conditions before formatting or helper calls. Migration 6b did not migrate additional `PLOG` call sites.

## OpenSSL error handling preservation

Migration 6b migrated OpenSSL error-drain helpers while preserving `ERR_get_error()`, `ERR_error_string()`, `SSL_get_error()` payloads, and existing OpenSSL error queue timing.

## VLOG result

No `VLOG` call sites were found under `src/core/socket/stream/tls`.

## Message/identity preservation notes

Migration 6a and 6b preserve connection names, instance names, SSL state/error codes, OpenSSL reason strings, errno code/text, byte counts, handshake phase, shutdown phase, retry wants, and SNI/client-hello identity payloads where applicable. Remaining SNI/client-hello overlap messages are deferred unchanged to 6c.

## Filter/backend/format compatibility

Migration 6a and 6b add production semantic calls in their scoped TLS socket-stream subsets. `TlsSocketStreamMigration06Test` covers LogManager filtering, backend `Logger::setLogLevel` filtering, JSON format selection, sink overload compatibility, OpenSSL-helper filtering, and suppressed expensive formatting.

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

Compiled validation is now part of this accumulated PR. See the 6a and 6b sections for the current build commands and results.

## Test commands run

Compiled tests are now part of this accumulated PR. See the 6a and 6b sections for the current focused/full test commands and results.

## ASan result or exact reason not run

ASan validation is now tracked in the staged migration sections below.

## Known follow-ups

- Migration 6b is implemented in this follow-up.
- Migration 6c remains open for TLS VLOG/unclear/SNI-overlap cleanup and final Migration 6 closure.
- Keep TLS configuration/SNI under `src/net/config/stream/tls` deferred unless 6c explicitly accepts a narrow overlap cleanup.

## Migration 6a — TLS reader/writer I/O paths

### 6a implementation summary

Migration 6a is complete. It migrates only the TLS reader/writer I/O macro call sites from the original inventory and keeps Migration 6b and Migration 6c unstarted. The PR is intentionally still open until 6b and 6c are completed.

The 6a production change adds minimal local semantic owners to the TLS `SocketReader` and `SocketWriter` classes because no existing owner was present directly in the TLS reader/writer classes. The owner uses `logger::LogOrigin::Framework`, existing `logger::LogBoundary::Connection`, and component `core.socket.stream.tls`. The already-available reader/writer instance name is used as both instance and connection identity without adding broader constructor/API plumbing.

The migrated reader/writer call sites now emit through `log().trace`, `log().debug`, `log().warn`, and `log().sysError` while preserving the original TLS diagnostic messages, retry direction, renegotiation phase, and errno payloads.

### 6a changed files

- `src/core/socket/stream/tls/SocketReader.h`
- `src/core/socket/stream/tls/SocketReader.cpp`
- `src/core/socket/stream/tls/SocketWriter.h`
- `src/core/socket/stream/tls/SocketWriter.cpp`
- `tests/unit/log/CMakeLists.txt`
- `tests/unit/log/TlsSocketStreamMigration06Test.cpp`
- `src/core/socket/stream/tls/SocketConnection.hpp`
- `src/core/socket/stream/tls/SocketConnector.hpp`
- `src/core/socket/stream/tls/SocketAcceptor.hpp`
- `src/core/socket/stream/tls/ssl_utils.cpp`
- `docs/logging/semantic-migration-06-tls-socket-stream-report.md`

### 6a worklist from the original inventory

| Original call site | 6a decision |
| --- | --- |
| `src/core/socket/stream/tls/SocketReader.cpp:77` `LOG(TRACE)` start renegotiation on read | Migrate in 6a. |
| `src/core/socket/stream/tls/SocketReader.cpp:80` `LOG(DEBUG)` renegotiation on read success | Migrate in 6a. |
| `src/core/socket/stream/tls/SocketReader.cpp:83` `LOG(WARNING)` renegotiation on read timeout | Migrate in 6a. |
| `src/core/socket/stream/tls/SocketReader.cpp:107` `PLOG(DEBUG)` EOF detected on read syscall path | Migrate in 6a with immediate errno capture. |
| `src/core/socket/stream/tls/SocketReader.cpp:109` `PLOG(WARNING)` syscall error on read | Migrate in 6a with immediate errno capture. |
| `src/core/socket/stream/tls/SocketWriter.cpp:71` `LOG(TRACE)` start renegotiation on read while writing | Migrate in 6a. |
| `src/core/socket/stream/tls/SocketWriter.cpp:74` `LOG(DEBUG)` renegotiation success while writing | Migrate in 6a. |
| `src/core/socket/stream/tls/SocketWriter.cpp:77` `LOG(WARNING)` renegotiation timeout while writing | Migrate in 6a. |
| `src/core/socket/stream/tls/SocketWriter.cpp:99` `PLOG(WARNING)` write syscall EPIPE/SIGPIPE | Migrate in 6a with immediate errno capture. |
| `src/core/socket/stream/tls/SocketWriter.cpp:101` `PLOG(WARNING)` write syscall ECONNRESET | Migrate in 6a with immediate errno capture. |
| `src/core/socket/stream/tls/SocketWriter.cpp:103` `PLOG(WARNING)` generic write syscall error | Migrate in 6a with immediate errno capture. |

### 6a migrated call-site table

| File | Original severity | New semantic call | Preservation notes |
| --- | --- | --- | --- |
| `SocketReader.cpp` | `LOG(TRACE)` | `log().trace` | Preserves `getName()` and `SSL/TLS: Start renegotiation on read`. |
| `SocketReader.cpp` | `LOG(DEBUG)` | captured `BoundaryLogger::debug` in callback | Preserves async success message and reader name without storing a logger reference. |
| `SocketReader.cpp` | `LOG(WARNING)` | captured `BoundaryLogger::warn` in callback | Preserves async timeout message and reader name without storing a logger reference. |
| `SocketReader.cpp` | `PLOG(DEBUG)` | `log().sysError(LogLevel::Debug, errnum, ...)` | Captures `errno` immediately in the `SSL_ERROR_SYSCALL` branch and preserves EOF message. |
| `SocketReader.cpp` | `PLOG(WARNING)` | `log().sysError(LogLevel::Warn, errnum, ...)` | Captures `errno` immediately in the `SSL_ERROR_SYSCALL` branch and preserves read syscall message. |
| `SocketWriter.cpp` | `LOG(TRACE)` | `log().trace` | Preserves `getName()` and original renegotiation message text. |
| `SocketWriter.cpp` | `LOG(DEBUG)` | captured `BoundaryLogger::debug` in callback | Preserves async success message and writer name without storing a logger reference. |
| `SocketWriter.cpp` | `LOG(WARNING)` | captured `BoundaryLogger::warn` in callback | Preserves async timeout message and writer name without storing a logger reference. |
| `SocketWriter.cpp` | `PLOG(WARNING)` | `log().sysError(LogLevel::Warn, errnum, ...)` | Captures `errno` immediately and preserves the original EPIPE/SIGPIPE message text, including the existing `Syscal` spelling. |
| `SocketWriter.cpp` | `PLOG(WARNING)` | `log().sysError(LogLevel::Warn, errnum, ...)` | Captures `errno` immediately and preserves ECONNRESET message text. |
| `SocketWriter.cpp` | `PLOG(WARNING)` | `log().sysError(LogLevel::Warn, errnum, ...)` | Captures `errno` immediately and preserves generic write syscall message text. |

### 6a deferred call-site table

| Call site group | Reason |
| --- | --- |
| `SocketReader.cpp` calls to `ssl_log(...)` for renegotiation/read/unexpected OpenSSL statuses | Deferred to 6b because generic OpenSSL error-drain helper migration and error queue timing are explicitly out of 6a. |
| `SocketWriter.cpp` calls to `ssl_log(...)` for renegotiation/write/unexpected OpenSSL statuses | Deferred to 6b because generic OpenSSL error-drain helper migration and error queue timing are explicitly out of 6a. |
| All remaining non-reader/writer macro call sites in `src/core/socket/stream/tls` | Deferred to 6b or 6c per the split. |
| TLS configuration/SNI call sites under `src/net/config/stream/tls` | Deferred; not part of Migration 6a and not modified. |

### Semantic owner(s) used or added for 6a

- Added `logger::LogScopeOwner` to `core::socket::stream::tls::SocketReader`.
- Added `logger::LogScopeOwner` to `core::socket::stream::tls::SocketWriter`.
- Added the standard default `log()` overload backed by `logger::Logger::semanticSink()`.
- Added the standard sink-taking `log(...)` overload for tests/custom capture.
- Scope: framework / connection / `core.socket.stream.tls`; instance and connection are set from the existing reader/writer `instanceName` constructor argument when non-empty.

### Severity mapping

| Original | 6a semantic mapping |
| --- | --- |
| `LOG(TRACE)` | `trace` |
| `LOG(DEBUG)` | `debug` |
| `LOG(WARNING)` | `warn` |
| `PLOG(DEBUG)` | `sysError(LogLevel::Debug, errnum, ...)` |
| `PLOG(WARNING)` | `sysError(LogLevel::Warn, errnum, ...)` |

No `INFO`, `ERROR`, or `FATAL` reader/writer macro call sites were part of 6a.

### PLOG/sysError errno handling

The migrated TLS reader/writer `PLOG` sites capture `errno` as `const int errnum = errno;` inside the `SSL_ERROR_SYSCALL` branch before semantic logging. A `utils::PreserveErrno` guard remains in place around semantic emission so the surrounding read/write return-path behavior keeps the old errno-preservation intent. The semantic records include the structured errno code/text via `BoundaryLogger::sysError`.

### OpenSSL I/O error handling preservation

6a does not move or rewrite the `SSL_get_error` calls and does not change `SSL_ERROR_WANT_READ`, `SSL_ERROR_WANT_WRITE`, `SSL_ERROR_ZERO_RETURN`, `SSL_ERROR_SYSCALL`, `SSL_ERROR_SSL`, or default branch control flow. Existing `ssl_log(...)` calls remain in place for 6b so OpenSSL error queue draining is not moved earlier or later in 6a.

### VLOG deferrals to 6c

No `VLOG` call sites were found in the TLS reader/writer 6a worklist. No `VLOG` call sites were migrated.

### Message/identity preservation notes

6a preserves the original message payloads, including connection/reader/writer names from `getName()`, retry direction (`renegotiation on read`), syscall read/write context, EOF context, SIGPIPE/EPIPE, ECONNRESET, and generic syscall wording. The existing `Syscal` spelling in the writer SIGPIPE message was intentionally preserved to avoid changing diagnostic text during the semantic migration.

### Post-6a inventory

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" \
  src/core/socket/stream/tls \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

Result: **56 call sites** remained after 6a. These were expected at the 6a checkpoint because 6b and 6c were not started yet; the 6b section below records the current post-6b inventory.

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
src/core/socket/stream/tls/SocketAcceptor.hpp:189:            LOG(DEBUG) << connectionName << " SSL/TLS: No sni certificate requested from client. Still using master certificate";
```

### Remaining work for 6b

Migration 6b has since been implemented in this accumulated PR; see the Migration 6b section below.

### Remaining work for 6c

Migration 6c is not started. Remaining 6c work includes unclear/SNI-overlap cleanup, any TLS configuration/SNI deferrals that are intentionally accepted into final Migration 6 scope, and final Migration 6 closure. Round 10 remains out of scope.

### Tests run

- `rg -n "\b(LOG|PLOG|VLOG)\s*\(" src/core/socket/stream/tls -g '*.h' -g '*.hpp' -g '*.cpp'`
- `clang-format -i src/core/socket/stream/tls/SocketReader.h src/core/socket/stream/tls/SocketReader.cpp src/core/socket/stream/tls/SocketWriter.h src/core/socket/stream/tls/SocketWriter.cpp tests/unit/log/TlsSocketStreamMigration06Test.cpp`
- `cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON`
- `cmake --build cmake-build --parallel 2`
- `ctest --test-dir cmake-build -R "SemanticLoggerRound2Test|LogScopeOwnerRound3Test|ProductionLogApiRound4Test|SocketEndpointLogApiRound5Test|SemanticBackendRound6Test|SemanticFilterRound7Test|ControlledMigrationRound8Test|SemanticCompatibilityRound9Test|SemanticOverheadRound9Test|SemanticProductionThresholdRepairTest|SocketConnectionMigration01Test|SocketConnectorAcceptorMigration02Test|SocketServerClientMigration03Test|CoreRuntimeMigration04Test|NetPhysicalSocketMigration05Test|TlsSocketStreamMigration06Test" --output-on-failure`
- `ctest --test-dir cmake-build --output-on-failure`

### ASan result or exact reason not run

The focused Migration 6 TLS socket-stream test was built and run under ASan:

- `cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON`
- `cmake --build cmake-build-asan --parallel 2 --target TlsSocketStreamMigration06Test`
- `ASAN=$(gcc -print-file-name=libasan.so) LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan -R TlsSocketStreamMigration06Test --output-on-failure`

Full ASan was not run in this 6a follow-up to keep the follow-up bounded; the required minimum focused ASan coverage for `TlsSocketStreamMigration06Test` passed.


## Migration 6b — TLS handshake/shutdown/OpenSSL helpers

### 6b implementation summary

Migration 6 inventory/split is complete. Migration 6a is complete. Migration 6b is complete. Migration 6c is not started. The PR is intentionally still open until 6c is completed.

Migration 6b migrates the TLS handshake/connect/accept paths, TLS shutdown/close_notify paths, SSL_CTX lifecycle logging inside `src/core/socket/stream/tls`, OpenSSL verification callback diagnostics, and OpenSSL error-drain helpers. It leaves SNI/client-hello/certificate-selection overlap sites for 6c.

### 6b changed files

- `src/core/socket/stream/tls/SocketConnection.hpp`
- `src/core/socket/stream/tls/SocketConnector.hpp`
- `src/core/socket/stream/tls/SocketAcceptor.hpp`
- `src/core/socket/stream/tls/ssl_utils.cpp`
- `tests/unit/log/TlsSocketStreamMigration06Test.cpp`
- `docs/logging/semantic-migration-06-tls-socket-stream-report.md`

### 6b worklist from the original inventory

| Original call-site group | 6b decision |
| --- | --- |
| `SocketConnection.hpp` close_notify/shutdown/EOF logs | Migrated in 6b using the existing socket connection semantic owner. |
| `SocketConnector.hpp` concrete connection handshake start/success/timeout/failure logs | Migrated in 6b using the concrete socket connection semantic owner. |
| `SocketConnector.hpp` SSL_CTX creation lifecycle logs | Migrated in 6b using the existing connector semantic owner. |
| `SocketAcceptor.hpp` concrete connection handshake start/success/timeout/failure logs | Migrated in 6b using the concrete socket connection semantic owner. |
| `SocketAcceptor.hpp` SSL_CTX creation lifecycle logs | Migrated in 6b using the existing acceptor semantic owner. |
| `ssl_utils.cpp` OpenSSL verification callback diagnostics | Migrated in 6b using a file-local TLS helper scope. |
| `ssl_utils.cpp` SSL_CTX CA/certificate lifecycle logs | Migrated in 6b using a file-local TLS helper scope. |
| `ssl_utils.cpp` `ssl_log_error`, `ssl_log_warning`, `ssl_log_info` queue-drain helpers | Migrated in 6b using a file-local TLS helper scope while preserving `ERR_get_error()` order. |
| `SocketAcceptor.hpp` client-hello/SNI certificate-selection logs | Deferred to 6c. |
| `src/net/config/stream/tls` TLS configuration/SNI logs | Deferred; outside 6b and not modified. |

### 6b migrated call-site table

| File | Original severity | New semantic call | Preservation notes |
| --- | --- | --- | --- |
| `SocketConnection.hpp` | `LOG(DEBUG)` | connection `log().debug` | Preserves passive/active close_notify messages and connection name. |
| `SocketConnection.hpp` | `LOG(ERROR)` | connection `log().error` | Preserves shutdown timeout and unexpected EOF messages. |
| `SocketConnector.hpp` | `LOG(TRACE)` | socket connection `log().trace` | Preserves handshake start and connection name. |
| `SocketConnector.hpp` | `LOG(DEBUG)` | captured connection `BoundaryLogger::debug` | Preserves handshake success payload in callback context. |
| `SocketConnector.hpp` | `LOG(ERROR)` | connection `log().error` / captured connection logger | Preserves handshake timeout/failure payloads. |
| `SocketConnector.hpp` | `LOG(TRACE/DEBUG/ERROR)` | connector `this->log()` | Preserves connector SSL_CTX creation lifecycle and instance name. |
| `SocketAcceptor.hpp` | `LOG(TRACE)` | socket connection `log().trace` | Preserves handshake start and connection name. |
| `SocketAcceptor.hpp` | `LOG(DEBUG)` | captured connection `BoundaryLogger::debug` | Preserves handshake success payload in callback context. |
| `SocketAcceptor.hpp` | `LOG(ERROR)` | connection `log().error` / captured connection logger | Preserves handshake timeout/failure payloads, including the original no-space timeout text. |
| `SocketAcceptor.hpp` | `LOG(TRACE/DEBUG/ERROR)` | acceptor `this->log()` | Preserves acceptor SSL_CTX creation lifecycle and instance name. |
| `ssl_utils.cpp` verify callback | `LOG(DEBUG)` | `tlsLog().debug` | Preserves connection name, verify depth, issuer, subject, and verify error reason text. |
| `ssl_utils.cpp` SSL_CTX lifecycle | `LOG(TRACE)` | `tlsLog().trace` | Preserves instance names and CA/certificate/cert-key path detail lines. |
| `ssl_utils.cpp` OpenSSL helpers | `LOG(ERROR/WARNING/INFO)` | `tlsLog().error/warn/info` | Preserves original message line plus first and remaining queued `ERR_error_string(...)` lines. |

### 6b deferred call-site table

| Call site group | Reason |
| --- | --- |
| `SocketAcceptor.hpp` SNI/client-hello certificate-selection logs | Deferred to 6c because they overlap SNI/configuration policy. |
| `src/net/config/stream/tls` deferred inventory | Deferred to 6c or a future TLS configuration/SNI migration; these files remain untouched. |
| VLOG cleanup | No VLOG sites were present in the post-6b core TLS inventory. |

### Semantic owner(s) used or added for 6b

- `SocketConnection.hpp` uses the existing socket-stream connection semantic owner via explicit `core::socket::stream::SocketConnection::log()` qualification to avoid ambiguity with the 6a reader/writer owners.
- `SocketConnector.hpp` uses the concrete socket connection owner for per-connection handshake logs and the existing connector owner for SSL_CTX lifecycle logs.
- `SocketAcceptor.hpp` uses the concrete socket connection owner for per-connection handshake logs and the existing acceptor owner for SSL_CTX lifecycle logs.
- `ssl_utils.cpp` adds a file-local static `logger::LogScopeOwner` with framework / connection / `core.socket.stream.tls` scope for helper diagnostics that only receive message strings, instance names, or connection names.

### Severity mapping

| Original | 6b semantic mapping |
| --- | --- |
| `LOG(TRACE)` | `trace` |
| `LOG(DEBUG)` | `debug` |
| `LOG(INFO)` | `info` |
| `LOG(WARNING)` | `warn` |
| `LOG(ERROR)` | `error` |

No 6b `PLOG` sites were present.

### PLOG/sysError errno handling

No errno-based `PLOG` call sites were part of Migration 6b. The 6a reader/writer `PLOG` migrations remain unchanged.

### OpenSSL error handling preservation

The OpenSSL helper migration preserves `ERR_get_error()` sequencing by reading the first queue entry before the first semantic error-detail line, then draining remaining entries in the existing loop shape. The original message line remains separate from the first and subsequent OpenSSL error-string lines. `ssl_log(...)` still preserves `errno` with `utils::PreserveErrno`, keeps the existing `SSL_ERROR_*` switch, and does not convert OpenSSL-only diagnostics into `sysError`.

### SNI/overlap deferrals to 6c

The remaining four macro call sites in `SocketAcceptor.hpp` are the client-hello/SNI certificate-selection overlap logs. They are intentionally deferred to 6c with their SNI names and certificate-selection message payloads preserved in place.

### VLOG deferrals to 6c

No `VLOG` call sites remain in `src/core/socket/stream/tls` after 6b.

### Message/identity preservation notes

6b preserves connection names, instance names, SSL_CTX creation/setup wording, handshake start/success/failure/timeout wording, shutdown/close_notify active/passive distinction, unexpected EOF text, verification depth, issuer/subject lines, and OpenSSL error-string payloads. Manual prefixes were retained where helper scopes do not safely carry a stable instance or connection identity.

### Post-6b inventory

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" \
  src/core/socket/stream/tls \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

Result: **4 call sites** remain after 6b. These are the expected SNI/client-hello/certificate-selection overlap sites for 6c.

```text
src/core/socket/stream/tls/SocketAcceptor.hpp:177:                LOG(DEBUG) << connectionName << " SSL/TLS: Setting sni certificate for '" << serverNameIndication << "'";
src/core/socket/stream/tls/SocketAcceptor.hpp:180:                LOG(ERROR) << connectionName << " SSL/TLS: No sni certificate found for '" << serverNameIndication
src/core/socket/stream/tls/SocketAcceptor.hpp:185:                LOG(WARNING) << connectionName << " SSL/TLS: No sni certificate found for '" << serverNameIndication
src/core/socket/stream/tls/SocketAcceptor.hpp:189:            LOG(DEBUG) << connectionName << " SSL/TLS: No sni certificate requested from client. Still using master certificate";
```

### Tests run

- `git status --short`
- `git diff --name-only master...HEAD`
- `git diff --check`
- `rg -n "\b(LOG|PLOG|VLOG)\s*\(" src/core/socket/stream/tls -g '*.h' -g '*.hpp' -g '*.cpp'`
- `clang-format -i src/core/socket/stream/tls/SocketConnection.hpp src/core/socket/stream/tls/SocketConnector.hpp src/core/socket/stream/tls/SocketAcceptor.hpp src/core/socket/stream/tls/ssl_utils.cpp tests/unit/log/TlsSocketStreamMigration06Test.cpp`
- `cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON`
- `cmake --build cmake-build --parallel 2`
- `ctest --test-dir cmake-build -R "SemanticLoggerRound2Test|LogScopeOwnerRound3Test|ProductionLogApiRound4Test|SocketEndpointLogApiRound5Test|SemanticBackendRound6Test|SemanticFilterRound7Test|ControlledMigrationRound8Test|SemanticCompatibilityRound9Test|SemanticOverheadRound9Test|SemanticProductionThresholdRepairTest|SocketConnectionMigration01Test|SocketConnectorAcceptorMigration02Test|SocketServerClientMigration03Test|CoreRuntimeMigration04Test|NetPhysicalSocketMigration05Test|TlsSocketStreamMigration06Test" --output-on-failure`
- `ctest --test-dir cmake-build --output-on-failure`

### ASan result or exact reason not run

The focused Migration 6 TLS socket-stream test was built and run under ASan:

- `cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON`
- `cmake --build cmake-build-asan --parallel 2 --target TlsSocketStreamMigration06Test`
- `ASAN=$(gcc -print-file-name=libasan.so) LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan -R TlsSocketStreamMigration06Test --output-on-failure`

Full ASan was not run in this 6b follow-up to keep the follow-up bounded; the required minimum focused ASan coverage for `TlsSocketStreamMigration06Test` passed.

### Remaining work for 6c

Migration 6c remains open for TLS VLOG/unclear/SNI-overlap cleanup and final Migration 6 closure. The remaining core TLS macro inventory is limited to SNI/client-hello/certificate-selection overlap sites in `SocketAcceptor.hpp`; the deferred `src/net/config/stream/tls` inventory remains outside 6b and was not modified.
