# Initial proposal for Linux macOS and Windows porting

**Superseded draft:** the current proposal is the [five-column implementation table](IMPLEMENTATION_PROPOSAL.md), with literal function bodies and larger operation capsules. The declarations below are historical design context, not a second API authority.

The subsequent [complete source inventory](SYSCALL_INVENTORY.md), [capsule contract proposal](CAPSULE_API.md) and [coverage audit](AUDIT.md) replace the seven-row illustrative excerpt below as the current porting record.

Reconstructed on 2026-10-13. This is an initial proposal for review, not authorization to implement new production abstractions. All subsequent porting records belong in `/home/voc/projects/snodec/snode.c/supplement/porting`.

The proposed invariant is that platform-dependent operations have one authoritative internal system boundary, while application APIs, framework behavior, resource ownership and event ordering remain unchanged. Existing wrappers are the starting point. Direct calls outside those wrappers and native types exposed through their headers are both part of the investigation.

The original repository is at `d4b2b2478`. Its real sibling `/home/voc/projects/snodec/mac-win-port` exists, has a clean working tree and is at `07ca9a293` on `master`, with GitHub origin `SNodeC/snode.c`. These revisions differ. Before rebuilding the complete inventory, verify GitHub's current default branch and update the clean sibling checkout to that revision, recording its full commit ID. Do not treat the older checkout as current or combine evidence from different revisions without labeling it.

1. Rebuild the complete inventory against the recorded revision.

   Account for every tracked source file, every conditional platform branch, demos, tests, tools, generated-code inputs and bundled dependencies. Include direct syscalls, libc and runtime calls, third-party library calls, macros that invoke platform APIs, and indirect calls through wrappers or function pointers. Record native types, constants, headers, platform-specific paths and build dependencies alongside the call inventory.

   Use exactly these main table dimensions:

   | Each call/library | Already extracted | Extractable | API proposal |
   | --- | --- | --- | --- |
   | One named operation per row; library identity included | Yes, partial or no; wrapper and inline call-site evidence | Yes, already complete, unnecessary or unresolved; reason | Declaration and caller-visible contract |

   A wrapper's existence does not mean extraction is complete. “Partial” means a wrapper exists and callers still bypass it. “No” identifies inline-only use. Every row must link to all observed call sites and conditional guards in a supporting occurrence ledger. Library summaries supplement individual operation rows. Portable library calls may need no new wrapper, but remain explicitly accounted for.

   Reconcile source scanning with headers, build configuration and compiler or symbol evidence where available. Explain exclusions individually. Counts alone and a single textual syscall search cannot establish completeness.

2. Propose the internal API before proposing backend implementations.

   Reuse `core::system`, `utils::system` and the existing TLS system boundary where their responsibilities fit. Keep operations close to their existing POSIX-shaped contracts. Each proposed declaration must specify inputs, outputs, error reporting, ownership, lifetime, blocking behavior, interruption and partial completion where relevant. Backend algorithms and native API mappings belong in a later implementation proposal.

   The following is an initial API excerpt based on the current original repository, not the completed inventory. Existing integer descriptor signatures are retained here provisionally; their suitability across descriptor owners and external library interfaces must be resolved before API approval.

   | Each call/library | Already extracted | Extractable | API proposal |
   | --- | --- | --- | --- |
   | `read` / OS runtime | Partial: `core::system::read`; direct calls also occur in `src/tools/snodec-control` | Yes; consolidate callers at the existing boundary | `ssize_t core::system::read(int fd, void* buffer, std::size_t count);` Borrow the descriptor and buffer; return bytes read, zero at EOF, or failure. Preserve partial reads and interruption reporting. |
   | `write` / OS runtime | Partial: `core::system::write`; direct use in `ConfigActions.cpp` | Yes; consolidate callers at the existing boundary | `ssize_t core::system::write(int fd, const void* buffer, std::size_t count);` Borrow inputs; return bytes written or failure. Preserve partial writes and interruption reporting. |
   | `close` / OS runtime | Partial: `core::system::close`; direct calls also occur in `ProcessRunner.cpp` | Yes; shared ownership contract must cover every descriptor kind | `int core::system::close(int fd);` Release the owned resource; define descriptor validity after success and each failure before implementation. |
   | `open` / OS runtime | Partial: two-argument wrapper; direct two- and three-argument calls in `src/net/un/phy/PhysicalSocket.hpp` | Yes; existing declaration does not express creation mode | Retain `int core::system::open(const char* path, int flags);`; propose `int core::system::open(const char* path, int flags, mode_t mode);` for creation. Return a newly owned descriptor or failure; specify path encoding and creation permissions. |
   | `fcntl` / descriptor flags | Inline examples in `Pipe.cpp`, poll and select multiplexers; no corresponding declaration in the inspected `unistd.h` | Yes for the observed flag operations; other commands require complete inventory | Candidate overloads: `int core::system::fcntl(int fd, int command);` and `int core::system::fcntl(int fd, int command, int value);`. Explicitly enumerate supported commands and their return and error semantics. |
   | `SSL_read` / OpenSSL | Existing declaration and implementation in `core::ssl`; full caller coverage still to audit | Reuse the existing boundary if extraction is needed | `int core::ssl::SSL_read(SSL* ssl, void* buffer, int count);` Borrow the TLS session; preserve OpenSSL return values and the caller's ability to classify retry and failure. |
   | `SSL_write` / OpenSSL | Existing declaration and implementation in `core::ssl`; full caller coverage still to audit | Reuse the existing boundary if extraction is needed | `int core::ssl::SSL_write(SSL* ssl, const void* buffer, int count);` Borrow the TLS session and input; preserve OpenSSL return values and retry requirements. |

   The complete API proposal must settle the shared descriptor contract, invalid values, socket versus file ownership, error access, address and length types, readiness semantics, clock behavior and native-handle interoperability. Do not assume that existing native headers or an `int` descriptor are automatically a portable contract. Compare retaining existing signatures with the smallest necessary contract changes, including source and ABI effects, before choosing.

3. Resolve behavior constraints before selecting implementations.

   Trace descriptor creation through registration, I/O, external-library use and destruction. Examine mixed socket/file/pipe readiness, signals, process creation and daemon behavior, UNIX descriptor passing, Bluetooth, dynamic loading, filesystem semantics and TLS/database integration. For every unsupported or uncertain target capability, identify the exact affected behavior and demonstrate feasibility. Do not count disabled demos, no-op stubs or altered framework control flow as a successful port.

   Preserve SocketContext, Connector/Acceptor, Server/Client and CLI11 configuration behavior. If equivalent behavior cannot be supplied beneath the existing boundary, bring that conflict back for a decision before implementation.

4. Keep platform selection and deployment coherent.

   Propose common internal headers with one selected implementation per target operating system, under the appropriate existing `system` owner. CMake should select sources from the target platform, including cross-compilation, without platform branches leaking into application code. Preserve library and executable target names, install components and deployment behavior. Treat native binary suffixes and any unavoidable platform deployment changes as explicit review items.

5. Establish verification in QEMU before claiming portability.

   Establish usable Linux, macOS and Windows guests and validate required networking, dependencies and device access. Record a Linux baseline before extraction. For each target, maintain a manifest of every demo and relevant configuration, including optional dependencies, with build, startup, functional exchange and shutdown checks. Exercise installation and a downstream consumer as well as the build tree. Record revision, guest image, toolchain, dependency versions, commands, expected behavior and actual results. A skipped or unprovisioned scenario remains unverified.

6. Implement only after the inventory and API decisions are reviewable.

   Proposed order: current default-branch baseline; complete inventory and feasibility findings; API review; Linux extraction with QEMU regression evidence; macOS and Windows implementations; complete demo and deployment verification. At each step, first remove duplication, then reshape existing code. Before any necessary net production growth, explain why reduction and modification are insufficient, quantify the proposed growth and obtain the required explicit approval. Keep one authority for each operation and remove superseded paths within the same approved migration stage.

The next concrete deliverables are the full four-column inventory, its occurrence and file-coverage ledgers, a complete API contract proposal, and a QEMU verification matrix. This reconstruction changes documentation only. It does not claim a complete inventory, portable API approval, guest availability or successful platform tests.
