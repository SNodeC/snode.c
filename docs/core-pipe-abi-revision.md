# Core pipe ABI revision

The redesign of `core::pipe::Pipe`, `PipeSource`, and `PipeSink` changes installed public C++ class layouts and semantics. This revision is not binary-compatible with earlier core builds.

All SNode.C libraries and every consuming application or plugin must be rebuilt. Binaries built against the old and new core must not be mixed.

Before any binary release, release engineering must assign a new SONAME or bump the project major version consistently for `snodec-core` and affected dependent components. The project version remains unchanged by this development correction; that must not be interpreted as a binary-compatibility claim.
