# JSON Schema validator dependency gate

The completion series evaluated jsoncons 1.10.0 (latest published release at evaluation), whose [jsonschema extension](https://github.com/danielaparker/jsoncons/blob/v1.10.0/doc/ref/jsonschema/jsonschema.md) supports 2020-12 since 0.174.0. Its [Boost Software License 1.0](https://github.com/danielaparker/jsoncons/blob/v1.10.0/LICENSE) is permissive and requires retaining source notices; it does not require relicensing the SNode.C sources. Its upstream CMake exports a header-only `jsoncons` target; a system `find_package(jsoncons CONFIG)` and private bundled-header fallback are mechanically possible without changing installed public headers. Neither dependency path was added because conformance failed first.

The official [JSON Schema Test Suite](https://github.com/json-schema-org/JSON-Schema-Test-Suite) required validation cases were run through CTest with the suite's dialect compatibility algorithm selecting 2020-12. Format assertions, which are not the default 2020-12 behavior, were excluded by their official tags. Results: **1,258 passing instances; one failing local dynamic-reference instance; three schema-compilation failures; 23 separately identified external-reference groups**. The external groups were not fetched and were not counted as passing.

Failures:

- `dynamicRef.json`: `$dynamicRef avoids the root of each schema, but scopes are still registered (2020)` / `data is not sufficient for schema at second#/$defs/length`.
- `pattern.json`: `pattern with Unicode property escape requires unicode mode`.
- `pattern.json`: `pattern with Unicode property escape`.
- `patternProperties.json`: `patternProperties with Unicode property escape`.

The three compilation failures report `Invalid range in '{}' in regular expression`. No tests or expectations were changed to make the dependency pass.

The owner gate therefore excludes Step 5: no jsoncons production dependency, no substitute dependency and no partial new validator. **The existing restricted validator remains a known conformance limitation.** Completing the other server features does not establish full JSON Schema or complete MCP conformance.

Reproducibility: release archive SHA-256 `c829350a2eece662beb378143e2ab1c1cbb07d703687bf168626665a8a7d8bc1`; official suite archive SHA-256 `ddcc582e5ce9ccd5ea0cba6a8932e519f948935db327bb9f12f9bc2d86bfd707`.

Alternatives, not implemented:

- Re-evaluate a later jsoncons release with the same required tests and packaging gate.
- [Blaze](https://github.com/sourcemeta/blaze/blob/main/LICENSE) is currently AGPL-3.0-or-later or commercial. This is not a drop-in permissive dependency for SNode.C's LGPL-or-MIT distribution; an owner licensing decision would be required.
- A maintained in-house 2020-12 evaluator is estimated at 8,000–15,000 production lines plus 3,000–6,000 integration/test lines, including reference scopes, dynamic references, annotation/evaluation tracking, Unicode regex integration and metaschema handling. This is an unverified planning estimate, not an implementation proposal or approved growth.
