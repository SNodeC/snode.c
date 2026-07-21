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
start [--cwd <path>] [--model <model>]
      [--model-provider <provider>]
      [--approval-policy <policy>]
      [--sandbox-mode <mode>]
      [--ephemeral]
resume <thread-id>
       [--cwd <path>] [--model <model>]
       [--model-provider <provider>]
       [--approval-policy <policy>]
       [--sandbox-mode <mode>]
new [thread-start-options] -- <prompt>
new <prompt>
read <thread-id>
turn <thread-id> <prompt>
interrupt <thread-id> <turn-id>
raw <json>
watch on
watch off
```

Normal commands are encoded from the public typed frontend message classes.
`raw` accepts only JSON that the public frontend codec validates as a client
message. `watch` is local: disabling it suppresses event-batch presentation but
does not change backend state or synchronization.

## Thread lifecycle

Every connection begins as an observer. The client never acquires controller
ownership automatically. Run `acquire` before `start`, `resume`, `new`, or
`turn`; otherwise the backend reports its normal `permission_denied` response.
`release` gives up that ownership.

To create a thread and submit its first turn explicitly, use the thread ID
reported by `start`:

```text
acquire
start --cwd /home/voc/projects/snode.c
turn <returned-thread-id> Review the repository.
```

`start` maps directly to Frontend Protocol v1 `thread.start`. Its options are
optional and remain unset when omitted, allowing Codex defaults to apply. A
successful human-readable response identifies the returned thread ID.

To continue an existing persisted thread:

```text
acquire
threads
resume <thread-id>
turn <thread-id> Continue the previous task.
```

`threads` can report a persisted thread whose completeness is `notLoaded`.
`resume` maps directly to `thread.resume` and loads that thread into the running
Codex App Server before a later `turn` command. It accepts the same overrides as
`start` except `--ephemeral`, which is not part of `thread.resume`.

> `read` retrieves thread data but does not resume or load the thread into the
> running Codex App Server. Use `resume <thread-id>` before starting a turn on a
> persisted `notLoaded` thread.

For the common create-and-prompt workflow, `new` performs both client-side
steps:

```text
acquire
new --cwd /home/voc/projects/snode.c -- Review the repository.
```

With no thread-start options, the separator may be omitted:

```text
acquire
new Explain the current repository architecture.
```

When options are present, `--` ends option parsing and everything after it is
the prompt, including words that begin with `--`. `new` is implemented only by
`codex-backend-client`: it sends `thread.start`, waits for that request's
successful response, extracts and validates `result.thread.id`, and then sends
`turn.start` with one text input containing the complete prompt. It does not add
a backend command or Frontend Protocol method, and it does not establish
implicit current-thread state.

If thread creation succeeds but the initial turn cannot be submitted or later
fails, the diagnostic preserves the created thread ID and reports the compound
operation as failed. The successful thread creation is not rolled back.

Human mode reports when the connection is waiting for its initial
synchronization and when commands are ready. Commands entered during that
handshake are acknowledged as queued and remain behind the `sync.complete`
barrier.

Human-readable output is the default. It gives concise stage summaries for
started and resumed threads and for both stages of `new`, rather than dumping
complete snapshots.

`--json` writes each decoded server message as one compact protocol JSON object
on stdout; connection notices, local command errors, and other diagnostics
remain on stderr so stdout can be consumed by scripts. For `new`, stdout
contains the complete real responses and events for its underlying
`thread.start` and `turn.start` requests. There is no synthetic `new` protocol
message and the original protocol messages are not hidden or rewritten.

Stdin and the Unix socket are both integrated with the SNode.C event loop.
Terminal and pipe input is nonblocking and line-buffered; regular-file stdin is
rejected because POSIX cannot make those reads nonblocking (pipe the file's
contents instead). Socket JSONL framing tolerates both fragmented records and
several records in one read.

Piped commands are retained until the connection's initial `sync.complete`,
then sent once in input order. EOF enters a deterministic drain: the client
waits for every command's correlated response and, for `snapshot` and
`replay`, the resulting `sync.complete` before disconnecting and exiting. A
pending `new` keeps the client alive through both its start and turn stages.
Connection, protocol, send, compound-operation, or premature-disconnect
failures during that drain produce a nonzero exit status. An explicit
interactive `quit` remains an immediate shutdown.

For example, this waits for the handshake and both command responses before
returning:

```sh
printf 'acquire\nthreads\n' | codex-backend-client --json
```

This piped convenience workflow likewise waits for the generated thread ID to
be handed from `thread.start` to `turn.start` before returning:

```sh
printf '%s\n' \
  'acquire' \
  'new --cwd /home/voc/projects/snode.c -- Review the repository.' \
  | codex-backend-client --json
```
