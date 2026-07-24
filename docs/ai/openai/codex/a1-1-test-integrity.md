# Codex A1.1 test-integrity accounting

## Accounting rules

The verified base for every cumulative comparison is
`a226e3df5efd55b5dbef12cb1760674388909d7c`. Counts below are the sum of
`git diff --numstat <base> <boundary> -- <path>` and therefore match the
requested base-to-boundary view rather than hiding earlier test changes behind
per-commit deltas. Binary entries, if any, would be reported separately; this
slice has none.

No A0 or A1.0 regression test is deleted, renamed, merged, or disabled in
Commits 1-6. No assertion is replaced by a generic “failed somehow” check.
Generated fixtures supplement the handwritten transport, lifecycle, reducer,
redaction, and compatibility tests; they do not replace them. Commit 4 raises
the fixture-infrastructure timeout from 180 to 300 seconds after the expanded
independent Draft-07 corpus took longer than 180 seconds in CI. Commit 6 raises
`CodexSourcePackageTest` from 180 to 360 seconds because the extracted source
archive now regenerates and validates the complete 3,714-record corpus plus
the final closure report. Both test bodies and all assertions are unchanged by
these timeout corrections.

## Cumulative test diff at each validated boundary

| Boundary | `tests/` | `tests/component/codex/` | `tests/policy/` | `tests/installed/codex/` |
|---|---:|---:|---:|---:|
| Commit 1 `6953b9205d900c72cdb0a3389b0c8ff9037bfd79` | 5 files, +1,093/-3 | 3 files, +1,064/-0 | 0 files, +0/-0 | 0 files, +0/-0 |
| Commit 2 `8007d602915a402f9c29661e162142990060c744` | 14 files, +2,703/-38 | 10 files, +2,596/-33 | 0 files, +0/-0 | 2 files, +50/-0 |
| Commit 3 `8fc94aa93fffb11038984f81f11d9a23878eb230` | 20 files, +6,370/-140 | 16 files, +6,239/-135 | 0 files, +0/-0 | 2 files, +51/-0 |
| Commit 4 `61f0ffd16781d9c465b7c983e3055591c938377b` | 27 files, +11,737/-202 | 23 files, +11,553/-197 | 0 files, +0/-0 | 2 files, +56/-0 |
| Commit 5 `bb43f4f022fe5b1fe354c98b089d0991313be3eb` | 29 files, +13,717/-208 | 25 files, +13,533/-203 | 0 files, +0/-0 | 2 files, +56/-0 |
| Commit 6 pre-commit tree, `Close and verify the Codex A1.1 slice` | 31 files, +14,156/-216 | 25 files, +13,549/-203 | 0 files, +0/-0 | 2 files, +150/-7 |

The policy-test tree is byte-identical to the base at all six boundaries.
The Commit 6 SHA is recorded in the draft pull request after the commit exists;
a commit cannot reproducibly contain its own SHA. Its tree counts above include
this file, the closure-evidence registration, installed-consumer expansion,
and package audits, all of which are verification-only changes.

## Modified pre-existing test contracts

### Commit 1 — audit

`CodexAppServerContractsToolTest.py` originally guarded the pinned operation
associations and ordinary missing-schema failures. It retains every original
assertion and adds the reviewed Unit mutation: adding one property to
`thread/archive` must derive Concrete and then fail the exact Unit identity
ratchet. `CodexSourcePackageTest.cmake` and
`StagedInstalledConsumerTest.cmake` retain their original source/binary
boundary assertions and add the new audit inputs. `CMakeLists.txt` only
registers the new deterministic audit test.

The new audit test compares exact diagnostic-code multisets. Its intrinsic
codes include `TaxonomyArithmeticMismatch`, `IdentityCountMismatch`,
`ResultKindMismatch`, `StatusSplitMismatch`, `DuplicateIdentity`,
`MissingIdentity`, `CrossSliceIdentity`, `UnstableIdentity`,
`AssignmentMismatch`, `ContractMismatch`, `BatchDependencyCycle`,
`DuplicateBatchAssignment`, `MissingClosureDisposition`, `OpaqueJsonMisuse`,
`ConflictingCppOwnership`, `CrossSliceDependency`,
`DirectionalityMismatch`, `DiscriminatorLiteralMismatch`,
`CrossSliceOwnershipMismatch`, `OptionalNullableMappingMismatch`,
`StrongIdentifierMappingMismatch`, `CppPathMappingMismatch`,
`CppDefinitionMappingMismatch`, `MalformedReviewedPublicMapping`,
`StagedRatchetMismatch`, and `StaleGeneratedAudit`. Association and
reachability mutations additionally assert exactly
`DuplicateModuleSliceAssignment`, `ReachabilityRootSetMismatch`, and
`WrongParameterType`.

### Commit 2 — common value and union foundation

The fixture and surface-tool tests originally guarded the A1.0 corpus,
assignment, and generated artifacts. They retain those assertions and add the
exact 26-key SharedCommon assignment, independent schema validity,
encode/decode direction, optional/null/value coverage, descriptor bijection,
and stale-generation checks. Descriptor mutations assert exact codes including
`ConversationUnionDescriptorAssignmentMismatch`,
`ConversationUnionDescriptorSchemaMismatch`, and
`ConversationUnionDescriptorDirectionMismatch`.

The registry and coverage tests retain all A0/A1.0 exact identity checks and
add the 26-identity B2 ratchet, target/descriptor bijection, false-completeness
rejection, and same-count replacement rejection. `CodexTypedTurnCodecTest`
retains the six existing operation codecs and adds complete typed `UserInput`,
tri-state, open-value, and malformed-known coverage. Installed consumers keep
their original API/layout compilation and add the intended B2 public types and
compatibility aliases. All unknown/malformed assertions distinguish exactly
`UnknownDiscriminator` + `ForwardCompatibility`,
`UnknownEnumValue` + `ForwardCompatibility`, and
`MalformedKnownPayload` + `ProtocolWarning`.

### Commit 3 — item models

`CodexTypedItemDecoderTest`, `CodexTypedEventDecoderTest`,
`CodexBackendReducerTest`, and the registry/coverage/fixture guards retain the
eight pre-A1.1 modeled item contracts. Their new contract is a complete,
direction-specific 18-alternative `ThreadItem` and 16-alternative
`ResponseItem` surface with raw retention and separate unknown alternatives.
The parity test proves each of the ten newly known ThreadItems has the same
observable generic-item or bounded-extension behavior it had as
`UnknownItem`; no modeled reducer assertion is removed.

The fixture/surface guards add exact ThreadItem, ResponseItem, and B3 nested
descriptor sets and reject target, direction, schema, and assignment drift.
Codec negatives assert the exact two-code outcomes
`UnknownDiscriminator`/`ForwardCompatibility`,
`UnknownEnumValue`/`ForwardCompatibility`, or
`MalformedKnownPayload`/`ProtocolWarning`, including missing required,
wrong nested type, conflicting discriminator, and secret-value omission.
The installed consumer retains all earlier compilation checks and adds both
distinct item variants and their unknown alternatives.

### Commit 4 — thread and turn operations

The six existing facade, thread, turn, event, reducer, and protocol-codec tests
retain their original success, error, cancellation, callback-ordering,
generation, observer, and compatibility assertions. Their new contract covers
all 22 exact wire methods, 22 result roots, complete Thread/Turn/session
aggregates, 16 B4 nested alternatives, and all field presence models. The
AF_UNIX transcript test observes actual JSONL bytes; destructive and shell
methods use deterministic fake transcripts only.

The production result corpus asserts only the exact intrinsic sets
`{Decoded}` and `{MalformedKnownPayload}`. Aggregate/nested tests use
`{Decoded}`, `{UnknownDiscriminator, ForwardCompatibility}`,
`{UnknownEnumValue, ForwardCompatibility}`, and
`{MalformedKnownPayload, ProtocolWarning}` as applicable. Surface mutations
assert exact codes including `OperationProductionCoverageTargetMismatch`,
`OperationProductionCoverageOutcomeMismatch`,
`ConversationProductionCoverageTargetMismatch`,
`ConversationProductionCoverageOutcomeMismatch`,
`OperationProductionCoverageAggregateValueTargetMismatch`, and
`OperationProductionCoverageAggregateValueOutcomeMismatch`. Registry guards
retain earlier negative cases while adding missing/duplicate/wrong
category/target, descriptor mismatch, false completeness, demotion, and
removal assertions.

### Commit 5 — notifications and preservation

`CodexTypedEventDecoderTest` retains all twelve modeled notification semantics
with complete canonical payloads. The former partial test payloads remain as
explicit malformed-known cases rather than disappearing. Registry, coverage,
fixture, surface, and audit tests retain every earlier batch ratchet and add
the exact 37-row B5 target/descriptor/fixture set plus exact global
167/8/164/48 metrics.

The notification corpus asserts only `{Decoded}`,
`{Decoded, MalformedKnownPayload, ProtocolWarning}`,
`{MalformedKnownPayload, ProtocolWarning}`, or
`{ProtocolEnvelopeRejected}`. Descriptor/report mutations assert exact codes
including `ServerNotificationDescriptorSliceMismatch`,
`ServerNotificationDescriptorAssignmentMismatch`, and the corresponding
production-coverage target/outcome mismatch codes. Backend preservation tests
add all 25 newly typed methods, high-volume audio/transcript bounds,
original-size accounting, raw retention, and frontend redaction without
altering any existing reducer-state expectation.

Across all five boundaries, line deletions are implementation updates to old
partial fixture shapes, aggregate construction, or expected completeness
counts. The replacement assertions are equivalent or stronger because they
check complete typed fields and exact diagnostic-code multisets while
retaining the old behavior, lifecycle, redaction, and boundedness contracts.

### Commit 6 — closure-only verification

The staged installed consumer originally compiled the A0/A1.0 representative
surface and the B2-B4 additions. It retains those expressions and adds
representative final notification aggregates, unknown alternatives, shared
nested unions, grouped operations, the common Unit result, and deprecated
compatibility APIs. Its object-layout assertions remain
`sizeof(AppServerClient) == 2 * sizeof(void*)` and
`sizeof(typed::Client) == sizeof(void*)`.

The source-package test retains every inclusion/exclusion rule and updates its
exact inventory to seven top-level Codex tools and thirteen evidence files.
It now runs the extracted final closure check, proving the archive is
self-contained and offline-reproducible. The staged binary-install test
retains every private-artifact exclusion and expands only the public consumer
compile. The new literal binary-package audit independently builds exactly the
three Codex component TGZ archives, checks their exact 27-header and three
SOVERSION-library inventories, and rejects source-only/private artifacts.
`CodexA11ClosureEvidenceTest` is a new registration, not a replacement: it
checks the exact 151-key final ratchet, staged nonregression, global metrics,
documentation lists, fixture/type-closure totals, and frontend/application
fingerprints.

Commit 6 deletes, renames, merges, disables, or weakens no test. Its seven
removed installed-consumer lines in the cumulative base comparison (nine in
the Commit 6 delta) are aggregate initializers replaced by more
complete public-API constructions; every prior type/layout/compatibility
assertion remains and the compile surface is strictly larger.
