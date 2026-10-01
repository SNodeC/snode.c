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

Never create, move, or push a version tag without the user's explicit approval for that
tag and action. Approval to modify code, commit, push a branch, or run CI does not
authorize creating or pushing version tags. Do not infer release approval from
the versioning policy.

For an explicitly approved release, commit the matching VERSION file and create the
`vMAJOR.MINOR.PATCH` tag on that commit. Moving and force-pushing a release
tag also requires explicit approval for that tag and action. Ordinary development builds do not require a new version or tag.

## README documentation policy

- Never mention or link to the SNode.C book or its repository in any README or supporting README guide. This prohibition includes root links, deep links to examples (including WebSocket and Server-Sent Events), assets, raw-content URLs, reference-style or HTML links, alternate labels and redirects. Do not disguise or replace a direct book link with an indirect one.
- Include the required examples directly in the README collection, with the code, configuration, prerequisites, run instructions and expected results needed to follow them. Do not delegate required example content to the book repository.
- Check both link text and destinations before handing off README changes, including generated previews. Preserve one physical source line per prose paragraph.
- Follow the recorded owner requirements in [README owner findings](docs/readme-owner-findings.md); do not mark pending changes complete merely because they have been recorded.
