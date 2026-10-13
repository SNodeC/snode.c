# Capsule API contract proposal

**Superseded draft:** the current proposal is the [five-column implementation table](IMPLEMENTATION_PROPOSAL.md), with literal function bodies and larger operation capsules. The declarations below are historical design context, not a second API authority.

The declarations in [the complete table](SYSCALL_INVENTORY.md) describe the API callable from SNode.C project code. They are proposals, not implemented headers. Native Linux, macOS and Windows entry points are in separate columns. Rows with missing semantic equivalents remain explicit design blockers; a signature alone does not resolve them.

The architectural invariant is one authority for native resource representation, OS error translation and platform-specific behavior. Project objects continue to own their resources and control their lifetimes. A capsule must not create a competing descriptor registry, background scheduler or duplicate lifecycle state merely to preserve an integer API.

`namespace sys = core::system;` is shorthand used in this proposal. Implementation should reshape the existing `core::system`, `utils::system` and TLS owners, with final placement following responsibility. It must not leave equivalent old wrappers and new capsules operating in parallel. The existing `net::un::peerCredentials` abstraction is also an authority to reuse, not duplicate.

1. Common data and error contracts.

   `Path` and `StringView` denote borrowed UTF-8 text; `Bytes` and `MutableBytes` denote bounded byte spans; `Span<T>` denotes `std::span<T>`. Capsules convert native path/address text at the boundary. POSIX filenames containing invalid UTF-8 require a deliberate contract decision before implementation; do not silently corrupt them. Unicode acceptance is a proposed common contract, not an existing verified guarantee.

   `Result<T>` denotes either a value or `Error`, with explicit success testing, `value()` and `error()` access. It can use an existing suitable result representation if one is found during implementation design; it does not require C++23 `std::expected`. `Error` contains a normalized category plus native domain and native code. At minimum categories distinguish would-block, interrupted, in-progress, not-found, already-exists, permission-denied, invalid-argument, bad-handle, connection-refused/reset, broken-pipe, unsupported and other. Resolver, OpenSSL, CRT, Win32 and Winsock codes must not be conflated. Native errors are captured before any other call can overwrite them.

   `IoRead` carries byte count and EOF status, distinguishing a zero-length request from end-of-stream. Other byte-count results preserve partial completion. Ordinary I/O capsules perform one logical operation and do not silently add retry loops or change event-loop scheduling. `random_bytes` is explicitly a fill-all-or-fail operation. Completion-based native I/O must satisfy the documented caller contract; it cannot be substituted for synchronous nonblocking I/O without a lifecycle design.

2. Resource ownership and external-library interoperability.

   `Handle` is an opaque, platform-neutral resource reference with an invalid state. It must preserve the full width and kind of native socket, file and pipe handles. Its representation is an implementation decision; it is not a public promise that a Windows SOCKET fits in `int`. `WaitSet`, `Library`, `Process` and `ProcessWatch` are typed resource references. `Accepted`, `PipeEnds`, `SocketPair` and `TempFile` contain newly owned handles and any returned addresses/paths.

   Functions that create or duplicate resources transfer ownership to the caller on success. Other input handles are borrowed unless the contract explicitly says consume. Closing an owned handle consumes its logical ownership once, even if the operation reports a native release error. The backend must specify how it settles native close failure without project code retrying a potentially reused descriptor. That platform-specific close contract must be proven before adoption. Borrowed handles from TLS, MariaDB or stdio must never be closed independently of their owner.

   Standard stream constants become `Result<Handle> sys::standard_handle(StandardStream stream);` and `Stream sys::standard_stream(StandardStream stream);`, returning borrowed references. Linux/macOS use the standard descriptor/FILE values; Windows uses GetStdHandle or the matching CRT stream. Owned strings have a paired `void sys::release_string(OwnedCString&);`. These are supporting contract operations required by the observed native constants and allocator lifetimes.

   Extend the current descriptor owner rather than maintaining both an integer descriptor and an independently mutable capsule handle. Translate TLS and MariaDB native-handle interfaces in their existing dependency owners. The library appendix proposes `core::ssl::attach`, `core::ssl::socket` and `database::mariadb::system::socket` for those boundaries. An OpenSSL BIO and a Connector/C socket are not interchangeable with arbitrary file descriptors; full-width native interoperability is a feasibility requirement.

   Propose close overloads for the typed resources: `Result<void> sys::close(WaitSet);`, `Result<void> sys::close(Process);`, `Result<void> sys::close(ProcessWatch);` and the listed `unload_library(Library)`. Closing a process reference does not terminate the process. `AddressList`, `ArgumentList`, `LaunchSpec`, `OwnedCString` and `Stream` use their stated paired release operation; if RAII replaces that release in final API review, use one ownership mechanism, never both.

3. Portable values replace exposed native structures.

   `Address` represents IPv4, IPv6, local or Bluetooth address data and preserves an IPv6 scope ID. It does not expose `sockaddr`, native padding or platform length fields. `AddressList` owns ordered resolver results; `EndpointName` owns host/service strings. Resolver options preserve canonical-name, passive, numeric-only and IPv4-mapped intent. Local path addresses and Linux abstract addresses are distinct capabilities.

   `FileInfo` includes object identity, file kind, byte size, link count and modification time. A path lookup can select whether to follow the final link. A Windows reparse point is not automatically a UNIX socket or a symlink. `Permissions` and `Principal` are semantic permissions and domain-tagged identities, not bitwise casts between Unix modes/UIDs and Windows ACLs/SIDs. Ownership-sensitive cleanup continues to compare the identity of the created file before removing its name.

   `OpenOptions`, `SocketFlags`, `SocketOption`, `OptionValue`, `Interest` and related enums describe existing used behavior rather than expose native numeric constants. Unknown options must fail explicitly. Reuse-port, no-follow, noninheritance, peer credentials and Bluetooth require the feature-specific decisions in [platform dependencies](PLATFORM_DEPENDENCIES.md).

4. Readiness, time and interruption.

   `WaitSet` owns registrations, not watched handles. `WatchKey` identifies the project receiver; `ReadyEvent` contains that key and normalized readable/writable/priority/error/hangup/peer-half-close events. `PollItem` combines a borrowed handle, requested interest and returned readiness; `PollItems` owns the temporary collection. No project code computes a maximum native descriptor or assumes a portable `fd_set` layout.

   `Deadline` represents an absolute monotonic deadline or explicit immediate/infinite wait. `Duration` is a signed nanosecond interval; adapters define rounding without restarting the deadline after interruption. `WallTime` represents seconds and subsecond precision since the Unix epoch and may jump. `CalendarTime` contains year/month/day/hour/minute/second, weekday, year-day and DST/zone information; `ParsedTime` also records consumed input. Wall clocks and monotonic scheduling must not be substituted for each other.

   `Signal`, `SignalSet`, `SignalDelivery` and `InterruptPolicy` express lifecycle notification policy. `SignalSubscription` and `SignalMaskScope` must support restoration of the prior state. Proposed explicit cleanup operations are `Result<void> sys::restore_signals(SignalSubscription);` and `Result<void> sys::restore_signal_mask(SignalMaskScope);`. A Windows console callback is not a POSIX signal handler, and an event is not a thread signal mask. The supported semantic set, delivery thread, worker exclusion and no-lost-wakeup behavior must be agreed before implementing these rows.

5. Launch and terminal contracts replace native setup sequences.

   `LaunchSpec` contains executable, ordered arguments, environment policy, standard-stream redirections, handle inheritance, child signal policy and job/detach intent. The builder functions replace the existing `posix_spawn` action and attribute objects; they do not introduce a second process manager. Strings remain owned through launch. The contract specifies how the chosen child runtime parses Windows arguments; arbitrary shell command strings are a separate `run_shell` contract.

   `Process` is a stable owned process reference; `ProcessId` is an identifier only. `ExitStatus` distinguishes normal exit, POSIX signal termination when known, requested forced termination when known and unknown native termination. A numeric Windows exit code alone cannot reconstruct POSIX `WTERMSIG`. `BackgroundStart` distinguishes parent acknowledgement and running-child context only on a platform capable of the selected launch model; arbitrary fork continuation is explicitly excluded from a three-platform promise.

   `TerminalState` preserves restorable native state and exposes only supported semantic controls. `ForegroundLease` represents a temporary terminal handoff; propose `Result<void> sys::restore_terminal(ForegroundLease);`. Windows console behavior must be specified rather than represented as a fictitious POSIX foreground process group. Identity changes similarly require an agreed `IdentityRequest` and `IdentityScope`, with `Result<void> sys::restore_identity(IdentityScope);`; a thread impersonation token does not emulate a process-wide effective UID change.

6. Implementation boundaries and approval.

   Each approved OS-facing declaration gets Linux, macOS and Windows implementations selected by CMake for the target, under the appropriate existing system owner. Pure conversions and operations already supplied portably by a dependency can share an implementation. Native calls and native types stay behind the boundary; the rightmost inventory column is the API, not backend pseudocode.

   Retaining the existing POSIX-shaped signatures minimizes caller changes but leaves integer handles, native structures and ambient errno exposed. The proposed typed contract removes those assumptions and needs coordinated descriptor, error and caller changes. That is the principal tradeoff for API review. Before production implementation, first reduce and reshape the existing wrappers; quantify any remaining net production growth and obtain the approval required by the engineering instructions. No estimate is presented as reliable before the unresolved resource/readiness contracts are settled.

   Validation must exercise behavior in the three QEMU targets, including mixed readiness, descriptor lifetime, close races, partial I/O, signal restoration, child stdout/stderr capture, foreground handoff, UNIX socket ownership and dependency handle interoperability. No QEMU or native framework test was run for this documentation inventory.
