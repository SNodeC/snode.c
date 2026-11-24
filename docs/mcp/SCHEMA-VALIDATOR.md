# JSON Schema validator decision and qualification

The completion series evaluated jsoncons 1.10.0 (latest published release at evaluation), whose [jsonschema extension](https://github.com/danielaparker/jsoncons/blob/v1.10.0/doc/ref/jsonschema/jsonschema.md) supports 2020-12 since 0.174.0. Its [Boost Software License 1.0](https://github.com/danielaparker/jsoncons/blob/v1.10.0/LICENSE) is permissive and requires retaining source notices; it does not require relicensing the SNode.C sources. Its upstream CMake exports a header-only `jsoncons` target; a system `find_package(jsoncons CONFIG)` and private bundled-header fallback are mechanically possible without changing installed public headers. Neither dependency path was added because conformance failed first.

The official [JSON Schema Test Suite](https://github.com/json-schema-org/JSON-Schema-Test-Suite) required validation cases were run through CTest with the suite's dialect compatibility algorithm selecting 2020-12. Format assertions, which are not the default 2020-12 behavior, were excluded by their official tags. Results: **1,258 passing instances; one failing local dynamic-reference instance; three schema-compilation failures; 23 separately identified external-reference groups**. The external groups were not fetched and were not counted as passing.

Failures:

- `dynamicRef.json`: `$dynamicRef avoids the root of each schema, but scopes are still registered (2020)` / `data is not sufficient for schema at second#/$defs/length`.
- `pattern.json`: `pattern with Unicode property escape requires unicode mode`.
- `pattern.json`: `pattern with Unicode property escape`.
- `patternProperties.json`: `patternProperties with Unicode property escape`.

These four cases have no optional tags in the suite and are compatible with 2020-12 (the dynamic-reference case is explicitly `=2020`). jsoncons defaults to the 2020-12 dialect. The three compilation failures report `Invalid range in '{}' in regular expression`. No tests or expectations were changed to make the dependency pass.

The original owner gate therefore excluded Step 5: no jsoncons production dependency, no substitute dependency and no partial new validator. **At that checkpoint the restricted validator remained a known conformance limitation.** The later owner-approved replacement below supersedes that decision.

Reproducibility: release archive SHA-256 `c829350a2eece662beb378143e2ab1c1cbb07d703687bf168626665a8a7d8bc1`; official suite archive SHA-256 `ddcc582e5ce9ccd5ea0cba6a8932e519f948935db327bb9f12f9bc2d86bfd707`.

Alternatives, not implemented:

- Re-evaluate a later jsoncons release with the same required tests and packaging gate.
- [Blaze](https://github.com/sourcemeta/blaze/blob/main/LICENSE) is currently AGPL-3.0-or-later or commercial. This is not a drop-in permissive dependency for SNode.C's LGPL-or-MIT distribution; an owner licensing decision would be required.
- A maintained in-house 2020-12 evaluator is estimated at 8,000–15,000 production lines plus 3,000–6,000 integration/test lines, including reference scopes, dynamic references, annotation/evaluation tracking, Unicode regex integration and metaschema handling. This is an unverified planning estimate, not an implementation proposal or approved growth.


## Owner-approved replacement

The owner selected the existing MIT-licensed `VolkerChristian/json-schema-validator` fork, rebased on upstream `pboettch/json-schema-validator` commit `d5199551f758ba067612979ae32ec35693640ccf`. The standalone development branch `draft-2020-12` ends at `133dbd6`; SNode.C pins that commit as a private static/PIC submodule dependency. No public MCP header exposes the validator. PCRE2 (`libpcre2-8`, BSD-3-Clause) is required; the validator is not installed separately. The fork's release version is 2.5.0: additive dialect APIs/features, with draft-07 still its library default. SNode.C selects 2020-12 explicitly.

The official test suite is vendored at commit `e2a68a15cd490750fd6b95e604e4054d7d978d1f`, using its dialect compatibility algorithm and inline external fixtures. Required tests, optional tests and optional format assertions are separate CTests. No network tests are silently counted as passing and no expected results were changed.

| Backend | Required draft-07 | Required 2020-12 | Optional draft-07 / 2020-12 | Format assertions draft-07 / 2020-12 |
| --- | --- | --- | --- | --- |
| PCRE2 10.49 | 972/972 | 1,319/1,319 | 100/126; 139/176 | 647/709; 707/789 |
| std::regex | 972/972 | 1,310/1,319 | 89/126; 120/176 | 644/709; 704/789 |

The nine fallback required failures are the two Unicode-property `pattern` groups and the Unicode-property `patternProperties` group (three instances each). PCRE2 closes these failures. Optional failures remain visible: ECMAScript regex edge cases, optional floating-point behavior and incomplete built-in format assertions. PCRE2 is not a complete JavaScript regex engine; variable-length unbounded lookbehind and some optional escape/whitespace behavior differ. 2020-12 format is annotation-only by default, so optional format-assertion failures are not represented as required conformance successes.

The corrected upstream baseline passed all 108 existing tests. The final PCRE2 run passes 110 of 115 CTests: the two official required suites and all nonoptional existing tests pass; four official optional/format aggregates and the pre-existing optional cross-draft test fail. That cross-draft test requests 2019-09, which is explicitly unsupported rather than silently interpreted as draft-07. The std::regex run additionally exposes its Unicode regex limitations. Neither optional tests nor assertions were weakened to hide these results. Passing the required suites satisfies the owner's dependency gate; it is not a claim that every optional JSON Schema or JavaScript-regex feature works.

The original schema evaluator was replaced, not retained as a fallback. Registration validates input/output schemas against bundled metaschemas, then compiles them to reject unresolved references. Tool arguments and successful structured outputs use the same evaluator; validation does not apply generated default patches. Unknown extension keywords remain annotations. The library resolves local `$ref`, `$defs`, `$anchor`, `$dynamicRef` and `$dynamicAnchor`, including schema-resource `$id` scopes. No automatic network loader is installed.

SNode.C bounds schema bytes to 16 KiB, compiled subschemas to 1,024, compilation/validation depth to 256 and validation evaluations to 100,000. The PCRE2 adapter bounds match work to 100,000 and depth to 256. These are implementation choices under the [2026 reference rules](https://modelcontextprotocol.io/specification/2026-07-28/basic#ref-resolution) and [composition guidance](https://modelcontextprotocol.io/specification/2026-07-28/basic#composition-keyword-resource-use). Unsupported dialects fail explicitly as required by the [2025](https://modelcontextprotocol.io/specification/2025-11-25/basic#implementation-requirements) and [2026](https://modelcontextprotocol.io/specification/2026-07-28/basic#implementation-requirements) implementation requirements.

A 2025 live elicitation channel retains its server-owned pending schema and validates accepted content before resuming. A 2026 retry is deliberately stateless: the new `RequestContext::getInputResponse(key, requestedSchema)` validates against the handler's known form schema, without trusting a schema or state supplied by the client. The raw response accessor stays explicitly untrusted. Invalid accepted retries produce InvalidParams; application business checks remain separate. `review.label` uses the validating accessor and retains its independent byte limit.

Downstream qualification exposed a 2020-12 reference-wrapper default regression: MQTT bridge startup lacked a referenced `mqtt` default. Default lookup is now one cycle-bounded traversal shared by reference and sibling-schema nodes; the validator regression covers inherited defaults and cycles. MQTTSuite requires no production workaround.
