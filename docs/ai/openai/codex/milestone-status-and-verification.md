# Codex App Server protocol-core status and verification

## Milestone status

The lifecycle and local stdio foundation was implemented on branch
`feature/codex-app-server-client-foundation`. The generic protocol core extends
that foundation on `feature/codex-app-server-protocol-core` without changing
stdio framing or process ownership.

Completed behavior includes:

- transport-independent `AppServerClient` ownership through composition;
- local `stdio::Client` using `posix_spawnp()` and a child process group;
- collision-safe stdin, stdout, and stderr descriptor mapping;
- nonblocking, readiness-driven parent I/O with bounded output buffering;
- JSONL stdout framing and independent stderr diagnostic framing;
- the initialize request, response correlation, initialized notification, and
  transition to `Ready`;
- arbitrary caller-owned requests with asynchronous, exactly-once response
  correlation;
- arbitrary outgoing and incoming notifications;
- retained server-initiated requests with explicit success or rejection
  responses;
- integer and string server request-ID preservation;
- bounded pre-ready dispatch, pending-request, and server-request registries;
- raw-envelope preservation and nonfatal unmatched-message reporting;
- deterministic request cancellation and connection-generation isolation;
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

The deterministic in-memory transport additionally covers:

- accepted server-request answers consuming ownership before synchronously
  received follow-on input is processed, including a subsequent request that
  reuses the same server ID;
- duplicate still-pending server-request IDs remaining a fatal protocol
  ambiguity;
- matched remote response callbacks remaining bound to their receiving
  connection generation; and
- lifetime-bound, exactly-once cancellation of requests still pending during an
  immediate stop and restart.

The optional integration test runs against a real `codex app-server` only when
the executable is available. Installation and authentication are not required
by the regular suite.

## Latest protocol-core verification

The following commands were run after the server-answer ordering and deferred
response generation fixes:

```bash
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure -R '^CodexProtocolCodecTest$'
ctest --test-dir build --output-on-failure -R '^CodexProtocolEngineTest$'
ctest --test-dir build --output-on-failure -R '^CodexStdioProtocolFlow_'
ctest --test-dir build --output-on-failure \
  -R '^Codex(StdioLifecycleTest|GlobalShutdown_.*|ShutdownAdmissionTest)$'
ctest --test-dir build --output-on-failure -L '^shutdown$'
ctest --test-dir build --output-on-failure -L '^codex$' -LE '^optional$'
ctest --test-dir build --output-on-failure -L '^tls$'
ctest --test-dir build --output-on-failure \
  -R '^CodexProtocolEngineTest$' --repeat until-fail:20
ctest --test-dir build --output-on-failure \
  -R '^StagedInstalledConsumerTest$'
ctest --test-dir build --output-on-failure
cmake --build /tmp/snodec-codex-asan --parallel 2 --target \
  ai-openai-codex CodexFakeAppServer CodexStdioLifecycleTest \
  CodexStdioProtocolFlowTest CodexProtocolCodecTest CodexProtocolEngineTest \
  CodexGlobalShutdownTest CodexShutdownAdmissionTest \
  CodexDescriptorCollisionTest CodexStdioScenarioTest
cmake -E env ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
  ctest --test-dir /tmp/snodec-codex-asan --output-on-failure \
  -L '^codex$' -LE '^optional$'
git diff --check
```

| Verification | Result |
| --- | --- |
| Generic protocol codec | 1/1 passed |
| Raw protocol engine | 1/1 passed |
| End-to-end stdio protocol flow and exit/restart | 2/2 passed |
| Codex lifecycle and shutdown | 6/6 passed |
| Complete shutdown label | 7/7 passed |
| Complete nonoptional Codex suite | 39/39 passed |
| TLS unit suite | 5/5 passed |
| Protocol-engine ordering and generation stress | 20/20 passed |
| Installed CMake consumer | 1/1 passed |
| Complete CI suite on the host | 203/203 passed |
| AddressSanitizer, complete nonoptional Codex suite | 39/39 passed; no ASan diagnostics |
| `git diff --check` | passed; 0 whitespace errors |

Leak detection was disabled for the AddressSanitizer run because the managed
environment does not permit its ptrace-based operation. AddressSanitizer
otherwise completed without diagnostics. The process tests left no surviving
fake App Server child.

The TLS and complete CI commands were run with host socket permissions. The
managed sandbox denies unrelated loopback socket operations with `EPERM`, so a
sandbox-only full-suite result is not authoritative for those tests.

## Deliberately deferred work

This milestone does not define typed APIs for threads, turns, items, approvals,
models, accounts, or authentication. Server requests are deliberately exposed
as neutral raw protocol requests; no request is silently approved. WebSocket
transport, automatic restart/reconnect, command-line configuration sections,
and a generic child process abstraction also remain out of scope.

The protocol codec is generic and preserves unknown methods and fields. Semantic
validation of thread, turn, item, approval, model, and authentication payloads
belongs in later typed layers.

## ABI and release note

The associated redesign of `core::pipe::Pipe`, `PipeSource`, and `PipeSink`
changes public C++ layouts and semantics. The shutdown notification virtual
interface also changes the core ABI. See
[`core-pipe-abi-revision.md`](../../../core-pipe-abi-revision.md).

The protocol-core extension adds public C++ types containing
`nlohmann::json`, strong request-ID classes, `AppServerClient::RawProtocol`, and
new exported member functions. This changes the Codex module's public C++ API
and ABI surface even where the outer `AppServerClient` object layout remains
pimpl-based.

All SNode.C libraries and consumers must be rebuilt together. Old and new core
binaries must not be mixed. Before a binary release, the project must address
SONAME or major-version handling; the unchanged development version must not be
interpreted as a binary-compatibility guarantee.

## Remaining technical risks

- Child-exit fallback is currently Linux `timerfd`; non-Linux support needs a
  native event-loop-compatible exit trigger.
- Higher-level Codex operations remain raw JSON until future typed layers add
  semantic payload validation.
- The pre-ready queue and both pending-request registries are deliberately
  bounded; capacity errors require caller handling rather than unbounded memory
  growth.
