# Database Module (`src/database/*`)

## Module role

Current database support focuses on MariaDB, implemented as an event-loop-aware connector with both synchronous and asynchronous command APIs.

## Main classes

- `MariaDBClient`: user-facing facade combining async and sync APIs.
- `MariaDBConnection`: low-level connection runtime integrated with read/write/exception event receivers.
- `MariaDBCommand`, `MariaDBCommandASync`, `MariaDBCommandSync`: command abstraction hierarchy.
- `MariaDBCommandSequence`: async command queue/chaining structure.
- `MariaDBConnectionDetails`: host/user/password/db/port/socket/flags input.
- command implementations in `commands/async/*` and `commands/sync/*`.

## Execution model

### Asynchronous flow

- commands are queued as sequences,
- start event schedules command progress,
- read/write/OOB callbacks continue operation based on connector status,
- completion transitions to next command.

### Synchronous flow

- synchronous command wrappers execute directly for blocking-style usage where acceptable.

This dual API is useful when one codebase needs both high-throughput async tasks and simpler admin/control operations.

## State and failure handling

- `MariaDBState` reports connection and error information via callback.
- `connectionVanished` and status checks centralize failure propagation.
- `unmanaged()` support exists for ownership/lifecycle edge cases.

## Functionality summary

- MariaDB connector integrated into SNode.C event model
- async command sequencing and sync command wrappers
- granular command classes for connect/query/transaction/result handling

## Pros

1. **Event-driven DB integration** aligns with framework philosophy.
2. **Command abstraction improves composability** of DB operations.
3. **Async + sync coexistence** gives ergonomic flexibility.
4. **Explicit state callback** helps operational monitoring.

## Cons / tradeoffs

1. **MariaDB-only scope** at present.
2. **Command-class explosion** can feel verbose for simple CRUD usage.
3. **Async flow debugging** may require deep understanding of command queue lifecycle.

## Practical recommendation

For landing-page communication, emphasize that SNode.C includes DB support but is primarily a networking framework; database integration is useful and capable, yet intentionally narrower in scope than dedicated ORM-heavy ecosystems.
