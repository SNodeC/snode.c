# Codex App Server foundation status and verification

## Milestone status

The lifecycle and local stdio foundation is implemented on branch
`feature/codex-app-server-client-foundation`.

Completed behavior includes:

- transport-independent `AppServerClient` ownership through composition;
- local `stdio::Client` using `posix_spawnp()` and a child process group;
- collision-safe stdin, stdout, and stderr descriptor mapping;
- nonblocking, readiness-driven parent I/O with bounded output buffering;
- JSONL stdout framing and independent stderr diagnostic framing;
- the initialize request, response correlation, initialized notification, and
  transition to `Ready`;
- asynchronous ordered state callbacks and diagnostic callbacks;
- pidfd exit observation with a forced timerfd polling test path;
- transactional descriptor-registration failure handling for lifecycle, pidfd,
  stdin, stdout, and stderr receivers;
- stdin flush, EOF, SIGTERM, SIGKILL, and nonblocking reap ordering;
- global SNode.C shutdown and client-destruction safety;
- nonfatal stderr closure;
- exactly-once failure reporting; and
- explicit `Failed -> Stopping -> Stopped` recovery and restart.

There is no lifecycle pipe. No background thread, direct `fork()`/`vfork()`,
blocking descriptor operation, or blocking `waitpid()` is used.

## Test coverage

The fake App Server covers:

- split and coalesced JSONL messages;
- exact outgoing stdio newline framing;
- independent stderr diagnostics and stderr closure;
- launch and post-spawn setup failure;
- initialization rejection and malformed JSON;
- stdout closure and exit before initialization;
- bounded output rejection, slow stdin, and partial writes;
- normal, blocked, and SIGTERM-resistant shutdown;
- descriptor collisions when parent FDs 0, 1, or 2 are closed;
- pidfd and forced polling lifecycles;
- injected lifecycle, pidfd, and partial parent-stream registration failures;
- shutdown while the client is active or already destroyed; and
- failure recovery followed by successful reuse of the same client object.

The optional integration test runs against a real `codex app-server` only when
the executable is available. Installation and authentication are not required
by the regular suite.

## Latest verification

The focused verification for the completed implementation produced:

| Verification | Result |
| --- | --- |
| Codex, pipe, and registration tests, epoll | 42/42 passed |
| Codex, pipe, and registration tests, poll | 42/42 passed |
| Codex, pipe, and registration tests, select | 42/42 passed |
| AddressSanitizer focused run | 42/42 passed; no ASan diagnostics |
| Former forced-fallback stdout-close failure | passed in 0.58 s |
| Full repository build | passed |
| Forced polling normal and early-exit stress | 200/200 invocations passed |
| Installed CMake consumer | passed |
| Optional real Codex integration | passed when available |
| Formatting and `git diff --check` | passed |

LeakSanitizer was disabled for the AddressSanitizer run because the managed test
environment does not permit its ptrace-based operation. The process tests found
no surviving fake App Server child after completion.

A full-suite attempt in the managed sandbox was not authoritative: the sandbox
denied unrelated loopback socket creation with `EPERM`, causing TLS and network
tests to fail. All Codex tests in that same run passed. A normal host run should
be used for the final repository-wide result.

## Deliberately deferred work

This milestone does not define APIs for threads, turns, items, approvals, or
automatic approval. It also does not implement WebSocket transport, automatic
restart/reconnect, command-line configuration sections, or a generic child
process abstraction.

The current protocol codec is intentionally minimal and only validates the
initialization exchange needed to establish `Ready`.

## ABI and release note

The associated redesign of `core::pipe::Pipe`, `PipeSource`, and `PipeSink`
changes public C++ layouts and semantics. The shutdown notification virtual
interface also changes the core ABI. See
[`core-pipe-abi-revision.md`](../../../core-pipe-abi-revision.md).

All SNode.C libraries and consumers must be rebuilt together. Old and new core
binaries must not be mixed. Before a binary release, the project must address
SONAME or major-version handling; the unchanged development version must not be
interpreted as a binary-compatibility guarantee.

## Remaining technical risks

- Child-exit fallback is currently Linux `timerfd`; non-Linux support needs a
  native event-loop-compatible exit trigger.
- Higher-level Codex operations are intentionally absent, so App Server requests
  received during initialization are diagnosed but not serviced.
