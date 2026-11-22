# Stdio SocketEndpoint architecture and implementation proposal

**Status: design record for the implemented stdio endpoint, locally merged at `fa8ef9600`; the subsequent MQTT-aligned MCP component decision is recorded in section 20.**

This proposal records the owner's accepted direction and incorporates review findings F1–F11, follow-up findings N1–N7 and decisions R1–R6. D1–D4 are decided below. All baseline source citations refer to read-only inspection at `e81147c4c`; that revision identifies the evidence, not a revision to pin future builds to. Source references below are relative to this checkout. The existing editor change in `src/mcp/server/Config.cpp` remains outside this work. The baseline sections retain their original design expectations and **UNVERIFIED** planning labels; sections 18–20 record the later owner-authorized implementation, executed checks, component layout and remaining qualifications. Source correctness, slimness and established SNode.C structure govern this series; release-time SOVERSION decisions belong to the owner.

## 1. Accepted direction and architectural invariant

The invariant is: **a stream protocol is implemented once as the original `core::socket::stream::SocketContext` and `SocketContextFactory`; transport selection does not require another protocol context, another factory interface or transport conditionals in the protocol.**

| Subject | Decision |
| --- | --- |
| Application-facing stdio endpoint | One `net::stdio::stream::SocketEndpoint<Factory>` |
| Activation | `attach()`, using already-open descriptors |
| Application protocol role | Chosen by the existing context/factory; the context sends first or waits |
| Server/client stdio wrappers | Omitted |
| Public `StdIn`, `StdOut`, `StdErr` classes | Omitted |
| Stdio TLS | Out of scope; no stdio TLS layer or shared TLS changes |
| Protocol context and factory | Original stream classes and factory signature; the approved context shutdown hook is recorded in sections 18–19 |
| Address | Descriptor-pair value using an internal sockaddr-shaped representation |
| Process startup | Application owns `SNodeC::init/start/free`; endpoint does not own the process |
| Scheduling | Existing SNode.C event loop; no worker thread or blocking-I/O fallback |
| Child process launching | Caller supplies descriptors; no new process supervisor in this change |

The same context/factory remains usable through `in`, `in6`, `un`, `l2`, `rc` and stdio. Network transports retain their existing plaintext/TLS choices. Protocol reuse does not imply that each transport must expose TLS.

**Clauses marked REVIEW GATE are unresolved implementation decisions. They must be resolved before a coding plan is approved. Do not interpret the diagrams or declaration sketches as proof that these seams already work.**

The descriptor-lifetime invariant is **I1: a descriptor number is closed or replaced only after all publisher interests for it are released**. Physical shutdown itself does not release stdio descriptors. Owner decision D4 reinstates independent pipe write-half-close through a narrow receiver release hook at the existing deferred publisher-release point, before possible connection deletion (sections 8.2–8.3). A separate Stage 0 core bug fix resolves unsafe connection activation before any stdio implementation begins (section 8.4).

## 2. Established architecture to preserve

The evidence inventory in section 17 gives exact file:line references.

1. The existing stream factory returns `SocketContext*` for a `stream::SocketConnection*` (E01). The connection calls that factory and attaches the result through the original path (E02).
2. The original stream context delegates reads, writes, source streaming, timeouts and closing to that connection (E03). Its `attach()` invokes `onConnected()` (E04).
3. `SocketConnectionT<PhysicalSocket, Reader, Writer, Config>` already separates physical endpoint, buffered I/O engines and configuration (E05). It owns context switching and final teardown (E06).
4. The reader/writer are DescriptorEventReceiver subclasses with virtual syscall hooks (E07, E08). The writer already handles buffering, watermarks, source suspension and drain notification; stdio must reuse them.
5. Server/client wrappers use `core::socket::Socket<Config>` for shared configuration and shared factory/callback context (E09, E10). The new endpoint follows this pattern without inheriting listening or connecting operations.
6. The transport-specific `net` layer binds physical endpoint and configuration types into convenient public templates (E11).
7. Instance configuration and connection policy are already provided by `ConfigInstance`, `ConfigSection` and `ConfigConnection` (E12, E13).
8. `net::SocketAddress<SockAddr>` and the address configuration templates already support a transport-specific address value (E14, E15). The internal stdio structure below makes these reusable.

Do not move existing network classes, rename the `stream` namespace, introduce a second connection engine, replace the context/factory interfaces, or make the protocol derive from a stdio-specific context.

## 3. Proposed layering and source layout

```text
Application's unchanged stream SocketContextFactory and SocketContext
                         |
net::stdio::stream::SocketEndpoint<Factory>       public binding
                         |
core::socket::stream::stdio::SocketEndpoint       configuration/factory facade
                         |
core::socket::stream::stdio::SocketConnection     thin concrete composition
                         |
original core::socket::stream::SocketConnectionT
           |                                |
stdio::SocketReader                  stdio::SocketWriter
           |                                |
original buffered reader             original buffered writer
           |                                |
ReadEventReceiver                    WriteEventReceiver
           |                                |
        input fd                         output fd
                         |
net::stdio::phy::stream::PhysicalSocket            ownership and fd bindings
```

The stdio connection is a thin concrete composition of the original connection template with stdio reader/writer types. Its purpose is construction and the existing protected factory-attachment call. It must not copy the original receive loop, context-switch logic, counters, shutdown notification policy or `unobservedEvent()` cleanup.

Proposed files, following existing file-per-class and `.h`/`.hpp` template conventions:

```text
src/core/socket/stream/stdio/
    CMakeLists.txt
    DescriptorOwnership.h
    SocketEndpoint.h / SocketEndpoint.hpp
    SocketConnection.h / SocketConnection.hpp
    SocketReader.h / SocketReader.cpp
    SocketWriter.h / SocketWriter.cpp

src/net/stdio/
    CMakeLists.txt
    SocketAddress.h / SocketAddress.cpp
    config/ConfigAddress.h / ConfigAddress.cpp
    phy/CMakeLists.txt
    phy/stream/CMakeLists.txt
    phy/stream/PhysicalSocket.h / PhysicalSocket.cpp
    stream/CMakeLists.txt
    stream/SocketEndpoint.h / SocketEndpoint.cpp
    stream/config/ConfigSocketEndpoint.h / ConfigSocketEndpoint.cpp
```

There is one endpoint implementation. The `net` header binds its types; it is not a second implementation. No `legacy`, `tls`, acceptor, connector or stdio server/client subtrees are proposed. `DescriptorOwnership` is a small configuration value, not another lifecycle object.

The core namespace is `core::socket::stream::stdio`, and its component is `core-socket-stream-stdio`. Like the existing TLS specialization, stdio overrides the stream syscall hooks; the base hooks use `recv`/`send`, not descriptor `read`/`write` (E57). This is a stream specialization alongside the existing specializations (E73), not a third sibling of `stream` and `dgram`. The `net/stdio` layout remains as shown.

All new installed declarations need contract comments covering ownership, lifetime, event-loop thread use and callback rules. Use the repository's include guards, direct includes, formatting and existing licensing conventions. Do not introduce unrelated formatting or tooling changes.

## 4. SocketAddress: reuse the existing sockaddr-based model

### 4.1 Internal representation

The owner's clarification is incorporated here: the internal address can contain the stdin/stdout descriptor numbers. A conceptual declaration is:

```cpp
namespace net::stdio::detail {

    // Proposed internal layout, declared in SocketAddress.h.
    struct SockAddr {
        sockaddr header;
        int inputFd;
        int outputFd;
    };

    static_assert(std::is_standard_layout_v<SockAddr>);
    static_assert(std::is_trivially_copyable_v<SockAddr>);
    static_assert(offsetof(SockAddr, header) == 0);

}
```

The first member is a complete `sockaddr`, at offset zero. That supplies actual storage for the header accesses made by the existing wrapper, instead of assuming that a leading integer is sufficient. Use `AF_UNSPEC` as the header family: this is a framework-local address, not a new operating-system address family. Never invent an `AF_STDIO` number or pass this value to OS `bind`, `connect`, `getsockname` or `getpeername`.

The wrapper would be:

```cpp
namespace net::stdio {

    // Proposed; follows the existing net::un::SocketAddress pattern.
    class SocketAddress final : public net::SocketAddress<detail::SockAddr> {
    public:
        /// Creates an unspecified, non-owning address with both fds equal to -1.
        SocketAddress();

        /// Describes process-local protocol input/output; does not access the fds.
        SocketAddress(int inputFd, int outputFd);

        /// Copies and validates the internal address representation and length.
        SocketAddress(const SockAddr& sockAddr, SockLen sockAddrLen);

        /// Checks configured descriptor numbers; actual fd access is checked at attachment.
        void init();

        /// Updates input binding without opening, closing or duplicating a descriptor.
        SocketAddress& setInputFd(int inputFd);

        /// Updates output binding without opening, closing or duplicating a descriptor.
        SocketAddress& setOutputFd(int outputFd);

        /// Returns the input number, or -1 when unspecified.
        int getInputFd() const;

        /// Returns the output number, or -1 when unspecified.
        int getOutputFd() const;

        /// Formats the process-local binding without accessing its descriptors.
        std::string toString(bool expanded = true) const override;
    };

}
```

This sketch omits file scaffolding, not contract requirements. No duplicate private fd fields are needed: the inherited `sockAddr` is the address's one authority. `SockAddr`/`SockLen` aliases come from the existing wrapper. Use the same explicit-instantiation pattern as `net::un::SocketAddress` (E16).

The existing wrapper calls `memset` on the whole raw structure (E14). Therefore default member initializers in `detail::SockAddr` would not establish the unspecified state. `SocketAddress()` must explicitly set both fd fields to `-1` after calling the base constructor. The raw-value constructor checks the supplied size and family before reading fields. It accepts the fully unspecified sentinel for remote-address reporting; `init()` rejects an unspecified address when used as an attachment configuration.

The three `static_assert`s belong beside the structure, with direct `<type_traits>`/`<cstddef>` includes; tests additionally cover length, family and wrapper round trips. They have not been compiled: **UNVERIFIED**. Review the supported platforms' header representation and existing wrapper casts; do not claim portability from visual resemblance alone.

`git grep -n AF_UNSPEC e81147c4c -- src` returned no matches; stdio would be its first explicit source use, within the existing raw-type wrapper (E60). The raw `getSockAddr()` accessor in the generic stream path reaches only OS `bind` and `connect` (E59). Both are unreachable for stdio: its physical adapter inherits neither OS-socket physical base nor client connector, and only performs local address copies. This is a design constraint, **UNVERIFIED** until the installed instantiation is compiled and tested. The repository also has Unix-specific datagram/probe users of the accessor (E59); the claim is about the generic stream path, not every occurrence. The unrelated `SocketAddrInfo::getSockAddr()` accessors return concrete Internet address structures and are not this wrapper accessor (E70).

A global `SockAddr` capsule and direct derivation from `core::socket::SocketAddress` were considered and rejected. The plain transport-local struct within `net::SocketAddress<>` preserves transport uniformity and the existing raw reconstruction contract (E14, E15). A global capsule would introduce another public abstraction without removing the need for raw sockaddr arguments at OS call boundaries (E59). Direct core-base derivation would lose the inherited `SockAddr`/`SockLen` contract required by shared configuration (E17, E58) and force a parallel address path. No shared address abstraction is changed.

### 4.2 Semantics

| Address | Meaning | Presentation |
| --- | --- | --- |
| `(0, 1)` | Read local stdin, write local stdout | `stdio:input=0,output=1` |
| `(7, 8)` | Read local fd 7, write local fd 8 | `stdio:input=7,output=8` |
| `(-1, -1)` | Unspecified | `stdio:unspecified` |

The configured default is `(0, 1)`; the default-constructed value is unspecified. Input/output always describe this process's operations, including a parent that reads a child's stdout and writes its stdin. Do not reverse local fd numbers to manufacture a remote address. `expanded` can follow the Unix address precedent and leave this compact representation unchanged (E16).

Stderr is not part of this protocol channel. Diagnostics continue through application logging. TLS state, descriptor ownership, pid, executable names and mutable receiver state do not belong in this address.

### 4.3 Preserve the existing address-discovery path

The new physical adapter provides the same typed methods used by the existing template:

```cpp
int getSockName(SocketAddress::SockAddr& value,
                SocketAddress::SockLen& length);

int getPeerName(SocketAddress::SockAddr& value,
                SocketAddress::SockLen& length);
```

These are local value-copy operations for stdio. `getSockName` supplies the effective attached pair; `getPeerName` supplies the fully unspecified sentinel. Check the destination length and return ordinary error information on invalid storage. Do not use real socket syscalls for these operations.

Consequently, the original `getLocalSocketAddress`/`getRemoteSocketAddress` helpers and `ConfigAddressBase::getSocketAddress(SockAddr, SockLen)` can remain unchanged (E15, E17). This supersedes the earlier suggestion to bypass the common address/configuration types with a separate address-discovery branch.

## 5. Endpoint API and main() activation

### 5.1 Facade

The core template takes physical endpoint, configuration and original factory types, following the composition pattern of the current server/client templates. The public `net::stdio::stream::SocketEndpoint<Factory, Args...>` binds those implementation types. Constrain `Factory` to derive from the original `stream::SocketContextFactory`; keep its original `create(stream::SocketConnection*)` signature.

The core facade derives from `core::socket::Socket<Config>`. It shares configuration and factory/callback context in the same way as existing wrappers. Factory arguments are forwarded to its constructor. Connections retain the configuration and callback lifetime they need; they never retain a pointer to a temporary facade. Do not add a separate registry of live connections, a process singleton or a once-only global activation flag.

Proposed application surface:

```cpp
using StatusCallback =
    std::function<void(const SocketAddress&, core::socket::State)>;

// Proposed constructors/operations on SocketEndpoint<Factory, Args...>:
explicit SocketEndpoint(const std::string& instanceName, Args&&... args);

// getConfig() is inherited from the existing Socket<Config> wrapper.

/// Queue one attachment attempt; report completion on the event-loop thread.
void attach(const StatusCallback& onStatus) const;

/// Configure an explicit local address and queue one attachment attempt.
void attach(const SocketAddress& address, const StatusCallback& onStatus) const;
```

Retain the familiar `setOnConnect`, `setOnConnected`, `setOnDisconnect` and `getSocketContextFactory` pattern with the original connection type. These callbacks report connection lifecycle; they do not introduce a network `connect()` operation. Exact overloads and callback chaining should copy the current wrapper conventions rather than inventing a second convention (E10).

There is no retry/reconnect FlowController in this reduced design. The proposed `void` return deliberately supersedes the earlier exploratory FlowHandle sketch: an already-open descriptor pair has no acquisition retry loop. Connection/protocol shutdown remains available through the original context and normal framework shutdown. Claude should explicitly review whether this surface is sufficient for application-side cancellation before adding any handle or controller.

### 5.2 Scheduling and callback ordering

Construction configures the instance and creates the factory; it does not register descriptors or run the event loop. `attach()` queues startup through the existing `EventReceiver::atNextTick`, capturing shared configuration/factory state and any explicit address value. This follows the startup scheduling used by `FlowController::startFlow` without importing its retry state (E18).

Resolve configured defaults after configuration parsing. Each explicit address overload captures its value so a later independent attachment does not silently change it. Do not serialize unrelated endpoint objects or unrelated descriptor pairs. Attachment against the same already-observed descriptors must be diagnosed according to the framework's real receiver semantics; do not invent a global descriptor reservation service.

Only execute attachment while the core is RUNNING. Reject requests made in an invalid lifecycle state without touching descriptors. A queued request that reaches shutdown before execution must not create a connection during STOPPING. A request queued before `start()` has no completion until the event loop executes it, consistent with existing queued activation.

For an executed successful attempt, the proposed order is descriptor activation, `onConnect`, `onConnected`, original factory/context attachment and `Status::OK`, matching the existing plaintext callback order where possible (E19). Factory failure is attachment failure, not success followed by an anonymous close. Disabled configuration reports `DISABLED` without touching fds. Report one status per executed attempt. Mark the activated connection established immediately before invoking `onConnect`; every such callback is paired with `onDisconnect`, including later close/null/throw establishment failures. First/second registration failures before `onConnect` do not emit an application disconnect (`src/core/socket/stream/stdio/SocketEndpoint.hpp:53–89`).

The separate Stage 0 activation fix in section 8.4 is a prerequisite for this sequence; a stdio-only deferred-constructor alternative is withdrawn. Review reentrancy when a callback closes the connection or requests shutdown; do not reuse a raw pointer afterward without a proven lifetime. Application exceptions must release the new connection through normal unregistration and propagate or report through the established convention, never leak registrations. These guarantees remain **UNVERIFIED**.

### 5.3 Application example

This is proposed usage with the existing Echo factory and protocol. It assumes normal foreground operation and logging directed to stderr. These are application options, not global side effects of `SocketEndpoint`.

```cpp
#include "Log.h"
#include "apps/echo/model/EchoSocketContext.h"
#include "core/SNodeC.h"
#include "net/stdio/stream/SocketEndpoint.h" // proposed header

int main(int argc, char* argv[]) {
    using Endpoint =
        net::stdio::stream::SocketEndpoint<
            apps::echo::model::EchoServerSocketContextFactory>;

    const Endpoint endpoint("echo-stdio");
    endpoint.getConfig()->Local::setInputFd(0)->setOutputFd(1);

    core::SNodeC::init(argc, argv);

    int attachmentFailure = 0;
    endpoint.attach([&attachmentFailure](const Endpoint::SocketAddress& address,
                                        const core::socket::State& state) {
        if (state == core::socket::State::OK) {
            snode::log::application().info() << "Attached " << address.toString();
        } else if (state != core::socket::State::DISABLED) {
            attachmentFailure = 1;
            snode::log::application().error() << state.what();
            core::SNodeC::stop();
        }
    });

    const int loopResult = core::SNodeC::start();
    return attachmentFailure != 0 ? attachmentFailure : loopResult;
}
```

Configuration before initialization follows the established HTTP application pattern (E20). The queued attachment executes after the event loop starts, so the failure callback can request stop during RUNNING. No new process-owning `run()` wrapper is introduced. Replacing the factory with the existing Echo client factory changes protocol initiation, not endpoint type.

## 6. Configuration, defaults and one authority per value

`ConfigSocketEndpoint` composes the original `ConfigInstance`, `ConfigConnection`, and stdio `ConfigAddress` using the existing typed-section style. It does not inherit `ConfigPhysicalSocketServer`/`Client`, because backlog, accept counts, retries, reconnect and OS socket options are not applicable.

`net::stdio::config::ConfigAddress` follows the existing address adapter's inheritance through `ConfigSection` and `ConfigAddressLocal<net::stdio::SocketAddress>`. The internal SockAddr representation makes that possible without modifying `ConfigAddressBase` (E15, E21).

Provide both `Config::Local` and `Config::Remote`: the shared discovery helpers call their respective `getSocketAddress(SockAddr, SockLen)` overloads at E17. Remote is a non-CLI part exposing that raw reconstruction overload and the unspecified sentinel only. Reuse `ConfigAddressBase<net::stdio::SocketAddress>` through a small non-CLI adapter; do not inherit `ConfigSection`, register a `remote` subcommand or add remote fd options. `ConfigAddressRemote` itself merely inherits constructors, `ConfigAddress` forwards them, and the `ConfigAddressBase` constructor does nothing; CLI section registration is performed separately by `ConfigSection` (E58). Thus a named Remote part does not require a configurable remote section or the Unix reverse adapter.

| Section | Proposed setting | Contract/default |
| --- | --- | --- |
| Instance | existing disabled/required/configuration behaviour | Reuse existing instance policy |
| Local | `--input-fd`, `setInputFd/getInputFd` | Nonnegative fd; default 0 |
| Local | `--output-fd`, `setOutputFd/getOutputFd` | Nonnegative fd; default 1 |
| Local | typed `setSocketAddress` | Updates the same CLI-backed fd values |
| Local | input/output ownership | TAKE default; explicit BORROW opt-in; see R3 |
| Connection | timeouts, block sizes, watermarks | Original ConfigConnection options and types |

Use `addOption`, validators, typed section access, `setDefaultValue`, and the existing configuration cache/renewal rules. Do not copy CLI values into a second long-lived address configuration object or add an independent parser. A resolved connection address is a snapshot for that connection, not a competing mutable configuration authority. Reconfiguration applies changed defaults to future attachments; it does not swap live descriptors beneath an active receiver.

Add `ConfigInstance::Role::ENDPOINT` at the end of the existing enum, preserving the existing enumerator values. Render its help text as "endpoint" and map it to an absent optional logging role. The logger already supports an optional role (E22). The current configuration helper and connection constructor assume every non-server is a client, so both require adjustment (E12, E23). Use one shared role-to-log mapping authority, with no new logger role. Existing server/client rendering remains unchanged.

## 7. Physical endpoint and descriptor contract

The proposed move-only physical endpoint owns one connection's descriptor bindings and ownership policy. It provides the structural interface required by the shared connection: address aliases, validity, directional fd access, bind/local/remote snapshots and shutdown. It does not own protocol framing or publisher state.

Do not derive it from the current OS-socket implementation: that constructor calls `getsockopt`, and open/bind call OS socket functions (E24). Do not derive from `core::Descriptor` or another single-fd owner: `Descriptor::~Descriptor()` closes its one fd (E56). The stdio physical object owns the pair and its teardown policy; address values never close descriptors.

Its proposed `getFd()` returns the **input fd for identity/logging only**. The complete generic-connection consumer list at this revision is constructor naming/log identity at `SocketConnection.hpp:133`, reader enable at :176, writer enable at :178, and the connection's forwarding accessor at :206–207 (E61). Only the two enable calls change to directional accessors. Acceptor/connector acquisition uses their own physical fds; TLS handshake code uses the connection accessor, but neither path is instantiated for stdio (E61). An application must not infer that the identity fd also accepts output.

Attachment uses `fstat` on both handles and **rejects `S_ISREG` uniformly for epoll, poll and select**, before changing flags or publishing either receiver. It also validates descriptor existence and access modes. Negative numbers, regular files and inappropriate modes are attachment errors. Do not rely on epoll's `EPERM`: select/poll can accept the same regular file. These OS outcomes are review scenarios, **UNVERIFIED** here. Identical input/output numbers require a genuinely duplex descriptor and consistent ownership, with one release. Distinct duplicated numbers remain distinct handles even when their open-file description is shared.

R3 / owner decision D1: **TAKE is the default**. TAKE transfers ownership only after successful activation. BORROW is an explicit opt-in requiring caller-prepared nonblocking handles; it never closes, replaces or changes their flags. The caller must retain borrowed handles until their publisher interests are released. TAKE requires exclusive use of its numbers, including absence of unrelated publisher interests; it cannot close another live receiver's descriptor. No global reservation registry is introduced. Directional release follows D4 in section 8.3, with the destructor releasing whatever remains.

TAKE records original `F_GETFL` flags at activation, enables `O_NONBLOCK` transactionally and restores the recorded flags **before releasing the handles**, at the I1-safe directional release point or final teardown. Roll back changes on failed attachment and retain caller ownership; report restoration/replacement failures explicitly instead of claiming successful rollback. Snapshot both handles before either is changed, so distinct duplicates do not record already-modified flags. Identical numbers need one snapshot/release. This is temporary physical-endpoint ownership state, not a second configuration authority. Duplication does not isolate file-status flags: with shell `2>&1`, making protocol stdout nonblocking also affects stderr and can lose diagnostic writes. Later restoration cannot recover lost logs. R4 refuses sharing only when stderr shares the protocol-output description **and console logging is active**. With `--quiet` or another verified no-console-output path, sharing is allowed; interactive tty runs with `--quiet` must work (E75). This exception does not make arbitrary non-logger stdout writes safe on the shared description. These behaviours are **UNVERIFIED** until tested.

For stdio second-registration failure (failed/releasing), replace the **existing onDisconnect slot with the attachment-failure report**, instead of Stage 0's default no-op. `unobservedEvent()` invokes it after publisher release; the physical destructor then rolls back flags in the **same call**, after the report (E31, E68, E77). No normal application onDisconnect is emitted. A retry from the status callback is safe because proposed `attach()` queues through `atNextTick` (E18): it executes after this teardown/rollback completes. The callback must not directly close/reuse the handles while rollback is pending. Provisional attachment ownership has not transferred: the new release hook only detaches that failed direction, leaving flag rollback and caller-owned handles to this failure path. This ordering is a required **UNVERIFIED** integration test.

After all publisher interests for a number are released, restore TAKE's flags and release that direction from the new hook; the physical destructor releases any remaining handles. For an owned number **0, 1 or 2, replace it with `/dev/null` using `dup2`, never leave it closed**; use a read-capable null handle for input, a write-capable one for output, and `O_RDWR` for a duplex standard handle. Own and clean up the temporary null handle too. Close owned numbers **>= 3** normally. Replacement obeys I1 just like closing. Leaving a standard number free permits a later `open` to reuse it, after which ordinary stdout/stderr writes or logging could target an unrelated resource. This required directional/final release contract is **UNVERIFIED** and is never an immediate-shutdown operation. External duplicate writers can still postpone peer EOF.

Required descriptor kinds are anonymous pipes, duplex local sockets and interactive tty with `--quiet`; execution across the supported multiplexers remains **UNVERIFIED**. FIFO physical shutdown has the same no-close rule, but FIFO runtime support needs demonstration before being claimed. R6 is resolved as uniform regular-file rejection; no blocking fallback or regular-file support belongs here.

## 8. Shared connection integration and exact core seams

### 8.1 Directional registration and dispatch-only writes

Add small compile-time helpers used by `SocketConnectionT` to obtain directional bindings from physical types that supply them; other types retain the original `getFd()` path. Stdio supplies the existing template's reader/writer arguments. Do not change network plaintext/TLS template arities or names.

Stdio overrides only the syscall hooks with `core::system::read/write`, inheriting buffering, counters, wakeup, streaming and drain (E07, E08, E25). `sendToPeer()` only appends/resumes; it does not synchronously call `write`. Writing occurs in private `doWrite()`, reached from private final `writeEvent()` through `WriteEventReceiver::dispatchEvent()` (E35, E62). The loop installs SIGPIPE `SIG_IGN` in both `init()` and `start()` (E63). The proposed endpoint activates only during RUNNING, so calls before `start()` cannot reach its unactivated writer. This is a source trace, not executed proof: **UNVERIFIED**. Require zero write-hook calls before start, enqueue from onConnected, and broken-peer `EPIPE` during dispatch without process termination. Arbitrary direct POSIX writes outside this path are not covered by the endpoint.

### 8.2 I1: shutdown requests removal; release follows publisher removal

**I1: a descriptor number is closed or replaced only after all publisher interests for it are released.** The existing write path calls `physicalSocket.shutdown(WR)` at `SocketConnection.hpp:328` **before** `onShutdown()` at :335 requests writer disable through :276–279 (E48). Disable is deferred; publisher cleanup/`muxDel` runs from `releaseExpiredResources` (E29, E30, E49). Even moving close to just after `disable()` violates I1.

For a socket fd, stdio physical `shutdown(RD/WR)` calls `::shutdown` on the corresponding direction, as the network physical adapter does (E69); it does **not** close the fd. For pipe/FIFO/tty, physical shutdown remains a no-op. The existing reader/writer state requests deferred removal; only the new publisher-release hook in section 8.3 permits the physical endpoint to release a direction. The destructor releases whatever remains after final `unobservedEvent()` deletion (E06, E31, E68). BORROW only detaches and never releases the caller's fd. This is proposed behaviour, **UNVERIFIED**.

Keep the original once-per-connection signal notification and drain path (E26, E27). The active original stream context receives its default-no-op virtual `onShutdown()` notification before signal handling and drainage for every framework shutdown reason (`src/core/socket/stream/SocketContext.h:117–118`; `src/core/socket/stream/SocketConnection.hpp:398–412`). The MCP context overrides that hook; there is no connection-owned std::function or setter. Factory ownership is unchanged. The Stage 5 regression below establishes why MCP needs this pre-drain seam. After queued output drains, writer disable reaches the release hook and closes/replaces TAKE's output handle under I1, delivering pipe EOF **while input can remain active**, assuming no external duplicate writer remains. This fixes the client-role scenario where a filter-style child waits for stdin EOF before producing its reply: teardown-only release keeps the pipe writer open while the parent waits for input, stalling until the terminate timeout. The new scenario and fix are **UNVERIFIED** until the two-pipe regression in section 12 runs. MCP's normal input-EOF-first sequence and bounded server-initiated shutdown remain required; existing terminate timeouts at `SocketConnection.hpp:323–324` are retained (E48). No new shutdown timer or next-tick dependency is introduced (E28).

### 8.3 R1 / D4: narrow deferred publisher-release hook

The close-before-DEL review scenario is particularly dangerous with a description shared by `2>&1`: closing one number can leave an epoll interest alive through the duplicate. Epoll stores the receiver pointer, treats DEL/`EBADF` as removed for accounting, and later spans returned pointers (E49). Deleting the writer while such an interest survives can produce a use-after-free. The ordering/pointer accesses are verified by inspection; kernel retention and UAF reproduction are **UNVERIFIED**, not an executed exploit. I1 avoids this by retaining the original number until DEL completes.

Owner decision **D4 reinstates independent pipe write-half-close** through one narrow core seam, **UNVERIFIED** until implemented and tested:

1. In `DescriptorEventPublisher::releaseDisabledEvents`, remove the disabled receiver from its fd list as today. If the list became empty, perform the existing `muxDel(fd)` and erase; otherwise leave the existing `triggered()`/muxOn/muxOff handover intact. **After that branch and before `setDisabled()`**, invoke a new virtual hook on the disabled receiver meaning "this receiver's publisher interest has been removed" (E82). The default hook does nothing. Do not invoke it from `disable()` or synchronous shutdown.
2. `SocketWriter` forwards the hook to a connection-side directional callback; `SocketReader` forwards symmetrically. `SocketConnectionT` dispatches to the physical endpoint through the existing compile-time detection/helper style (E85). Existing directional virtual shutdown forwarding is the structural precedent, not an existing release API (E86). Network physical sockets do nothing at this hook and retain their current shutdown/destructor behaviour. The names and declarations of the new hooks must follow the receiver's event-hook conventions (E84).
3. For a successfully activated TAKE pair, the stdio physical endpoint restores saved flags, then closes output >= 3 or replaces output 0–2 with `/dev/null`. Input release is symmetric. BORROW only detaches. Mark released handles in the physical endpoint's ownership record so destruction releases only what remains, once. The hook does not delete the receiver, invoke application callbacks, throw, or recursively run publisher cleanup; `setDisabled()` performs the existing observation decrement and may immediately delete the complete object (E83).
4. **Why I1 holds:** for a distinct, exclusively owned direction there is no other receiver on that fd, in this publisher or another one. Thus the empty-list branch's DEL precedes physical close/dup2. TAKE's exclusive-use precondition is essential; a hook invocation after removal from a nonempty list is not permission to close a shared fd. Identical duplex input/output numbers defer physical release until both direction interests have been removed; socket `::shutdown(WR)` still provides its logical half-close earlier. Distinct duplicates also need coordinated saved-flag restoration when they share a description: do not turn a still-active direction blocking. These cases, shared descriptions including `2>&1`, and immediate number reuse after directional release require tests; **UNVERIFIED**.

Reader/writer share the original virtual `Observer`; final `unobservedEvent()` still follows their combined observation release (E31). The hook runs before that possible deletion, making physical release independent of context/connection destruction without replacing their lifetime authority. Provisional attachment failure retains section 7's rollback/report order and never closes caller-owned handles through this hook.

The previous synchronous backend-removal recommendation stays **withdrawn**. It changes framework-wide publisher timing, interacts with select/poll iteration over receiver lists, and alters multi-receiver handover through `triggered()` (E50). It also fails to fix shutdown-before-disable: the physical call already precedes the disabling callback (E48). D4 is **not** that design: it adds only a notification at the existing deferred release point, after the unchanged removal/handover branch. Publisher timing, select/poll iteration and multi-receiver handover remain unchanged; their regression proof is **UNVERIFIED**. No multiplexer implementation change is proposed.

Only the narrow receiver hook and its publisher invocation belong in Stage 2; none belong in Stage 0. No polling timer, recursive cleanup, reservation registry or forwarding thread/socketpair is proposed. D4's design choice is settled; execution tests remain **UNVERIFIED**.

### 8.4 R2: separate Stage 0 core bug fix before any stdio code

This is a **pre-existing critical lifetime/UB defect**, not a stdio-only seam. `SocketConnectionT` executes `delete this` on either registration failure (E32). The acceptor and both connector paths subsequently dereference the returned pointer and invoke callbacks (E51). TLS continues derived member initialization and its constructor body on freed storage (E52). Reader-success/writer-failure also leaves a published reader dangling after deletion. These are source-derived failure paths; runtime/ASan reproduction is **UNVERIFIED**.

Failures are reachable: select rejects fd >= `FD_SETSIZE`; epoll ADD propagates failures and handles allocation failure (E53). Triggers to exercise are a high accepted fd under select, epoll `ENOSPC`/`ENOMEM`, and regular-file stdin's `EPERM` if it bypassed the future attach precheck. OS-trigger reproductions are **UNVERIFIED**. Regular-file rejection is stdio validation, not a fix for network activation failures.

Stage 0 is a **separate authorized core bug-fix branch, completed and verified before any stdio feature code**. Its simplified activation contract is:

1. Constructors only construct; they never publish receivers or self-delete. Fully construct the plaintext/TLS object first.
2. Every construction site calls **nonvirtual `bool activate()`** before logging success, attaching a context, starting TLS or invoking connection callbacks. Contract: **false: the connection is released or being released; the caller must not use the pointer again.** There is no public disposition enum.
3. First-registration failure: `activate()` preserves errno with `utils::PreserveErrno` and performs `delete this` on the complete, entirely unobserved object. Self-deletion is outside construction, consistent with the final cleanup idiom at E77. No successful connection/disconnection callback fires. The caller only reports the captured errno; it never deletes or dereferences the pointer again.
4. Second-registration failure: preserve errno, reset the **existing** stored `onDisconnect` to a no-op, disable the reader, and return false. Normal publisher release reaches `unobservedEvent()` and deletes the complete object. The callback member is at E68; disable/release uses E29–E31. Future stdio integration uses that same slot for a failure report, as specified in section 7.
5. Success: suspend the writer as before and return true, then retain the successful construction-site callback order. Failures use caller failure handling; no success log/status may precede activation. Keep one activation algorithm across transports.

All allocation sites found by `git grep -n -E 'new[[:space:]]+(SocketConnection|TestConnection)[[:space:]]*\(' e81147c4c -- src tests` are:

| Site | Existing path requiring post-new activation |
| --- | --- |
| `src/core/socket/stream/SocketAcceptor.hpp:253–266` | Accepted connection; allocation at :254 (E51) |
| `src/core/socket/stream/SocketConnector.hpp:209–224` | Immediate connect success; allocation at :210 (E51) |
| `src/core/socket/stream/SocketConnector.hpp:269–283` | Deferred connect success; allocation at :270 (E51) |
| `tests/unit/core/StreamFrameworkShutdownTest.cpp:501–521` | Direct construction followed by context attachment (E54) |

Plaintext/TLS wrappers forwarding to the base must also stop depending on constructor activation (E52, E69). Searching `SocketConnectionT<` under tests found the one direct fixture at :272–274 (E54); implementation must repeat the search for alternate allocation syntax and downstream callers. This is the inspected revision's list, not a promise about unknown external callers.

Recompiled out-of-tree direct constructor callers that omit `activate()` **silently receive an unactivated connection: no registered receivers and no error**. This is a source-level behaviour change, not a compilation diagnostic (constructor/instantiation evidence: E55, E69, E76). Document the required post-construction call and never access the pointer after false. The direct-caller recompile regression is **UNVERIFIED** here. Do not retain an unsafe auto-activate compatibility path.

Owner decision **D2 = (a)**: document the direct-constructor behaviour change; leaf constructors stay public. The intended construction path is framework-internal, as illustrated by the existing acceptor/connector friendships, which are not access restrictions (E76). Owner decision **D3: Stage 0 PATCH; stdio feature MINOR** remains recorded; SOVERSION decisions belong to the owner at release time. Neither decision itself authorizes stdio feature implementation.

N5 disposition: **accept** the existing `"transport disconnected"` cleanup log even for a never-established connection. It occurs after the stored callback and before deletion at `SocketConnection.hpp:443–447` (E77); it does not mean a successful connection or a normal application disconnect callback occurred. No suppression branch or Stage 0 estimate increase is proposed. Runtime log/failure evidence is reported separately from this proposal.

Required Stage 0 regression: select backend, raise `RLIMIT_NOFILE` above `FD_SETSIZE`, prepare a real listener/client connection so the accepted fd is >= `FD_SETSIZE`, and assert clean rejection, no success/context callback, no leak/stale published receiver, and clean strict ASan. If the host cannot raise the limit, report the prerequisite failure rather than count a skip as proof. Force second-registration failure to verify delayed deletion/no-op disconnect, and cover both connector success paths and TLS construction. Executed Stage 0 results are reported separately; this proposal does not claim unexecuted stdio checks as passing.

## 9. Resource and callback lifetime

| Resource | Authority | Release |
| --- | --- | --- |
| CLI/default values | Existing configuration sections | Instance destruction |
| Config/factory/callback shared context | Endpoint's shared wrapper context | Last endpoint/active callback reference |
| Descriptor ownership, saved flags and provisional transfer | Physical endpoint | Directional publisher-release hook under I1; destructor releases remaining handles; restore flags, replace owned 0–2, close owned >= 3; provisional failures roll back |
| Byte buffers/counters/watermarks | Original reader/writer | Existing connection destruction |
| Protocol context and pending context switch | Original stream connection | Original detach/teardown path |
| Startup work | Existing one-shot event-queue entry | Execution or normal event-loop cleanup |

The endpoint facade never owns an independently scheduled destruction path for the context. Do not add shared ownership of `SocketContext`; the original framework owns its raw result and destroys it. Do not delete a receiver while the publisher or event queue can still reference it. `close()` keeps its current immediate-close-request meaning; graceful output completion uses the original `shutdownWrite()` path, not a redefinition of `close()`.

## 10. MCP adaptation without another vertical I/O layer

Introduce an ordinary MCP stream `SocketContext` and `SocketContextFactory`, using the original core base classes. The context owns newline framing, protocol-era metadata, pending request/reply bookkeeping, cancellation and JSON dispatch. It borrows or retains the MCP server/config under an explicitly documented application lifetime. Those responsibilities are protocol state; they do not move into core stdio.

Replace the current MCP `Input`/`Output` DescriptorEventReceiver classes and `outgoing` string with the original connection's reader/writer. Keep the MCP HTTP Router on the existing express/HTTP transport. Reuse of a byte-stream factory does not make newline-framed MCP interchangeable with its HTTP binding.

Important migration details that must appear in code review:

1. **Idle timeouts:** the current MCP receiver uses timeout zero; general `ConfigConnection` defaults are read/write 60 seconds (E33, E34). Set MCP's connection defaults to disabled read/write inactivity timeouts before parsing, preserving normal typed configuration overrides. Do not accidentally disconnect idle native clients after one minute.
2. **Accepted output versus backpressure:** `SocketWriter::sendToPeer()` appends bytes before returning its watermark result (E35). False is not proof that the bytes were rejected. Never retry the same serialized reply solely because it returned false.
3. **Hard MCP output budget:** the original MCP output limit is a hard queued-byte check (E36). The core's maximum-write-queue option bounds its watermark; it is not itself a hard rejection before append (E35, E37). Check the serialized message against MCP's remaining budget before enqueueing, using the original connection's cumulative queued/sent counters as the buffer-occupancy authority. Do not retain a second output queue or a mirrored queued-byte counter. Explicitly review the relation between this protocol budget and the existing connection watermarks.
4. **Counter scopes:** use connection-level cumulative counters together when deriving queued bytes. The context's queued counter is adjusted for context attachment while its sent counter is delegated directly (E38); mixing those scopes would produce incorrect occupancy after a context switch.
5. **Receive progress:** the original connection closes when bytes are available but the context reports zero consumption (E39). The MCP context must consume incoming bytes correctly while retaining partial lines. Bounded processing of already-framed messages needs an explicit RUNNING-state continuation; do not return zero as a scheduling mechanism or silently keep the old periodic I/O polling timer.
6. **Limits and cancellation:** preserve existing message size, pending-request bounds, cancellation, deadlines, protocol eras and request/error wire formats. Retain protocol deadline timers; removing transport polling does not remove legitimate per-request timers.
7. **Process bootstrap:** the current `Transport::run()` preserves stdout, enforces foreground initialization, runs readiness registration and owns the core loop (E40). Move process policy into the executable bootstrap and use the endpoint for attachment. Keep protocol stdout free of logs and preserve existing behaviour with normal client configurations. The endpoint itself never redirects global stdout or injects argv options.

### 10.1 R4: application-local stdout retention

Reuse the existing executable policy from `Transport.cpp:278–280`: retain original stdout with `fcntl(STDOUT_FILENO, F_DUPFD_CLOEXEC, 3)`, then `dup2(STDERR_FILENO, STDOUT_FILENO)`, **before `SNodeC::init`** (E74). Move this small retention sequence into the MCP executable; leave nonblocking setup to endpoint activation so TAKE records/restores the original flags. This is application policy, never generic endpoint behaviour. With independent descriptions, logger output, configuration's direct `std::cout` (E72) and third-party stdout writes all go to diagnostics rather than protocol output, independent of parse-phase precedence. Do not change the `--log-console` default, install a console validator, add a preparse hook or introduce a global setter.

The new four-step sequence is:

1. **Retain stdout + shared-description check.** Obtain `retainedFd >= 3`, redirect stdout to stderr, and classify whether stderr shares the retained protocol-output description before any nonblocking change. Carry that classification to the post-parse check; do not refuse merely because descriptions are shared. Refuse only when sharing **and active console logging** coexist. `--quiet` disables console logging through the existing option/application path (E75); another verified no-console-output path is also acceptable. Interactive tty with `--quiet` must work. A refusal reports **exit status only**, never text into protocol output. Descriptor-identity detection and tty behaviour are **UNVERIFIED**; `fstat` identity is a conservative check, not a universal proof of shared open descriptions. Quiet does not suppress arbitrary third-party writes or direct `std::cout`, so shared/quiet operation does not promise the isolation available with independent descriptions.
2. **Init + daemon refusal.** Create application configuration as before (E71), call `SNodeC::init(argc, argv)`, then inspect current `--daemonize` and shared-description-plus-active-console settings. On refusal call `core::SNodeC::free()` and return nonzero **before calling start**, reporting through exit status only (`src/apps/mcp/main.cpp:82–84`). After that initial check, install `root.getOption("--daemonize")->check(CLI::IsMember({"false"}))` for the bootstrap reparse (E64, E65). A daemon request then becomes a CLI parse error: `ConfigRoot::parse2` handles it and returns false (E79), and `EventLoop::start` sets `stopsig = -2`, restores signal dispositions and calls `free()` normally (E81). The diagnostic goes to fd 1, the diagnostics alias established by stdout retention. Do not throw an application-local exception from a final callback: it escapes the typed catches in parse2 and the unguarded bootstrap call, bypassing that cleanup (E79, E81). No final callback or non-CLI throw mechanism is proposed. **Accepted residual:** a configuration change between init and start that re-enables console logging with a shared description is not re-guarded. Initial and bootstrap daemon refusal/cleanup remain **UNVERIFIED** until tested.
3. **Endpoint + attach `(0, retainedFd)`.** Construct the endpoint with the original MCP factory, set disabled read/write inactivity defaults before bootstrap parses endpoint options, preserve MCP's output-budget defaults, and register tools. Call proposed `attach(SocketAddress(0, retainedFd), onStatus)` with TAKE ownership. The executable owns retainedFd until successful transfer and cleans it up on earlier failure. At each direction's publisher release TAKE restores its flags, replaces input fd 0 with `/dev/null` or closes retainedFd (>= 3), under I1; final teardown releases any remainder. Fd 1 remains the application's diagnostics alias; no endpoint redirection occurs. Keep server/config alive for all contexts. Failure/retry ordering follows section 7; **UNVERIFIED**.
4. **Start.** Call `core::SNodeC::start()` and return its result on the accepted startup path (E67). The daemon-only validator guards bootstrap reparse before descriptor activation. Preserve normal cleanup on refusal/exception; no private event loop or polling transport timer.

Required **UNVERIFIED** tests: protocol bytes stay on retainedFd while logger, configuration and third-party stdout writes use independent diagnostics; default and explicit stdout/stderr console selections both work; initial and bootstrap `--daemonize=true` are refused before daemon startup; shared descriptions with active console logging refuse silently; shared descriptions with `--quiet` (including interactive tty) work; bootstrap daemon rejection follows normal signal restoration/free. A bootstrap change that re-enables console logging with shared output is the accepted unguarded residual. Keep idle-timeout/output-budget, flag restoration and EOF/shutdown regressions. The removed console-default/validator precedence gates are not requirements of this design.

Removing an installed MCP entry changes that module's source API and needs explicit migration notes. SOVERSION decisions belong to the owner at release time; no version change is authorized by this document.

## 11. CMake, exports and installation

Follow the existing MQTT component layout: the transport-independent stream adapter belongs to the protocol library, and only web-dependent bindings need a separate MCP component. Proposed targets/components are `core-socket-stream-stdio`, `net-stdio`, `net-stdio-phy-stream`, and `net-stdio-stream`; introduce a separate `net-stdio-phy` component only if its directory actually contains a target, not solely to add another empty layer.

Proposed dependency direction:

```text
core-socket-stream-stdio -> core-socket-stream
net-stdio -> net
net-stdio-phy-stream -> net-stdio
net-stdio-stream -> net-stdio-phy-stream + core-socket-stream-stdio
mcp-server -> core-socket-stream
snodec-dev-stdio -> mcp-server + net-stdio-stream
```

The exact placement of explicit template instantiations and dependency visibility must be reviewed so the core library does not acquire a reverse link dependency on a `net::stdio` implementation. Existing core templates can be instantiated in the transport layer, as in the current transports. Do not hide a core/net dependency cycle behind transitive includes.

Add the `stdio` subdirectory under `core/socket/stream`, beside the existing specialization subdirectories (E73), and direct includes, exports, install exclusions and component dependency wiring using the existing pattern (E43). Update the component model used for packages and installed consumers (E44). Release-time SOVERSION decisions belong to the owner. Do not add a new test option, pin a dependency, change licenses or publish a release as part of this work.

The installed consumer must compile the same custom context/factory against a network backend and `snodec::net-stdio-stream`, using only staged installed headers/libraries. An in-tree example alone is insufficient. The current staged-consumer test already provides this boundary (E45).

## 12. Verification matrix for implementation

Use the existing plain CTest/support mechanism (E46). All checks below are **UNVERIFIED** proposed execution gates, not results. Do not weaken existing tests to accommodate the new endpoint.

| Boundary | Required regression evidence |
| --- | --- |
| Raw address/wrapper | Header at offset zero, standard-layout/trivial-copy assertions, length/family validation, explicit -1 initialization, round-trip copy, typed setters, formatting |
| Config | 0/1 defaults, CLI/default precedence, show/write config, cache renewal, neutral role, unchanged server/client descriptions |
| Protocol reuse | One unchanged context/factory source instantiated for in/in6/un/l2/rc and stdio; no protocol conditional compilation |
| Runtime equivalence | Existing Echo roles over two prepared pipe pairs and over available network backends; bounded shutdown after exact bytes |
| Attach scheduling | Before-start queueing, RUNNING attachment, disabled instance, invalid lifecycle, independent concurrent endpoint objects |
| Descriptor selection | Arbitrary nonstandard numbers; identical duplex fd; no fd 0/1 hard-coding |
| Ownership | BORROW untouched; TAKE saved flags restored, owned 0–2 replaced with null handles, >= 3 closed once, originals retained on failure; shared-description tests |
| Stage 0 failure paths | Select high accepted fd with raised RLIMIT_NOFILE; first/second registration failures, both connector paths, TLS complete construction; no UAF/leak/success/disconnect misreporting |
| Attach validation | fstat rejects S_ISREG on all three backends before flag/publication changes; invalid fd/mode, factory null/throw, rollback |
| Lifetime/reentrancy | Facade destruction after scheduling, close from context onConnected, stop from status callback, context switch during callbacks |
| I/O | Fragmented input, coalesced messages, partial writes, EINTR/EAGAIN, read EOF, EPIPE, byte-exact delivery; interactive tty with --quiet |
| Backpressure | False-after-accept semantics, no duplicate output, high/low watermark and onDrain behaviour, concurrent timer/request responsiveness |
| R1 / I1 | No close/dup2 before publisher release; release hook after removal/DEL and before possibly deleting setDisabled; same number and duplicated descriptions (including 2>&1); standard-number reuse cannot redirect output |
| D4 / pipe write-half-close | Client sends exact bytes through two pipes, requests shutdownWrite while input remains active; filter-style child waits for EOF before replying; EOF and complete reply arrive before terminate timeout, with input active during output release; symmetric input release, BORROW detachment, no duplicate close/flag restore |
| SIGPIPE / F11 | No pre-start write-hook call; sendToPeer enqueue only; broken-peer EPIPE through event dispatch without termination |
| Multiplexer | epoll/poll/select removal, hook called once after receiver removal, default no-op, unchanged triggered handover and suspended stacks, live shutdown iteration, no stale receiver or interest-count leak |
| Framework shutdown | Requested/signal/no-observer paths, duplicate notifications, terminate timeout, no new shutdown timer/next-tick dependency |
| MCP / R4 | Existing wire/SDK, idle >60 seconds, output cap, framing/cancellation/eras, EOF-first and server shutdown bound, stdout retention, initial/reparse daemon refusal, active-console-only alias refusal and quiet tty acceptance; HTTP unchanged |
| Installed consumer | Same context/factory compiled and executed using staged components, no source-tree include leakage |

Runtime coverage for hardware-dependent l2/rc transports is UNVERIFIED until actually executed. Report compile coverage separately from runtime coverage; do not claim all transports were exercised from a successful template instantiation.

Stage 0 affects all stream construction paths; focused stdio/MCP checks alone are insufficient. Stage 0 has no publisher change; Stage 2 alone adds D4's narrow deferred release notification, with no multiplexer implementation change. Run existing descriptor, pipe, stream lifecycle, network, express and TLS regressions appropriate to the actual core diff, then full GCC and strict Clang+ASan CTest with `TMPDIR=/tmp/snc`. Require zero failures and zero unexplained skips, and report unavailable hardware separately. Preserve native MCP stdio/HTTP transcripts and wire comparisons using normal client configuration; no user-profile modifications.

## 13. Source API and lifecycle constraints

Keep the original context/factory ownership, transport-independent protocol surface and network template arities/names. The later owner-approved A1 amendment adds only the default-no-op virtual `SocketContext::onShutdown()` hook to the original context; it replaces the connection callback and follows existing context-hook ownership. Protocols must remain transport-independent. Necessary members or virtual hooks are judged by source correctness, slimness and established SNode.C structure; D4's narrowly scoped release notification is the approved design direction for Stage 2, not an implementation authorized by this document.

Stage 0 requires every construction site, including out-of-tree direct callers, to invoke `activate()` after complete construction. Newly compiled callers that omit it silently remain unactivated; false makes the pointer unusable. D2 documents that source-level behaviour change with public leaf constructors retained (E55, E69, E76). D3 records Stage 0 PATCH and stdio feature MINOR; release-time SOVERSION decisions are the owner's. D4 preserves deferred publisher timing and handover, introducing only the removal notification specified in section 8.3.

Report additional source API or behaviour changes with their affected callers and migration requirements before extending the agreed scope. Do not preserve an unsafe auto-activation or early-close path under the name of compatibility.

## 14. Proposed implementation stages and accounting

This document is a proposal, not stdio implementation authorization. Stage 0 is separately authorized with net production <= +70 and test additions <= 260; its row below is measured from master through the bool-activation simplification. All stdio estimates are **UNVERIFIED planning ranges**, including headers/comments/includes/guards/CMake. No stdio feature ceiling has been approved. Test code is separate; documentation is not production. Before coding, agree per-stage bounds and replace estimates with measured accounting. Prior MCP ceilings do not apply.

| Stage | Deliverable / dependency | Production added | Production removed | Net production | Test lines added |
| --- | --- | ---: | ---: | ---: | ---: |
| 0 (measured) | Separate core bug-fix branch: construct then bool activate; all construction sites, select/ASan and second-failure regression; complete before stdio | 66 | 28 | +38 | 250 |
| 1 | Stdio address, local/non-CLI remote config, physical ownership/fstat/flag restoration/null replacement | 440–650 | 0 | +440 to +650 | 140–220 |
| 2 | `core/socket/stream/stdio` reader/writer and thin connection; directional shared-template selection; I1-safe release hook and pipe write-half-close | 185–285 | 0–8 | +177 to +285 | 110–180 |
| 3 | Endpoint facade, net binding/config role, component/export/install and staged consumer | 430–600 | 8–18 | +412 to +592 | 110–180 |
| 4 | MCP ordinary context/factory and retained-stdout bootstrap; remove obsolete receivers/output queue/run entry | 335–500 | 320–440 | -105 to +180 | 130–210 |
| 5 | Full toolchains/multiplexers/native clients; evidence only, no speculative fix allowance | 0 | 0 | 0 | 0 |
| Stdio feature (1–5) | Excludes separately reviewed Stage 0 | **1,390–2,035** | **328–466** | **+924 to +1,707** | **490–790** |
| Combined envelope | Measured Stage 0 plus stdio planning ranges, not a single branch budget | **1,456–2,101** | **356–494** | **+962 to +1,745** | **740–1,040** |

Per-file-group estimates make the responsibility and costs reviewable:

| Stage | File group | Production added / removed | Test group / added |
| --- | --- | --- | --- |
| 0 (measured) | `src/core/socket/stream/SocketConnection.h/.hpp` | 18 / 3 | Existing fixtures plus registration-failure tests/CMake: 250 total |
| 0 (measured) | `src/core/socket/stream/SocketAcceptor.hpp`, `SocketConnector.hpp` | 46 / 25 | Included in Stage 0 test total; no double counting |
| 0 (measured) | `src/core/socket/stream/legacy/SocketConnection.h`, `tls/SocketConnection.h` direct-caller comments | 2 / 0 | Included in Stage 0 test total |
| 1 | `src/net/stdio/SocketAddress.h/.cpp` | 100–150 / 0 | Address/config: 50–80 |
| 1 | `src/net/stdio/config/ConfigAddress.h/.cpp` (local + non-CLI Remote) | 120–180 / 0 | Included in address/config total |
| 1 | `src/net/stdio/phy/stream/PhysicalSocket.h/.cpp` | 220–320 / 0 | Ownership/validation/flags: 90–140 |
| 2 | `src/core/socket/stream/stdio/SocketReader.*`, `SocketWriter.*` | 70–110 / 0 | Dispatch/shutdown/I1: 110–180 total |
| 2 | Same directory: thin `SocketConnection.h/.hpp` | 70–105 / 0 | Included in Stage 2 total |
| 2 | Existing shared `SocketConnection.hpp` directional helpers | 30–45 / 0–8 | Included in Stage 2 total |
| 2 | `src/core/DescriptorEventReceiver.h`, `DescriptorEventPublisher.cpp`, shared reader/writer and connection hook forwarding | 15–25 / 0 | D4 half-close/removal/handover cases included in Stage 2 total |
| 3 | `src/core/socket/stream/stdio/SocketEndpoint.h/.hpp`, `DescriptorOwnership.h` | 200–250 / 0 | Facade/config: 70–115 |
| 3 | `src/net/stdio/stream/SocketEndpoint.*`, endpoint config; existing instance/connection role mapping | 170–260 / 8–18 | Included in facade/config total |
| 3 | Component CMake/package/export/install wiring | 60–90 / 0 | Installed consumer: 40–65 |
| 4 | `src/mcp/server/stdio` ordinary context/factory and deleted transport engine | 300–440 / 300–410 | MCP protocol/bootstrap regressions: 130–210 total |
| 4 | `src/apps/mcp/main.cpp` bootstrap/adaptation | 35–60 / 20–30 | Included in Stage 4 total |

The executable bootstrap group remains **35–60 added lines**, replacing the final callback with initial refusal plus the existing daemon validator. Per N7, the Stage 3 facade group targets **<= 250 lines** (planning range 200–250): one constructor, two attach overloads, callback setters and queued attachment, using the established wrapper conventions (E80). Do not copy the full server/client constructor-overload sets. The actual Stage 3 ceiling will be set from a **measured diff**; the target and all totals are **UNVERIFIED**, and no feature ceiling is approved.

The synchronous publisher-removal redesign remains excluded. D4 reinstates independent pipe write-half-close with a **15–25-line core release-hook allowance in Stage 2**; no multiplexer implementation change is proposed. Other feature ranges are unchanged, with ownership records handling directional release in Stage 1 and the new half-close test within Stage 2's existing test range. The two-phase activation fix remains a separately bounded prerequisite. Physical saved flags/ownership replace implicit single-fd destruction, while original buffers and lifecycle remain authoritative. MCP's private receivers, output buffer and process-owning run entry are deleted in the migration; do not leave a parallel engine. All feature ranges remain **UNVERIFIED**.

Each future implementation commit must build and pass relevant focused tests. Run the Stage 0 select regression before depending on it. Stage 2 must prove D4's EOF-before-reply scenario and unchanged publisher handover on all backends. Stage 4 cannot remove the old bootstrap until R4's retention/daemon/quiet-diagnostics tests pass. Do not use Stage 5 as an unbounded code-growth reserve; unexpected fixes require a revised estimate/authorization.

## 15. Updated R1–R6 decision table

| Gate | Revised decision | Remaining gate |
| --- | --- | --- |
| R1 | I1 and D4: physical shutdown never closes; narrow hook after publisher removal/DEL and before setDisabled releases directions; pipe half-close with active input restored | Owner-decided; EOF-before-reply, fd-reuse/duplication and unchanged handover proof **UNVERIFIED** |
| R2 | Separate Stage 0: constructors never self-delete/publish; all sites call bool activate; first failure self-deletes after construction, second suppresses disconnect and defers deletion; false pointer unusable | Bounded Stage 0 verification reported separately before stdio |
| R3 | TAKE default (D1); stdio failed/releasing reports through the existing slot, then rolls back flags; shared descriptions allowed with no console output | Owner-decided; rollback/report/retry and quiet tty tests **UNVERIFIED** |
| R4 | Retain stdout + shared-description check; init + daemon refusal; endpoint attach (0, retainedFd); return start result. Initial refusal plus daemon-only validator guards reparse; console re-enable residual accepted | Retention, silent active-console-only sharing refusal, quiet tty acceptance and daemon reparse tests **UNVERIFIED** |
| R5 | Stage 0 measured separately; Stage 2 adds only D4's narrow core hook, no synchronous publisher redesign or multiplexer implementation change | No approved feature ceiling; measured budget required; feature estimates **UNVERIFIED** |
| R6 | fstat/S_ISREG rejection at attach uniformly for epoll/poll/select; no file fallback | Design settled; all-backend pre-publication rejection proof **UNVERIFIED** |

### Owner decisions D1–D4

| Decision | Options / recommendation | Status |
| --- | --- | --- |
| D1 | **TAKE default**, with the specified restoration/null-replacement contract; BORROW explicit opt-in | Decided |
| D2 | **(a) Document silently unactivated recompiled direct callers; keep leaf constructors public** | Decided |
| D3 | **Stage 0 PATCH; stdio feature MINOR**; owner decides SOVERSION at release time | Decided; no tag/release action authorized |
| D4 | **Independent pipe write-half-close**, I1-safe through a narrow receiver hook after publisher removal/DEL and before setDisabled; symmetric input release | Decided design direction; Stage 2 implementation not authorized here |

Endpoint/context/factory naming, no stdio TLS and no stdio server/client wrappers are settled and unchanged.

## 16. Follow-up Claude review requested

Claude's follow-up verdict was **Stage 0 ready for a bounded implementation plan; stdio feature requires revision**. This revision applies N1–N7 and the owner's D4 and bool-activation decisions; subsequent review of these changes has not run. Review the revised source-backed design, without stdio implementation or operational changes. Findings should name severity, section, file:line, failing scenario and the smallest correction. Mark unexecuted behaviour **UNVERIFIED**.

- Check I1 and D4's post-removal/pre-setDisabled release hook, independent pipe half-close with active input, standard-number null replacement, shared descriptions/flags and unchanged deferred publisher timing/handover.
- Check Stage 0's complete construction-site list, bool activation and unusable false pointer, Stage 0 no-op versus stdio failure report in the existing slot, TLS complete construction, direct-caller behaviour and D1–D4.
- Check the plain SockAddr static assertions, AF_UNSPEC sentinel, unreachable OS-address calls, no-CLI Remote part and input-only identity getFd.
- Verify dispatch-only writes and SIGPIPE, ownership rollback, factory/context lifetimes and callbacks under reentrancy.
- Verify the retained-stdout four-step MCP bootstrap, conditional shared-description refusal, quiet tty acceptance and silent refusal. Prove initial refusal and bootstrap daemon validation retain normal start cleanup. Console re-enable between init/start with shared output is explicitly accepted; no final/preparse callback or new global setter is proposed.
- Check MCP idle timeouts, hard output cap versus watermark semantics, counter scope, bounded framing and EOF-first/server shutdown.
- Audit each estimate against proposed file responsibilities, source API migration and stable public-boundary tests. Separate compile-only hardware coverage from runtime execution.

Return **ready for a bounded implementation plan**, **requires proposal revision**, or **blocked by a source API/lifecycle conflict**. Review approval does not authorize stdio implementation, branch creation or publishing.

## 17. Source evidence inventory

All baseline references were inspected at **e81147c4c**. Source inspection and grep results support the existing-code claims; document links/anchors, whitespace and scope are checked separately. Proposed stdio behaviour, extensions and tests remain **UNVERIFIED**; executed Stage 0 results are reported separately. E82–E86 add the exact release-hook insertion/lifetime and forwarding precedents. E81 retains final-review cleanup evidence; E74–E80 retain follow-up evidence. Refresh anchors if sources change before implementation.

| ID | Source | Verified observation |
| --- | --- | --- |
| E01 | [src/core/socket/stream/SocketContextFactory.h:69](../../src/core/socket/stream/SocketContextFactory.h#L69) | Original factory signature |
| E02 | [src/core/socket/stream/SocketConnection.cpp:87](../../src/core/socket/stream/SocketConnection.cpp#L87) | Factory invocation and original attachment path |
| E03 | [src/core/socket/stream/SocketContext.h:73](../../src/core/socket/stream/SocketContext.h#L73) | Original context constructor and delegated operations |
| E04 | [src/core/socket/stream/SocketContext.cpp:209](../../src/core/socket/stream/SocketContext.cpp#L209) | attach calls onConnected |
| E05 | [src/core/socket/stream/SocketConnection.h:170](../../src/core/socket/stream/SocketConnection.h#L170) | Four-way connection composition |
| E06 | [src/core/socket/stream/SocketConnection.hpp:353](../../src/core/socket/stream/SocketConnection.hpp#L353), [427](../../src/core/socket/stream/SocketConnection.hpp#L427) | Context switching and final teardown |
| E07 | [src/core/socket/stream/SocketReader.h:65](../../src/core/socket/stream/SocketReader.h#L65), [89](../../src/core/socket/stream/SocketReader.h#L89) | Buffered ReadEventReceiver and virtual read hook |
| E08 | [src/core/socket/stream/SocketWriter.h:69](../../src/core/socket/stream/SocketWriter.h#L69), [96](../../src/core/socket/stream/SocketWriter.h#L96) | Buffered WriteEventReceiver and virtual write hook |
| E09 | [src/core/socket/Socket.h:55](../../src/core/socket/Socket.h#L55) | Shared configuration wrapper |
| E10 | [src/core/socket/stream/SocketServer.h:86](../../src/core/socket/stream/SocketServer.h#L86), [301](../../src/core/socket/stream/SocketServer.h#L301) | Shared context and callback accessors |
| E11 | [src/net/in/stream/tls/SocketServer.h:67](../../src/net/in/stream/tls/SocketServer.h#L67) | Public transport binding pattern |
| E12 | [src/net/config/ConfigInstance.h:65](../../src/net/config/ConfigInstance.h#L65), [ConfigInstance.cpp:58](../../src/net/config/ConfigInstance.cpp#L58) | Instance role and current two-role mapping/help |
| E13 | [src/net/config/ConfigConnection.h:61](../../src/net/config/ConfigConnection.h#L61), [ConfigSection.h:61](../../src/net/config/ConfigSection.h#L61) | Existing policy sections and types |
| E14 | [src/net/SocketAddress.hpp:53](../../src/net/SocketAddress.hpp#L53), [82](../../src/net/SocketAddress.hpp#L82) | memset, header cast, raw-address and family access |
| E15 | [src/net/config/ConfigAddressBase.hpp:61](../../src/net/config/ConfigAddressBase.hpp#L61), [ConfigAddress.hpp:70](../../src/net/config/ConfigAddress.hpp#L70) | Raw-value reconstruction and cached configured address |
| E16 | [src/net/un/SocketAddress.h:58](../../src/net/un/SocketAddress.h#L58), [SocketAddress.cpp:96](../../src/net/un/SocketAddress.cpp#L96) | Address wrapper and presentation precedent |
| E17 | [src/core/socket/stream/SocketConnection.hpp:86](../../src/core/socket/stream/SocketConnection.hpp#L86), [108](../../src/core/socket/stream/SocketConnection.hpp#L108) | Typed physical address-discovery interface |
| E18 | [src/core/socket/stream/FlowController.hpp:172](../../src/core/socket/stream/FlowController.hpp#L172), [src/core/EventReceiver.cpp:50](../../src/core/EventReceiver.cpp#L50) | Existing next-tick startup scheduling |
| E19 | [src/core/socket/stream/legacy/SocketAcceptor.hpp:63](../../src/core/socket/stream/legacy/SocketAcceptor.hpp#L63) | onConnected followed by factory attachment |
| E20 | [src/apps/http/httpserver.cpp:62](../../src/apps/http/httpserver.cpp#L62), [src/apps/echo/echoserver.cpp:78](../../src/apps/echo/echoserver.cpp#L78) | Pre-init configuration and normal activation/status pattern |
| E21 | [src/net/un/config/ConfigAddress.h:61](../../src/net/un/config/ConfigAddress.h#L61), [ConfigAddress.cpp:70](../../src/net/un/config/ConfigAddress.cpp#L70) | Typed address section and reverse adapter precedent |
| E22 | [src/log/LogScopeOwner.h:51](../../src/log/LogScopeOwner.h#L51) | Optional logging role already supported |
| E23 | [src/core/socket/stream/SocketConnection.cpp:75](../../src/core/socket/stream/SocketConnection.cpp#L75) | Current non-server-to-client assumption |
| E24 | [src/net/phy/PhysicalSocket.hpp:62](../../src/net/phy/PhysicalSocket.hpp#L62), [80](../../src/net/phy/PhysicalSocket.hpp#L80) | Existing physical implementation performs OS socket operations |
| E25 | [src/core/system/unistd.cpp:62](../../src/core/system/unistd.cpp#L62), [src/core/EventLoop.cpp:293](../../src/core/EventLoop.cpp#L293) | Existing read/write wrappers and event-loop SIGPIPE handling |
| E26 | [src/core/socket/stream/SocketConnection.hpp:339](../../src/core/socket/stream/SocketConnection.hpp#L339) | Shared once-only framework shutdown |
| E27 | [src/core/socket/stream/SocketContext.cpp:194](../../src/core/socket/stream/SocketContext.cpp#L194), [SocketWriter.cpp:263](../../src/core/socket/stream/SocketWriter.cpp#L263) | EOF response and existing delayed write shutdown |
| E28 | [docs/design/stream-shutdown-lifecycle.md](stream-shutdown-lifecycle.md) | Existing bounded shutdown and scheduling constraints |
| E29 | [src/core/DescriptorEventReceiver.cpp:140](../../src/core/DescriptorEventReceiver.cpp#L140), [226](../../src/core/DescriptorEventReceiver.cpp#L226) | Disable request and later observation decrement |
| E30 | [src/core/DescriptorEventPublisher.cpp:107](../../src/core/DescriptorEventPublisher.cpp#L107), [127](../../src/core/DescriptorEventPublisher.cpp#L127) | Deferred removal and backend interest cleanup |
| E31 | [src/core/DescriptorEventReceiver.h:81](../../src/core/DescriptorEventReceiver.h#L81), [DescriptorEventReceiver.cpp:66](../../src/core/DescriptorEventReceiver.cpp#L66) | Shared virtual Observer and final unobserved callback |
| E32 | [src/core/socket/stream/SocketConnection.hpp:175](../../src/core/socket/stream/SocketConnection.hpp#L175) | Registration and delete-this failure branches |
| E33 | `src/mcp/server/stdio/Transport.cpp:60` (baseline `e81147c4c`), `src/mcp/server/stdio/Transport.cpp:102` (baseline `e81147c4c`) | Existing zero-timeout MCP receivers |
| E34 | [src/net/CMakeLists.txt:115](../../src/net/CMakeLists.txt#L115) | Connection read/write timeout defaults |
| E35 | [src/core/socket/stream/SocketWriter.cpp:156](../../src/core/socket/stream/SocketWriter.cpp#L156) | Bytes are queued before watermark return value |
| E36 | `src/mcp/server/stdio/Transport.cpp:150` (baseline `e81147c4c`) | Existing hard queued-output check |
| E37 | [src/core/socket/stream/SocketWriter.cpp:232](../../src/core/socket/stream/SocketWriter.cpp#L232), [src/net/config/ConfigConnection.cpp:86](../../src/net/config/ConfigConnection.cpp#L86) | Watermark capacity semantics |
| E38 | [src/core/socket/stream/SocketContext.cpp:153](../../src/core/socket/stream/SocketContext.cpp#L153) | Context versus connection counter scopes |
| E39 | [src/core/socket/stream/SocketConnection.hpp:353](../../src/core/socket/stream/SocketConnection.hpp#L353) | Zero receive consumption closes connection |
| E40 | `src/mcp/server/stdio/Transport.cpp:277` (baseline `e81147c4c`), `src/mcp/server/stdio/Transport.cpp:288` (baseline `e81147c4c`) | Existing stdout preservation and process-owning run |
| E41 | [src/utils/Config.cpp:702](../../src/utils/Config.cpp#L702), [785](../../src/utils/Config.cpp#L785) | Console logging default and application |
| E42 | [src/utils/Daemon.cpp:148](../../src/utils/Daemon.cpp#L148) | Daemon startup closes/replaces standard descriptors |
| E43 | [src/core/socket/stream/CMakeLists.txt:71](../../src/core/socket/stream/CMakeLists.txt#L71), [src/net/in/stream/CMakeLists.txt:66](../../src/net/in/stream/CMakeLists.txt#L66) | Target/alias/export/install conventions |
| E44 | [cmake/Packing.cmake:191](../../cmake/Packing.cmake#L191), [cmake/PackageConfig.cmake:4](../../cmake/PackageConfig.cmake#L4) | Package dependency and component inventory authority |
| E45 | [tests/installed-consumer/CMakeLists.txt:9](../../tests/installed-consumer/CMakeLists.txt#L9), [tests/CMakeLists.txt:57](../../tests/CMakeLists.txt#L57) | Staged component consumer and test |
| E46 | [tests/cmake/AddSNodeCTest.cmake:41](../../tests/cmake/AddSNodeCTest.cmake#L41) | Existing plain CTest helper |
| E48 | [src/core/socket/stream/SocketConnection.hpp:263](../../src/core/socket/stream/SocketConnection.hpp#L263), [276](../../src/core/socket/stream/SocketConnection.hpp#L276), [323](../../src/core/socket/stream/SocketConnection.hpp#L323), [328](../../src/core/socket/stream/SocketConnection.hpp#L328), [335](../../src/core/socket/stream/SocketConnection.hpp#L335) | RD/WR physical calls, terminate timeouts, and physical shutdown before disabling callback |
| E49 | [src/core/EventMultiplexer.cpp:175](../../src/core/EventMultiplexer.cpp#L175), [src/core/multiplexer/epoll/DescriptorEventPublisher.cpp:98](../../src/core/multiplexer/epoll/DescriptorEventPublisher.cpp#L98), [124](../../src/core/multiplexer/epoll/DescriptorEventPublisher.cpp#L124), [130](../../src/core/multiplexer/epoll/DescriptorEventPublisher.cpp#L130), [220](../../src/core/multiplexer/epoll/DescriptorEventPublisher.cpp#L220) | Deferred release entry, epoll receiver pointer, DEL/EBADF accounting and returned-pointer dispatch |
| E50 | [src/core/DescriptorEventPublisher.cpp:142](../../src/core/DescriptorEventPublisher.cpp#L142), [183](../../src/core/DescriptorEventPublisher.cpp#L183), [src/core/multiplexer/select/DescriptorEventPublisher.cpp:121](../../src/core/multiplexer/select/DescriptorEventPublisher.cpp#L121), [src/core/multiplexer/poll/DescriptorEventPublisher.cpp:88](../../src/core/multiplexer/poll/DescriptorEventPublisher.cpp#L88) | triggered handover, shutdown traversal, select/poll list iteration |
| E51 | [src/core/socket/stream/SocketAcceptor.hpp:253](../../src/core/socket/stream/SocketAcceptor.hpp#L253), [src/core/socket/stream/SocketConnector.hpp:209](../../src/core/socket/stream/SocketConnector.hpp#L209), [269](../../src/core/socket/stream/SocketConnector.hpp#L269) | All three production allocations and subsequent dereferences/callbacks |
| E52 | [src/core/socket/stream/tls/SocketConnection.hpp:71](../../src/core/socket/stream/tls/SocketConnection.hpp#L71), [78](../../src/core/socket/stream/tls/SocketConnection.hpp#L78) | TLS derived initialization/body continues after base construction |
| E53 | [src/core/multiplexer/select/DescriptorEventPublisher.cpp:90](../../src/core/multiplexer/select/DescriptorEventPublisher.cpp#L90), [src/core/multiplexer/epoll/DescriptorEventPublisher.cpp:87](../../src/core/multiplexer/epoll/DescriptorEventPublisher.cpp#L87), [102](../../src/core/multiplexer/epoll/DescriptorEventPublisher.cpp#L102) | Select high-fd rejection; epoll allocation failure and propagated ADD errors |
| E54 | [tests/unit/core/StreamFrameworkShutdownTest.cpp:272](../../tests/unit/core/StreamFrameworkShutdownTest.cpp#L272), [501](../../tests/unit/core/StreamFrameworkShutdownTest.cpp#L501), [521](../../tests/unit/core/StreamFrameworkShutdownTest.cpp#L521) | Direct derived fixture, allocation and following context attachment |
| E55 | [src/net/in/stream/legacy/SocketServer.h:95](../../src/net/in/stream/legacy/SocketServer.h#L95), [SocketServer.cpp:53](../../src/net/in/stream/legacy/SocketServer.cpp#L53), [src/net/in/stream/tls/SocketClient.h:95](../../src/net/in/stream/tls/SocketClient.h#L95), [SocketClient.cpp:53](../../src/net/in/stream/tls/SocketClient.cpp#L53) | Extern declarations and common-library wrapper/connection instantiations |
| E56 | [src/core/Descriptor.cpp:74](../../src/core/Descriptor.cpp#L74) | Single-fd owning destructor must not own a stdio pair |
| E57 | [src/core/socket/stream/SocketReader.cpp:87](../../src/core/socket/stream/SocketReader.cpp#L87), [SocketWriter.cpp:104](../../src/core/socket/stream/SocketWriter.cpp#L104), [tls/SocketWriter.cpp:82](../../src/core/socket/stream/tls/SocketWriter.cpp#L82) | Base recv/send hooks and TLS overriding precedent |
| E58 | [src/net/config/ConfigAddressRemote.h:54](../../src/net/config/ConfigAddressRemote.h#L54), [63](../../src/net/config/ConfigAddressRemote.h#L63), [ConfigAddress.hpp:54](../../src/net/config/ConfigAddress.hpp#L54), [ConfigAddressBase.hpp:51](../../src/net/config/ConfigAddressBase.hpp#L51), [ConfigAddressBase.h:73](../../src/net/config/ConfigAddressBase.h#L73), [ConfigSection.hpp:57](../../src/net/config/ConfigSection.hpp#L57) | Remote inherits constructors, bare address constructors register nothing, raw reconstruction contract; ConfigSection separately registers a section |
| E59 | [src/net/phy/PhysicalSocket.hpp:107](../../src/net/phy/PhysicalSocket.hpp#L107), [stream/PhysicalSocketClient.hpp:56](../../src/net/phy/stream/PhysicalSocketClient.hpp#L56), [src/net/un/dgram/Socket.cpp:74](../../src/net/un/dgram/Socket.cpp#L74), [src/net/un/phy/PhysicalSocket.hpp:115](../../src/net/un/phy/PhysicalSocket.hpp#L115) | Generic bind/connect accessor consumers, plus Unix-only datagram/probe consumers outside stdio's path |
| E60 | [src/net/SocketAddress.h:56](../../src/net/SocketAddress.h#L56), [59](../../src/net/SocketAddress.h#L59) | Existing raw-type wrapper/aliases; separate revision-scoped AF_UNSPEC grep found no matches |
| E61 | [src/core/socket/stream/SocketConnection.hpp:133](../../src/core/socket/stream/SocketConnection.hpp#L133), [176](../../src/core/socket/stream/SocketConnection.hpp#L176), [178](../../src/core/socket/stream/SocketConnection.hpp#L178), [206](../../src/core/socket/stream/SocketConnection.hpp#L206), [SocketConnection.cpp:70](../../src/core/socket/stream/SocketConnection.cpp#L70), [SocketAcceptor.hpp:185](../../src/core/socket/stream/SocketAcceptor.hpp#L185), [SocketConnector.hpp:196](../../src/core/socket/stream/SocketConnector.hpp#L196), [tls/SocketAcceptor.hpp:72](../../src/core/socket/stream/tls/SocketAcceptor.hpp#L72), [tls/SocketConnector.hpp:73](../../src/core/socket/stream/tls/SocketConnector.hpp#L73) | Complete generic connection getFd consumer/forwarder list and distinct acquisition/TLS uses |
| E62 | [src/core/socket/stream/SocketWriter.h:87](../../src/core/socket/stream/SocketWriter.h#L87), [SocketWriter.cpp:94](../../src/core/socket/stream/SocketWriter.cpp#L94), [108](../../src/core/socket/stream/SocketWriter.cpp#L108), [src/core/eventreceiver/WriteEventReceiver.cpp:71](../../src/core/eventreceiver/WriteEventReceiver.cpp#L71) | Private write dispatch and doWrite chain; no synchronous sendToPeer write |
| E63 | [src/core/EventLoop.cpp:183](../../src/core/EventLoop.cpp#L183), [293](../../src/core/EventLoop.cpp#L293) | SIGPIPE ignore installation during init and start |
| E64 | [src/utils/Config.cpp:1065](../../src/utils/Config.cpp#L1065), [src/utils/SubCommand.cpp:452](../../src/utils/SubCommand.cpp#L452) | Root options created inside init; ordinary option lookup |
| E65 | [src/utils/CLI11.hpp:5852](../../src/utils/CLI11.hpp#L5852), [6552](../../src/utils/CLI11.hpp#L6552) | Existing option validators; retained source evidence only, no MCP console validator proposed |
| E66 | [src/utils/Config.cpp:721](../../src/utils/Config.cpp#L721), [756](../../src/utils/Config.cpp#L756), [825](../../src/utils/Config.cpp#L825), [836](../../src/utils/Config.cpp#L836), [1074](../../src/utils/Config.cpp#L1074), [src/core/EventLoop.cpp:316](../../src/core/EventLoop.cpp#L316) | Real daemon option/default, initial parse and bootstrap reparse/daemon action before RUNNING |
| E67 | [src/core/SNodeC.cpp:60](../../src/core/SNodeC.cpp#L60) | start returns EventLoop::start result |
| E68 | [src/core/socket/stream/SocketConnection.h:247](../../src/core/socket/stream/SocketConnection.h#L247), [249](../../src/core/socket/stream/SocketConnection.h#L249) | Physical member and existing onDisconnect storage |
| E69 | [src/net/phy/stream/PhysicalSocket.hpp:51](../../src/net/phy/stream/PhysicalSocket.hpp#L51), [src/core/socket/stream/legacy/SocketConnection.hpp:54](../../src/core/socket/stream/legacy/SocketConnection.hpp#L54) | Network shutdown syscall and plaintext wrapper's forwarding constructor |
| E70 | [src/net/in/SocketAddrInfo.cpp:86](../../src/net/in/SocketAddrInfo.cpp#L86), [src/net/in6/SocketAddrInfo.cpp:78](../../src/net/in6/SocketAddrInfo.cpp#L78) | Distinct concrete getSockAddr accessors, not net::SocketAddress's raw wrapper accessor |
| E71 | [src/apps/mcp/main.cpp:33](../../src/apps/mcp/main.cpp#L33) | Existing pre-init application configuration creation |
| E72 | [src/utils/Config.cpp:1052](../../src/utils/Config.cpp#L1052) | Direct stdout diagnostics not routed through console logging |
| E73 | [src/core/socket/stream/CMakeLists.txt:117](../../src/core/socket/stream/CMakeLists.txt#L117) | Existing stream specialization subdirectories |
| E74 | `src/mcp/server/stdio/Transport.cpp:278` (baseline `e81147c4c`), `src/mcp/server/stdio/Transport.cpp:280` (baseline `e81147c4c`) | Existing F_DUPFD_CLOEXEC retention and stdout-to-stderr redirection; new endpoint owns later nonblocking setup |
| E75 | [src/utils/Config.cpp:705](../../src/utils/Config.cpp#L705), [789](../../src/utils/Config.cpp#L789), [src/log/detail/SpdlogBackend.cpp:200](../../src/log/detail/SpdlogBackend.cpp#L200), [538](../../src/log/detail/SpdlogBackend.cpp#L538) | Real quiet option/application and console-suppression condition; file logging is separate |
| E76 | [src/core/socket/stream/legacy/SocketConnection.h:77](../../src/core/socket/stream/legacy/SocketConnection.h#L77), [82](../../src/core/socket/stream/legacy/SocketConnection.h#L82), [85](../../src/core/socket/stream/legacy/SocketConnection.h#L85), [src/core/socket/stream/SocketConnection.h:190](../../src/core/socket/stream/SocketConnection.h#L190) | Public leaf construction, acceptor/connector friendships and protected common constructor; proposed direct-caller behaviour needs an explicit decision |
| E77 | [src/core/socket/stream/SocketConnection.hpp:443](../../src/core/socket/stream/SocketConnection.hpp#L443), [445](../../src/core/socket/stream/SocketConnection.hpp#L445), [447](../../src/core/socket/stream/SocketConnection.hpp#L447) | Stored callback precedes cleanup log and deletion/physical destruction |
| E78 | [src/utils/SubCommand.h:161](../../src/utils/SubCommand.h#L161), [SubCommand.cpp:347](../../src/utils/SubCommand.cpp#L347) | Existing final callback API; not used by the revised MCP bootstrap |
| E79 | [src/utils/Config.cpp:900](../../src/utils/Config.cpp#L900), [925](../../src/utils/Config.cpp#L925), [937](../../src/utils/Config.cpp#L937), [945](../../src/utils/Config.cpp#L945) | Typed catches handle CLI errors and write diagnostics to retained fd-1 alias; no generic catch for an application exception |
| E80 | [src/core/socket/stream/SocketServer.h:86](../../src/core/socket/stream/SocketServer.h#L86), [115](../../src/core/socket/stream/SocketServer.h#L115), [181](../../src/core/socket/stream/SocketServer.h#L181), [301](../../src/core/socket/stream/SocketServer.h#L301), [src/core/socket/stream/SocketClient.h:91](../../src/core/socket/stream/SocketClient.h#L91), [120](../../src/core/socket/stream/SocketClient.h#L120), [187](../../src/core/socket/stream/SocketClient.h#L187), [337](../../src/core/socket/stream/SocketClient.h#L337) | Facade estimate reference: shared contexts/constructors, acquisition/flow block to omit, callback accessors to retain in style |
| E81 | [src/core/EventLoop.cpp:316](../../src/core/EventLoop.cpp#L316), [343](../../src/core/EventLoop.cpp#L343), [351](../../src/core/EventLoop.cpp#L351), [356](../../src/core/EventLoop.cpp#L356) | Unguarded bootstrap call; false result uses stopsig -2 then signal restoration and free |
| E82 | [src/core/DescriptorEventPublisher.cpp:127](../../src/core/DescriptorEventPublisher.cpp#L127), [132](../../src/core/DescriptorEventPublisher.cpp#L132), [137](../../src/core/DescriptorEventPublisher.cpp#L137), [142](../../src/core/DescriptorEventPublisher.cpp#L142), [150](../../src/core/DescriptorEventPublisher.cpp#L150) | Existing deferred receiver removal, empty-list DEL/erase, nonempty-list triggered handover, then setDisabled; proposed hook goes between branch end and setDisabled |
| E83 | [src/core/DescriptorEventReceiver.cpp:226](../../src/core/DescriptorEventReceiver.cpp#L226), [66](../../src/core/DescriptorEventReceiver.cpp#L66), [69](../../src/core/DescriptorEventReceiver.cpp#L69) | setDisabled decrements shared observation count; reaching zero invokes potentially deleting unobservedEvent |
| E84 | [src/core/DescriptorEventReceiver.h:123](../../src/core/DescriptorEventReceiver.h#L123), [131](../../src/core/DescriptorEventReceiver.h#L131), [150](../../src/core/DescriptorEventReceiver.h#L150) | Existing event-hook declarations and publisher friendship; new release hook is a proposal, not an existing API |
| E85 | [src/core/socket/stream/SocketConnection.hpp:58](../../src/core/socket/stream/SocketConnection.hpp#L58), [61](../../src/core/socket/stream/SocketConnection.hpp#L61), [69](../../src/core/socket/stream/SocketConnection.hpp#L69) | Existing detail helpers use compile-time requires detection with fallback; precedent for optional physical release dispatch |
| E86 | [src/core/socket/stream/SocketWriter.h:93](../../src/core/socket/stream/SocketWriter.h#L93), [src/core/socket/stream/SocketReader.h:81](../../src/core/socket/stream/SocketReader.h#L81), [src/core/socket/stream/SocketConnection.hpp:320](../../src/core/socket/stream/SocketConnection.hpp#L320) | Existing writer-to-connection virtual shutdown and reader callback structure; proposed directional release forwarding follows that layering |

## 18. Owner-authorized implementation notes

The later owner execution request authorizes Stages 1–5 on the feature branch, with budgets as guidance and no merge into master or publication. The baseline evidence above remains at its original revision; references in this section describe the implementation checkout.

- Preparation is performed inside `SocketConnectionT::activate`, after complete construction and before the first receiver publication (`src/core/socket/stream/SocketConnection.hpp:197–203`). This small ordering correction keeps one flag-snapshot authority and avoids preparing twice. Ownership is committed after both registrations succeed (:218–221), before facade callbacks/factory attachment, as required by the execution request. Factory failure after activation therefore releases owned handles through normal deferred teardown; it does not return ownership to the caller.
- An optional, nonthrowing failure callback extends the existing boolean activation operation (:197–217). First failure deletes and rolls back before reporting; second failure stores the report in the existing onDisconnect slot and disables the reader. Existing network callers omit the callback. The facade never dereferences a false activation result (`src/core/socket/stream/stdio/SocketEndpoint.hpp:63–85`). This implements the previously specified deferred failure report without another destruction mechanism.
- The shared role-to-log mapping is inline in `ConfigInstance::getLogRole` (`src/net/config/ConfigInstance.h:92–101`). An out-of-line method would require core-socket-stream to link back to net, violating the existing dependency direction. ENDPOINT has no logger role; server/client mappings are retained.
- TAKE's exclusive-use precondition remains mandatory. The existing publisher has no public per-fd interest query (`src/core/DescriptorEventPublisher.h:77–97`); adding one or a global descriptor reservation registry is excluded. Runtime diagnosis of a caller violating exclusivity is therefore not claimed (**UNVERIFIED**). No publisher/multiplexer implementation change beyond the D4 notification is introduced.
- The staged consumer exercises the installed `net-stdio-stream` component. This exposed missing public component-list entries; the stdio components are now registered in `src/CMakeLists.txt` alongside the existing transport components. GCC focused endpoint/ownership/registration/installed-consumer execution passed; final cross-toolchain and multiplexer results are recorded in the task report.

- MCP newline framing is now an ordinary `mcp::server::stream::SocketContext`/`SocketContextFactory`, initially shipped by the separate stream component at the prior gate and now part of `mcp-server`, with the source directory and namespace unchanged (`src/mcp/server/CMakeLists.txt:4–17,48–51`; section 20). There is no separate output string or polling timer. The connection's queued/sent counters are the output-budget authority (`src/mcp/server/stream/SocketContext.cpp:41–51`); eight framed messages per slice use a RUNNING-only queued continuation (:142–167). Input is consumed through the original context read contract. The fixture deliberately uses a one-byte write watermark to verify false-after-accept does not duplicate frames.
- The executable owns stdout retention and the initial daemon/shared-console checks; the generic endpoint never redirects process descriptors (`src/apps/mcp/main.cpp:46–61`). Bootstrap CLI failures retain the nonzero `SNodeC::start` result instead of being masked by an attachment rejection during STOPPING cleanup. The baseline start path returns `onExit(-stopsig)` (`src/core/EventLoop.cpp:358`), so bootstrap parse failure is exit status 2, not 254. Reparse daemon validation and quiet shared pipe/tty operation have executed in the focused MCP bootstrap test. The accepted shared-description/console reconfiguration residual remains unchanged. Historical `docs/mcp/API-PROPOSAL.md` is outside the authorized documentation edits and retains its old process-owning API examples.

- Stage 5 exposed two real gaps after the initial 340-test full runs passed. Stopping from facade `onConnect` still constructed a protocol; the facade now rechecks RUNNING before `onConnected` and factory attachment (`src/core/socket/stream/stdio/SocketEndpoint.hpp:72–76`). Requested `SNodeC::stop()` reached writer shutdown without a protocol notification, unlike the signal path, so an MCP subscription cancellation was lost. The first implementation used a connection-owned callback; the owner-reviewed A1 revision replaces it with the active context virtual hook, once under the existing shutdown guard (`src/core/socket/stream/SocketConnection.hpp:398–412`). MCP overrides `onShutdown` (`src/mcp/server/stream/SocketContext.cpp:240–242`); the factory remains unchanged. There is no new receiver, publisher change, timer or process lifecycle wrapper. Both regressions failed before the fix and passed afterward; the final full reruns also passed. This is the minimal added seam needed for requested shutdown, rather than reintroducing a private transport shutdown guard.


### Previous gate at d6b0eff29 (historical results)

The final revision passed full GCC and strict Clang+ASan suites, 341/341 each, and the requested poll/select subsets, 25/25 and 26/26. Every invocation used the agreed short TMPDIR and suites ran sequentially. The real idle test waited 61 seconds. These executed results qualify the previously unverified design scenarios only to the extent asserted by the named tests: descriptor ownership, deferred release/handover, fragmented duplex I/O, filter EOF-before-reply, descriptor-number reuse, queued attachment, failed activation/retry, context/factory reuse, bootstrap retention/daemon/quiet tty and requested shutdown. Hardware l2/rc execution and untested platforms remain **UNVERIFIED**.

Official SDK calls and Inspector checks passed for both protocol eras over stdio and HTTP. SDK execution included all three tools, exact header bytes, resource read, prompt get and cancellation. Native Codex stdio/HTTP and Claude Code HTTP calls returned the init declaration and the resource. Native prompt retrieval and cancellation were not exposed to the model; those client actions remain **UNVERIFIED**. The first Claude stdio invocation discovered the server but model execution failed authentication; a later retry was blocked by automatic approval review because the prior CLI had updated its own profile state. No profile was edited by implementation commands. The later owner explicitly approved Claude updating its own runtime state file during native runs; no other profile/configuration changes are authorized.

A standalone installed-consumer probe exposed an existing configuration-layer boundary outside this feature scope: `ConfigRoot::terminate` unconditionally applies `F_SETFL(O_NONBLOCK)` and drains fd 0 (`src/utils/Config.cpp:798–807`). A BORROW endpoint on fd 0 retained flags throughout connection teardown, but the later global cleanup changed the observed flags from 3074 to 2050, removing O_APPEND. The probe attached successfully and the loop returned zero; its flag assertion failed. The physical BORROW policy itself does not change flags (`src/net/stdio/phy/stream/PhysicalSocket.cpp:53,62–74,166–186`). No compensating endpoint hook or out-of-scope configuration edit is introduced. That historical failure is addressed by owner-approved C1: cleanup drains only terminals and restores their original flags (`src/utils/Config.cpp:798–816`). The new two-pipe `StdioEndpoint-borrow` regression first reproduced flags 3072→2048 and loss of 31 unread bytes, then passed after the fix. The later fresh installed probe passed with flags 3074 unchanged and exit zero; its final result is recorded below.

The owner execution request superseded the old planning-only bounds in section 14 with guidance: Stage 2 production +285/tests 200; Stage 3 production +600/tests 200; Stage 4 production +180/tests 230. Measured commits: Stage 2 production +240 and 194 test additions; Stage 3 +363 and 268 test additions; Stage 4 -6 and 224 test additions. The facade group is 221 lines. Stage 3 exceeds test guidance by 68 added lines to retain failed-first/second-registration retry, lifecycle/reentrancy, cross-transport protocol reuse and the installed runtime consumer. Stage 5's two regression fixes add 19 net production lines and 22 test lines. No release, tag, push or merge into master is performed; the document remained untracked at that gate.

The additional native Codex SIGINT probes did send an interrupt after observing tools/call, but each tool completed normally and no wire cancellation was observed. They are recorded as an incomplete cancellation qualification, not a pass. The final formatting-only commit changes one CMake argument wrap (two added lines, one removed); ownership checks passed afterward on all four builds.


## 19. Owner-reviewed corrections and qualification gate

B1 pairs every onConnect with teardown, even if onConnected closes or factory creation returns null/throws. Per-connection state is allocated by onConnect and erased only by onDisconnect in the regression (`tests/component/net/StdioEndpointTest.cpp:167–180`); all four new failure assertions failed before the production fix and the endpoint group passed afterward. B2 adds free() to post-init refusal paths, including the fixture, with exit/purity assertions and final strict-ASan qualification.

A1 makes shutdown a protocol hook on the active original context, rather than stored connection state; it preserves the once guard, pre-signal/pre-drain point and original factory. The core regression asserts active-context-only, exactly-once and pre-signal notification (`tests/unit/core/StreamFrameworkShutdownTest.cpp`). At the `fa8ef9600` gate, A2 moved the CMake authority to stream/ and introduced a separate stream component, updating live consumers and staging installed headers under mcp/server/stream. The subsequent owner decision folds that adapter into the protocol component instead (section 20). No stdio compatibility directory or target remains; SOVERSION stays 3. AGENTS.md is not edited; its historical component spelling is a recorded scope exception, not a compatibility alias.

S1 restores declaration/function spacing, removes packed declarations, factors three callback chains through one private helper (`src/core/socket/stream/stdio/SocketEndpoint.hpp:115–130`) and replaces the EOF receive("{") trick with State::parseError (`src/mcp/server/stream/SocketContext.cpp:81–91,182–187`). It passes a null request ID through the shared era serializer: 2026 omits unknown id, 2025 emits null (`src/mcp/server/Server.cpp:230–240`). Regression tests assert each exact wire form; no parallel envelope builder is added.

C1 changes only the authoritative configuration cleanup policy: non-terminal stdin is neither modified nor drained, terminal cleanup temporarily applies flags|O_NONBLOCK and restores flags. No endpoint compensation, new ownership flag or release path is introduced. The final GCC/strict-ASan, poll/select MCP/endpoint/registration checks, installed probe and native execution results are recorded below. Native cancellation is a manual owner check; SDK/Inspector era coverage is accepted. Claude prompt retrieval must execute through its advertised MCP slash-command form. A normal local merge, retaining feature/fix branches, is authorized only after the gates pass; no push/tag/public action is authorized.


### Final owner-review execution result

The six review fixes are committed individually: B1 899324029, B2 ea55e5ec9, A1 b7e7f6c41, A2 2c5d7551d, S1 af28b9a2c and C1 419f217c6. Full GCC and strict Clang+ASan CTest each passed 342/342; poll passed 47/47 and select 49/49, including all MCP tests and the installed consumer, with no failures or skips. Suites ran sequentially. The fresh installed BORROW probe passed with flags 3074 unchanged, and a tty cleanup probe preserved flags 1026 while draining terminal bytes. C1 is therefore resolved for the executed scenarios.

SDK and Inspector checks passed both eras/transports; SDK covered cancellation and a multiword prompt argument. Claude Code and Codex completed all three tools, header text and resource reads on both transports. Claude MCP slash commands issued prompts/get on both transports with exact code static_assert(true); and returned successful reviews. Claude 2.1.330 splits slash-command arguments at whitespace even inside quotes; this is a client input limitation, not server truncation (captured requests already contained only the first token). The final one-argument declaration qualified the native form without changing server parsing. Codex prompt retrieval is not supported by the tested client mode. Native cancellation remains the explicitly accepted manual owner check.

Only Claude's approved runtime state changed; the other sampled normal client settings remained unchanged. The feature was merged locally into master with normal merge commit fa8ef9600, retaining feature/fix branches and all history. Its tree equals the validated feature tree; the requested post-merge GCC CTest passed 342/342. There was no push, tag, public action or ABI check. MCP Config.cpp retained its original editor hash and this proposal remained untracked at that gate. AGENTS.md and historical MCP proposals retain old component/API spellings because those files were explicitly excluded from edits. Other platforms and hardware-specific l2/rc runtime execution remain UNVERIFIED.


## 20. MQTT-aligned protocol component decision

The owner replaces the separate MCP stream component with the established MQTT rule: the stream adapter belongs to the protocol library. MQTT lists its SocketContext in `mqtt` and links `core-socket-stream` publicly (`src/iot/mqtt/CMakeLists.txt:68–91`). MCP now lists `stream/SocketContext.{h,cpp}` and `stream/SocketContextFactory.{h,cpp}` directly in `mcp-server`, links `core-socket-stream` publicly and installs both headers under `snode.c/mcp/server/stream` as part of `mcp-server` (`src/mcp/server/CMakeLists.txt:4–17,48–51`). The source directory, namespace, original factory/context behavior and header include paths remain unchanged. There is no separate stream target, component, export or compatibility alias. The HTTP binding remains a separate web-dependent library (`src/mcp/server/CMakeLists.txt:61`; `src/mcp/server/http/CMakeLists.txt`). `SNODEC_MCP_SOVERSION` remains 3 (`CMakeLists.txt:50`).

The installed consumer requests and links `snodec::mcp-server`, rejects an obsolete separate stream target, and constructs the MCP stream factory using the installed header (`tests/installed-consumer/CMakeLists.txt:9–22`; `tests/installed-consumer/main.cpp:80–84`). Live applications/tests, package dependency inventory, README and companion skill use the protocol component. Historical component decisions above are retained as history, not current API guidance. The design record contains repository-relative source citations; no machine-specific paths or links to private local evidence are required.


The layout refactor is commit `9617538f1`, merged locally with normal merge commit `78223adb2`. Full GCC and strict Clang+ASan CTest each passed 342/342; the post-merge master GCC rerun also passed 342/342, with zero failures or skips and sequential suites using `TMPDIR=/tmp/snc`. The installed consumer passed in each full suite, and a component-only install verified that the protocol component owns both stream headers and its single export. Formatting and diff checks passed. Repository IWYU checking was enabled: no diagnostics were reported for the task files, while the unchanged `src/net/stdio/SocketAddress.h:27` still requests a direct `net/SocketAddress.hpp` include. That existing advisory is recorded rather than folded into an unrelated component refactor. No push, tag or public action accompanied this design record.
