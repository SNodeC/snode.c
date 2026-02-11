# Database module (`src/database/*`) — MariaDB client integration (sync + async)

This document covers `src/database/*`.

In the provided code base, the database module focuses on **MariaDB / MySQL** integration via the MariaDB (MySQL) C client library:

- synchronous convenience API for simple queries
- **non-blocking async API** integrated into the SNode.C event loop
- a command/sequence model that makes DB workflows composable

---

## 1. Overview

Most “async DB in C++” designs fall into one of two camps:

- “Put DB I/O on a thread pool” (easy, but introduces concurrency, locking, context switches)
- “Use the DB library’s non-blocking API and integrate with an event loop” (harder, but matches event-driven networking)

SNode.C takes the second path for MariaDB:

- It configures the MariaDB client library for **non-blocking mode** (`MYSQL_OPT_NONBLOCK`).
- It obtains the underlying DB socket FD (`mysql_get_socket(mysql)`).
- It registers that FD with SNode.C’s **Read/Write/Exceptional event receivers**.
- It runs DB operations as **commands** that return “wait masks” (`MYSQL_WAIT_READ`, `MYSQL_WAIT_WRITE`, …).
- The event loop resumes the appropriate receiver(s) and calls back into the command until completion.

This yields an asynchronous database client that behaves like the network modules:
no dedicated DB threads are required, and DB operations naturally interleave with network I/O.

---

## 2. Public API entry points

### High-level client
- `#include "database/mariadb/MariaDBClient.h"`

### Connection / lifecycle
- `#include "database/mariadb/MariaDBConnection.h"`
- `#include "database/mariadb/MariaDBConnectionDetails.h"`
- `#include "database/mariadb/MariaDBState.h"`

### Async API facade
- `#include "database/mariadb/MariaDBClientASyncAPI.h"`

### Sync API facade
- `#include "database/mariadb/MariaDBClientSyncAPI.h"`

### Command model
- `#include "database/mariadb/MariaDBCommand.h"`
- `#include "database/mariadb/MariaDBCommandSequence.h"`
- `#include "database/mariadb/MariaDBCommandASync.h"`
- `#include "database/mariadb/MariaDBCommandSync.h"`

### Concrete commands
- `src/database/mariadb/commands/async/*`
- `src/database/mariadb/commands/sync/*`

---

## 3. Key types and responsibilities

### 3.1 `MariaDBLibrary`

`MariaDBLibrary::ensureInitialized()` is called during connection construction.
This is the module’s “global init gate” to ensure the MariaDB library is properly initialized once per process.

### 3.2 `MariaDBConnection`

`database::mariadb::MariaDBConnection` (`src/database/mariadb/MariaDBConnection.*`) is the core integration type.

It inherits:

- `core::ReadEventReceiver`
- `core::WriteEventReceiver`
- `core::ExceptionalConditionEventReceiver`

and thus plugs directly into the SNode.C event loop.

Key behaviors from `MariaDBConnection.cpp`:

- `mysql = mysql_init(nullptr);`
- `mysql_options(mysql, MYSQL_OPT_NONBLOCK, nullptr);`  
  → this enables non-blocking operations in the MariaDB client library.
- The connection is established via an async command:
  - `MariaDBConnectCommand`
- Once connected:
  - `fd = mysql_get_socket(mysql);`
  - all three event receivers are enabled on that FD
  - receivers are initially suspended until a command requires them
- The connection owns a **queue** of `MariaDBCommandSequence` objects and runs them serially.

#### Status-driven receiver scheduling

The MariaDB client library uses a “wait mask” model:

- A command step returns a bitmask (read/write/except/timeout).
- `MariaDBConnection::checkStatus(status)` translates that mask into:
  - which receivers to `resume()` or `suspend()`
  - and whether to set an FD timeout from `mysql_get_timeout_value(mysql)`

When `status == 0`, the command is considered ready to complete and `commandCompleted()` is called.

This is a clean fit for an event loop:
commands progress only when the DB socket signals readiness.

### 3.3 `MariaDBClient`

`MariaDBClient` (`src/database/mariadb/MariaDBClient.*`) is the user-facing manager that:

- owns or manages connections
- exposes the sync and async APIs
- handles connection vanish / shutdown transitions

The client also surfaces a “state changed” callback (`onStateChanged`) to report:
- connected state
- errors and error messages

### 3.4 Command model: `MariaDBCommand` and `MariaDBCommandSequence`

#### `MariaDBCommand`
A `MariaDBCommand` is a small state machine:

- `commandStart(mysql, currentTime)` initializes the command
- `commandContinue(status)` advances it after readiness notification
- `commandCompleted()` indicates completion
- `commandError(msg, errno)` reports errors

Concrete commands implement these virtual methods and call the relevant MariaDB C API functions.

#### `MariaDBCommandSequence`
A sequence allows composing multiple commands:

- connect → query → fetch rows → free result, etc.
- transactions:
  - auto-commit off → exec many statements → commit/rollback → auto-commit on

This sequence model is the right abstraction because a typical DB operation is not one function call in non-blocking mode:
it often requires several steps and result-processing commands.

---

## 4. Async API: how users interact

`MariaDBClientASyncAPI` (`src/database/mariadb/MariaDBClientASyncAPI.h`) exposes high-level async operations:

- `query(sql, onRow, onError)`
- `exec(sql, onExec, onError)`
- transaction helpers:
  - `startTransactions`
  - `endTransactions`
  - `commit`
  - `rollback`

Each call produces a `MariaDBCommandSequence&`, which you can further chain or observe.

Under the hood, the async API calls:

- `execute_async(new MariaDBCommand(...))`

and the connection schedules the command sequence into its queue.
The event loop drives the queue.

### Error handling
The async API takes `onError(errorString, errorNumber)` callbacks.
Internally, if `core::SNodeC` is still running and the connection is alive, pending commands are failed with that error on shutdown.

---

## 5. Sync API: when you want the simple path

`MariaDBClientSyncAPI` provides synchronous helpers.

Synchronous commands are executed via:

- `MariaDBConnection::execute_sync(MariaDBCommand* cmd)`

This is useful for:

- small test programs,
- startup migrations,
- low-throughput tools.

**But** in an event-driven server, sync calls can block the entire loop if called from a callback.
Use sync commands either before entering the event loop or in a dedicated thread.

---

## 6. Concrete commands: what exists

In `src/database/mariadb/commands/async/*`, you can see commands for:

- connect (`MariaDBConnectCommand`)
- query/exec (`MariaDBQueryCommand`, `MariaDBExecCommand`)
- fetch rows (`MariaDBFetchRowCommand`)
- free results (`MariaDBFreeResultCommand`)
- transaction helpers (`MariaDBAutoCommitCommand`, `MariaDBCommitCommand`, `MariaDBRollbackCommand`)

In `commands/sync/*`, you can see helpers such as:

- `MariaDBFieldCountCommand`
- `MariaDBUseResultCommand`
- `MariaDBAffectedRowsCommand`

These commands match the MariaDB C API semantics and keep the integration modular.

---

## 7. Integration with the SNode.C event loop

The database module is a good demonstration of SNode.C’s core abstractions:

- A DB connection is “just another FD” in the multiplexer.
- Commands request read/write/except/timeouts via wait masks.
- The event receivers are resumed/suspended accordingly.
- Timeouts are fed from the DB library (`mysql_get_timeout_value`), not guessed.

This design has two major advantages:

1. **No “DB thread” required** — the same loop drives everything.
2. **Deterministic concurrency model** — all callbacks happen on the loop thread (unless you create additional threads).

---

## 8. Example: async query flow (conceptual)

A typical async query looks like:

```cpp
database::mariadb::MariaDBClient db(details, [](auto const& st) {
  if (st.connected) { /* connected */ }
  if (st.error)     { /* handle error */ }
});

db.query(
  "SELECT id, name FROM users",
  [](MYSQL_ROW row) {
    // called for each row
  },
  [](std::string const& msg, unsigned err) {
    // query error
  }
);
```

The important conceptual point: `query(...)` does not block.
It builds a command sequence that runs as the event loop sees readiness on the MariaDB socket.

For a runnable program, see:
- `src/apps/database/testmariadb.cpp`

---

## 9. Pros / cons

### Pros
- True event-loop integration using MariaDB’s non-blocking API.
- Clear command/sequence model for composing DB workflows.
- Consistent with the rest of SNode.C (FD receivers, timeouts, logging).

### Cons / tradeoffs
- The MariaDB C API’s non-blocking model is low-level; correctness depends on command implementations being precise.
- A single connection processes commands serially; for parallel queries you need multiple connections (which is normal for DB clients, but worth stating explicitly).

### Gotchas
- Don’t call synchronous commands from within event-loop callbacks (unless you accept stalling the loop).
- Make sure you drain and free results (`FreeResultCommand`) to avoid memory growth.
- If the DB connection is lost (`unobservedEvent`), pending commands are failed and the connection deletes itself; application code must be ready for this lifecycle.

---

## 10. Next steps

- [`iot_mqtt.md`](iot_mqtt.md) — storing MQTT data often goes hand-in-hand with DB integration.
- [`core.md`](core.md) — understand event receivers and timeouts at the core level.
