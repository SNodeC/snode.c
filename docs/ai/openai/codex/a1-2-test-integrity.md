# Codex A1.2 test-integrity accounting

## Scope

This document records the test changes made between the verified Phase A1.2
base, `9f7b2955d017cab189fb7b7a80211d0c2788f819`, and each logical task
commit. It distinguishes new coverage from changes to pre-existing tests and
records the retained contract for every pre-existing test that changed.

No pre-existing test was deleted, renamed, merged, disabled, or moved out of
ordinary CTest registration in Commits 1 through 5. Commit 6 adds closure
checks; it does not remove or weaken an A0, A1.0, A1.1, or earlier A1.2
regression.

## Cumulative test diffs at each boundary

The figures below are the exact output of `git diff --shortstat` from the
verified base to the named boundary. A blank entry means no diff.

| Boundary | `tests/` | `tests/component/codex/` | `tests/policy/` | `tests/installed/codex/` |
|---|---:|---:|---:|---:|
| Commit 1 `98556b899` | 6 files, +2,912/-6 | 5 files, +2,881/-0 | — | — |
| Commit 2 `3cbbafe63` | 20 files, +7,111/-103 | 16 files, +6,956/-93 | — | 1 file, +106/-1 |
| Commit 3 `fef4d7d47` | 23 files, +9,423/-108 | 19 files, +9,200/-98 | — | 1 file, +164/-1 |
| Commit 4 `77113414a` | 26 files, +11,323/-195 | 22 files, +11,030/-185 | — | 1 file, +225/-1 |
| Commit 5 `29a065a71` | 28 files, +13,235/-197 | 24 files, +12,834/-187 | — | 1 file, +331/-1 |
| Commit 6 final tree | 37 files, +14,360/-198 | 25 files, +13,152/-187 | 3 files, +659/-0 | 5 files, +450/-1 |

Commit 5 alone changed 10 files below `tests/` by +1,981/-71: 8
component-Codex files by +1,871/-69, no policy tests, and one installed
consumer by +106/-0.

## Pre-existing test contract audit

### Package and installed-consumer tests

`tests/CodexBinaryPackageTest.cmake`

- Original contract: pin the exact installed Codex public-header inventory,
  three binary component archives, three SOVERSION-1 libraries, and exclusion
  of private evidence.
- New contract: the same checks now include exactly the three intended public
  A1.2 headers, for a 30-header inventory.
- Retained assertions: archive count, component names, SOVERSION, runtime
  package contents, and private schema/evidence/fixture exclusion.
- Diagnostic codes: not applicable; CMake failures identify the violated
  inventory rule.
- Integrity result: strictly stronger; nothing was deleted, renamed, merged,
  or weakened.

`tests/CodexSourcePackageTest.cmake`

- Original contract: produce a real source archive, pin all offline Codex
  inputs, extract it, and rerun provenance, contracts, descriptor, audit,
  closure, fixture-generation, Draft-07, and mutation checks without network
  access.
- New contract: the same archive and extracted-package checks cover the A1.2
  audit, final closure report, complete 4,815-record fixture corpus, A1.2
  tests, documentation, and the global synthetic-secret policy guard.
- Retained assertions: exact hashes, exact prefix inventories, no stale or
  extra fixture, deterministic regeneration, all required/wrong-type
  mutations, local-artifact exclusions, and complete A1.1 closure.
- Diagnostic codes: the invoked tools retain their intrinsic codes; the
  archive harness fails the exact named retained/extracted check rather than
  accepting a generic partial package.
- Integrity result: strictly stronger; nothing was deleted, renamed, merged,
  or weakened.

`tests/StagedInstalledConsumerTest.cmake`

- Original contract: install into a clean stage, reject private headers and
  evidence, compile/run representative consumers, and retain SOVERSION 1.
- New contract: the exact 30-header inventory includes `Accounts.h`,
  `Models.h`, and `Configuration.h`; isolated consumers compile each header
  independently in addition to the aggregate consumer.
- Retained assertions: every previous public header and consumer, private
  artifact exclusion, three `.so.1` libraries, CMake package exports, and
  execution of the existing consumers.
- Diagnostic codes: not applicable; CMake failures name the missing,
  unexpected, or non-self-contained installed header.
- Integrity result: strictly stronger; nothing was deleted, renamed, merged,
  or weakened.

`tests/installed/codex/CodexTypedConsumer.cpp`

- Original contract: compile installed typed conversation types, operations,
  aliases, unknown variants, callbacks, and the one-pointer
  `typed::Client`/PIMPL ABI boundary.
- New contract: it additionally compiles every A1.2 facade method,
  account/login alternatives, auth-refresh canonical and legacy responses,
  model/provider/configuration aggregates, open enums, strong identifiers,
  arbitrary schema-authorized JSON, and omitted/null/value states.
- Retained assertions: all prior conversation calls and type relationships,
  compatibility aliases, callback signatures, `sizeof(typed::Client)`, and
  `sizeof(AppServerClient)`.
- Diagnostic codes: not applicable; compile-time assertions and overload
  resolution are the contract.
- Integrity result: strictly stronger; nothing was deleted, renamed, merged,
  or weakened.

### A1.1 corpus compatibility

`tests/component/codex/CodexA11OperationResultCorpusTest.cpp`

- Original contract: traverse exactly the 22 A1.1 client-operation
  descriptors and compare decoded successful results with their indexed raw
  results.
- New contract: selects those same 22 descriptors by the canonical A1.1
  registry row after the one production descriptor set gained later-slice
  entries; it also understands the schema-complete login response raw view.
- Retained assertions: exact 22-row A1.1 set, exact fixture roles, production
  result dispatch, raw equality, malformed-result rejection, and no
  test-shadow dispatch table.
- Diagnostic codes: the corpus retains the exact indexed Draft-07 codes
  `any_of_zero`, `enum_mismatch`, `minimum`, `one_of_zero`,
  `required_missing`, and `type_mismatch`.
- Integrity result: equivalent for A1.1 and additive for A1.2; no A1.1
  identity or assertion was deleted.

`tests/component/codex/CodexA11NotificationCodecTest.cpp`

- Original contract: exercise the exact 37 A1.1 notification descriptors and
  prove the then-residual partial notification rows.
- New contract: still selects exactly those 37 A1.1 descriptors while
  recognizing that A1.2 completed `model/rerouted` and `configWarning`; only
  `error` remains the notification-side residual Partial row.
- Retained assertions: all 37 A1.1 fixture roles, method bindings, raw
  envelopes, known/malformed/future separation, and decoder behavior.
- Diagnostic codes: typed decode cases retain
  `ForwardCompatibility`, `MalformedKnownPayload`, `ProtocolWarning`, and
  `UnknownEnumValue` according to the original branch contract.
- Integrity result: the old count-only assumption was replaced with a
  stronger exact slice-membership assertion; no A1.1 coverage was removed.

### Generated fixture and audit infrastructure

`tests/component/codex/CodexAppServerFixtureToolTest.py`

- Original contract: independently generate and Draft-07-validate the complete
  offline fixture corpus, verify deterministic bytes, and exercise required,
  optional, nullable, wrong-type, range, union, open-enum, and Unit mutations.
- New contract: the same infrastructure includes all A1.2 roots and paths,
  caches immutable schema/report snapshots to avoid redundant exhaustive
  passes, and retains one independent committed-corpus validation plus a
  controlled second hash-seed process.
- Retained assertions: exact generated bytes, all fixture roles, every
  mutation class, stale-record rejection, no C++ decoder introspection, and
  the unchanged 300-second CTest timeout.
- Exact intrinsic schema-code set:
  `any_of_zero`, `enum_mismatch`, `minimum`, `one_of_zero`,
  `required_missing`, and `type_mismatch`. Each negative fixture records and
  asserts its exact code list.
- Integrity result: equivalent exhaustive coverage with fewer duplicate
  traversals; no mutation class, fixture role, or negative assertion was
  deleted or weakened.

`tests/component/codex/CodexAppServerSurfaceToolTest.py`

- Original contract: reproduce the canonical registry and A1.1 private
  descriptors, reject stale/coherently drifted artifacts, and enforce exact
  operation contracts and Unit roots.
- New contract: extends the same generator/check paths to the A1.2
  account/model/configuration union and operation descriptors and the final
  40-operation descriptor set.
- Retained assertions: A1.1 descriptor bytes and 151-identity ratchet,
  canonical-row authority, deterministic generation, Unit schema-property
  mutation, and exact method/result associations.
- Exact descriptor negative codes include
  `ClientOperationDescriptorAssignmentMismatch`,
  `WrongResultType`, `StaleGeneratedClientOperationDescriptors`,
  `AccountsModelsConfigurationUnionDescriptorAssignmentMismatch`,
  `DuplicateAccountsModelsConfigurationUnionDescriptorTarget`,
  `AccountsModelsConfigurationUnionDescriptorDirectionMismatch`,
  `StaleGeneratedAccountsModelsConfigurationUnionDescriptors`,
  `ServerRequestDescriptorAssignmentMismatch`,
  `ServerRequestDescriptorContractMismatch`, and
  `StaleGeneratedServerRequestDescriptors`. Operation-association negatives
  retain the exact codes `MissingAssociation`, `DuplicateAssociation`,
  `StaleAssociation`, `WrongAssociationCategory`, `WrongParameterType`,
  `WrongResultType`, `ConflictingAssociationEvidence`,
  `UnitWithNonUnitResultType`, `ConcreteWithoutResultType`,
  `ContractOnNonRequest`, and
  `ExperimentalAssociationCountedAsStable`.
- Integrity result: strictly stronger and bidirectional; no prior negative was
  relaxed to a generic failure.

`tests/component/codex/CodexA12AuditToolTest.py` is new rather than a modified
test. Its intrinsic negatives assert exact singleton or multiset codes,
including `ProgressIdentityMismatch`, and verify coherent row/evidence drift
cannot hide a status mismatch.

### Canonical registry and exact coverage ratchets

`tests/component/codex/CodexProtocolSurfaceCoverageGuardTest.cpp`

- Original contract: exact manifest/registry parity, exact A0/A1.0/A1.1 typed
  floors, and bidirectional runtime-target validation.
- New contract: preserves those floors, selects later descriptors by their
  canonical A1 slice, and adds the exact 45-key A1.2 ratchet plus the final
  212/6/121/48 global lock.
- Retained exact coverage codes:
  `MissingRegistryEntry`, `DuplicateRegistryEntry`, `WrongCategory`,
  `WrongStability`, `TypedWithoutRuntimeTarget`,
  `DuplicateRuntimeTargetRegistration`, `WrongRuntimeTargetCategory`,
  `InvalidRuntimeTarget`, `ImplementedWithoutTypedDisposition`,
  `StaleRegistryEntry`, `RuntimeTargetWithoutTypedImplementation`, and
  `MissingRuntimeTargetRegistration`.
- Retained/additive exact ratchet codes:
  `BaselineIdentityMissing`, `BaselineIdentityNotStable`,
  `BaselineIdentityNotImplemented`, `B2IdentityWrongAssignment`,
  `B2IdentityNotComplete`, `B4IdentityMissing`,
  `B4IdentityWrongAssignment`, `B4IdentityNotComplete`,
  `B5IdentityMissing`, `B5IdentityWrongAssignment`, and
  `B5IdentityNotComplete`.
- Integrity result: strictly stronger; all earlier floors and negative
  code-multisets remain.

`tests/component/codex/CodexProtocolSurfaceRegistryTest.cpp`

- Original contract: registry invariants, target/descriptor bijection,
  authoritative request/result contracts, exact mechanical completeness, and
  exact diagnostic-code multisets.
- New contract: adds every A1.2 target and descriptor, exact 16 Concrete/2
  Unit A1.2 associations, final schema metrics, no-dropped-field facts, and
  coherent row-plus-descriptor drift attacks.
- Retained exact contract/security codes:
  `MissingAssociation`, `DuplicateAssociation`,
  `ConflictingAssociationEvidence`, `ConcreteWithoutResultType`,
  `ContractOnNonRequest`, `ExperimentalAssociationCountedAsStable`,
  `MissingTypedModuleAssignment`, `MissingSliceAssignment`, and
  `FrontendSecurityMismatch`.
- Retained/additive descriptor codes:
  `DuplicateCodecDescriptor`, `RegistryRowWithoutCodecDescriptor`,
  `CodecDescriptorWithoutRegistryRow`,
  `CodecDescriptorCanonicalKeyMismatch`,
  `CompleteWithoutCodecDescriptor`,
  `CodecDescriptorWithoutTypedRegistryRow`,
  `CodecDescriptorTargetMismatch`,
  `RuntimeTargetCanonicalKeyMismatch`,
  `MissingRuntimeTargetRegistration`,
  `InvalidCodecDescriptorShape`, and
  `CodecDescriptorContractMismatch`.
- Exact Unit mutation codes are
  `CodecDescriptorContractMismatch` plus
  `UnitWithNonUnitResultType`.
- Exact completeness codes remain
  `ClaimedCompleteWithoutAuthoritativeAssociation`,
  `ClaimedCompleteWithoutPositiveFixtureCoverage`,
  `RequiredFieldNotExercised`, `SchemaPropertyNotExercised`,
  `OptionalPresentCaseMissing`, `OptionalOmittedCaseMissing`,
  `NullableSemanticsMissing`, `ReachableUnionAlternativeMissing`,
  `DirectionAssertionMissing`, `StaleFixture`,
  `CompletenessRuntimeTargetMismatch`, `UnrecordedOpaqueField`,
  `ClaimedCompleteWithoutIndependentValidation`,
  `KnownSchemaFieldDropped`, and `TypedSchemaStatusMismatch`.
- Integrity result: strictly stronger; every negative compares the complete
  intrinsic code multiset and rejects unrelated diagnostics.

### Grouped typed API policy

`tests/component/codex/CodexTypedClientFacadeTest.cpp`

- Original contract: all existing grouped facades share one `RawProtocol`,
  callbacks retain their lifecycle contract, and `typed::Client` remains one
  pointer.
- New contract: adds `accounts()`, `models()`, and `configuration()` to those
  exact assertions.
- Retained assertions: threads/turns/events/requests behavior, synchronous
  local rejection, asynchronous acceptance, cancellation/generation,
  observer coexistence, callback exception containment, and object layouts.
- Diagnostic codes: lifecycle results retain the established local
  submission and protocol-error codes; the structural checks are compile/run
  assertions.
- Integrity result: strictly stronger; no direct `AppServerClient` accessor
  was accepted.

`tests/component/codex/CodexTypedFacadeUsageGuardTest.py`

- Original contract: reject generic/direct facade bypass and require grouped
  typed API use.
- New contract: accepts only the three reviewed new grouped accessors and
  rejects direct `AppServerClient::accounts/models/configuration`, generic
  string requests, and speculative facade families.
- Diagnostic codes: the policy test asserts its exact violation identities
  and fails on any unexpected match rather than a count-only condition.
- Integrity result: strictly stronger; existing facade restrictions remain.

`tests/component/codex/CMakeLists.txt`

- Original contract: register every Codex component test with focused labels,
  dependencies, and timeouts.
- New contract: adds the A1.2 audit, documentation, codec, wire,
  preservation, and closure tests while preserving every prior registration;
  the exhaustive fixture timeout remains 300 seconds.
- Diagnostic codes: not applicable; `ctest -N` is the registration ratchet.
- Integrity result: additive only; no test was renamed, disabled, or given a
  looser timeout.

## Commit 6 additions

Commit 6 adds rather than replaces:

- an exact A1.2 final-closure report/check;
- a path-exact, repository-level synthetic-secret leak guard with a planted
  negative self-test;
- independent installed-header self-containment/include-guard checks; and
- final documentation/package inventory checks.

The closure tree passed its independent boundary with 285 exact CTest
registrations. `CodexA12ClosureEvidenceTest`,
`CodexA12PublicHeaderPolicyTest`, and
`CodexSyntheticSecretLeakGuardTest` are registered as tests 179, 284, and
285 respectively.
