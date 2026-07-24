# Codex App Server census tooling

Phase A0 pins the App Server surface emitted by the exact locally installed
`codex-cli 0.144.6` binary. The starting SNode.C commit is
`138f5022c19b24847bee42a21242aaaf7dde5a04`.

The authoritative vendored inputs are:

- `app-server-schema/0.144.6/stable/`;
- `app-server-schema/0.144.6/experimental/`; and
- `app-server-schema/0.144.6/PROVENANCE.json`.

Ordinary builds and tests consume these files. They do not invoke Codex, use
the network, or require an account, credentials, or quota.

## Upstream provenance and attribution

The authority executable is byte-identical to the executable in OpenAI
Codex's official `rust-v0.144.6` Linux release asset. The annotated release
tag resolves to source commit
`5d1fbf26c43abc65a203928b2e31561cb039e06d`. The tag and commit are unsigned,
so this record does not claim Git signature verification. The exact executable
and release-archive hashes, tag object, source commit, and verification method
are pinned in `PROVENANCE.json`.

The tagged upstream source is licensed as `Apache-2.0` and includes a NOTICE.
Byte-exact copies are retained as `LICENSE.openai-codex` and
`NOTICE.openai-codex` beside the schemas; their sizes, SHA-256 values, Git blob
identities, and source URLs are guarded by the offline provenance test. The
generated schema files remain unmodified.

## Offline verification and reproduction

Run these commands from the repository root:

```sh
SCHEMA_ROOT=tools/codex/app-server-schema/0.144.6
PROVENANCE="$SCHEMA_ROOT/PROVENANCE.json"
SURFACE=tools/codex/app-server-surface/0.144.6.json
REGISTRY=src/ai/openai/codex/detail/ProtocolSurfaceRegistryData.inc
SOURCE_ROOT=tools/codex/app-server-protocol-source/0.144.6
EVIDENCE_ROOT=tools/codex/app-server-evidence/0.144.6
FIXTURE_ROOT=tools/codex/app-server-fixtures/0.144.6

python3 tools/codex/app_server_surface.py verify \
  --schema-root "$SCHEMA_ROOT" \
  --provenance "$PROVENANCE" \
  --manifest "$SURFACE"

python3 tools/codex/app_server_surface.py extract \
  --schema-root "$SCHEMA_ROOT" \
  --output /tmp/snodec-codex-surface-a.json
python3 tools/codex/app_server_surface.py extract \
  --schema-root "$SCHEMA_ROOT" \
  --output /tmp/snodec-codex-surface-b.json
cmp /tmp/snodec-codex-surface-a.json /tmp/snodec-codex-surface-b.json
cmp /tmp/snodec-codex-surface-a.json "$SURFACE"

python3 tools/codex/app_server_surface.py registry \
  --manifest "$SURFACE" \
  --evidence-root "$EVIDENCE_ROOT" \
  --output "$REGISTRY" \
  --check

python3 tools/codex/app_server_surface.py operation-descriptors \
  --manifest "$SURFACE" \
  --evidence-root "$EVIDENCE_ROOT" \
  --output src/ai/openai/codex/detail/ClientOperationCodecDescriptors.inc \
  --check

python3 tools/codex/app_server_surface.py operation-production-coverage \
  --manifest "$SURFACE" \
  --evidence-root "$EVIDENCE_ROOT" \
  --fixture-index "$FIXTURE_ROOT/index.json" \
  --repo-root . \
  --output "$EVIDENCE_ROOT/a1-1-operation-production-coverage.json" \
  --check

python3 tools/codex/app_server_surface.py notification-descriptors \
  --manifest "$SURFACE" \
  --evidence-root "$EVIDENCE_ROOT" \
  --output src/ai/openai/codex/detail/ServerNotificationCodecDescriptors.inc \
  --check

python3 tools/codex/app_server_surface.py notification-production-coverage \
  --manifest "$SURFACE" \
  --evidence-root "$EVIDENCE_ROOT" \
  --fixture-index "$FIXTURE_ROOT/index.json" \
  --repo-root . \
  --output "$EVIDENCE_ROOT/a1-1-notification-production-coverage.json" \
  --check

python3 tools/codex/app_server_surface.py conversation-descriptors \
  --manifest "$SURFACE" \
  --schema-root "$SCHEMA_ROOT" \
  --evidence-root "$EVIDENCE_ROOT" \
  --output src/ai/openai/codex/detail/ConversationUnionCodecDescriptors.inc \
  --check

python3 tools/codex/app_server_surface.py item-descriptors \
  --manifest "$SURFACE" \
  --schema-root "$SCHEMA_ROOT" \
  --evidence-root "$EVIDENCE_ROOT" \
  --thread-output src/ai/openai/codex/detail/ThreadItemCodecDescriptors.inc \
  --response-output src/ai/openai/codex/detail/ResponseItemCodecDescriptors.inc \
  --check

python3 tools/codex/app_server_surface.py docs \
  --manifest "$SURFACE" \
  --registry "$REGISTRY" \
  --provenance "$PROVENANCE" \
  --coverage-output docs/ai/openai/codex/app-server-api-coverage.md \
  --security-output docs/ai/openai/codex/app-server-security-decisions.md \
  --check
```

`verify` treats a missing, added, or hash-mismatched vendored schema as an
error. It also treats missing, altered, or mismatched upstream attribution as
an error. Extraction resolves repository-local references, rejects an unknown
schema layout, scans both the combined and standalone v2 aggregate definitions,
and compares the resulting manifest exactly. The two extraction runs
demonstrate deterministic output independently of the upstream generator's
object-member order.

## A1.0 offline contracts and fixtures

Client request/result associations are not present in the generated JSON
Schema. A1.0 therefore retains the exact authoritative upstream Rust
`client_request_definitions!` source at release `rust-v0.144.6`, source commit
`5d1fbf26c43abc65a203928b2e31561cb039e06d`. The extractor verifies every
vendored byte and derives all 87 stable client associations from that macro.
It derives the ten stable server-request parameter/response pairs from the
vendored schema tree, which is authoritative for those pairs.

Run the complete offline checks from the repository root:

```sh
SCHEMA_ROOT=tools/codex/app-server-schema/0.144.6
PROVENANCE="$SCHEMA_ROOT/PROVENANCE.json"
SURFACE=tools/codex/app-server-surface/0.144.6.json
SOURCE_ROOT=tools/codex/app-server-protocol-source/0.144.6
EVIDENCE_ROOT=tools/codex/app-server-evidence/0.144.6
FIXTURE_ROOT=tools/codex/app-server-fixtures/0.144.6

python3 tools/codex/app_server_contracts.py \
  --source-root "$SOURCE_ROOT" \
  --schema-root "$SCHEMA_ROOT" \
  --manifest "$SURFACE" \
  --schema-provenance "$PROVENANCE" \
  --evidence-root "$EVIDENCE_ROOT" \
  --check

python3 tools/codex/app_server_fixtures.py check \
  --schema-root "$SCHEMA_ROOT" \
  --manifest "$SURFACE" \
  --contracts "$EVIDENCE_ROOT/operation-contracts.json" \
  --fixture-root "$FIXTURE_ROOT" \
  --evidence-root "$EVIDENCE_ROOT"

python3 tools/codex/app_server_fixtures.py validate \
  --schema-root "$SCHEMA_ROOT" \
  --manifest "$SURFACE" \
  --contracts "$EVIDENCE_ROOT/operation-contracts.json" \
  --fixture-root "$FIXTURE_ROOT" \
  --evidence-root "$EVIDENCE_ROOT"
```

`generate` replaces `check` only when intentionally refreshing generated
artifacts after reviewing an authoritative-input or deterministic-rule
change. The fixture tool uses `draft07.py`, not a production decoder. Its
validation mode checks every positive fixture, required-field removals,
wrong-type mutations, discriminator mutations, and committed-output identity.
Neither tool invokes Codex, accesses the network, or reads credentials.

The TypeScript audit found method-to-parameter mapping in `ClientRequest.ts`
but no independent request-to-response association. The generated TypeScript
tree is consequently not vendored in bulk; its pinned hashes and the
reproducible negative audit remain checked in. After regenerating the exact
trees, reproduce the full-tree scanner with:

```sh
python3 tools/codex/app_server_contracts.py --check \
  --ts-stable /tmp/snodec-codex-ts-stable \
  --ts-experimental /tmp/snodec-codex-ts-experimental
```

The scanner anchors complete tree and path-set hashes, exact client-method
literals, parameter/response type co-references, broader association-like
names, and conditional/mapped constructs. Its only co-reference findings in
this pin are flat generated export indexes.

## A1.1 conversation-domain audit

The A1.1 audit derives its implementation plan and complete transitive
schema-definition closure from the pinned A1.0 assignment, reachability,
operation-contract, completeness, fixture, schema, and canonical-registry
inputs. Run it from the repository root using only the checked-in offline
inputs:

```sh
python3 tools/codex/app_server_a1_1.py generate
python3 tools/codex/app_server_a1_1.py check
python3 tools/codex/app_server_a1_1_closure.py generate
python3 tools/codex/app_server_a1_1_closure.py check
```

Commit 1 created
`app-server-evidence/0.144.6/a1-1-start-state.json` once with
`freeze-start-state --base-sha a226e3df5efd55b5dbef12cb1760674388909d7c`.
That mode accepts only the reviewed base SHA and refuses to overwrite an
existing snapshot unless `--force-start-state` is supplied. The force option
is reserved for the reviewed initial freeze or correction of that same
commit; it is not an ordinary regeneration command.

The start-state is immutable review evidence for the exact 26 Partial / 125
NotImplemented A1.1 split and the starting global metrics. Ordinary
`generate` and `check` read both that baseline and the live registry,
schema-completeness evidence, and fixture coverage. They keep the audit's
starting fields anchored to the baseline while permitting implementation and
completeness to advance monotonically. Static identity drift, status or layer
demotion, invalid runtime promotion, loss of completeness facts, and fixture
coverage regression fail with stable intrinsic diagnostic codes.

`generate` intentionally refreshes:

- `app-server-evidence/0.144.6/a1-1-implementation-plan.json`; and
- `app-server-evidence/0.144.6/a1-1-type-closure.json`.

`check` regenerates the same documents in memory and rejects missing, stale,
duplicate, mismatched, unassigned, or cyclic evidence. Ordinary builds and CI
use `check`; neither mode invokes Codex, accesses the network, or requires
credentials.

After B5, `app_server_a1_1_closure.py` projects the live canonical registry
into `a1-1-closure-report.json`. Its exact identity-list ratchet requires all
151 A1.1 rows to be stable, typed, implemented, mechanically Complete, and
bound to a production target. It also checks the exact 167/8/164/48 global
metrics, the cumulative B2/B3/B4/B5 identity sets, the eight residual Partial
identities, operation/notification/item/union documentation lists, fixture
and type-closure totals, Frontend Protocol v1 byte identity, and fingerprints
of the frontend and two reference-application source boundaries. The report
is closure evidence only; all production dispositions still come from
`ProtocolSurfaceRegistry`.

The implementation-plan denominator is exactly 151 registry identities:

| A1.1 registry category | Count |
|---|---:|
| Client-request methods | 22 |
| Server-notification methods | 37 |
| `ThreadItem` discriminators | 18 |
| `ResponseItem` discriminators | 16 |
| Nested tagged-union discriminators | 58 |
| **Total** | **151** |

Each client request also has one paired successful-result schema, producing
22 result implementation obligations. These result roots are not registry
identities and are never added to the 151 denominator.

The 100 unique schema roots reach exactly 164 named definitions: 19 registered
item or nested-union definitions and 145 ordinary definitions. Every reachable
definition and nested schema position has one reviewed disposition:
`PublicHandwrittenType`, `PrivateCodecHelper`, `OpenStringEnum`,
`StrongIdentifier`, `ProtocolDefinedOpaqueJson`, or `ReusedExistingType`.
Known objects cannot be treated wholesale as JSON merely for convenience.

The named-definition dispositions are 127 handwritten public types, 26 open
string enums, ten reused types, and one strong identifier. Their derived
directions are 29 encode-only, 121 decode-only, and 14 bidirectional. The 698
schema-path mappings are 107 encode-only, 554 decode-only, and 37
bidirectional; they include 156 exact strong-ID positions and the complete
4/413/5/15/218 defaulted/required-value/required-nullable/optional-value/
optional-nullable presence matrix. Cross-slice ownership is 122 A1.1-local definitions, 39
A1.1-owned definitions reused by later slices, and the exact three prior-slice
reuses `CodexErrorInfo`, `NonSteerableTurnKind`, and `TurnError`.

The dependency-closed implementation partition is:

| Batch | Registry identities | Result obligations | Closure definitions |
|---|---:|---:|---:|
| B2: shared union and value foundations | 26 | 0 | 12 |
| B3: item models | 50 | 0 | 32 |
| B4: thread and turn operations | 38 | 22 | 72 |
| B5: notifications and preservation | 37 | 0 | 48 |
| **Total** | **151** | **22** | **164** |

The operation-contract regression test independently copies the current
`thread/archive` Unit successful-response schema, adds a property, proves its
schema-derived contract becomes `Concrete`, and requires the reviewed
`EXPECTED_UNIT_RESULT_METHODS` ratchet to fail with its exact changed-identity
diagnostic. The guard deliberately does not impose the false inverse that a
Concrete result must have direct properties.

The checked `a1-1-operation-production-coverage.json` table binds the exact 22
operation descriptors to 1,166 indexed successful-result records (403
accepted and 763 malformed-known), 73 aggregate/value records, 60 B4 nested
union records, and four `ThreadActiveFlag` records. The result and
aggregate/value corpora traverse the same structured production result
dispatcher used by `typed::Threads` and `typed::Turns`; that dispatcher selects
its result codec from the generated private descriptor binding. The table
also hashes every production/test source it claims to cover. It is reproducible
review evidence, not a second runtime disposition or dispatch registry.

## A1.2 accounts, models, and configuration audit

The A1.2 entry point reuses the shared A1 evidence loader, registry-transition
checks, schema walker, type-mapping mechanics, and fixture-obligation
derivation. Slice-specific policy remains data in the A1.2 audit rather than a
copy of the A1.1 generator:

```sh
python3 tools/codex/app_server_a1_2.py generate
python3 tools/codex/app_server_a1_2.py check
python3 tools/codex/app_server_a1_2_closure.py generate
python3 tools/codex/app_server_a1_2_closure.py check
```

The checked start state, implementation plan, and full transitive type/path
closure are:

- `app-server-evidence/0.144.6/a1-2-start-state.json`;
- `app-server-evidence/0.144.6/a1-2-implementation-plan.json`; and
- `app-server-evidence/0.144.6/a1-2-type-closure.json`;
- `app-server-evidence/0.144.6/a1-2-closure-report.json`.

The audit derives the exact 45-identity denominator, 18 successful client
result obligations plus the auth-refresh response obligation, four dependency
batches, schema-authorized opaque JSON paths, and sensitivity policies from
the pinned offline inputs. It does not write the production registry or make
runtime dispatch decisions. The reviewed counts, public API map, and boundary
decisions are documented in
`docs/ai/openai/codex/a1-2-accounts-models-configuration.md`.

The thin final-closure entry point rebuilds the audit reports in memory and
projects their exact 45 identities into one checked ratchet. It requires all
45 rows to be stable, typed, implemented, mechanically Complete, and bound to
the reviewed production target. It also locks the 18 client-result and one
server-response obligations, 18/7/1/19 taxonomy, 26/11/8 classification,
global 212/6/121/48 metrics, residual Partial list, closure dispositions,
fixture/mutation totals, and frontend/application boundary fingerprints. The
report remains evidence-side metadata; the production registry remains the
only runtime authority.

## Re-running the pinned upstream generation

The TypeScript trees are an independent method and discriminator cross-check;
they are not vendored. Recreate all six staging trees with the pinned binary:

```sh
CODEX_BIN="${CODEX_BIN:-codex}"
"$CODEX_BIN" --version

rm -rf /tmp/snodec-codex-schema-stable-a \
       /tmp/snodec-codex-schema-stable-b \
       /tmp/snodec-codex-schema-experimental-a \
       /tmp/snodec-codex-schema-experimental-b \
       /tmp/snodec-codex-ts-stable \
       /tmp/snodec-codex-ts-experimental

"$CODEX_BIN" app-server generate-json-schema \
  --out /tmp/snodec-codex-schema-stable-a
"$CODEX_BIN" app-server generate-json-schema \
  --out /tmp/snodec-codex-schema-stable-b
"$CODEX_BIN" app-server generate-json-schema \
  --out /tmp/snodec-codex-schema-experimental-a \
  --experimental
"$CODEX_BIN" app-server generate-json-schema \
  --out /tmp/snodec-codex-schema-experimental-b \
  --experimental

"$CODEX_BIN" app-server generate-ts \
  --out /tmp/snodec-codex-ts-stable
"$CODEX_BIN" app-server generate-ts \
  --out /tmp/snodec-codex-ts-experimental \
  --experimental
```

The version line must be exactly `codex-cli 0.144.6`. Do not substitute a web
schema, remembered list, or differently versioned generator. One first
generation was vendored byte-for-byte; no generated schema was normalized or
hand-edited. For this pin, duplicate stable and experimental generations are
semantically equal but can differ in the member order of `/definitions` in
`codex_app_server_protocol.v2.schemas.json`. The exact observed orders are
recorded in `PROVENANCE.json`.

With those staging trees present, reproduce the generation comparison,
per-file and aggregate hashes, and TypeScript cross-check evidence:

```sh
python3 tools/codex/app_server_surface.py provenance \
  --schema-root tools/codex/app-server-schema/0.144.6 \
  --stable-first /tmp/snodec-codex-schema-stable-a \
  --stable-second /tmp/snodec-codex-schema-stable-b \
  --experimental-first /tmp/snodec-codex-schema-experimental-a \
  --experimental-second /tmp/snodec-codex-schema-experimental-b \
  --ts-stable /tmp/snodec-codex-ts-stable \
  --ts-experimental /tmp/snodec-codex-ts-experimental \
  --output /tmp/snodec-codex-PROVENANCE.json

python3 tools/codex/app_server_surface.py extract \
  --schema-root tools/codex/app-server-schema/0.144.6 \
  --ts-stable /tmp/snodec-codex-ts-stable \
  --ts-experimental /tmp/snodec-codex-ts-experimental \
  --output /tmp/snodec-codex-surface-with-ts.json
```

Because the upstream ordering variation is real, a later provenance run may
record a different ordering-only permutation. It must not silently normalize
or replace the exact checked-in generation evidence. A deliberate version-pin
update requires a new version directory plus review of the tool's pinned
version, starting SHA, local dispositions, generated manifest, registry data,
coverage report, and security worksheet.

## Generated and reviewed sources

Do not hand-edit these generated artifacts:

- the two vendored upstream schema trees;
- `PROVENANCE.json`;
- `app-server-surface/0.144.6.json`;
- `app-server-protocol-source/0.144.6/PROVENANCE.json`;
- `app-server-evidence/0.144.6/operation-contracts.json`;
- `app-server-evidence/0.144.6/typescript-audit.json`;
- `app-server-evidence/0.144.6/module-slice-assignment.json`;
- `app-server-evidence/0.144.6/nested-reachability.json`;
- `app-server-evidence/0.144.6/schema-completeness-evidence.json`;
- `app-server-evidence/0.144.6/fixture-coverage.json`;
- `app-server-evidence/0.144.6/schema-keywords.json`;
- `app-server-evidence/0.144.6/a1-1-start-state.json` (one-time immutable
  Commit 1 freeze; never ordinary regeneration);
- `app-server-evidence/0.144.6/a1-1-implementation-plan.json`;
- `app-server-evidence/0.144.6/a1-1-type-closure.json`;
- `app-server-evidence/0.144.6/a1-1-closure-report.json`;
- `app-server-evidence/0.144.6/a1-1-operation-production-coverage.json`;
- `app-server-evidence/0.144.6/a1-1-notification-production-coverage.json`;
- `app-server-evidence/0.144.6/a1-2-implementation-plan.json`;
- `app-server-evidence/0.144.6/a1-2-type-closure.json`;
- `app-server-evidence/0.144.6/a1-2-closure-report.json`;
- `app-server-fixtures/0.144.6/`;
- `ClientOperationCodecDescriptors.inc`;
- `ConversationUnionCodecDescriptors.inc`;
- `ThreadItemCodecDescriptors.inc`;
- `ResponseItemCodecDescriptors.inc`;
- `ProtocolSurfaceRegistryData.inc`;
- `docs/ai/openai/codex/app-server-api-coverage.md`; or
- `docs/ai/openai/codex/app-server-security-decisions.md`.

`app_server_surface.py` mechanically discovers the upstream inventory. It also
contains the explicit, reviewable mapping from discovered entries to current
local runtime and architectural dispositions. The private C++ registry
machinery and production dispatch are reviewed implementation source; the
generated `.inc` is their single inventory data source. The three architectural
documents `typed-api.md`, `backend-core.md`, and `frontend-protocol-v1.md` are
maintained prose, not census authorities.

Registration is not implementation. Raw-preserved, opaque-preserved,
unsupported, deferred, and owner-decision-required entries do not count as
typed, BackendCore, canonical-state, or frontend support.

## Typed-coverage no-regression floor

`tests/component/codex/CodexTypedSurfaceBaseline.h` is an intentionally
reviewed, non-generated floor containing the 34 exact stable typed identities
implemented at A0: seven client requests, one client notification, fourteen
server notifications, four server requests, and eight `ThreadItem`
discriminators. A1.0 adds the separate reviewed
`CodexErrorInfoTypedSurfaceBaseline.h`, containing the 16 exact stable
`CodexErrorInfo` discriminator identities. The coverage guard requires the
current implemented stable typed identity set to equal the union of both
headers, so the A1.0 no-regression floor and implementation ceiling are both
exactly 50 identities. This strict equality prevents unreviewed identities
from crossing the A1.0 boundary.

Each later A1 domain slice must advance the exact reviewed baseline and its
corresponding equality guard in the same change. Decreasing coverage,
replacing a locked identity, or adding an identity outside its assigned slice
requires an explicit reviewed baseline change and an explanation. The guard
is an identity ratchet, not a count-only completeness claim: at A1.0 the 16
error identities are mechanically `Complete`, while the original 34 remain
`Partial`.

## Packaging policy

The schema trees and A0/A1 support artifacts are source and test inputs.
Explicit CMake install rules and CPack binary packages exclude the schemas,
vendored Rust, provenance, tools, manifest, fixtures, evidence reports, and
private registry headers/data. `StagedInstalledConsumerTest` recursively
guards that installed/binary boundary.

CPack source packages retain the vendored schemas and Rust evidence,
attribution, manifest, tools, generated evidence and fixture corpus, private
registry sources, tests, and documentation so all offline checks remain
self-consistent. Local build trees, Python caches, VCS data, and
execution-environment metadata are excluded. `CodexSourcePackageTest`
generates a real TGZ source archive and verifies both retention and exclusion.
CMake also exposes the ordinary `package_source` target; there is no separate
`dist` target.

## Phase sequencing

A0 establishes the pinned census, canonical runtime registry, guards, metrics,
and owner worksheet only. After the owner freezes module grouping and security
decisions:

1. A1 implements the frozen stable typed App Server API.
2. A2 adds transport-neutral BackendCore commands and canonical reducer state.
3. A3 exposes only owner-approved operations through the Frontend Protocol.

Completing this tooling does not complete full Phase A.
