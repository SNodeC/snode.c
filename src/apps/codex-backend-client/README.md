# Codex backend reference client

`codex-backend-client` is a small terminal client for the local
`codex-backend` reference server. It speaks only Codex Frontend Protocol v1
over an SNode.C Unix-domain stream connection; it never connects to the Codex
App Server directly.

Start the backend in one terminal and the client in another:

```sh
codex-backend
codex-backend-client
```

The client connects to
`$XDG_RUNTIME_DIR/snodec-codex-backend.sock` when `XDG_RUNTIME_DIR` is set and
nonempty. Otherwise it uses
`/tmp/snodec-codex-backend-<numeric-uid>.sock`.

The ordinary SNode.C remote-address option overrides that default:

```sh
codex-backend-client codex-backend-client remote \
  --sun-path /run/user/1000/my-codex-backend.sock
```

The client sends `hello` automatically after connecting. Interactive commands
are:

```text
help
quit
snapshot
replay <sequence>
acquire
release
threads
read <thread-id>
turn <thread-id> <text>
interrupt <thread-id> <turn-id>
raw <json>
watch on
watch off
```

Normal commands are encoded from the public typed frontend message classes.
`raw` accepts only JSON that the public frontend codec validates as a client
message. `watch` is local: disabling it suppresses event-batch presentation but
does not change backend state or synchronization.

Human-readable output is the default. `--json` writes each decoded server
message as one compact protocol JSON object on stdout; connection notices,
local command errors, and other diagnostics remain on stderr so stdout can be
consumed by scripts.

Stdin and the Unix socket are both integrated with the SNode.C event loop.
Terminal and pipe input is nonblocking and line-buffered; regular-file stdin is
rejected because POSIX cannot make those reads nonblocking (pipe the file's
contents instead). Socket JSONL framing tolerates both fragmented records and
several records in one read. `quit` and stdin EOF close the client connection
before stopping the event loop.
