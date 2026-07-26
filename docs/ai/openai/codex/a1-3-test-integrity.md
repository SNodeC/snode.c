# Codex A1.3 test-integrity accounting

## Scope

This report accounts for test changes between the verified A1.3 base
`304a817d371597fc764c3404c36cc880bf65536d` and the completed implementation
boundary whose subject is `Complete Codex reviews and guardian protocol`.
Commit 6 adds closure, packaging, and documentation guards only.

No pre-existing test was deleted, renamed, disabled, merged, or moved out of
ordinary CTest registration. The implementation boundary changes 30 files
under `tests/` by 10,082 insertions and 163 deletions. The deletions are
maintenance edits to shared assertions and inventories; they do not remove an
operation, fixture class, lifecycle case, diagnostic, test registration, or
label.

## Tests added

The A1.3 audit is covered by `CodexA13AuditToolTest.py`. Each implementation
batch adds one schema/codec test, one real AF_UNIX/JSONL wire test, and one
BackendCore/frontend compatibility test:

| Batch | Codec/schema | Real socket and lifecycle | Boundary |
|---|---|---|---|
| Commands | `CodexA13CommandCodecTest` | `CodexA13CommandWireTest` | `CodexA13CommandBackendCompatibilityTest` |
| Filesystem and fuzzy search | `CodexA13FilesystemCodecTest` | `CodexA13FilesystemWireTest` | `CodexA13FilesystemBackendCompatibilityTest` |
| Approvals and permissions | `CodexA13ApprovalCodecTest` | `CodexA13ApprovalWireTest` | `CodexA13ApprovalBackendCompatibilityTest` |
| Reviews and guardian | `CodexA13ReviewCodecTest` | `CodexA13ReviewWireTest` | `CodexA13ReviewBackendCompatibilityTest` |

The approval wire test keeps all five reverse-request types pending
concurrently, responds out of arrival order, and checks request IDs, response
schemas, occurrence tokens, request types, decision types, duplicate
rejection, generation invalidation, disconnect/reconnect, and absence of
cross-delivery over the existing socket path.

Four isolated installed-header consumers were added for `Commands.h`,
`Filesystem.h`, `PermissionProfiles.h`, and `Reviews.h`. The aggregate
installed consumer now compiles the complete A1.3 API, exact public variant
sizes, and the unchanged outer PIMPL sizes.

Commit 6 adds `CodexA13ClosureEvidenceTest.py`. It checks deterministic closure
generation and exact diagnostic codes for logical and authority mutations.

## Pre-existing tests changed

`CodexAppServerSurfaceToolTest.py`,
`CodexProtocolSurfaceRegistryTest.cpp`, and
`CodexProtocolSurfaceCoverageGuardTest.cpp` retain their earlier registry,
descriptor, result-association, completeness, and diagnostic-code checks and
extend them to the exact A1.3 identities. No exact assertion was replaced by a
substring-only assertion.

`CodexAppServerFixtureToolTest.py` retains deterministic Draft-07 generation,
all positive roles, required-field removals, wrong-type mutations, integer
bounds, nullable/optional states, open values, known/future/malformed union
cases, and stale-artifact guards. It extends the same shared engine to the
final 5,883-record corpus; it does not introduce an A1.3-only schema engine.

`CodexTypedClientFacadeTest.cpp` retains the single-`RawProtocol`, callback,
reentrancy, cancellation, generation, and PIMPL checks and adds the four A1.3
facade accessors. `CodexA11AuditToolTest.py` and
`CodexA12ClosureEvidenceTest.py` retain frozen predecessor evidence while
allowing only the reviewed monotonic A1.3 successor.

The package and staged-install tests retain every previous header, library,
consumer, private-artifact exclusion, and extracted offline check. Their
inventories are extended to the A1.3 public headers, tests, audit, closure,
fixtures, evidence, and documentation.

## Tests removed

None.

## Labels, timeouts, and execution policy

All new component registrations retain the existing
`component;ai;openai;codex` label family with focused `a1-3` and domain labels.
No unconditional skip, `DISABLED` property, `WILL_FAIL` inversion, or timeout
increase hides a failure. The exhaustive fixture infrastructure retains its
existing independently reviewed timeout; focused closure checks use a short
timeout.

Long-running targets intentionally skipped: `ctest (unfiltered full repository
suite)`. It is an unrelated repository-wide aggregate whose scope cannot add
A1.3-specific evidence beyond the exact Codex labels plus the required
application, install, package, sanitizer, ABI, security, and integrity checks.
Residual risk is limited to interactions with unrelated non-Codex components;
the relevant application targets are still built and the full focused Codex
suite is run. The repository contains no separately registered Codex
stress/soak/fuzz/benchmark target. `CodexSourcePackageTest` is not skipped: its
timeout exceeds 300 seconds to accommodate packaging, but its observed runtime
is about 94 seconds and it is required for A1.3 closure.

The optional live App Server smoke remains gated because deterministic offline
fixtures and AF_UNIX mock-server tests are authoritative and do not require a
real account, credentials, network access, or developer data.

## Sanitizer, compiler, and integration coverage

The final implementation tree was built with `-j8`, selected from 28 logical
CPUs and 62 GiB RAM (41 GiB available at the final measurement). The focused
A1.3 matrix passed under all three sanitizers:

| Configuration | Command | Result |
|---|---|---:|
| AddressSanitizer | `LD_PRELOAD=/usr/lib/gcc/x86_64-linux-gnu/15/libasan.so ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 ctest --test-dir build-a13-final-asan --output-on-failure -L '^a1-3$'` | 12/12 passed |
| LeakSanitizer | `LSAN_OPTIONS=exitcode=23:halt_on_error=1 ctest --test-dir build-a13-final-lsan --output-on-failure -L '^a1-3$'` | 12/12 passed, no leaks |
| UndefinedBehaviorSanitizer | `UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ctest --test-dir build-a13-final-ubsan --output-on-failure -L '^a1-3$'` | 12/12 passed |

LeakSanitizer was run outside the ptrace-restricted workspace sandbox; its
in-sandbox initialization failure was environmental and was not counted as a
test result. AddressSanitizer needed the GCC 15 runtime preloaded before
system libraries, and leak detection was intentionally left to the separate
LeakSanitizer run.

Clang 21.1.8 with the repository's strict `-Werror -Weverything` flags compiled
the changed Codex library, registry, codecs, wire tests, and facade test; all
10 resulting strict tests passed. The four A1.3 BackendCore/frontend
compatibility tests passed in the repository's Clang compatibility build,
giving 12/12 focused A1.3 tests there. A repository-wide strict Clang build is
blocked before the Codex targets by the unchanged
`src/web/http/decoder/Chunked.cpp:172`
`-Wtautological-type-limit-compare`; after that warning is locally demoted in
the build only, unchanged frontend code exposes existing `-Wnrvo` findings.
Neither baseline issue is caused by A1.3, and no unrelated production source
was changed to hide it.

A focused IWYU rebuild reports no include finding in the new command,
filesystem, approval, or review codecs and confirms the A1.3
`ServerRequestDecoder.cpp` implementation has correct direct includes. Its
only Codex suggestion in that focused run is the unchanged
`ServerRequestDecoder.h` preference for a forward declaration over its
existing direct `Protocol.h` include; repository-wide IWYU also reports
unrelated baseline diagnostics. No suppression pragma was added.

`CodexSyntheticSecretLeakGuardTest` remains registered with its planted
negative self-test. The build-independent closure checker scans the explicit
`src`, `tests`, `tools`, and `docs` source scopes. The registered final
validation additionally scans the retained build/package tree after source and
binary packaging. Neither path captures developer paths, credentials,
environment values, command output, file content, patches, or approval
rationale.

Installed-consumer, source-package, and binary-package tests remain ordinary
CTest integrations. The source package reruns provenance, contracts,
descriptor generation, A1.1/A1.2/A1.3 audits and closures, fixture
determinism, Draft-07 validation, and mutations from the extracted archive
without network access.

The final GCC Codex label contained 129 registered tests: 128 executed and
passed, and the credential-bearing live integration was explicitly skipped
by its opt-in gate. The run included:

| Evidence | Result |
|---|---:|
| Full GCC Debug build, including applications | 1,153/1,153 build steps passed; final-tree incremental rebuild passed |
| A1.3 codec, wire, and boundary label | 12/12 passed |
| A1.3 audit and closure mutation tools | 2/2 passed |
| Installed consumer | passed in 24.17 seconds |
| Source package and extracted offline validation | passed in 95.96 seconds |
| Binary package inspection | passed in 1.05 seconds |
| Draft-07 fixture corpus | 5,883 records; test passed in 105.09 seconds |
| Synthetic-secret guard | passed across 29,291 paths and all retained build/package trees |

The live `CodexTypedAppServerIntegrationTest` was not enabled because
`SNODEC_RUN_CODEX_TYPED_INTEGRATION=1` may use configured credentials and
quota. The exact pinned binary being present is not sufficient to make that
test credential-free or network-free. Offline schemas, generated fixtures,
and real AF_UNIX mock-App-Server lifecycle tests remain the mandatory
deterministic evidence.

## Integrity result

The A1.3 test delta is additive or strictly stronger. Earlier A0–A1.2
contracts remain registered, predecessor evidence remains byte-identical, all
new A1.3 roots have primary codec and wire coverage, and Commit 6 contains no
scheduled runtime implementation.
