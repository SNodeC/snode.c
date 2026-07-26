# Codex A1.3 commands, filesystem, reviews, and approvals

## Status and authority

Phase A1.3 completes the stable Codex App Server 0.144.6 commands,
filesystem, reviews, and approvals typed protocol surface. The audited base is
commit `304a817d371597fc764c3404c36cc880bf65536d`, tree
`d0c8f21a415139655d4e9d92bb6efd0892b6bf4c`. The pin remains upstream tag
`rust-v0.144.6`.

The frozen schema inventory, vendored Rust contracts, module/slice assignment,
fixture evidence, and `ProtocolSurfaceRegistryData.inc` are authoritative.
The A1.3 audit and closure reports are deterministic guards; they are not a
second runtime registry.

The denominator is exactly 68 identities:

| Category | Count |
|---|---:|
| Client requests | 17 |
| Server notifications | 7 |
| Server requests | 5 |
| Nested-union alternatives | 39 |
| **Total** | **68** |

The starting slice state is 0 Complete, 2 Partial, and 66 NotImplemented.
The two Partial identities are
`item/commandExecution/requestApproval` and
`item/fileChange/requestApproval`. The starting global state is 212 Complete,
6 Partial, 121 NotImplemented, and 48 NotApplicable. Completion targets 68
Complete in A1.3 and global 280 Complete, 4 Partial, 55 NotImplemented, and 48
NotApplicable.

After the review and guardian implementation batch, the final A1.3 state is 68
Complete, 0 Partial, and 0 NotImplemented. The corresponding global state is
280 Complete, 4 Partial, 55 NotImplemented, and 48 NotApplicable. This is the
exact mechanically derived B5 checkpoint. Both formerly Partial approval roots
and all 66 formerly NotImplemented A1.3 identities are Complete. The four
remaining Partial identities are `initialize`, `initialized`, `error`, and
`item/tool/requestUserInput`; none belongs to A1.3.

The transitive stable closure contains 59 root/union seeds, 123 named
definitions (97 v2 and 26 legacy), and 480 property, array-element, and
map-value paths. The generated evidence records required, optional, nullable,
default-bearing, integer-format, bound, container, additional-properties, and
opaque-value behavior.

## Exact roots

The 17 client requests are:

- one-off command execution: `command/exec`, `command/exec/resize`,
  `command/exec/terminate`, and `command/exec/write`;
- filesystem: `fs/copy`, `fs/createDirectory`, `fs/getMetadata`,
  `fs/readDirectory`, `fs/readFile`, `fs/remove`, `fs/unwatch`, `fs/watch`,
  and `fs/writeFile`;
- search and permissions: `fuzzyFileSearch` and `permissionProfile/list`;
- review and guardian: `review/start` and
  `thread/approveGuardianDeniedAction`.

Eight client requests have concrete results and nine have Unit results. The
five reverse-direction requests all have concrete response contracts:
`applyPatchApproval`, `execCommandApproval`,
`item/commandExecution/requestApproval`,
`item/fileChange/requestApproval`, and
`item/permissions/requestApproval`.

The seven notifications are `command/exec/outputDelta`, `fs/changed`,
`fuzzyFileSearch/sessionCompleted`, `fuzzyFileSearch/sessionUpdated`,
`guardianWarning`, `item/autoApprovalReview/completed`, and
`item/autoApprovalReview/started`.

The 39 union alternatives are the complete frozen alternatives of
`CommandExecutionApprovalDecision`, `FileChange`, `FileSystemPath`,
`FileSystemSpecialPath`, `GuardianApprovalReviewAction`, `ParsedCommand`,
`ReviewDecision`, and `ReviewTarget`. Registry `$variant` is inventory
notation for an untagged alternative and is not emitted as a JSON member.
The known literals `unknown` in `FileSystemSpecialPath` and `ParsedCommand`
remain distinct from a genuinely unknown future alternative.

## Public API and implementation batches

The typed public map is:

- `client.typed().commands()` with `exec`, `resize`, `terminate`, and `write`;
- `client.typed().filesystem()` with the nine filesystem operations and
  one-shot `fuzzyFileSearch`;
- `client.typed().permissionProfiles().list()`;
- `client.typed().reviews().start()`;
- `client.typed().threads().approveGuardianDeniedAction()`;
- the existing `events()` observer for all seven notifications; and
- the existing `requests()` occurrence/response mechanism for all five
  reverse requests.

Commit batches are frozen as one-off commands (5 identities),
filesystem/fuzzy search (13), approvals/permissions/file changes (35), and
reviews/guardian (15). Every identity, registry promotion, codec, descriptor,
dispatcher, public method, installed header, and primary test belongs to its
owning implementation batch.

`typed::Client` retains its one-pointer PIMPL representation. New facade
objects remain behind that PIMPL. Event and reverse-request variants preserve
the order of existing alternatives and append A1.3 alternatives. Appending
alternatives is source compatible for non-exhaustive users but changes the
layout and index space of those public `std::variant` types; the closure audit
must report the final object-layout evidence explicitly. The project ABI
policy, rather than symbol-list equivalence alone, decides whether a
SOVERSION change is warranted.

The established `TypedServerRequest` alternatives keep
indices 0 through 4 and the three newly typed legacy/permission requests are
appended at indices 5 through 7. `CommandApprovalRequest` and
`FileChangeApprovalRequest` retain their existing source-level field names but
gain schema-complete canonical parameters and diagnostics. Those aggregate
and variant layout changes are ABI changes: consumers using the installed
typed aggregates must rebuild. `typed::Client` and `AppServerClient` keep their
one-pointer and PIMPL object layouts, respectively. SOVERSION remains 1 under
the frozen A1 policy, which defers the milestone-wide bump decision to A1.4;
this document does not claim binary compatibility from an unchanged symbol
list.

The final B5 event additions follow the same append-only source policy.
`Event` keeps its previous alternatives at indices 0 through 49 and appends
`GuardianWarningNotification`,
`ItemGuardianApprovalReviewCompletedNotification`, and
`ItemGuardianApprovalReviewStartedNotification` at indices 50 through 52.
`CanonicalServerNotification` likewise keeps indices 0 through 47 and appends
the three notification payloads at indices 48 through 50. These public
variants therefore change size/layout and require an installed-consumer
rebuild; their preserved existing indices do not constitute binary
compatibility. The new `ReviewTarget` and
`GuardianApprovalReviewAction` variants each place a raw-preserving
future-unknown alternative after all known alternatives.

## Architecture and lifecycles

All operations use the existing path:

```
AppServerClient -> RawProtocol -> typed::Client -> typed facade/events/requests
```

There is one transport, JSONL engine, request-ID allocator, pending-operation
map, cancellation path, generation counter, notification dispatcher, and
pending-server-request registry.

Filesystem paths are carried as protocol strings without normalization,
canonicalization, separator rewriting, relative-path resolution, or local
access. File data remains in the stable base64-bearing string representation.
`fs/watch` and `fs/unwatch` preserve the supplied connection-scoped watch
identifier; `fs/changed` uses the existing event observer and does not create a
local watcher object or backend watcher state. The one-shot
`fuzzyFileSearch` preserves server ordering and scores. Its stable session
notifications are observable even though the three experimental session
control requests remain unavailable.

`command/exec` is connection-scoped one-off execution. It does not create a
thread, turn, or `ThreadItem`. Its output notification is distinct from A1.1
`item/commandExecution/outputDelta`. Incoming output is delivered in wire
order before the final request callback. Pending JSON-RPC cancellation is not
remote process termination; `command/exec/terminate` is the explicit remote
operation.

The five reverse requests use the existing occurrence token and connection
generation. A typed response is validated before enqueue, accepted at most
once, and written with the original JSON-RPC request id. Duplicate, stale,
wrong-type, and wrong-generation responses fail locally. All five types may
remain pending concurrently and may be answered out of arrival order without
cross-delivery.

The deprecated legacy `applyPatchApproval` and `execCommandApproval` roots
retain distinct request and response types from the v2 command, file-change,
and permission approvals. `permissionProfile/list` is a typed read operation;
its `allowed` field remains server data and does not authorize or select a
profile. The six B4 union families expose all 29 known alternatives, preserve
the literal `unknown` alternatives, and keep a separate future-unknown payload
path. Scope-broadening command decisions and execution/network policy
amendments are never reduced to a boolean.

`serverRequest/resolved` belongs to A1.4. It is excluded from A1.3 and is not a
response transport dependency. A1.3 responds directly through the existing
RawProtocol response path.

## Stable and experimental boundary

Stable fuzzy session notifications are typed, but the experimental
`sessionStart`, `sessionUpdate`, and `sessionStop` requests are not exposed.
No `process/*` operation, `item/tool/requestUserInput`,
`serverRequest/resolved`, MCP/plugin/marketplace/external-agent operation, or
other A1.4 surface is included. Stable `CommandExecParams` does not gain the
experimental `permissionProfile` member.

The library does not execute commands, access the host filesystem, apply
patches, perform fuzzy matching, enforce guardian policy, or reduce review
state. No A1.3 `BackendCommand`, `BackendState`, frontend message, application
operation, worker thread, or second protocol engine is introduced.

## Review and guardian behavior

`review/start` reuses the existing typed `Turn` in its response. Review
targets remain the exact `uncommittedChanges`, `baseBranch`, `commit`, and
`custom` alternatives. Review delivery is an open value with known `inline`
and `detached` values. Findings continue through the existing thread, turn,
item, and event surfaces.

Guardian actions remain explicit typed alternatives for `applyPatch`,
`command`, `execve`, `mcpToolCall`, `networkAccess`, and
`requestPermissions`. Opaque leaves remain JSON only where the stable schema
is deliberately opaque. In particular,
`thread/approveGuardianDeniedAction.event` is the schema's unconstrained
serialized guardian event; the surrounding thread-scoped request remains
typed and uses the existing Threads facade. Command and execve actions retain
their distinct command/program/argv, cwd, and source fields. Patch actions
retain ordered files. Network actions retain host, uint16 port, protocol, and
target. MCP actions preserve the three optional-null connector/tool labels.
Permission actions reuse the complete permission-profile model and preserve
the optional-null reason.

`guardianWarning`, `item/autoApprovalReview/started`, and
`item/autoApprovalReview/completed` use the existing Events observer. The two
review notifications preserve their action, review status/rationale/risk/user
authorization, review and target identities, int64 timestamps, and terminal
decision source. Incoming open values remain forward compatible. The library
makes no automatic guardian or approval decision.

The generated fixture evidence covers all 15 B5 identities: two operation
roots and results, three notification roots, all ten union alternatives,
review delivery omitted/null/inline/detached, opaque guardian-event JSON
shapes, empty and non-empty action arrays, uint16 network-port and int64
timestamp boundaries, every reviewed open-enum value, future values, and
malformed-known mutations. The complete deterministic corpus contains 2,268
positive fixtures after B5; its exact count remains generated evidence rather
than runtime authority.

## Compatibility and security plan

Incoming open enums preserve future values. Unknown future union alternatives
preserve their discriminator and raw payload with a forward-compatibility
diagnostic. A malformed known branch remains a malformed-known-payload
diagnostic and does not by itself disconnect the transport.

Arguments, environment values, process I/O, paths, file data, patches,
approval reasons, guardian details, policy amendments, permission-profile
data, and review instructions are sensitive. Production diagnostics identify
only the method/union, structural reason, and safe field path. Tests and
evidence use synthetic paths and values.

The B5 implementation checkpoint proves all 68 registry identities Complete,
every root/response and union alternative fixture-covered, and the exact final
global metrics above. Final closure additionally requires public headers
installed and self-contained, direct reverse-response and five-way concurrent
socket evidence, no experimental leakage, no frontend/backend semantic
expansion, package and consumer checks, API/ABI evidence, secret and
test-integrity guards, and the same four residual Partial identities.
