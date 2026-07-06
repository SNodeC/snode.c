# Semantic logging migration 05 — net physical sockets

## Implementation summary

This is a clean Migration 05 PR from current master after merged PR #151, #154, #155, #156, and #157. It does not duplicate any previous PR and does not start Round 10.

The migration covers the manageable net physical-socket inventory under `src/net` only. It adds a minimal semantic owner to the Unix-domain physical socket boundary and migrates the Unix-domain lock-file lifecycle `LOG`/`PLOG` sites plus IPv4/IPv6 address-info trace dumps. Config and TLS matches remain deferred.

## Exact changed files

- `src/net/in/SocketAddrInfo.cpp`
- `src/net/in6/SocketAddrInfo.cpp`
- `src/net/un/phy/PhysicalSocket.h`
- `src/net/un/phy/PhysicalSocket.hpp`
- `tests/unit/log/CMakeLists.txt`
- `tests/unit/log/NetPhysicalSocketMigration05Test.cpp`
- `docs/logging/semantic-migration-05-net-physical-sockets-report.md`

## Base verification result

`git fetch origin` could not run because this container has no `origin` remote. The local base HEAD was the current merged line containing PR #151, #154, #155, #156, and #157 merge commits. The required prerequisite files for production threshold repair, migrations 01-04, Round 8, and Round 9 were present.

## Initial inventory command/result

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" \
  src/net \
  -g '*.h' -g '*.hpp' -g '*.cpp' \
  -g '!src/net/config/ConfigInstance.cpp' \
  -g '!src/net/config/ConfigInstance.h'
```

Result:

```text
src/net/in6/SocketAddrInfo.cpp:129:            LOG(TRACE) << formatted.str();
src/net/un/phy/PhysicalSocket.hpp:82:                LOG(DEBUG) << "Remove sun path: " << Super::getBindAddress().getSunPath();
src/net/un/phy/PhysicalSocket.hpp:84:                PLOG(ERROR) << "Remove sun path: " << Super::getBindAddress().getSunPath();
src/net/un/phy/PhysicalSocket.hpp:88:                LOG(DEBUG) << "Remove lock from file: " << Super::getBindAddress().getSunPath().append(".lock");
src/net/un/phy/PhysicalSocket.hpp:90:                PLOG(ERROR) << "Remove lock from file: " << Super::getBindAddress().getSunPath().append(".lock");
src/net/un/phy/PhysicalSocket.hpp:94:                LOG(DEBUG) << "Remove lock file: " << Super::getBindAddress().getSunPath().append(".lock");
src/net/un/phy/PhysicalSocket.hpp:96:                PLOG(ERROR) << "Remove lock file: " << Super::getBindAddress().getSunPath().append(".lock");
src/net/un/phy/PhysicalSocket.hpp:108:                LOG(DEBUG) << "Opening lock file: " << bindAddress.getSunPath().append(".lock").data();
src/net/un/phy/PhysicalSocket.hpp:110:                    LOG(DEBUG) << "Locking lock file: " << bindAddress.getSunPath().append(".lock").data();
src/net/un/phy/PhysicalSocket.hpp:113:                            LOG(WARNING) << "Removed stalled sun_path: " << bindAddress.getSunPath().data();
src/net/un/phy/PhysicalSocket.hpp:115:                            PLOG(ERROR) << "Removed stalled sun path: " << bindAddress.getSunPath().data();
src/net/un/phy/PhysicalSocket.hpp:119:                    PLOG(ERROR) << "Locking lock file " << bindAddress.getSunPath().append(".lock").data();
src/net/un/phy/PhysicalSocket.hpp:125:                PLOG(ERROR) << "Opening lock file: " << bindAddress.getSunPath().append(".lock").data();
src/net/in/SocketAddrInfo.cpp:136:            LOG(TRACE) << formatted.str();
src/net/config/ConfigPhysicalSocket.cpp:86:                           LOG(ERROR) << err.what();
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

| Class | Count | Files | Decision |
| --- | ---: | --- | --- |
| A. generic physical socket helpers | 0 | n/a | none |
| B. IPv4 / in | 1 | `src/net/in/SocketAddrInfo.cpp` | migrated |
| C. IPv6 / in6 | 1 | `src/net/in6/SocketAddrInfo.cpp` | migrated |
| D. Unix domain / un | 13 | `src/net/un/phy/PhysicalSocket.hpp` | migrated |
| E. Bluetooth RFCOMM / rc | 0 | n/a | none |
| F. Bluetooth L2CAP / l2 | 0 | n/a | none |
| G. address/address-info helpers | 2 | `src/net/in*/SocketAddrInfo.cpp` | migrated with `net.address` scope |
| H. TLS or encryption-related matches | 10 | `src/net/config/stream/tls/ConfigSocketServer.hpp` | deferred as TLS/config |
| I. config/other out-of-scope net matches | 1 | `src/net/config/ConfigPhysicalSocket.cpp` | deferred as config |

## Ownership discovery summary

Command:

```sh
rg -n "\blog\(\)|logger::|BoundaryLogger|LogScopeOwner|semantic|scope|Instance|connectionName|getInstanceName" \
  src/net \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

Result summary: existing semantic ownership is concentrated in `net::config::ConfigInstance`, with a `Configuration` boundary owner and `log()` overloads. Physical socket classes did not have an object-owned semantic logger before this migration. The Unix-domain physical socket has a narrow transport boundary, a stable component identity, and no required broad API churn, so this PR adds a minimal local owner there only.

## Stop/split decision and reason

The allowed physical-socket scope contained 15 production macro call sites, which is below the 35-call-site stop/split threshold. No split was required.

## Post-migration inventory command/result

Command:

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" \
  src/net \
  -g '*.h' -g '*.hpp' -g '*.cpp' \
  -g '!src/net/config/ConfigInstance.cpp' \
  -g '!src/net/config/ConfigInstance.h'
```

Result:

```text
src/net/config/ConfigPhysicalSocket.cpp:86:                           LOG(ERROR) << err.what();
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

All suitable LOG/PLOG call sites in the Migration 5 net physical-socket scope were migrated. Remaining LOG/PLOG sites are config/TLS and are deferred. No VLOG sites were found.

## Migrated call-site table

| File | Old severity | New semantic call | Notes |
| --- | --- | --- | --- |
| `src/net/in/SocketAddrInfo.cpp` | `LOG(TRACE)` | `net.address` `trace` | preserves formatted address-info payload |
| `src/net/in6/SocketAddrInfo.cpp` | `LOG(TRACE)` | `net.address` `trace` | preserves formatted address-info payload |
| `src/net/un/phy/PhysicalSocket.hpp` | `LOG(DEBUG)` x5 | `net.un` `debug` | preserves Unix socket path/lock-file lifecycle text |
| `src/net/un/phy/PhysicalSocket.hpp` | `LOG(WARNING)` x1 | `net.un` `warn` | preserves stalled `sun_path` removal detail |
| `src/net/un/phy/PhysicalSocket.hpp` | `PLOG(ERROR)` x7 | `net.un` `sysError(Error, errnum, ...)` | captures `errno` immediately after failing syscall |

## Deferred call-site table

| File | Call sites | Reason |
| --- | ---: | --- |
| `src/net/config/ConfigPhysicalSocket.cpp` | 1 | config-layer callback logging; out of default Migration 5 physical-socket production scope |
| `src/net/config/stream/tls/ConfigSocketServer.hpp` | 10 | TLS/SNI configuration logging; explicitly out of scope |

## Semantic owner(s) used or added

- Added `logger::LogScopeOwner` inside `net::un::phy::PhysicalSocket`, with `LogOrigin::Framework`, `LogBoundary::System`, and component `net.un`.
- Added public `log()` and sink-taking `log(...)` overloads to that physical socket class only.
- Used static local `LogScopeOwner` instances in IPv4/IPv6 address-info helpers with component `net.address`.

## Severity mapping

- `LOG(TRACE)` -> `trace`
- `LOG(DEBUG)` -> `debug`
- `LOG(WARNING)` -> `warn`
- `PLOG(ERROR)` -> `sysError(logger::LogLevel::Error, errnum, ...)`

No `LOG(INFO)`, `LOG(FATAL)`, or `VLOG` sites were present in migrated scope.

## PLOG/sysError errno handling

Each migrated `PLOG(ERROR)` branch captures `const int errnum = errno;` immediately after the failing syscall branch and before semantic formatting/logging. The semantic `sysError` call preserves errno code and text.

## VLOG result

No `VLOG` call sites were found. No VLOG migration was performed.

## Message/identity preservation notes

The migration keeps Unix domain socket path and lock-file lifecycle details, including remove/open/lock/stalled-path messages. Address-info logs retain the full formatted address-info diagnostic dump. Manual prefixes were not removed unless semantic component scope supplied the transport identity.

## Filter/backend/format compatibility

`NetPhysicalSocketMigration05Test` verifies the new net physical-socket owner emits when enabled, carries framework/system/`net.un` scope, respects `LogManager` filtering, respects backend `Logger::setLogLevel` filtering, respects JSON format selection, preserves `sysError` errno code/text, keeps the sink-taking overload compatible, and does not evaluate `ExpensiveValue` when production formatting is suppressed.

## Production-scope confirmations

- This PR changes only allowed Migration 5 files.
- This PR does not modify `SemanticLogger.*`.
- This PR does not modify `LogScopeOwner.*`.
- This PR does not modify `Logger.*`.
- This PR does not modify `ConfigInstance.cpp`.
- This PR does not modify previous socket-stream migration files.
- This PR does not modify previous core-runtime migration files.
- This PR does not modify previous migration tests.
- This PR does not change LOG/PLOG/VLOG macro definitions.
- This PR does not change LogManager precedence.
- This PR does not change LogManager freeze behavior.
- This PR does not change JSON v1 schema.
- This PR does not start Round 10.

## Build commands run

```sh
cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON
cmake --build cmake-build --parallel 2
```

## Test commands run

```sh
ctest --test-dir cmake-build -R "SemanticLoggerRound2Test|LogScopeOwnerRound3Test|ProductionLogApiRound4Test|SocketEndpointLogApiRound5Test|SemanticBackendRound6Test|SemanticFilterRound7Test|ControlledMigrationRound8Test|SemanticCompatibilityRound9Test|SemanticOverheadRound9Test|SemanticProductionThresholdRepairTest|SocketConnectionMigration01Test|SocketConnectorAcceptorMigration02Test|SocketServerClientMigration03Test|CoreRuntimeMigration04Test|NetPhysicalSocketMigration05Test" --output-on-failure
ctest --test-dir cmake-build --output-on-failure
```

Both commands passed. The full suite reported 122 tests with skipped component tests controlled by the existing test gating.

## ASan result

Full ASan was not run because it requires rebuilding and running the entire large app/test tree; instead the required minimum ASan validation was run for the new Migration 05 test:

```sh
cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON
cmake --build cmake-build-asan --target NetPhysicalSocketMigration05Test --parallel 2
ASAN=$(gcc -print-file-name=libasan.so) LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan -R NetPhysicalSocketMigration05Test --output-on-failure
```

The ASan-focused `NetPhysicalSocketMigration05Test` passed.

## Known follow-ups

- A future config/TLS-specific semantic migration should handle the deferred config/TLS call sites.
- Bluetooth RFCOMM/L2CAP had no current macro call sites in this inventory, but future logging additions there should use the same semantic owner policy.
