# README owner findings — 1 October 2026

These are the owner's requirements for the SNode.C and MQTTSuite README redesign, recorded in both repositories for coordinated follow-up. This record does not implement the pending README, example or graphic changes.

## 1. SNode.C banner — approved wording, pending implementation

Use exactly **“Your own protocols · HTTP · WebSocket · SSE · MQTT”**. The complete enumeration must appear on **one line**, never as a custom-protocol slogan followed by a separate built-in-protocol line. This makes custom factories and contexts visible alongside the ready-made protocols.

## 2. SNode.C echo example — pending implementation

Show both a native SNode.C server and the corresponding native SNode.C client. Reuse the same context implementation, with an explicit `Role` enum distinguishing client and server behavior. A reusable factory parameterized by the role enum is an acceptable option, not a mandatory design choice. Retain the Python client as an additional interoperability example.

## 3. Installation structure — pending implementation in both projects

Both installation presentations must start with **“Choose your route”** and use exactly the same section structure and ordering for installation routes. Commands, package names and requirements remain project-specific.

## 4. Book references and links — prohibition recorded; removal verification pending

The owner reports remaining links to the SNode.C book repository, including WebSocket and Server-Sent Events examples. Their locations have not been re-verified as part of recording these findings; the earlier draft validation must not be treated as proof that every repository or published version is clean.

No README or supporting README guide may mention or link to the book or its repository. This includes repository-root links, deep links to examples or assets, raw-content URLs, reference-style links, HTML links, alternate link labels and redirects used to conceal the destination. Examples must be directly included in the README collection rather than delegated to that repository.

The prohibition is now stated in each repository's `AGENTS.md`. Inspect the actual publication files and linked example destinations before marking removal complete.

## Publication status

The owner subsequently authorized replacing the repository's root README with the new draft and copying its supporting guides, example configurations and graphics into `docs/readme/`. That collection is copied as drafted; findings 1–3 are not implemented by the copy. The prohibition is recorded in `AGENTS.md`; the copied collection is checked for forbidden references before committing. Earlier local or published versions are not thereby certified as clean.
