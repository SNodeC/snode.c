# Migrating Codex integration from SNode.C to AISuite

## Ownership cutover

Codex ownership moved to [AISuite](https://github.com/SNodeC/AISuite) so its
implementation, public package, applications, protocol evidence, and policy
tests have one authority. SNode.C returns to being the independently
consumable networking and runtime dependency and no longer contains an
AI-provider layer.

The frozen SNode.C extraction baseline is
`d18b231a1d2ec2235fd6f204786b0a761cc24ff5`, tree
`88a63edc985a851b2b76b0c56df19fae74ea8069`, project version `1.0.1`, and
`SNODEC_SOVERSION` `1`.

The AISuite ownership authority is merge commit
`0c3a5838359eb283aca67840325ce6019345b462`, tree
`f86196b41d695f7165dca6a80ec017a8b9166de1`, with subject
`Merge pull request #3 from SNodeC/extraction/complete-codex-policy-ownership`
and ordered parents `19de4f50be64e187761274f043091090609d27a3` and
`d3da338eed6c11ed05163c3c8c4363702d817147`. Its tree is byte-identical
to its second parent's tree.

The original SNode.C implementation remains available in existing Git history
and tags; this cutover rewrites neither.

## Removed SNode.C package surface

The following optional SNode.C components and corresponding libraries no
longer exist:

- `ai-openai-codex`
- `ai-openai-codex-backend`
- `ai-openai-codex-frontend`

The removed applications are:

- `codex-backend`
- `codex-backend-client`

SNode.C also no longer installs its former 34 Codex public headers or retains
Codex schemas, fixtures, generators, protocol evidence, implementation tests,
or policy tests.

## Updating consumers

AISuite is now the CMake package authority:

```cmake
find_package(snodec CONFIG REQUIRED COMPONENTS core net-un-stream-legacy)
find_package(AISuite CONFIG REQUIRED)

target_link_libraries(
    consumer
    PRIVATE
        AISuite::OpenAICodex
        AISuite::OpenAICodexBackend
        AISuite::OpenAICodexFrontend
)
```

Public include forms remain `<ai/openai/codex/...>`, for example:

```cpp
#include <ai/openai/codex/AppServerClient.h>
```

AISuite supplies the Codex headers and libraries. Networking dependencies
still come from the separately installed SNode.C package. The dependency
direction is therefore:

`AISuite -> installed SNode.C`

SNode.C has no source, build, runtime, or package dependency on AISuite. There
is no forwarding header, compatibility library, deprecated target, or other
SNode.C shim.

## Cleaning an existing installation

> CMake installation into an already populated prefix does not delete obsolete
> files. Users upgrading from a pre-cutover build must remove the old
> `snodec-ai-openai-codex*` packages/files or install into a clean prefix.

For package-managed installations, uninstall the old three optional Codex
component packages before installing the cleaned SNode.C and AISuite packages;
exact package names depend on the distribution.

For a manually managed prefix, prefer replacing the prefix atomically. If that
is impractical, first verify ownership and then remove stale
`include/snode.c/ai/`, `lib/libsnodec-ai-openai-codex*`,
`bin/codex-backend`, `bin/codex-backend-client`, and obsolete Codex files under
`lib*/cmake/snodec/`. Do not remove AISuite-owned `include/aisuite/`,
`lib/libaisuite-openai-codex*`, applications, or CMake metadata.

## Source-package determinism

The frozen baseline source-package audit found six ignored, locally generated
files that were not tracked by Git: five `certs/snode.c_*` certificate
artifacts and `docs/Doxyfile`. The final source-package rules explicitly
exclude those six workspace products so archive contents do not depend on
whether a developer previously configured or ran the project. This is a
reproducibility cleanup, not removal of a reviewed non-Codex source file; every
tracked non-Codex baseline file remains required by the cutover evidence.

## Compatibility and release policy

This removal is a package and API break for consumers of the former optional
Codex components. It does not change any retained SNode.C public class,
retained vtable, retained shared-library ABI, retained SONAME, or retained
component. The SNode.C project version remains `1.0.1` and
`SNODEC_SOVERSION` remains `1`. Consumers of retained SNode.C libraries do not
need a cutover-specific ABI migration.

No release tag is created by this cutover. The intended release sequence is:

1. Merge the SNode.C cutover.
2. Verify post-merge SNode.C CI.
3. Update AISuite's normal SNode.C dependency pin while preserving extraction
   provenance.
4. Verify AISuite CI against the cleaned SNode.C installation.
5. Create the SNode.C book/release tag separately.

## Required AISuite follow-up

After the SNode.C cutover is merged:

1. Update AISuite's normal SNode.C build-dependency pin from
   `d18b231a1d2ec2235fd6f204786b0a761cc24ff5` to the merged clean SNode.C
   cutover commit or release tag.
2. Retain `d18b231a1d2ec2235fd6f204786b0a761cc24ff5` separately as immutable
   extraction provenance.
3. Update AISuite cutover evidence to record that the SNode.C cutover
   occurred.
4. Rerun AISuite CI.

AISuite is not modified by the SNode.C removal pull request.

## Historical references retained in SNode.C

The Phase 2, Phase 3, and test-suite consolidation reports retain pinned
pre-cutover Codex counts and decisions as historical evidence. Frozen cutover
evidence and permanent guards retain old names only to describe or reject the
removed surface. Package and install tests likewise use the former paths,
targets, applications, and component names only as negative assertions.

The following 18 logging documents contain historical `codex/...` Git branch
names, merge subjects, or references to the tooling execution environment as
incorporated by the pinned pre-cutover tree. Those references are development
provenance, not SNode.C product ownership:

- `docs/logging/round-01-baseline-contract-report.md`
- `docs/logging/round-02-semantic-core-report.md`
- `docs/logging/round-04-object-log-api-report.md`
- `docs/logging/round-05-socket-endpoint-log-api-report.md`
- `docs/logging/round-06-backend-unification-report.md`
- `docs/logging/round-07-startup-filter-configuration-report.md`
- `docs/logging/round-08-controlled-subsystem-migration-report.md`
- `docs/logging/round-09-compatibility-sanitizer-overhead-report.md`
- `docs/logging/semantic-logging-contract.md`
- `docs/logging/semantic-migration-01-socketconnection-hpp-report.md`
- `docs/logging/semantic-migration-02-socketconnector-acceptor-report.md`
- `docs/logging/semantic-migration-03-socketserver-client-report.md`
- `docs/logging/semantic-migration-04-core-runtime-report.md`
- `docs/logging/semantic-migration-07-http-websocket-report.md`
- `docs/logging/semantic-migration-07b-http-server-report.md`
- `docs/logging/semantic-migration-07c-websocket-report.md`
- `docs/logging/semantic-output-double-wrapping-report.md`
- `docs/logging/semantic-production-threshold-repair-report.md`

The ten non-Codex landing documents under
`docs/landing-pages/github-landing-openai-codex-pullreq/` are retained because
their contents document surviving SNode.C subsystems; the directory name is
historical document-generation provenance. The `/.codex/` source-package
ignore pattern describes local developer metadata, not a product component.
