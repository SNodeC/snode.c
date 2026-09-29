# SNode.C versioning policy

Use semantic release versions `MAJOR.MINOR.PATCH`:

- Increment **major** for a backward-incompatible public API or ABI change.
- Increment **minor** for API additions or new features that preserve both API
  and ABI compatibility.
- Increment **patch** for backward-compatible bug fixes that preserve API and ABI
  compatibility. Correcting observable buggy behaviour can be a patch change.

Assess source and binary compatibility separately. In C++, adding a data member
to a public class or changing its virtual interface can break ABI even when
existing application source still compiles. Such changes require a major release.
Use the highest required increment among the changes included in a release.

Library VERSION follows the complete release version; SOVERSION follows its major
number. Version numbering does not automatically prove ABI compatibility.

Never create or push a version tag without the user's explicit approval for that
tag and action. Approval to modify code, commit, push a branch, or run CI does not
authorize creating or pushing version tags. Do not infer release approval from
the versioning policy.

For an explicitly approved release, commit the matching VERSION file and create the
immutable `vMAJOR.MINOR.PATCH` tag on that commit. Do not move existing release
tags. Ordinary development builds do not require a new version or tag.
