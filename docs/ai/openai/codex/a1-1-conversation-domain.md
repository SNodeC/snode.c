# Codex A1.1 conversation-domain audit

## Status and authority

This document records both the frozen Phase A1.1 implementation plan and the
final closure of that plan. The immutable start-state sections remain Commit 1
audit evidence; the final closure section is regenerated and checked against
the live canonical registry after implementation batches B2-B5.

The audited starting `origin/master` is
`a226e3df5efd55b5dbef12cb1760674388909d7c`. This matches the expected prompt
base. The merged A1.0 commit
`cfc129ec879c70fc09c165035f6d1390f61289b8` is an ancestor.

A1.1 is pinned to the stable Codex App Server 0.144.6 protocol. Experimental
client operations remain inventory-only. Stable inbound realtime
notifications are included even though their corresponding outbound realtime
operations are experimental.

The mechanically checked inputs are:

- `tools/codex/app-server-evidence/0.144.6/module-slice-assignment.json`;
- `tools/codex/app-server-evidence/0.144.6/nested-reachability.json`;
- `tools/codex/app-server-evidence/0.144.6/operation-contracts.json`;
- `tools/codex/app-server-evidence/0.144.6/schema-completeness-evidence.json`;
- `tools/codex/app-server-evidence/0.144.6/fixture-coverage.json`;
- `tools/codex/app-server-evidence/0.144.6/a1-1-start-state.json`;
- `tools/codex/app-server-schema/0.144.6/stable/`;
- `tools/codex/app-server-surface/0.144.6.json`; and
- `src/ai/openai/codex/detail/ProtocolSurfaceRegistryData.inc`.

`ai::openai::codex::detail::ProtocolSurfaceRegistry` remains the only local
production authority for runtime disposition, runtime target, typed
implementation status, typed module, A1 slice, schema status, BackendCore
status, canonical-state status, and frontend disposition. The generated audit,
type closure, fixtures, and private codec descriptors are checked evidence;
they are not a second production registry.

The start-state file is immutable review evidence captured once from the
verified A1.0 base. Its freeze command requires the exact reviewed base SHA
and refuses to overwrite an existing snapshot; the force override is reserved
for the reviewed initial freeze or correction in Commit 1. Commits 2-6 do not
regenerate that baseline.

Ordinary audit generation and checking deliberately read two views. The
immutable snapshot preserves the exact starting 26 Partial / 125
NotImplemented split and starting global metrics. The live canonical registry,
schema-completeness evidence, and fixture coverage prove monotonic
implementation progress. Static identity drift, implementation or schema
demotion, layer-disposition regression, invalid runtime promotion, lost
completeness facts, and reduced fixture coverage are rejected. Thus later
commits can advance production coverage without rewriting the evidence of
what Commit 1 actually audited.

## Corrected frozen taxonomy

The A1.1 denominator is exactly 151 registry identities:

| Registry category | Count |
|---|---:|
| Client-request methods | 22 |
| Server-notification methods | 37 |
| `ThreadItem` discriminators | 18 |
| `ResponseItem` discriminators | 16 |
| Nested tagged-union discriminators | 58 |
| **Total** | **151** |

A1.1 contains zero client notifications, zero server requests, and zero server
responses. The 22 paired successful result schemas are implementation
obligations for the 22 requests. They are not additional registry identities,
so the denominator is 151, never 173.

The generated classification and module totals are:

| Classification | Count |
|---|---:|
| `StablePublicRoot` | 59 |
| `StableItemRoot` | 34 |
| `SharedCommon` | 26 |
| `RootOwnedNestedUnion` | 16 |
| `SharedWithinSlice` | 16 |
| **Total** | **151** |

| Typed module | Count |
|---|---:|
| `ThreadsTurnsSessions` | 125 |
| `Common` | 26 |
| **Total** | **151** |

All 151 identities are stable. `thread/rollback`,
`item/fileChange/outputDelta`, and `thread/compacted` are stable deprecated
identities and remain in the typed surface.

## Starting and target completeness

At the audited base, A1.0 has already implemented but left schema-partial 26
A1.1 identities:

| Category | Partial | Not implemented |
|---|---:|---:|
| Client requests | 6 | 16 |
| Server notifications | 12 | 25 |
| `ThreadItem` | 8 | 10 |
| `ResponseItem` | 0 | 16 |
| Nested tagged unions | 0 | 58 |
| **Total** | **26** | **125** |

The six partial requests are `thread/list`, `thread/read`, `thread/resume`,
`thread/start`, `turn/interrupt`, and `turn/start`. The twelve partial
notifications are:

- `item/agentMessage/delta`;
- `item/commandExecution/outputDelta`;
- `item/completed`;
- `item/fileChange/patchUpdated`;
- `item/reasoning/summaryTextDelta`;
- `item/reasoning/textDelta`;
- `item/started`;
- `thread/started`;
- `thread/status/changed`;
- `thread/tokenUsage/updated`;
- `turn/completed`; and
- `turn/started`.

The eight partial `ThreadItem` alternatives are `agentMessage`,
`commandExecution`, `dynamicToolCall`, `fileChange`, `mcpToolCall`,
`reasoning`, `userMessage`, and `webSearch`.

The global starting metrics are mechanically derived as:

| Schema status | Starting count |
|---|---:|
| Complete | 16 |
| Partial | 34 |
| Not implemented | 289 |
| Not applicable | 48 |

Completing exactly A1.1, without promoting another slice, produces:

| Schema status | Expected final count |
|---|---:|
| Complete | 167 |
| Partial | 8 |
| Not implemented | 164 |
| Not applicable | 48 |

The eight expected residual partial identities are `initialize`,
`initialized`, `error`, `model/rerouted`,
`account/chatgptAuthTokens/refresh`,
`item/commandExecution/requestApproval`,
`item/fileChange/requestApproval`, and `item/tool/requestUserInput`. A1.1 does
not claim them complete.

## Client operations and grouped API

All 19 thread operations belong to `client.typed().threads()`. All three turn
operations belong to `client.typed().turns()`. A1.1 does not add a
`sessions()` facade. The old direct grouped accessors remain deprecated
forwarders, and no operation is added directly to `AppServerClient`.

The reviewed semantic names and authoritative contracts are:

| Wire method | Facade method | Params schema | Successful public result | Authoritative response schema | Kind |
|---|---|---|---|---|---|
| `thread/archive` | `Threads::archive` | `ThreadArchiveParams` | `Unit` | `ThreadArchiveResponse` | Unit |
| `thread/compact/start` | `Threads::startCompaction` | `ThreadCompactStartParams` | `Unit` | `ThreadCompactStartResponse` | Unit |
| `thread/delete` | `Threads::remove` | `ThreadDeleteParams` | `Unit` | `ThreadDeleteResponse` | Unit |
| `thread/fork` | `Threads::fork` | `ThreadForkParams` | `ThreadForkResponse` | `ThreadForkResponse` | Concrete |
| `thread/goal/clear` | `Threads::clearGoal` | `ThreadGoalClearParams` | `ThreadGoalClearResponse` | `ThreadGoalClearResponse` | Concrete |
| `thread/goal/get` | `Threads::getGoal` | `ThreadGoalGetParams` | `ThreadGoalGetResponse` | `ThreadGoalGetResponse` | Concrete |
| `thread/goal/set` | `Threads::setGoal` | `ThreadGoalSetParams` | `ThreadGoalSetResponse` | `ThreadGoalSetResponse` | Concrete |
| `thread/inject_items` | `Threads::injectItems` | `ThreadInjectItemsParams` | `Unit` | `ThreadInjectItemsResponse` | Unit |
| `thread/list` | `Threads::list` | `ThreadListParams` | `ThreadListResponse` | `ThreadListResponse` | Concrete |
| `thread/loaded/list` | `Threads::listLoaded` | `ThreadLoadedListParams` | `ThreadLoadedListResponse` | `ThreadLoadedListResponse` | Concrete |
| `thread/metadata/update` | `Threads::updateMetadata` | `ThreadMetadataUpdateParams` | `ThreadMetadataUpdateResponse` | `ThreadMetadataUpdateResponse` | Concrete |
| `thread/name/set` | `Threads::setName` | `ThreadSetNameParams` | `Unit` | `ThreadSetNameResponse` | Unit |
| `thread/read` | `Threads::read` | `ThreadReadParams` | `ThreadReadResponse` | `ThreadReadResponse` | Concrete |
| `thread/resume` | `Threads::resume` | `ThreadResumeParams` | `ThreadResumeResponse` | `ThreadResumeResponse` | Concrete |
| `thread/rollback` | `Threads::rollback` (deprecated) | `ThreadRollbackParams` | `ThreadRollbackResponse` | `ThreadRollbackResponse` | Concrete |
| `thread/shellCommand` | `Threads::shellCommand` | `ThreadShellCommandParams` | `Unit` | `ThreadShellCommandResponse` | Unit |
| `thread/start` | `Threads::start` | `ThreadStartParams` | `ThreadStartResponse` | `ThreadStartResponse` | Concrete |
| `thread/unarchive` | `Threads::unarchive` | `ThreadUnarchiveParams` | `ThreadUnarchiveResponse` | `ThreadUnarchiveResponse` | Concrete |
| `thread/unsubscribe` | `Threads::unsubscribe` | `ThreadUnsubscribeParams` | `ThreadUnsubscribeResponse` | `ThreadUnsubscribeResponse` | Concrete |
| `turn/interrupt` | `Turns::interrupt` | `TurnInterruptParams` | `Unit` | `TurnInterruptResponse` | Unit |
| `turn/start` | `Turns::start` | `TurnStartParams` | `TurnStartResponse` | `TurnStartResponse` | Concrete |
| `turn/steer` | `Turns::steer` | `TurnSteerParams` | `TurnSteerResponse` | `TurnSteerResponse` | Concrete |

The exact A1.1 result split is seven Unit and fifteen Concrete. The Unit
methods are:

- `thread/archive`;
- `thread/compact/start`;
- `thread/delete`;
- `thread/inject_items`;
- `thread/name/set`;
- `thread/shellCommand`; and
- `turn/interrupt`.

One explicit `Unit` type is used for all seven contracts.
`TurnInterruptResult` can remain as a source-compatible alias. A Unit decoder
accepts only the pinned empty-object response contract; it does not interpret
arbitrary missing or empty JSON as success.

The existing convenience overloads for `thread/list`, `thread/read`,
`thread/resume`, `thread/start`, `turn/interrupt`, and `turn/start` remain
forwarding compatibility APIs where their old contract can be expressed
without ambiguity. Canonical overloads account for every response-wrapper
field and preserve the complete raw result in `OperationResult`.

## Stable server notifications

The 37 notification identities are:

### Item notifications

- `item/agentMessage/delta`;
- `item/commandExecution/outputDelta`;
- `item/commandExecution/terminalInteraction`;
- `item/completed`;
- `item/fileChange/outputDelta` (deprecated);
- `item/fileChange/patchUpdated`;
- `item/mcpToolCall/progress`;
- `item/plan/delta`;
- `item/reasoning/summaryPartAdded`;
- `item/reasoning/summaryTextDelta`;
- `item/reasoning/textDelta`; and
- `item/started`.

### Thread notifications

- `thread/archived`;
- `thread/closed`;
- `thread/compacted` (deprecated);
- `thread/deleted`;
- `thread/goal/cleared`;
- `thread/goal/updated`;
- `thread/name/updated`;
- `thread/realtime/closed`;
- `thread/realtime/error`;
- `thread/realtime/itemAdded`;
- `thread/realtime/outputAudio/delta`;
- `thread/realtime/sdp`;
- `thread/realtime/started`;
- `thread/realtime/transcript/delta`;
- `thread/realtime/transcript/done`;
- `thread/settings/updated`;
- `thread/started`;
- `thread/status/changed`;
- `thread/tokenUsage/updated`; and
- `thread/unarchived`.

### Turn notifications

- `turn/completed`;
- `turn/diff/updated`;
- `turn/moderationMetadata`;
- `turn/plan/updated`; and
- `turn/started`.

The stable inbound realtime methods are decoded and observable. Experimental
outbound realtime client methods remain absent.

## Item roots

`ThreadItem` is a complete, direction-specific public variant with these 18
known alternatives:

- `agentMessage`;
- `collabAgentToolCall`;
- `commandExecution`;
- `contextCompaction`;
- `dynamicToolCall`;
- `enteredReviewMode`;
- `exitedReviewMode`;
- `fileChange`;
- `hookPrompt`;
- `imageGeneration`;
- `imageView`;
- `mcpToolCall`;
- `plan`;
- `reasoning`;
- `sleep`;
- `subAgentActivity`;
- `userMessage`; and
- `webSearch`.

`using Item = ThreadItem;` remains as the transitional compatibility alias.

`ResponseItem` is a distinct public variant with these 16 known alternatives:

- `agent_message`;
- `compaction`;
- `compaction_trigger`;
- `context_compaction`;
- `custom_tool_call`;
- `custom_tool_call_output`;
- `function_call`;
- `function_call_output`;
- `image_generation_call`;
- `local_shell_call`;
- `message`;
- `other`;
- `reasoning`;
- `tool_search_call`;
- `tool_search_output`; and
- `web_search_call`.

`ResponseItem` does not alias `ThreadItem`, and response alternatives are not
inserted into `ThreadItem`. `UnknownItem` and `UnknownResponseItem` are
separate direction-specific alternatives.

## Nested tagged unions

The exact 58 alternatives are split across 17 families:

| Family | Discriminator | Known alternatives | Count | Batch |
|---|---|---|---:|---|
| `AgentMessageInputContent` | `type` | `encrypted_content`, `input_text` | 2 | B3 |
| `AskForApproval` | `$variant` | `granular`, `never`, `on-request`, `untrusted` | 4 | B2 |
| `CommandAction` | `type` | `listFiles`, `read`, `search`, `unknown` | 4 | B2 |
| `ContentItem` | `type` | `input_image`, `input_text`, `output_text` | 3 | B3 |
| `DynamicToolCallOutputContentItem` | `type` | `inputImage`, `inputText` | 2 | B2 |
| `FunctionCallOutputContentItem` | `type` | `encrypted_content`, `input_image`, `input_text` | 3 | B3 |
| `LocalShellAction` | `type` | `exec` | 1 | B3 |
| `PatchChangeKind` | `type` | `add`, `delete`, `update` | 3 | B2 |
| `ReasoningItemContent` | `type` | `reasoning_text`, `text` | 2 | B3 |
| `ReasoningItemReasoningSummary` | `type` | `summary_text` | 1 | B3 |
| `ResponsesApiWebSearchAction` | `type` | `find_in_page`, `open_page`, `other`, `search` | 4 | B3 |
| `SandboxPolicy` | `type` | `dangerFullAccess`, `externalSandbox`, `readOnly`, `workspaceWrite` | 4 | B2 |
| `SessionSource` | `$variant` | `appServer`, `cli`, `custom`, `exec`, `subAgent`, `unknown`, `vscode` | 7 | B4 |
| `SubAgentSource` | `$variant` | `compact`, `memory_consolidation`, `other`, `review`, `thread_spawn` | 5 | B4 |
| `ThreadStatus` | `type` | `active`, `idle`, `notLoaded`, `systemError` | 4 | B4 |
| `UserInput` | `type` | `image`, `localImage`, `mention`, `skill`, `text` | 5 | B2 |
| `WebSearchAction` | `type` | `findInPage`, `openPage`, `other`, `search` | 4 | B2 |
| **Total** |  |  | **58** |  |

The exact branch field closure is:

- `AgentMessageInputContent`: `input_text` requires `text` and `type`;
  `encrypted_content` requires `encrypted_content` and `type`.
- `AskForApproval`: `untrusted`, `on-request`, and `never` are scalar
  variants; `granular` requires `mcp_elicitations`, `rules`, and
  `sandbox_approval`, and optionally carries `request_permissions` and
  `skill_approval`.
- `CommandAction`: `read` requires `command`, `name`, `path`, and `type`;
  `listFiles` requires `command` and `type` and optionally carries nullable
  `path`; `search` requires `command` and `type` and optionally carries
  nullable `path` and `query`; `unknown` requires `command` and `type`.
- `ContentItem`: `input_text` and `output_text` require `text` and `type`;
  `input_image` requires `image_url` and `type` and optionally carries
  nullable `detail`.
- `DynamicToolCallOutputContentItem`: `inputText` requires `text` and `type`;
  `inputImage` requires `imageUrl` and `type`.
- `FunctionCallOutputContentItem`: `input_text` requires `text` and `type`;
  `input_image` requires `image_url` and `type` and optionally carries
  nullable `detail`; `encrypted_content` requires `encrypted_content` and
  `type`.
- `LocalShellAction::exec` requires `command` and `type`, and optionally
  carries nullable `env`, `timeout_ms`, `user`, and `working_directory`.
- `PatchChangeKind`: `add` and `delete` require only `type`; `update` requires
  `type` and optionally carries nullable `move_path`.
- Both `ReasoningItemContent` alternatives require `text` and `type`;
  `ReasoningItemReasoningSummary::summary_text` also requires `text` and
  `type`.
- `ResponsesApiWebSearchAction::search` requires `type` and optionally carries
  nullable `queries` and `query`; `open_page` requires `type` and optionally
  carries nullable `url`; `find_in_page` requires `type` and optionally
  carries nullable `pattern` and `url`; `other` requires only `type`.
- `SandboxPolicy::dangerFullAccess` requires only `type`; `readOnly`
  optionally carries `networkAccess`; `externalSandbox` optionally carries
  `networkAccess`; `workspaceWrite` optionally carries `excludeSlashTmp`,
  `excludeTmpdirEnvVar`, `networkAccess`, and `writableRoots`.
- `SessionSource` has five scalar variants. `custom` carries a string;
  `subAgent` carries `SubAgentSource`.
- `SubAgentSource` has three scalar variants. `thread_spawn` requires `depth`
  and `parent_thread_id` and optionally carries nullable `agent_nickname`,
  `agent_path`, and `agent_role`; `other` carries a string.
- `ThreadStatus::active` requires `activeFlags` and `type`; the other three
  alternatives require only `type`.
- `UserInput::text` requires `text` and `type` and optionally carries
  `text_elements`; `image` requires `type` and `url` and optionally carries
  nullable `detail`; `localImage` requires `path` and `type` and optionally
  carries nullable `detail`; `skill` and `mention` require `name`, `path`, and
  `type`.
- `WebSearchAction::search` requires `type` and optionally carries nullable
  `queries` and `query`; `openPage` requires `type` and optionally carries
  nullable `url`; `findInPage` requires `type` and optionally carries nullable
  `pattern` and `url`; `other` requires only `type`.

Responses API web-search actions and thread web-search actions remain distinct.
Input and output content families remain direction-specific.

## Full stable schema-definition closure

The closure is derived from:

- 22 request parameter definitions;
- 22 named successful-response schema definitions, including the seven named
  schemas mapped to public `Unit`;
- 37 notification parameter definitions;
- `ThreadItem` and `ResponseItem`; and
- the 17 registered nested-union family definitions.

These are 100 unique roots. Their transitive closure contains exactly 164
unique named definitions, all in the v2 namespace, with no dependency cycle:

| Closure partition | Count |
|---|---:|
| Registered item or nested-union definitions | 19 |
| Ordinary named definitions | 145 |
| **Total named definitions** | **164** |

The 145 ordinary definitions consist of 81 request/result/notification roots
and 64 transitive helper definitions. Their structural shapes are:

| Schema shape | Count |
|---|---:|
| Object | 106 |
| Direct string enum | 27 |
| `oneOf` | 23 |
| `anyOf` | 2 |
| String scalar/alias | 6 |
| **Total** | **164** |

The closure additionally contains:

| Schema-position evidence | Count |
|---|---:|
| Object property positions | 655 |
| Required property positions | 418 |
| Optional property positions | 237 |
| Required-and-nonnullable positions | 413 |
| Optional-and-nonnullable positions | 19 |
| Optional-and-nullable tri-state positions | 218 |
| Required-and-nullable positions | 5 |
| Array element positions | 38 |
| Actual map-value positions | 5 |
| Protocol-defined opaque JSON positions | 14 |

The reviewed named-definition ownership dispositions are:

| Definition disposition | Count |
|---|---:|
| `PublicHandwrittenType` | 127 |
| `OpenStringEnum` | 26 |
| `ReusedExistingType` | 10 |
| `StrongIdentifier` | 1 |
| **Total** | **164** |

The ten reused definitions include the three exact prior-slice types
`CodexErrorInfo`, `NonSteerableTurnKind`, and `TurnError`. The other 161
definitions are divided into 122 A1.1-local definitions and 39 definitions
that A1.1 owns but later stable slices also reach. Cross-slice ownership is
therefore explicit and guarded; a later slice is not allowed to acquire a
second C++ owner for one of these definitions.

Directionality is derived from the stable roots rather than from similar
field shapes:

| Direction | Named definitions | Schema paths |
|---|---:|---:|
| Encode only | 29 | 107 |
| Decode only | 121 | 554 |
| Bidirectional | 14 | 37 |
| **Total** | **164** | **698** |

The 698 path mappings contain 92 public type positions, 76 private
discriminator-codec positions, 14 protocol-opaque JSON positions, 360 reused
scalar/container positions, and 156 strong-identifier positions. Those 156
strong-ID positions are exactly 68 `ThreadId` positions (62 scalar
properties, three vector properties, and their three element positions), 24
`TurnId`, 29 `ItemId`, 14 direction-specific `ResponseItemId`, 10 `ModelId`,
seven `ResponseCallId`, three `ClientUserMessageId`, and one App-Server
`SessionId` position.

The App-Server `SessionId` represents the session tree attached to a `Thread`;
it is distinct from both `ThreadId` and BackendCore's frontend-connection
session identifier. `ClientUserMessageId` is shared by outbound turn start and
steer parameters and by the corresponding inbound user-message `clientId`
echo. All three client-message fields retain their schema-required
omitted/null/value representation through
`OptionalNullable<ClientUserMessageId>`.

Every property also records its exact public representation:

| Presence model | Count | C++ representation |
|---|---:|---|
| Required, non-nullable | 413 | `T` |
| Required, nullable | 5 | `std::optional<T>` |
| Optional, non-nullable with a reviewed schema default | 4 | `T`, initialized from the schema default on omission |
| Optional, non-nullable without a reviewed schema default | 15 | `std::optional<T>` |
| Optional, nullable | 218 | `OptionalNullable<T>` |
| **Total** | **655** | |

The four materialized defaults are `Turn.itemsView = "full"` and the empty
`instructionSources` arrays on the thread start, resume, and fork responses.
Their containing incoming aggregates retain the complete raw JSON, so callers
can still distinguish physical wire omission from an explicitly transmitted
default when needed. A `default: null` does not collapse the wire states:
the ten reviewed nullable-default paths remain `OptionalNullable<T>` so
omitted, explicit null, and value stay distinct.

Every named definition and nested schema position receives exactly one reviewed
mapping using `PublicHandwrittenType`, `PrivateCodecHelper`,
`OpenStringEnum`, `StrongIdentifier`, `ProtocolDefinedOpaqueJson`, or
`ReusedExistingType`. Structural inference does not silently choose a public
API. In particular, ordinary known objects cannot be represented wholesale as
`Json` merely for implementation convenience, and string enum values remain
open for forward compatibility.

The exact 14 schema-authorized opaque JSON positions are:

| Vendored v2 schema path | Protocol reason |
|---|---|
| `#/definitions/v2/McpToolCallResult/properties/_meta` | Upstream true schema |
| `#/definitions/v2/McpToolCallResult/properties/content/items` | Upstream true array-element schema |
| `#/definitions/v2/McpToolCallResult/properties/structuredContent` | Upstream true schema |
| `#/definitions/v2/ResponseItem/oneOf/5/properties/arguments` | Tool-search arguments are protocol-opaque |
| `#/definitions/v2/ResponseItem/oneOf/9/properties/tools/items` | Tool-search output entries are protocol-opaque |
| `#/definitions/v2/ThreadForkParams/properties/config/additionalProperties` | Arbitrary configuration override value |
| `#/definitions/v2/ThreadInjectItemsParams/properties/items/items` | Raw Responses API item |
| `#/definitions/v2/ThreadItem/oneOf/7/properties/arguments` | MCP tool arguments are protocol-opaque |
| `#/definitions/v2/ThreadItem/oneOf/8/properties/arguments` | Dynamic tool arguments are protocol-opaque |
| `#/definitions/v2/ThreadRealtimeItemAddedNotification/properties/item` | Realtime item is an upstream true schema |
| `#/definitions/v2/ThreadResumeParams/properties/config/additionalProperties` | Arbitrary configuration override value |
| `#/definitions/v2/ThreadStartParams/properties/config/additionalProperties` | Arbitrary configuration override value |
| `#/definitions/v2/TurnModerationMetadataNotification/properties/metadata` | Moderation metadata is an upstream true schema |
| `#/definitions/v2/TurnStartParams/properties/outputSchema` | Upstream annotation-only schema accepts arbitrary JSON |

The seven empty successful-response object definitions are mapped to `Unit`;
they are not opaque JSON. The three configuration containers are typed as
`OptionalNullable<std::map<std::string, Json>>`; only their protocol-defined
arbitrary values are opaque. The other two map values are fully typed:
`LocalShellAction::env` maps to strings and
`ThreadItem::collabAgentToolCall::agentsStates` maps to
`CollabAgentState`.

The frozen fixture plan records 173 positive fixture roles. The A1.0 corpus
already supplies 104 A1.1-positive records and the plan assigns 83 new
schema-derived fixtures where the existing records do not satisfy every
role. Its negative and boundary obligations are 810 required-field removals,
477 optional-field omissions, 435 nullable omitted/null/value trios, and
1,287 wrong-type mutations (1,279 applicable and eight explicitly
protocol-opaque). It also records 151 future-discriminator cases, 118
applicable missing-discriminator cases, four applicable
conflicting-discriminator cases, 130 applicable malformed-known cases, and 55
identity-specific future-open-enum occurrences. Every non-applicable case has
an exact schema reason.

At final closure the indexed corpus contains 3,714 records: 1,415 positive and
2,299 negative. The independent surface-wide coverage report records 15,766
rejected required-field removals, 15,701 rejected wrong-type mutations, 65
explicitly unconstrained wrong-type exclusions, and 13,452 accepted
optional-field omissions. The A1.1 operation production corpus contributes
1,166 result records (403 positive and 763 negative), while the notification
production corpus contributes 798 records (161 positive and 637 negative).
These counts are projections of the one indexed fixture corpus, not duplicate
fixture authorities.

## Dependency-closed implementation batches

Commit 1 freezes four implementation batches for Commits 2 through 5:

| Batch | Registry identities | Result obligations | Newly owned closure definitions |
|---|---:|---:|---:|
| B2: shared union and value foundations | 26 | 0 | 12 |
| B3: item models | 50 | 0 | 32 |
| B4: thread and turn operations | 38 | 22 | 72 |
| B5: notifications and preservation | 37 | 0 | 48 |
| **Total** | **151** | **22** | **164** |

B2 owns all 26 `SharedCommon` alternatives in `AskForApproval`,
`CommandAction`, `DynamicToolCallOutputContentItem`, `PatchChangeKind`,
`SandboxPolicy`, `UserInput`, and `WebSearchAction`. Its 12 named closure
definitions are `AbsolutePathBuf`, `AskForApproval`, `ByteRange`,
`CommandAction`, `DynamicToolCallOutputContentItem`, `ImageDetail`,
`NetworkAccess`, `PatchChangeKind`, `SandboxPolicy`, `TextElement`,
`UserInput`, and `WebSearchAction`.

B3 owns the 34 item identities and the 16 `RootOwnedNestedUnion`
alternatives. B4 owns the 22 request identities, all 22 paired result
obligations, and the 16 `SharedWithinSlice` alternatives in `SessionSource`,
`SubAgentSource`, and `ThreadStatus`. B5 owns the 37 notification identities.

Definitions reached by multiple later consumers are assigned to the earliest
batch that can fully exercise them. This produces an acyclic
`12 + 32 + 72 + 48 = 164` definition partition and does not postpone the 58
nested alternatives to the notification or closure commit.

## Dispatch, decoding, and raw retention

Production method or discriminator dispatch begins with the exact canonical
registry row. Client requests and server notifications use registry method
targets. `ThreadItem` and `ResponseItem` use distinct target families.
Nested-union codecs use private generated descriptors keyed by exact
`ProtocolSurfaceKey`. At final closure every one of the 151 exact rows has one
implemented production target, and every generated descriptor is checked in
both directions against that row.

Public C++ types and semantic operation names remain handwritten. Generated
descriptor data is private, deterministic, checked in, and never a production
disposition authority. A typed row without its required target and descriptor,
or a descriptor without one implemented row, is an error in both directions.

Incoming aggregates retain raw JSON at the narrowest useful typed aggregate or
union alternative. Outgoing-only params are authoritative encodings and do not
gain a raw member without a protocol round-trip reason.

For every method and union:

- a future method, discriminator, or open-enum value is
  `ForwardCompatibility`;
- a future discriminator produces the direction-specific explicit unknown
  alternative and retains raw JSON;
- a known outer alternative with a future open-enum value retains the typed
  outer value and records `UnknownEnumValue`;
- a known discriminator with a missing field, wrong type, conflicting
  discriminator, or other invalid payload is `MalformedKnownPayload` with
  `ProtocolWarning`; and
- diagnostics contain the exact identity and field path, never secret payload
  values.

Typed decoding does not fail the transport connection.

## One protocol engine and async contract

Every A1.1 facade and dispatcher reuses
`AppServerClient::RawProtocol`. There remains exactly one transport
connection, JSONL engine, request-ID allocator, pending client-request
registry, pending server-request registry, connection-generation boundary,
callback-ordering mechanism, cancellation mechanism, server-request
occurrence-token mechanism, raw observer set, and typed observer set.

Operations retain:

```text
Submission operation(Params, Handler)
```

Callbacks remain primary. Accepted completions are asynchronous, submission
failures are synchronous, and A1.1 adds no future, coroutine, blocking wait,
worker thread, completion timer, or second pending-request system.

## Compatibility and rebuild boundary

A1 is one documented consumer-rebuild boundary. A1.1 exposes unavoidable
public variant corrections instead of preserving incomplete field models.
Practical compatibility includes:

- `Item = ThreadItem`;
- `ToolCallItem = McpToolCallThreadItem` for the pre-A1.1 MCP-only
  compatibility name;
- a compatible `TurnInterruptResult` alias to the common `Unit`;
- deprecated direct grouped accessors forwarding to `client.typed()`;
- reviewed overloads and conversions for the six previously typed A1.1
  operations;
- a canonical complete `UserInput` model with an explicit compatibility path
  from the old `TurnInput`;
- complete `AskForApproval`, `SandboxPolicy`, and tagged `ThreadStatus`
  models; and
- distinct `ThreadItem` and `ResponseItem` variants with distinct unknown
  alternatives.

Ambiguous compatibility encodings are rejected, including requests that set
old and new representations of the same protocol field simultaneously.

The compatibility aliases retain the familiar item names, but the following
public member-contract corrections are unavoidable source changes:

| Public item | A1.1 source change |
|---|---|
| `Thread` | The previously optional partial snapshot fields are replaced by the required upstream aggregate: `cwd` is `AbsolutePathBuf`, `sessionId` is the strong `SessionId`, `status` is the tagged `ThreadStatus`, timestamps and core metadata are required, and source/name/goal/agent metadata plus raw diagnostics are represented. Aggregate initialization and callers that treated required fields as absent must be updated. |
| `ThreadPage` | The old page model is now the compatibility alias for the complete `ThreadListResponse`; its complete response fields and raw retention are therefore visible to consumers. |
| `Turn` | `items` is the complete `ThreadItem` variant vector, `itemsView` is an open protocol value, `error` changes from `optional<Json>` to `OptionalNullable<TurnError>`, and temporal fields use explicit tri-state semantics. Raw retention and diagnostics are added, so aggregate order and field types change. |
| `ThreadItem` | The variant expands from eight to nineteen alternatives and changes its alternative order; exhaustive visitors and index-based code must be updated. `ResponseItem` is now a separate seventeen-alternative variant. |
| `AgentMessageItem` | `phase` changes from `optional<string>` to tri-state `MessagePhase`; `memoryCitation` and structured diagnostics are added. |
| `UserMessageItem` | `content` changes from untyped `Json` to `vector<UserInput>`; `clientId` becomes tri-state `ClientUserMessageId` and follows `content` in aggregate order. The exact incoming content remains in `metadata.raw`. |
| `ReasoningItem` | `summary` and `content` become optional vectors to represent omitted fields; `summaryOrDefault()` and `contentOrDefault()` provide read-only empty fallbacks. |
| `CommandExecutionItem` | `cwd`, `status`, and `commandActions` become protocol types; nullable fields become tri-state; `output` is renamed to the canonical `aggregatedOutput`; `source` and diagnostics are added and aggregate order changes. |
| `FileChangeItem` | `changes` becomes `vector<FileUpdateChange>` and `status` becomes `PatchApplyStatus`; the exact incoming changes remain in `metadata.raw`. |
| `ToolCallItem` | The name now aliases only `McpToolCallThreadItem`; MCP status/result/error fields become typed or tri-state, `server` is required, and dynamic-tool-only fields move to `DynamicToolCallThreadItem`. |
| `WebSearchItem` | `action` changes from untyped `Json` to tri-state `WebSearchAction`. |

The old `ToolCallItem` name cannot also denote the newly distinct
`DynamicToolCallThreadItem`: visitors that previously used `ToolCallItem` as a
catch-all for tool activity must add an explicit dynamic-tool alternative.
This is an intentional source change at the A1 consumer-rebuild boundary; the
MCP spelling remains available through the alias above.

`AppServerClient` keeps its PIMPL-only object layout.
`typed::Client` remains a one-pointer PIMPL. Public variants may change under
the documented rebuild boundary. A1.1 does not bump `SOVERSION`.

## Backend and frontend non-change

A1.1 does not add a `BackendCommand`, canonical domain semantic, frontend
command, frontend event, frontend snapshot field, remotely callable
operation, or frontend-security decision.

Existing modeled item and notification reducer semantics remain unchanged.
New known but A2-unmodeled notifications follow:

```text
typed event
    -> preserveUnmodeledTypedEvent(...)
    -> CodexExtensionReceived
    -> bounded recent-extension state
```

That notification path retains surface identity, bounded raw payload, structured
diagnostic, legacy optional decode error, original-size accounting, and the
existing redaction boundary. High-volume audio and transcript deltas remain
bounded.

Newly typed `ThreadItem` alternatives with a stable item ID retain the
pre-typing `UnknownItem` behavior through the existing `ItemUpserted` path:
the canonical item state keeps exact raw JSON and common metadata, while the
Frontend Protocol v1 snapshot remains the same metadata-only generic item.
An item without a stable ID uses the existing bounded recent-extension
fallback, including original-size accounting and snapshot-boundary redaction.
Thus an item does not disappear merely because it is no longer
`UnknownItem`, and no new item semantic is invented.

Frontend Protocol v1, its schema, version and identity, command set, event
set, and snapshot fields remain unchanged. `codex-backend-client` receives no
functional change. `codex-backend` receives no production diff. The only
production diff under `src/ai/openai/codex/frontend/` is the reviewed
`BackendAdapter.cpp` mechanical construction/conversion update required by
the completed `UserInput` and `SandboxPolicy` types; its command mapping and
wire behavior are unchanged. Delete and shell operations are not exposed
remotely and are exercised only with deterministic fake/socket transcripts in
ordinary tests. Exact source-tree fingerprints guard all three application
boundaries, in addition to the pre-existing byte-identity guard for Frontend
Protocol v1 and its JSON schema.

## Final closure evidence

At the end of implementation batch B5, the exact A1.1 ratchet is:

```text
A1.1 Complete:          151
A1.1 Partial:             0
A1.1 NotImplemented:      0
```

The resulting global schema-status metrics are:

| Schema status | Final count |
|---|---:|
| Complete | 167 |
| Partial | 8 |
| Not implemented | 164 |
| Not applicable | 48 |

The final generated report lists all 151 complete `ProtocolSurfaceKey`
identities rather than accepting count equality. Its staged cumulative
ratchets are B2 26, B3 76, B4 114, and B5 151, so a later batch cannot mask a
regression in an earlier one. The exact eight residual Partial identities are
also listed and remain outside A1.1.

The 22 result roots remain implementation obligations, not registry
identities. Their final split is seven Unit and fifteen Concrete, and all 22
are bound to the exact request descriptor, complete result decoder, indexed
fixture records, and raw-result retention path.

The installed-consumer audit pins the exact 27 public Codex headers, exercises
the completed typed and compatibility APIs, and retains the two-pointer
`AppServerClient` and one-pointer `typed::Client` layouts. The source package
retains all 3,715 fixture files, 13 evidence files, and seven offline tools.
Three independently generated Codex component TGZ archives contain only the
same public header inventory and the three SOVERSION-1 libraries; schemas,
fixtures, evidence, generators, tests, documentation, private descriptors,
and implementation sources are rejected from those binary packages.

Unknown future methods, discriminators, and open-enum values remain
`ForwardCompatibility`; malformed known payloads remain
`MalformedKnownPayload` / `ProtocolWarning`. Both outcomes retain bounded raw
input at the narrowest useful aggregate, and diagnostics contain the surface
identity and field path without payload values.

No A1.1 runtime implementation is deferred to Commit 6. Commit 6 contains
verification, guard, documentation, installed-consumer, and package closure
only. A missing handler, public type, codec, or production target would require
amending its owning B2-B5 commit.

A1.2-A1.4, A2, A3, Context UI, MQTT, REST, WebSocket, Qt, and other
application integration are outside this milestone.
