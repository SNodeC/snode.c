# Core pipe ABI revision

The redesign of `core::pipe::Pipe`, `PipeSource`, and `PipeSink` changes installed public C++ class layouts and semantics. This revision is not binary-compatible with earlier core builds.

The shutdown notification callback added to `core::DescriptorEventReceiver` also changes that public class's virtual interface. It has the same rebuild and binary-mixing restrictions.

`DescriptorEventPublisher::enable()` now returns registration success, and its
multiplexer `muxAdd()` virtual contract now returns `bool`. This also changes the
core C++ ABI and is covered by the same rebuild requirement.

All SNode.C libraries and every consuming application or plugin must be rebuilt. Binaries built against the old and new core must not be mixed.

SNode.C 2.0 assigns the required new ABI epoch consistently: the project version is `2.0.0` and all SNode.C shared libraries use `SOVERSION 2`. Binaries linked against the SNode.C 1.x SONAMEs cannot be mixed with SNode.C 2.0. See [Migrating to SNode.C 2.0](migration-2.0.md) for the complete rebuild and source-migration guidance.

`Pipe::isValid()` now means that the object owns at least one endpoint. Code
that requires a newly created complete pipe must test both `hasReadFd()` and
`hasWriteFd()`.

Transfers through `releaseReadAsSink()` and `releaseWriteAsSource()` verify or
set `O_NONBLOCK` and complete descriptor registration before ownership moves
from `Pipe`. A failed transfer leaves the endpoint owned by `Pipe` and reports
the failure to the caller.

`PipeSource` and `PipeSink` are final, self-owned EventReceivers. Their
construction and destruction are not public; normal creation is through
`Pipe`. `close()` and `eof()` initiate asynchronous removal and destruction.
Closure callbacks must not delete the receiver and must not use its pointer
after the callback returns.
