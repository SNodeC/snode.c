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
  --output "$REGISTRY" \
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
discriminators. The coverage guard requires the current implemented stable
typed identity set to be a superset of that floor.

Coverage may grow without changing the baseline. Decreasing coverage or
replacing a locked identity requires an explicit reviewed baseline change and
an explanation. A later A1 change may deliberately advance the baseline to
lock in newly reviewed typed coverage. The floor is a no-regression ratchet,
not evidence of future completeness.

## Packaging policy

The schema trees and A0 support artifacts are source and test inputs. Explicit
CMake install rules and CPack binary packages exclude the schemas, provenance
tool, manifest, fixtures, and private registry headers/data. The staged
installed-consumer test recursively guards that exclusion.

CPack source packages retain the vendored schemas, attribution files, manifest,
tool, fixtures, and private registry sources so the offline A0 tests remain
self-consistent. The generated schemas and provenance occupy approximately
6 MiB before archive compression. The repository's existing CPack source
configuration assumes a build-artifact-free source checkout; Git cleanliness
alone is insufficient because an ignored in-tree build is not excluded. CMake
exposes `package_source`; there is no separate `dist` target.

## Phase sequencing

A0 establishes the pinned census, canonical runtime registry, guards, metrics,
and owner worksheet only. After the owner freezes module grouping and security
decisions:

1. A1 implements the frozen stable typed App Server API.
2. A2 adds transport-neutral BackendCore commands and canonical reducer state.
3. A3 exposes only owner-approved operations through the Frontend Protocol.

Completing this tooling does not complete full Phase A.
