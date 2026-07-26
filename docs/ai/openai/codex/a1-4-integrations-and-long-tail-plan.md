# Codex A1.4 integrations, long tail, and final-A1 plan

## Scope and authority

This document freezes the audit and implementation plan for A1.4. It does not
mark A1.4 implemented or complete. The audit branch contains no production
implementation, public API, registry promotion, SOVERSION change, backend or
frontend expansion, application behavior change, descriptor regeneration, or
fixture change.

The verified base is commit
`bed5ee05184739d54cddc6518bd43b264e2b496d`, tree
`8bf44f0eed6bd8c9d5309176db07a7d6a6240ef3`, the merge of PR #222. The
pinned authority remains Codex CLI 0.144.6, upstream tag `rust-v0.144.6`,
source commit `5d1fbf26c43abc65a203928b2e31561cb039e06d`. Provenance and aggregate
hashes are frozen in `a1-4-start-state.json`.

Authority order is:

1. pinned stable schema and provenance;
2. vendored protocol source and operation contracts;
3. the unchanged module/slice assignment;
4. `ProtocolSurfaceRegistryData.inc`, the sole local production authority;
5. generated A1.4 reports, which are guards and plans rather than another
   runtime registry.

Experimental schemas are used only to prove the boundary. They are not
implementation authority for A1.

## Total partition and start state

The inventory, assignment, and registry key sets are an exact 387-key
bijection:

| Bucket | Identities |
|---|---:|
| A1.0 | 19 |
| A1.1 | 151 |
| A1.2 | 45 |
| A1.3 | 68 |
| A1.4 | 56 |
| InventoryOnly | 48 |
| **Total** | **387** |

The equivalent stability accounting is:

| Classification | Identities |
|---|---:|
| Stable identities assigned to A1.0–A1.4 | 339 |
| Stable-unreachable InventoryOnly | 12 |
| Experimental-only InventoryOnly | 36 |
| **Total stable inventory** | **351** |
| **Total inventory** | **387** |

InventoryOnly is not an experimental synonym. It contains 36
`ExperimentalInventoryOnly` rows and 12
`StableUnreachableInventory` rows.

The current global status is:

| Status | Count |
|---|---:|
| Complete | 280 |
| Partial | 4 |
| NotImplemented | 55 |
| NotApplicable | 48 |

The native A1.4 status is `0 Complete / 1 Partial / 55 NotImplemented`.
`item/tool/requestUserInput` is the one native Partial. The other three
Partials—`initialize`, `initialized`, and `error`—remain Common/A1.0-owned
and are scheduled as inherited final-A1 closure obligations.

Native A1.4 completion projects the global state to
`336 Complete / 3 Partial / 0 NotImplemented / 48 NotApplicable`. Completing
the inherited A1.0 rows then projects final A1 to
`339 Complete / 0 Partial / 0 NotImplemented / 48 NotApplicable`.

## Exact native A1.4 surface

Every native row is stable, assigned to module
`IntegrationsAndLongTail`, and assigned to slice A1.4. Its taxonomy is
29 client requests, 16 server notifications, four server requests, and seven
tagged-union alternatives.

### Client requests and result contracts

| Method | Parameters | Result | Result kind |
|---|---|---|---|
| `app/list` | `AppsListParams` | `AppsListResponse` | Concrete |
| `externalAgentConfig/detect` | `ExternalAgentConfigDetectParams` | `ExternalAgentConfigDetectResponse` | Concrete |
| `externalAgentConfig/import` | `ExternalAgentConfigImportParams` | `ExternalAgentConfigImportResponse` | Concrete |
| `externalAgentConfig/import/readHistories` | `Unit` | `ExternalAgentConfigImportHistoriesReadResponse` | Concrete |
| `feedback/upload` | `FeedbackUploadParams` | `FeedbackUploadResponse` | Concrete |
| `hooks/list` | `HooksListParams` | `HooksListResponse` | Concrete |
| `marketplace/add` | `MarketplaceAddParams` | `MarketplaceAddResponse` | Concrete |
| `marketplace/remove` | `MarketplaceRemoveParams` | `MarketplaceRemoveResponse` | Concrete |
| `marketplace/upgrade` | `MarketplaceUpgradeParams` | `MarketplaceUpgradeResponse` | Concrete |
| `mcpServer/oauth/login` | `McpServerOauthLoginParams` | `McpServerOauthLoginResponse` | Concrete |
| `mcpServer/resource/read` | `McpResourceReadParams` | `McpResourceReadResponse` | Concrete |
| `mcpServer/tool/call` | `McpServerToolCallParams` | `McpServerToolCallResponse` | Concrete |
| `mcpServerStatus/list` | `ListMcpServerStatusParams` | `ListMcpServerStatusResponse` | Concrete |
| `plugin/install` | `PluginInstallParams` | `PluginInstallResponse` | Concrete |
| `plugin/installed` | `PluginInstalledParams` | `PluginInstalledResponse` | Concrete |
| `plugin/list` | `PluginListParams` | `PluginListResponse` | Concrete |
| `plugin/read` | `PluginReadParams` | `PluginReadResponse` | Concrete |
| `plugin/share/checkout` | `PluginShareCheckoutParams` | `PluginShareCheckoutResponse` | Concrete |
| `plugin/share/delete` | `PluginShareDeleteParams` | `Unit` | Unit |
| `plugin/share/list` | `PluginShareListParams` | `PluginShareListResponse` | Concrete |
| `plugin/share/save` | `PluginShareSaveParams` | `PluginShareSaveResponse` | Concrete |
| `plugin/share/updateTargets` | `PluginShareUpdateTargetsParams` | `PluginShareUpdateTargetsResponse` | Concrete |
| `plugin/skill/read` | `PluginSkillReadParams` | `PluginSkillReadResponse` | Concrete |
| `plugin/uninstall` | `PluginUninstallParams` | `Unit` | Unit |
| `skills/config/write` | `SkillsConfigWriteParams` | `SkillsConfigWriteResponse` | Concrete |
| `skills/extraRoots/set` | `SkillsExtraRootsSetParams` | `Unit` | Unit |
| `skills/list` | `SkillsListParams` | `SkillsListResponse` | Concrete |
| `windowsSandbox/readiness` | `Unit` | `WindowsSandboxReadinessResponse` | Concrete |
| `windowsSandbox/setupStart` | `WindowsSandboxSetupStartParams` | `WindowsSandboxSetupStartResponse` | Concrete |

The result split is exactly 26 Concrete and three Unit. The Unit-result
operations are only `plugin/share/delete`, `plugin/uninstall`, and
`skills/extraRoots/set`.

### Server notifications

The exact 16 notifications are:

```text
app/list/updated
deprecationNotice
externalAgentConfig/import/completed
externalAgentConfig/import/progress
hook/completed
hook/started
mcpServer/oauthLogin/completed
mcpServer/startupStatus/updated
process/exited
process/outputDelta
remoteControl/status/changed
serverRequest/resolved
skills/changed
warning
windows/worldWritableWarning
windowsSandbox/setupCompleted
```

`process/exited` and `process/outputDelta` are stable incoming notifications.
The outgoing `process/spawn`, `process/kill`, `process/writeStdin`, and
`process/resizePty` methods remain experimental-only InventoryOnly.
`remoteControl/status/changed` is stable; all outgoing remote-control methods
remain experimental-only. Stable observation does not promote experimental
control.

### Server requests and direct responses

| Method | Parameters | Response |
|---|---|---|
| `attestation/generate` | `AttestationGenerateParams` | `AttestationGenerateResponse` |
| `item/tool/call` | `DynamicToolCallParams` | `DynamicToolCallResponse` |
| `item/tool/requestUserInput` | `ToolRequestUserInputParams` | `ToolRequestUserInputResponse` |
| `mcpServer/elicitation/request` | `McpServerElicitationRequestParams` | `McpServerElicitationRequestResponse` |

All four have concrete responses and remain on the existing Requests
occurrence machinery. The frozen path is:

```text
incoming server request
  -> existing occurrence token and transport generation
  -> typed callback
  -> typed response validation
  -> direct JSON-RPC response with the original request ID
```

### Tagged unions

`McpServerElicitationRequestParams` is a Draft-07 object plus `oneOf`,
discriminated by `mode`. Outer `serverName` and `threadId` are required;
outer `turnId` is optional and nullable. All outer and branch objects allow
future properties.

| Alternative | Required branch fields | Optional/nullable fields |
|---|---|---|
| `form` | `mode`, `message`, `requestedSchema` | `_meta` optional/nullable/opaque |
| `openai/form` | `mode`, `message`, `requestedSchema` | `_meta` optional/nullable/opaque; `requestedSchema` nullable/opaque |
| `url` | `mode`, `message`, `elicitationId`, `url` | `_meta` optional/nullable/opaque |

Its only reaching root is `mcpServer/elicitation/request`, and the MCP/reverse
request batch owns it.

`PluginSource` is a Draft-07 object plus `oneOf`, discriminated by `type`.
All branches allow future properties.

| Alternative | Required fields | Optional nullable fields |
|---|---|---|
| `local` | `type`, `path` | none |
| `git` | `type`, `url` | `path`, `refName`, `sha` |
| `npm` | `type`, `package` | `registry`, `version` |
| `remote` | `type` | none |

It is reached only by `plugin/installed`, `plugin/list`, `plugin/read`, and
`plugin/share/list`; the user-integrations batch owns it.

Both union families preserve a raw future-unknown alternative plus a
diagnostic. A known discriminator with missing or wrong-typed required fields
is malformed-known and must not be coerced to unknown.

## Stable type closure

The shared schema catalog, definition graph, Draft-07 handling, and schema
walker derive 81 seed definitions and 189 reachable definitions (34 legacy,
155 v2). The frozen closure contains:

- 646 value paths: 548 properties, 91 array elements, seven map values;
- 288 required and 260 optional property paths;
- 243 nullable and 22 default-bearing paths;
- 24 intentionally opaque JSON paths;
- 162 object-policy nodes: 143 open, 12 closed, seven schema-valued maps;
- 11 minimum-bearing paths and no maximum-bearing path;
- integer/number formats: three `double`, one `int32`, eight `int64`, one
  `uint`, four `uint32`, and six `uint64`;
- 127 mechanically flagged sensitive paths.

`a1-4-type-closure.json` records every definition, edge, field state, array
element, map value, numeric constraint, object policy, union branch, and
reaching root. The 127 sensitive-path heuristic is a minimum rather than an
allowlist.

## Cross-slice completion ledger

| Identity | Owner | Current status | Completion stage |
|---|---|---|---|
| `initialize` | Common / A1.0 | Partial | final A1 closure in A1.4 |
| `initialized` | Common / A1.0 | Partial | final A1 closure in A1.4 |
| `error` | Common / A1.0 | Partial | final A1 closure in A1.4 |
| `item/tool/requestUserInput` | IntegrationsAndLongTail / A1.4 | Partial | native A1.4 implementation |

All four rows currently lack the same seven schema-completeness predicates:
schema-property coverage, nullable semantics, reachable-union coverage,
direction assertions, registry/decoder agreement, opaque-field declaration,
and proof that no known field is dropped.

`initialize` must preserve automatic initialization and existing constructors
while representing optional-null `clientInfo.title`, optional-null
capabilities, capability defaults, form elicitation support, optional-null
notification opt-outs, and attestation request support. `initialized` has no
payload and needs exact internal direction/encoder evidence, not a facade.
`error` preserves `TurnErrorEvent` at Event index 44 while adding a complete
canonical error view without changing backend/frontend behavior.

`item/tool/requestUserInput` preserves its existing public compatibility types,
response overload, direct response path, and `TypedServerRequest` index 2.
Completion adds canonical parameters/response and diagnostics, distinguishes
omitted/null/value `autoResolutionMs` and options, and exercises all response
fields and one-shot lifecycle behavior.

The assignment file remains unchanged. The three inherited rows are never
counted inside the native 56 denominator.

## `serverRequest/resolved` semantics

The stable payload is
`ServerRequestResolvedNotification{threadId: string, requestId: string|int64}`.
Both fields are required and non-nullable; future properties are allowed.
There is no outcome, method, item ID, result, error, generation, or completion
status. `requestId` is the original server-generated JSON-RPC request ID, not
an item ID or local occurrence token. String IDs must still be preserved.

The pinned production method matrix is:

| Slice | Server request | Emits resolved |
|---|---|---:|
| A1.3 | `item/commandExecution/requestApproval` | yes |
| A1.3 | `item/fileChange/requestApproval` | yes |
| A1.3 | `item/permissions/requestApproval` | yes |
| A1.3 | `applyPatchApproval` | no |
| A1.3 | `execCommandApproval` | no |
| A1.4 | `item/tool/requestUserInput` | yes |
| A1.4 | `mcpServer/elicitation/request` | yes |
| A1.4 | `item/tool/call` | no |
| A1.4 | `attestation/generate` | no |

For the five positive methods, normal results, domain decline/cancel results,
JSON-RPC errors, malformed successful results, and turn-transition cleanup can
emit it. There is no generic timeout or server-owned `autoResolutionMs` timer,
and disconnect alone does not emit it.

The direct JSON-RPC response is causally earlier: App Server removes the
callback and wakes the handler before that handler queues the lifecycle
notification. The notification is informational and lifecycle-significant for
shared pending UI, but it does not report the operation outcome. A responding
SNode.C client normally has already removed its local occurrence when the
notification arrives; another subscriber can still have the shared upstream
ID pending.

The payload itself is not generation-tagged. SNode.C must scope it to the
transport generation that delivered it. A matching current ID/thread may
retire an occurrence idempotently as externally resolved. Unknown,
already-consumed, duplicate, stale-generation, or thread-mismatched IDs are
nonfatal no-ops for registry state, while the typed event is still delivered.
They never reopen a request or cause wire output.

Typing this event never makes it a response transport, response prerequisite,
acknowledgement prerequisite, second terminal completion, or retroactive
change to the A1.3 contract. Future lifecycle tests cover both ID forms, the
method matrix, all terminal paths, response-before-event ordering, absence of
the event, already-consumed and external-resolution cases, duplicates,
thread/generation mismatch, reconnect ID reuse, and multi-subscriber behavior.
Exact pinned source links and line anchors are recorded in
`a1-4-implementation-plan.json`.

## InventoryOnly hard boundary

The 36 experimental-only identities are:

```text
collaborationMode/list
environment/add
environment/info
fuzzyFileSearch/sessionStart
fuzzyFileSearch/sessionStop
fuzzyFileSearch/sessionUpdate
memory/reset
mock/experimentalMethod
process/kill
process/resizePty
process/spawn
process/writeStdin
remoteControl/client/list
remoteControl/client/revoke
remoteControl/disable
remoteControl/enable
remoteControl/pairing/start
remoteControl/pairing/status
remoteControl/status/read
thread/backgroundTerminals/clean
thread/backgroundTerminals/list
thread/backgroundTerminals/terminate
thread/decrement_elicitation
thread/increment_elicitation
thread/items/list
thread/memoryMode/set
thread/realtime/appendAudio
thread/realtime/appendSpeech
thread/realtime/appendText
thread/realtime/listVoices
thread/realtime/start
thread/realtime/stop
thread/search
thread/settings/update
thread/turns/list
currentTime/read
```

The 12 stable-but-unreachable union alternatives are:

```text
CapabilityRootLocation:type:environment
ConfiguredHookHandler:type:agent
ConfiguredHookHandler:type:command
ConfiguredHookHandler:type:prompt
DynamicToolNamespaceTool:type:function
DynamicToolSpec:type:function
DynamicToolSpec:type:namespace
MultiAgentMode:$variant:custom
MultiAgentMode:$variant:explicitRequestOnly
MultiAgentMode:$variant:proactive
ThreadRealtimeStartTransport:type:webrtc
ThreadRealtimeStartTransport:type:websocket
```

All 48 remain NotApplicable for A1. They receive no public API, production
target, implementation batch, registry promotion, or status change.

## Recommended implementation sequence

Three native implementation PRs are the smallest reviewable split. One PR
mixes nine facade domains, reverse-request lifecycle, high-volume process
events, platform behavior, and both union families. Two PRs still leave one
review spanning unrelated API and lifecycle risks. More than three fragments
cohesive schemas without another honest closure boundary.

### PR A: user-facing integrations

- Branch: `codex/a1-4-user-integrations`
- Title: `Type the Codex A1.4 user-facing integrations`
- 33 identities: 23 client requests, six notifications, four `PluginSource`
  alternatives
- Result split: 20 Concrete, three Unit
- Native arithmetic: `0/1/55 -> 33/1/22`
- Global arithmetic: `280/4/55/48 -> 313/4/22/48`
- Closure: 52 seeds, 118 v2 definitions, 411 paths

It owns the app, external-agent, feedback, hooks, marketplace, plugins, and
skills requests; their six matching notifications; and `PluginSource`.
Proposed headers are `Apps.h`, `ExternalAgents.h`, `Feedback.h`, `Hooks.h`,
`Marketplace.h`, `Plugins.h`, and `Skills.h`.

### PR B: MCP and reverse requests

- Branch: `codex/a1-4-mcp-reverse-requests`
- Title: `Type the Codex A1.4 MCP and reverse requests`
- 13 identities: four client requests, two notifications, four server
  requests, three elicitation alternatives
- Native arithmetic: `33/1/22 -> 46/0/10`
- Global arithmetic: `313/4/22/48 -> 326/3/10/48`
- Closure: 18 seeds, 55 definitions (34 legacy, 21 v2), 204 paths

It owns all `mcpServer/*` and status operations in A1.4, all four incoming
requests, and the elicitation union. `UserInputRequest` stays request-variant
index 2; append Attestation at 8, DynamicTool at 9, and MCP Elicitation at 10.
This PR adds `Mcp.h` and extends existing Events/Requests without replacing the
occurrence lifecycle.

### PR C: runtime and platform long tail

- Branch: `codex/a1-4-runtime-platform`
- Title: `Type the Codex A1.4 runtime and platform long tail`
- 10 identities: two client requests and eight notifications
- Native arithmetic: `46/0/10 -> 56/0/0`
- Global arithmetic: `326/3/10/48 -> 336/3/0/48`
- Closure: 11 seeds, 17 v2 definitions, 31 paths

It owns both Windows sandbox requests plus deprecation, warning, process,
remote status, resolved-request, and Windows notifications. It follows PR B
because resolved-request lifecycle tests need every A1.3/A1.4 request kind.
It adds `WindowsSandbox.h`; it does not add process or remote-control outgoing
facades.

The only cross-batch schema dependency is existing
`v2::AbsolutePathBuf`, primarily accounted for in PR A and reused in PR C.
The union families are each owned once.

### Final A1 closure

A distinct `codex/a1-final-closure` PR, titled
`Close the Codex A1 typed surface and bump its SOVERSION`, completes
`initialize`, `initialized`, and `error` in Common/A1.0. Its arithmetic is
`336/3/0/48 -> 339/0/0/48`; native A1.4 remains 56 Complete.

Production completion commits precede a pure final closure commit. The pure
closure commit contains no deferred production implementation.

## Public API, ABI, and SOVERSION

The proposed facade ownership is:

- `client.typed().apps().list()`;
- external agents: detect, import, and read import histories;
- feedback: upload;
- hooks: list;
- marketplace: add, remove, and upgrade;
- MCP: OAuth login, resource read, tool call, and server-status list;
- plugins: install, installed, list, read, checkout/delete/list/save/update
  shares, read skill, and uninstall;
- skills: write config, set extra roots, and list;
- Windows sandbox: readiness and setup start.

Every facade remains behind `typed::Client`'s one-pointer PIMPL. No member is
added to `AppServerClient`; no public virtual interface or generic
invoke-by-method API is introduced. Events remain on `Events`, and incoming
requests remain on `Requests`.

Exact existing reuse is limited to `AbsolutePathBuf`,
`DynamicToolCallOutputContentItem`, and `WindowsSandboxSetupMode`, plus shared
infrastructure vocabulary such as `Unit`, `OperationResult`, IDs, JSON,
nullable state, diagnostics, and request tokens. Similar command-process,
warning, conversation-input, MCP thread-item, account-authentication,
configuration-source, hook-source, and sandbox-policy types are not schema
substitutes.

Current ABI-sensitive variants have 51 canonical notification alternatives,
53 Event alternatives, and eight typed server-request alternatives. Existing
indices remain fixed. PR A appends canonical indices 51–56 and Event indices
53–58; PR B appends 57–58 and 59–60; PR C appends 59–66 and 61–68.
Final `ErrorNotification` is canonical index 67 while `TurnErrorEvent` remains
Event index 44.

The root project is version 1.0.1 and
`SNODEC_SOVERSION = SNode.C_VERSION_MAJOR`, currently 1. There are 68 uses of
the global authority: three Codex shared libraries and 65 unrelated targets.
Existing A1 policy commits to one final bump, and A1.3 ABI evidence already
proved incompatibility (`TypedServerRequest` grew from 312 to 960 bytes).

The frozen action is a scoped Codex SOVERSION `1 -> 2` at final A1 closure,
after the 339/0/0/48 proof and before final ABI/package capture. A
Codex-specific authority should switch `ai-openai-codex`,
`ai-openai-codex-backend`, and `ai-openai-codex-frontend`; unrelated targets
must not be bumped. Binary packages move from `.so.1` to `.so.2`, and installed
consumers rebuild and relink.

Layout probes cover `AppServerClient`, `typed::Client`, all three public
variants, `UserInputRequest`, `TurnErrorEvent`, `ClientInfo`,
`InitializeResult`, every introduced aggregate/variant, and all facades.
Symbol and package evidence is recaptured. No earlier implementation PR may
claim binary compatibility merely because SONAME 1 remains unchanged.

## Security and sensitive data

Treat every raw envelope, opaque JSON field, and flagged field path as
sensitive. Redaction covers:

- app metadata, configuration, branding, screenshots, and reviews;
- external-agent configuration, cwd/home detection, histories, progress, and
  failures;
- feedback content, reasons, tags, attachments/logs, and thread IDs;
- hook definitions, handlers, sources, paths, inputs, outputs, and failures;
- marketplace/plugin sources, URLs, paths, refs, SHAs, packages, registries,
  checkouts, principals, shares, targets, roles, and errors;
- skill names, paths, roots, dependencies, configuration, and errors;
- MCP server identity, OAuth state/scopes/URLs, resource identity/content,
  tool schemas/arguments/results, metadata, and elicitation content;
- all user questions, options, and answers, regardless of `isSecret`;
- all attestation inputs and outputs;
- process handles/output, remote status/identity, warning/deprecation text,
  IDs, paths, and Windows diagnostics.

Diagnostics may contain method, structural path, expected type, and safe
counts, never values. Audit evidence and future fixtures use synthetic IDs,
`/synthetic/...` paths, `example.invalid` URLs, and fake tokens. They never
capture credentials, OAuth tokens, environment variables, private
repositories or paths, MCP configuration, feedback content, process output,
user answers, or attestation material.

## Packaging, validation, and non-goals

Source packaging retains the audit tool, focused test, five evidence files,
and this document. The pinned counts move from 22 to 27 evidence files, 18 to
19 Codex documents, and 13 to 14 top-level Codex tools. The extracted archive
runs `app_server_a1_4.py check`. Audit tools, tests, documentation, evidence,
JSON, and Python remain excluded from installed and binary packages; the
public-header count remains 34 in this audit.

The focused test is registered in `tests/CMakeLists.txt` rather than
`tests/component/codex/CMakeLists.txt`. The latter is hashed by the immutable
A1.3 closure report, so root registration proves discovery without rewriting
existing A1.3 evidence.

The A1.3 closure checker also treats its own captured source record and the
source-package guard record as historical A1.3 evidence. It retains those two
frozen records while live successor-aware token checks require every A1.4
package entry and extracted check. This lets the required predecessor closure
check remain byte-stable without hiding or rewriting the A1.4 source-package
expansion.

Audit validation runs the A1.3 audit and closure checks, the A1.4 checker and
mutation test, test-enabled CMake configuration, focused Codex CTest, source
package retention, deterministic regeneration, syntax/import checks,
`git diff --check`, and the synthetic-secret guard. It does not run
unfiltered CTest, stress/soak/fuzz/benchmark/sanitizer matrices, or the
credential-bearing live integration. Those targets add no audit-specific
signal; runtime integration risk remains scheduled for the three
implementation PRs.

Explicit A2 non-goals include every InventoryOnly identity, experimental
process/remote/fuzzy-search/collaboration/environment/memory/realtime/search
and settings control, stable-unreachable union alternatives, and any backend,
frontend, or application expansion outside the typed A1 boundary.
