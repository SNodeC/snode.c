# Codex A1.2 accounts, models, and configuration audit

## Status and authority

This document records the frozen Phase A1.2 start state, implementation
batches, public API review, and boundary decisions for the stable Codex App
Server 0.144.6 surface. It is review evidence, not a production registry or
dispatch input.

The audited starting `origin/master` is
`9f7b2955d017cab189fb7b7a80211d0c2788f819`. It is the merge of PR 220. The
required correction commit
`d32d1cb03aea86b9dcd8deb8078f02a5cee7e5ad` is its second parent and an
ancestor. The two commits have the same tree
`adb042243c7d0942fc5a702fd56638f34dce931f`, so the merge changed ancestry but
not the reviewed A1.1 content. The `master` push workflow run 334 completed
successfully at this SHA.

The merged A1.1 evidence proves 151/151 A1.1 identities Complete and global
schema metrics of 167 Complete, 8 Partial, 164 NotImplemented, and 48
NotApplicable. Its fixture corpus, closure report, exact ratchets, and
params-only `codex.extension` preservation regression remain independently
guarded. The six checked-in A1.1 slice artifacts are byte-frozen while current
live checks allow only reviewed monotonic A1.2 progress.

A1.2 is pinned to the stable surface. Experimental-only inventory remains
registered and untyped. The stable wire methods
`experimentalFeature/enablement/set` and `experimentalFeature/list` are in
scope because the pinned stable schemas expose those operations.

The mechanically checked inputs are:

- `tools/codex/app-server-evidence/0.144.6/module-slice-assignment.json`;
- `tools/codex/app-server-evidence/0.144.6/nested-reachability.json`;
- `tools/codex/app-server-evidence/0.144.6/operation-contracts.json`;
- `tools/codex/app-server-evidence/0.144.6/schema-completeness-evidence.json`;
- `tools/codex/app-server-evidence/0.144.6/fixture-coverage.json`;
- `tools/codex/app-server-evidence/0.144.6/a1-2-start-state.json`;
- `tools/codex/app-server-schema/0.144.6/stable/`;
- `tools/codex/app-server-surface/0.144.6.json`; and
- `src/ai/openai/codex/detail/ProtocolSurfaceRegistryData.inc`.

`ai::openai::codex::detail::ProtocolSurfaceRegistry` remains the only local
production authority for runtime disposition, runtime target, typed
implementation status, typed module, A1 slice, schema status, BackendCore
status, canonical-state status, and frontend disposition. Classification is
derived evidence-side metadata from the frozen module/slice assignment. It is
not added to the production registry. Generated audit records, fixtures, and
private codec descriptors provide reproducibility and exact associations; they
do not make runtime or exposure decisions.

The A1 audit tools share exact-key/evidence loading, registry and fixture
transitions, dependency-ordered progress, C++ schema-type mechanics,
presence wrappers, deterministic rendering, and diagnostic handling through
`app_server_a1_shared.py`. They share one state-carrying schema walker in
`app_server_schema_paths.py`, the schema catalog/definition graph and
path-derived fixture obligations in `app_server_fixtures.py`, and registry
and authoritative-association parsing in `app_server_surface.py`. The A1.2
entry point supplies slice-specific reviewed mappings, closure policy, and
exact report guards rather than cloning the A1.1 generator. Six frozen A1.1
artifact hashes and their byte-identity test prove that the generalization
does not rewrite prior outputs.

## Frozen denominator and start state

The A1.2 denominator is exactly 45 registry identities:

| Registry category | Count |
|---|---:|
| Client-request methods | 18 |
| Server-notification methods | 7 |
| Server-request methods | 1 |
| Tagged-union alternatives | 19 |
| **Total** | **45** |

A1.2 contains no client notification, `ThreadItem`, or `ResponseItem`
identity. The 18 client successful-result roots and the auth-refresh
server-request response root are 19 implementation obligations. They are not
registry identities, so the denominator is 45, never 64.

The generated classification and production-module totals are:

| Evidence classification | Count |
|---|---:|
| `StablePublicRoot` | 26 |
| `RootOwnedNestedUnion` | 11 |
| `SharedWithinSlice` | 8 |
| **Total** | **45** |

| Production typed module | Count |
|---|---:|
| `AccountsModelsConfiguration` | 45 |

At the audited base, `model/rerouted` and
`account/chatgptAuthTokens/refresh` are Partial. The other 43 identities are
NotImplemented. No newer-master exception was needed.

Completing only A1.2 is expected to move the global metrics as follows:

| Boundary | Complete | Partial | NotImplemented | NotApplicable |
|---|---:|---:|---:|---:|
| Merged A1.1 base | 167 | 8 | 164 | 48 |
| After B2 | 191 | 7 | 141 | 48 |
| After B3 | 196 | 6 | 137 | 48 |
| After B4 | 207 | 6 | 126 | 48 |
| After B5 | 212 | 6 | 121 | 48 |

The expected residual Partial identities after B5 are `initialize`,
`initialized`, `error`, `item/commandExecution/requestApproval`,
`item/fileChange/requestApproval`, and `item/tool/requestUserInput`.

## Client operations and grouped API

The three new facades are `client.typed().accounts()`,
`client.typed().models()`, and `client.typed().configuration()`. They share
the existing `AppServerClient::RawProtocol`. No direct `AppServerClient`
forwarder, generic string request facade, MCP facade, feature facade, session
facade, protocol engine, pending map, request-ID allocator, connection
generation, timer, retry machine, or worker thread is introduced.

The reviewed wire map and authoritative contracts are:

| Wire method | Facade method | Params schema | Successful result | Kind |
|---|---|---|---|---|
| `account/login/cancel` | `Accounts::cancelLogin` | `CancelLoginAccountParams` | `CancelLoginAccountResponse` | Concrete |
| `account/login/start` | `Accounts::startLogin` | `LoginAccountParams` | `LoginAccountResponse` | Concrete |
| `account/logout` | `Accounts::logout` | Unit | Unit | Unit |
| `account/rateLimitResetCredit/consume` | `Accounts::consumeRateLimitResetCredit` | `ConsumeAccountRateLimitResetCreditParams` | `ConsumeAccountRateLimitResetCreditResponse` | Concrete |
| `account/rateLimits/read` | `Accounts::readRateLimits` | Unit | `GetAccountRateLimitsResponse` | Concrete |
| `account/read` | `Accounts::read` | `GetAccountParams` | `GetAccountResponse` | Concrete |
| `account/sendAddCreditsNudgeEmail` | `Accounts::sendAddCreditsNudgeEmail` | `SendAddCreditsNudgeEmailParams` | `SendAddCreditsNudgeEmailResponse` | Concrete |
| `account/usage/read` | `Accounts::readUsage` | Unit | `GetAccountTokenUsageResponse` | Concrete |
| `account/workspaceMessages/read` | `Accounts::readWorkspaceMessages` | Unit | `GetWorkspaceMessagesResponse` | Concrete |
| `model/list` | `Models::list` | `ModelListParams` | `ModelListResponse` | Concrete |
| `modelProvider/capabilities/read` | `Models::readProviderCapabilities` | `ModelProviderCapabilitiesReadParams` | `ModelProviderCapabilitiesReadResponse` | Concrete |
| `config/batchWrite` | `Configuration::batchWrite` | `ConfigBatchWriteParams` | `ConfigWriteResponse` | Concrete |
| `config/mcpServer/reload` | `Configuration::reloadMcpServers` | Unit | Unit | Unit |
| `config/read` | `Configuration::read` | `ConfigReadParams` | `ConfigReadResponse` | Concrete |
| `config/value/write` | `Configuration::writeValue` | `ConfigValueWriteParams` | `ConfigWriteResponse` | Concrete |
| `configRequirements/read` | `Configuration::readRequirements` | Unit | `ConfigRequirementsReadResponse` | Concrete |
| `experimentalFeature/enablement/set` | `Configuration::setExperimentalFeatureEnablement` | `ExperimentalFeatureEnablementSetParams` | `ExperimentalFeatureEnablementSetResponse` | Concrete |
| `experimentalFeature/list` | `Configuration::listExperimentalFeatures` | `ExperimentalFeatureListParams` | `ExperimentalFeatureListResponse` | Concrete |

The exact result split is two Unit and sixteen Concrete. The Unit methods are
`account/logout` and `config/mcpServer/reload`. Unit retains the A1.1
empty-object schema ratchet; an added response property must fail generation
and validation. Unit request parameters follow the one existing raw-engine
contract rather than adding an operation-specific omission convention.

Callbacks remain primary. Accepted completions are asynchronous, local
submission rejection is synchronous, and the established cancellation,
generation, ordering, observer, and callback-exception behavior applies to all
18 operations.

## Stable inbound methods

The seven notification identities are:

- `account/login/completed`;
- `account/rateLimits/updated`;
- `account/updated`;
- `configWarning`;
- `model/rerouted`;
- `model/safetyBuffering/updated`; and
- `model/verification`.

`model/rerouted` retains its existing reducer and frontend-visible semantics.
The other six notifications use the existing bounded unmodeled typed-event
path. A typed notification may retain the complete envelope internally, but
`CodexExtensionReceived.payload` is `raw["params"]`. Frontend
`codex.extension.params` is the recursively sanitized params value, never a
complete JSON-RPC envelope nested below a field named `params`.

The one stable server request is
`account/chatgptAuthTokens/refresh`. Its decoded request carries:

- required open `reason`, currently known as `unauthorized`;
- optional-nullable prior account/workspace identifier; and
- the existing request occurrence token.

Its response carries:

- required access token;
- required ChatGPT account identifier; and
- optional-nullable plan type.

Canonical schema-complete types preserve omission, explicit null, and value.
Existing `AuthenticationRequest`, `AuthenticationResponse`, and
`Requests::respond(...)` source behavior is retained through an unambiguous
alias, view, conversion, or forwarding overload. The existing single pending
server-request registry, occurrence ownership, stale-token rejection,
connection-generation isolation, validation-before-enqueue, and exactly-once
response rule remain authoritative.

## Tagged unions

The 19 registered alternatives are:

| Union | Discriminator | Alternatives |
|---|---|---|
| `Account` | `type` | `amazonBedrock`, `apiKey`, `chatgpt` |
| `LoginAccountParams` | `type` | `apiKey`, `chatgpt`, `chatgptAuthTokens`, `chatgptDeviceCode` |
| `LoginAccountResponse` | `type` | `apiKey`, `chatgpt`, `chatgptAuthTokens`, `chatgptDeviceCode` |
| `ConfigLayerSource` | `type` | `enterpriseManaged`, `legacyManagedConfigTomlFromFile`, `legacyManagedConfigTomlFromMdm`, `mdm`, `project`, `sessionFlags`, `system`, `user` |

Request and response login unions have direction-specific alternatives and
direction-specific future values. Incoming account and login results retain
their raw JSON. Outgoing login params do not gain raw state for convenience.
A future discriminator produces an explicit unknown alternative and
`ForwardCompatibility`; a malformed known branch remains
`MalformedKnownPayload`/`ProtocolWarning`. A future open-enum value inside a
known branch preserves the outer typed alternative and emits
`UnknownEnumValue`.

## Transitive type and path closure

The audit starts from 18 request params, 18 successful results, seven
notification params, the auth-refresh request and response, and the 19
registered union alternatives. Namespace-aware traversal currently derives
40 unique definition seeds, 104 reachable named definitions, and 302 schema
paths. These counts are outputs of the checked closure traversal rather than
guessed denominators.

Every reachable definition is assigned exactly one of:

- `PublicHandwrittenType`;
- `PrivateCodecHelper`;
- `OpenStringEnum`;
- `StrongIdentifier`;
- `ProtocolDefinedOpaqueJson`; or
- `ReusedExistingType`.

The frozen definition dispositions are:

| Definition disposition | Count |
|---|---:|
| `PublicHandwrittenType` | 70 |
| `OpenStringEnum` | 26 |
| `ReusedExistingType` | 8 |
| **Total** | **104** |

Every property, array-element, and explicit map-value path records direction,
required/default/optional state, nullability, C++ owner and member,
container/wrapper shape, sensitivity, raw-retention permission, reaching
roots, and implementation batch.

| Schema-path disposition | Count |
|---|---:|
| `PublicHandwrittenType` | 47 |
| `PrivateCodecHelper` | 19 |
| `OpenStringEnum` | 2 |
| `StrongIdentifier` | 42 |
| `ProtocolDefinedOpaqueJson` | 7 |
| `ReusedExistingType` | 185 |
| **Total** | **302** |

The 272 property declarations contain 134 RequiredValue, 5
RequiredNullable, 117 OptionalNullable, 5 OptionalValue, and 11
DefaultedValue mappings. The remaining path records are 21 array elements and
9 explicit map values. Directionally, 260 paths are DecodeOnly and 42 are
EncodeOnly. Semantic JSON-object keys are independently typed: config origin
keys use `ConfigKeyPath`, feature maps use `ExperimentalFeatureId`,
permission-profile maps use `PermissionProfileName`, and rate-limit maps use
`RateLimitId`.

Arbitrary JSON is authorized only by exact pinned schema evidence. In
particular, `config/value/write.value` and
`ConfigBatchWriteParams.ConfigEdit.value` remain exact JSON values. Known
configuration objects and known fields are still typed. `additionalProperties:
true` permits separately retained unknown properties where present; it does
not permit dropping known fields or replacing `ConfigReadResponse` with a raw
JSON wrapper. Mixed known objects explicitly map those values to separate
`AnalyticsConfig::unknownProperties` and `Config::unknownProperties`
containers; pure schema maps retain their reviewed typed container members.

Open string-backed enums are used for upstream-extensible values, including
plan/auth/credential/rate-limit, reroute, model capability, merge-strategy,
configuration-source, and feature-status domains. This includes string-enum
domains expressed by the pinned schema as `oneOf` literal branches, such as
the auth-refresh reason, rather than misclassifying those as closed tagged
unions. Registered tagged-union branch fields are owned by their distinct
public alternative types, not by the containing union. `OptionalNullable<T>`
preserves omitted, explicit null, and value wherever the schema distinguishes
them. Directionally distinct aggregates are not collapsed without exact
schema evidence.

## Batches and boundaries

The dependency-ordered identity batches are:

| Batch | Scope | Identities |
|---|---|---:|
| B2 | Accounts and authentication | 24 |
| B3 | Models and providers | 5 |
| B4 | Configuration read | 11 |
| B5 | Configuration mutation and feature management | 5 |

B2 contains nine client requests, three notifications, one server request,
and eleven account/login alternatives. B3 contains two client requests and
three notifications. B4 contains `config/read`,
`configRequirements/read`, `configWarning`, and eight `ConfigLayerSource`
alternatives. B5 contains the five mutation/feature client requests.
Ordinary helper definitions are owned by the earliest dependency-closed batch
that needs them; registry identities cannot move between batches.

BackendCore receives compatibility changes only. No new `BackendCommand`,
canonical account/config/model state, frontend command, frontend event,
snapshot field, response shape, remote operation, or exposure decision is
created. Frontend Protocol v1, its machine-readable schema, and
`codex-backend-client` behavior remain byte- and semantics-identical.

The three facades and `typed::Client` preserve one-pointer PIMPL ownership.
`AppServerClient` retains its PIMPL-only layout. A1.2 does not change
SOVERSION, add public data members, or add/reorder virtual functions.

## Sensitivity policy

API keys, access tokens, account/workspace identity, device user codes while
active, authentication response payloads, and potentially credential-bearing
configuration values are marked sensitive in the path closure.

Sensitive values are never placed in diagnostics, exception messages, logs,
generated evidence, Markdown, BackendCore state, frontend snapshots/events,
or generic extension preservation. Tests assert stable codes and field paths,
not values, and use only deterministic low-risk synthetic fixture data.
Secret-bearing aggregates receive no stream or formatter support. Auth-refresh
responses remain local typed protocol responses and never become remotely
callable.

The closure records 207 NonSensitiveProtocolData, 13 AuthenticationFlowData,
41 AccountUsageData, 7 PersonallyIdentifyingAccountData, 19
PotentialCredentialBearingConfiguration, 7 WorkspaceContent, 4
AccountWorkspaceIdentity, 3 CredentialSecret, and 1
EphemeralAuthenticationCode paths. Even non-sensitive path diagnostics are
value-free; sensitive classifications additionally forbid logging and
frontend/backend preservation. Aggregate, reference, map, and array paths
inherit the strongest descendant sensitivity, so a container cannot be
marked loggable while a nested account, workspace, token, or arbitrary
configuration value is protected.

The final closure includes a repository-level exact-path synthetic sentinel
leak guard over build output, evidence, documentation, retained test logs, and
package staging. Only fixture source files that intentionally contain a
synthetic input are allow-listed; indexes may name fixture paths but may not
duplicate values.

The Commit 1 fixture plan contains 65 required positive roles: 18 request
params, 18 client results, 7 notification envelope-and-params cases, the auth
request envelope, auth params and response, and 19 union alternatives. Forty
roles already exist at the merged base and 25 are planned. The closure derives
229 required-field, 406 wrong-type, 155 optional-omission, and 141 nullable
tri-state path obligations without inspecting C++ decoder behavior. These
totals are identity-specific: a registered union alternative includes only
its own branch paths and transitive dependencies, while an operation root
that accepts the whole union still covers every branch.

The base roles are fixture seeds, not a claim of complete coverage. Each
identity additionally records 229 required-property-present, 155
optional-present, 155 optional-omitted, 416 schema-valid nullable-state, 17
defaulted-path, 27 integer-boundary, 8 authorized opaque-JSON, 27
empty/non-empty collection, 156 known open-enum-value, and 140 registered
union-alternative obligations. These totals intentionally count an obligation
again when more than one identity independently reaches it. Every seed and
expansion must pass the independent Draft-07 path before its identity can be
Complete.

## Closure target

At the end of B5, the exact A1.2 ratchet must be:

```text
A1.2 Complete:          45
A1.2 Partial:            0
A1.2 NotImplemented:     0
```

Commit 6 is closure and verification only. Missing runtime types, handlers,
codecs, descriptors, or facade operations must be repaired in their owning
B2-B5 commit rather than added during closure.
