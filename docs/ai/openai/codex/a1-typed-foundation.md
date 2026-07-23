# Codex A1 typed foundation

Phase A1 types the stable Codex App Server `0.144.6` protocol by domain. It
does not expose experimental-only operations and it does not change Frontend
Protocol v1.

## A1.0 review boundaries

A1.0 is delivered by one pull request with two independently reviewable
commits:

1. `Establish offline Codex operation evidence` adds authoritative operation
   contracts, the frozen domain assignment, independent schema validation,
   and deterministic evidence fixtures.
2. `Introduce Codex typed foundation and migrate existing surface` adds the
   grouped typed client, structured decode diagnostics, complete
   `CodexErrorInfo`, and migrates the existing typed surface without changing
   its wire behavior.

The first commit contains no grouped public client facade or
`CodexErrorInfo` implementation. Checking it out independently must still
configure, build, test, install, and package.

## Offline operation authority

The vendored JSON Schema identifies client request methods and parameter
types, but it does not associate those requests with successful result types.
The request/result association is therefore extracted offline from the exact
upstream `client_request_definitions!` declaration at release
`rust-v0.144.6`, source commit
`5d1fbf26c43abc65a203928b2e31561cb039e06d`.

Only the minimal authoritative Rust file is vendored:

```text
codex-rs/app-server-protocol/src/protocol/common.rs
```

Its provenance records the original path, byte length, Git blob identity, and
SHA-256 digest. For this pin those values are 140,314 bytes, Git blob
`27a8b0205da7998b91edca9b94733b1d3a0b92e3`, and SHA-256
`5679cbc5b3e935a2edd69fbe62b82b956eff4d5d050c9969b090ccc9d367fdee`.
The bytes are not edited locally. Exact upstream Apache-2.0 `LICENSE` and
`NOTICE` files accompany the source evidence.

The extractor parses the macro declaration and invocation; it does not infer
results from filenames or matching type names. Its stable join is exact and
bidirectional:

```text
stable client request associations: 87/87
Concrete successful results:          66
Unit successful results:              21
```

The three non-experimental Rust declarations absent from the stable JSON
Schema (`getAuthStatus`, `getConversationSummary`, and `gitDiffToRemote`) stay
outside stable evidence.

The ten stable server requests have uniquely paired parameter and response
schemas. Their authority is the vendored schema tree, not Rust:

```text
stable server request associations: 10/10
Concrete successful responses:      10
```

Missing, duplicate, stale, ambiguous, category-mismatched, or conflicting
associations fail generation.

Across both directions, the combined 97-operation contract set is 76
`Concrete` and 21 explicit `Unit` results.

### TypeScript audit

The exact pinned stable and experimental TypeScript generations were audited.
`ClientRequest.ts` associates method and parameter types. Response types are
exported as separate files, but there is no generated `ClientResponse.ts` or
other request-to-response mapping. TypeScript therefore supplies no
independent result association and is not vendored in bulk. The negative audit
and the already pinned tree hashes remain guarded:

```text
stable tree:        598 files, 73c95b6a19d5559939519a771e0f7285cc60bbfaa1aac8bd9c52ba308c6e6811
experimental tree:  671 files, d561ef0b4ef8a921fff50de4e3c662a0fdff643e82868f2b05a3e71f912aec8c
```

The negative audit is reproducible when the exact trees are available:

```sh
python3 tools/codex/app_server_contracts.py --check \
  --ts-stable /tmp/snodec-codex-ts-stable \
  --ts-experimental /tmp/snodec-codex-ts-experimental
```

The scanner rechecks the complete tree and path-set hashes, locates every
client-method literal, rejects response/result fields in method unions, and
searches for a `ClientResponse` declaration. It also reviews every file that
co-references a known client parameter type and a response-named type, broader
Request/Response/Result/Map names, and conditional or mapped association
constructs. In both pins every client method literal occurs only in
`ClientRequest.ts`; the only parameter/response co-references are the two flat
generated export indexes. The five broader names are server-request response
types and carry no client association. This is a recorded, reproducible audit
of the exact trees, not a TypeScript-derived result authority.

## One canonical registry

`detail::ProtocolSurfaceRegistry` remains the sole local authority. Its
existing rows are extended with:

- parameter and successful result/response type identities;
- `Concrete`, `Unit`, `Nullable`, `ProtocolSpecial`, `Unresolved`, and
  `NotApplicable` result contract kinds;
- authoritative evidence kind and exact evidence key;
- typed module and fixed A1 slice;
- mechanically derived schema-completeness evidence.

Offline association artifacts contain upstream protocol facts only. They do
not carry independent runtime, BackendCore, canonical-state, frontend, or
security dispositions. Guards compare evidence and registry in both
directions and reject prohibited disposition fields recursively, including
inside schema-pair, Rust cross-check, fixture, and reachability records.
Association validation reparses the exact vendored Rust macros and loads the
referenced vendored parameter/result schema roots independently. It checks
source variants and raw type declarations, schema existence and titles,
schema-pair branches, and schema-derived result-contract semantics, so a
coordinated edit to evidence and registry does not become self-authorizing.

Stable diagnostics distinguish missing, duplicate key or evidence identity,
stale, wrong-category, wrong-parameter, wrong-result, conflicting-evidence,
invalid unit/concrete, non-request, and experimental-as-stable failures.
Negative tests require the exact diagnostic-code multiset and reject
collateral diagnostics.

## Frozen A1 slices

Every one of the 387 A0 identities is assigned exactly once:

- A1.0: common operation infrastructure, initialization, cross-cutting decode
  diagnostics, and `CodexErrorInfo`;
- A1.1: threads, turns, sessions, their items and notifications, and types
  transitively required by those roots;
- A1.2: accounts, models, configuration, config requirements, and stable
  `experimentalFeature/*` operations;
- A1.3: commands, filesystem, reviews, approvals, guardian and permission
  profiles, and stable fuzzy-file search;
- A1.4: skills, plugins, MCP, marketplaces, apps, hooks, feedback, external
  agent configuration, Windows sandbox/attestation, and the explicitly
  reported stable long tail;
- InventoryOnly: experimental-only identities plus stable identities proven
  unreachable from every stable public root.

The public-root classifier has no stable catch-all. Its 49 reviewed,
category-specific A1.4 public roots cover the pinned manifest exactly, and a
future unrecognized stable root fails with
`A1_SLICE_UNRECOGNIZED_STABLE_ROOT` until its ownership is reviewed.

Reachability starts at stable request params/results, notifications, server
request params/responses, `ThreadItem`, and `ResponseItem`. A nested tagged
union is assigned to the earliest fixed slice whose stable root reaches it.
All reaching roots are recorded; shared/common and stable-unreachable
inventory classifications are explicit. `CodexErrorInfo` is the deliberate
cross-cutting A1.0 exception.

The guarded assignment contains:

```text
A1.0             19
A1.1            151
A1.2             45
A1.3             68
A1.4             56
InventoryOnly    48  (36 experimental-only and 12 stable-unreachable)
```

The twelve stable-unreachable exact identities are the `environment`
alternative of `CapabilityRootLocation`; `agent`, `command`, and `prompt` of
`ConfiguredHookHandler`; `function` of `DynamicToolNamespaceTool`;
`function` and `namespace` of `DynamicToolSpec`; `custom`,
`explicitRequestOnly`, and `proactive` of `MultiAgentMode`; and `webrtc` and
`websocket` of `ThreadRealtimeStartTransport`. They remain explicit
`StableUnreachableInventory` entries with zero-root evidence.

## Mechanical schema completeness

The registry enum is not sufficient to claim completeness. `Complete` is
derived only when all applicable evidence is present:

- an authoritative root association and current positive fixture;
- every required and represented schema property exercised;
- optional-present and optional-omitted cases exercised, plus distinct
  nullable null/value/omitted cases where applicable;
- every reachable known tagged-union alternative exercised;
- direction-appropriate encode/decode or response-helper assertions;
- production target/decoder identity matched to the registry;
- every intentionally opaque JSON field declared;
- independent schema validation;
- no known field silently dropped.

Outgoing parameters require encode semantics, incoming results/events require
decoded mapped fields plus raw retention, server responses must validate after
encoding, and protocol-opaque JSON requires exact raw retention. Unsatisfied
implemented identities are `Partial`; unimplemented and inventory-only rows
remain `NotImplemented` or `NotApplicable`. A1.0 does not claim that the
existing partial thread/turn/item models are schema-complete.

At the first commit boundary the mechanically derived metrics are:

```text
Complete            0
Partial             34
NotImplemented     305
NotApplicable       48
```

The generated schema-completeness report contains schema/fixture facts only.
Runtime target matching, direction assertions, opaque-field declarations, and
no-dropped-field evidence remain implementation-owned facts in the production
registry.

## Offline Draft-07 subset

`tools/codex/draft07.py` is independent of the production C++ codecs. It
supports and tests:

```text
$ref definitions type required properties additionalProperties items
enum const oneOf anyOf allOf not minimum maximum
minItems maxItems minLength maxLength pattern
```

Annotations such as `$schema`, `default`, `description`, `format`, and `title`
are accepted without being treated as validation semantics. Any unsupported
schema-position keyword fails loudly. `$id` is deliberately unsupported
because it changes reference-resource scope; the pin contains no `$id`.
References are document-local because all references in the pin are
document-local.

`oneOf` diagnostics report the instance path, schema path, and exact matching
branch set. Isolated self-tests cover valid and invalid objects, recursive
references, zero/multiple branch matches, combinators, additional properties,
array items, bounds, patterns, unsupported keywords, cycles, and invalid
references.

## Deterministic evidence fixtures

The checked-in fixture generator consumes only the vendored schemas, pinned
surface manifest, guarded operation contracts, and reviewed deterministic
synthesis rules. Those rules include the exact existing-34 fixture scope and
the exhaustive slice routing. The generator derives the frozen assignment and
reachability report first, then uses that assignment while producing fixture
evidence. A focused test independently compares the reviewed 34-scope rule
with the non-generated C++ ratchet; the generator itself never reads a
production decoder or test header.

The A1.0 evidence corpus covers every stable client request parameter and
result root, every stable server-request parameter and response root, roots
corresponding to the existing 34 typed identities where present, and
representative nested unions. Fixtures prove authoritative schema evidence;
they do not by themselves claim that a public codec is complete.

The first-commit corpus has 289 indexed cases: 278 positive and eleven
negative.
The 194 operation-role positives are exactly 87 client params, 87 client
results, 10 server-request params, and 10 server-request responses. It also
contains 34 existing-runtime identity fixtures and all 50 stable
`CodexErrorInfo`, `ThreadItem`, and `ResponseItem` union branches used by this
foundation.

Negative evidence mutates all four method envelopes (`ClientRequest`,
`ClientNotification`, `ServerNotification`, and `ServerRequest`) to future
methods, mutates known discriminators in `CodexErrorInfo`, `ThreadItem`, and
`ResponseItem` to future values, and exercises malformed-known and nested
invalidity across those union families.

Generation, committed-output checking, and validation are separate offline
modes. Two generations must be byte-identical. Every positive fixture passes
the independent validator; every required field is removed in turn, and every
required value receives a wrong-type mutation attempt. Edited, missing, extra,
stale, wrong-identity, or wrong-provenance fixtures fail.

Synthetic values are path-derived and contain no real credentials, tokens,
accounts, or user content. Recorded App Server transcripts remain useful
supplemental regression inputs, but they are not the primary fixture
authority.

Across the positives, generation finds 1,261 selected-branch required
locations and proves every removal invalid against the exact schema fragment
selected during fixture generation. One removal remains valid against the
full root by legitimately selecting another `anyOf` branch; its evidence
records both that root result and the selected-fragment rejection rather than
excluding the mutation. The corpus also proves 1,248 wrong-type replacements
invalid. Thirteen intentionally unconstrained protocol-JSON values are
explicit exclusions because their schemas accept every JSON type candidate.
The same independent mutation pass exercises 1,061 optional properties in
both present and omitted forms; every omission remains schema-valid.

## Generated review reports

The checked-in, reproducibly guarded reports under
`tools/codex/app-server-evidence/0.144.6/` are:

- `operation-contracts.json`, the combined 87 client request/result and ten
  server request/response association report;
- `typescript-audit.json`, the negative independent-mapping audit;
- `module-slice-assignment.json`, one assignment for each of the 387 A0
  identities;
- `nested-reachability.json`, the stable-root reachability graph and explicit
  shared/unreachable classifications;
- `schema-completeness-evidence.json`, schema/fixture facts for all 387
  identities without any self-asserted runtime status;
- `fixture-coverage.json`, the 289-case corpus coverage and mutation summary;
- `schema-keywords.json`, validating-keyword use and the supported/unsupported
  audit.

Vendored Rust file provenance is independently recorded in
`app-server-protocol-source/0.144.6/PROVENANCE.json`; fixture paths and hashes
are indexed in `app-server-fixtures/0.144.6/index.json`. Generated indexes,
reports, and descriptors identify their generator and say not to edit them.
Individual fixture payloads remain exact schema-valid JSON and are identified
as generated through the guarded index rather than an extra payload field.

## Packaging and ABI boundary

Source packages retain the Rust evidence, schema trees, provenance,
extractors, validator, generator, reports, fixtures, tests, and documentation.
Installed SDKs, binary/runtime packages, and component installs exclude them,
along with private registry data and generated private descriptors.
An archive-level source-package test verifies the retained reproducibility
inputs. The staged installed-consumer test verifies that none of those private
inputs or generated reports enters an installed or binary package.

A1 is one deliberate in-progress rebuild boundary for public typed variant
changes. `AppServerClient` remains a one-`Impl`-pointer installed object.
SOVERSION remains `1` in A1.0 and is bumped once at A1 closure in A1.4. This
document does not claim binary compatibility for public types changed during
A1.

## Later slices

- A1.1 completes threads, turns, sessions, and their transitively reachable
  item/response unions.
- A1.2 completes accounts, models, and configuration.
- A1.3 completes commands, filesystem, reviews, and approvals.
- A1.4 completes the remaining stable domains, verifies full-depth union
  coverage, and closes the A1 rebuild/SOVERSION decision.

A2 BackendCore/canonical-state modeling, A3 frontend exposure, and Qt work are
not part of A1.0.
