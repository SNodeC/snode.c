# AISuite cutover boundary

This document freezes the reviewed boundary before SNode.C removes its
duplicated Codex integration. It describes the current tree at
`d18b231a1d2ec2235fd6f204786b0a761cc24ff5` (tree
`88a63edc985a851b2b76b0c56df19fae74ea8069`). It does not perform the
cutover.

AISuite is the ownership authority at merge commit
`0c3a5838359eb283aca67840325ce6019345b462` (tree
`f86196b41d695f7165dca6a80ec017a8b9166de1`). That merge has subject
`Merge pull request #3 from
SNodeC/extraction/complete-codex-policy-ownership` and ordered parents
`19de4f50be64e187761274f043091090609d27a3` and
`d3da338eed6c11ed05163c3c8c4363702d817147`. Its tree is byte-identical
to its second parent's tree.

The generated start-state evidence beside this document is authoritative:

- `aisuite-cutover-start-state.json` records every removal path with its Git
  blob ID, SHA-256 digest, and size, plus the configured build, install,
  package, policy, and CTest inventories.
- `aisuite-cutover-plan.json` assigns every functional change to the four
  bounded cutover commits.
- `aisuite-cutover-baseline-ctest.json` normalizes the configured 300-test
  baseline and classifies every test as removed, adapted, or preserved.

The pinned SNode.C component hierarchy registers 123 Codex-owned tests, not
131. The pinned AISuite hierarchy contains all 123, byte-backed by the
extraction evidence, plus eight AISuite-only user-integration completion
tests. The frozen comparison is therefore SNode.C 123 to AISuite 131. This
arithmetic is a post-extraction completion delta; it is not missing ownership
or a reason to retain duplicated code.

The cutover is deliberately ordered:

1. Freeze this boundary without changing production, API, ABI, build,
   installation, packaging, shared policy, or CI behavior.
2. Detach and remove the SNode.C production/build/package/test surface.
3. Remove the remaining duplicated residue, document migration, and enforce a
   permanent ownership boundary.
4. Generate and verify closure evidence only.

The dependency direction after removal is `AISuite -> installed SNode.C`.
SNode.C must never acquire a source, build, runtime, or package dependency on
AISuite.
