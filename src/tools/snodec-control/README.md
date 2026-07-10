# snodec-control

`snodec-control` is a companion command-line tool for inspecting and configuring SNode.C-based
applications. It is a **standalone, out-of-tree project**: it is not part of the SNode.C CMake build,
is never `add_subdirectory()`'d by it, and links against none of its libraries. It only needs a C++20
standard library and POSIX (`posix_spawn`, `pipe`, `select`, `mkstemp`).

* **Phase 1** implemented discovery and static inspection: parsing a target's `-s`/`--show-config`
  output into an in-memory model, and printing/exporting it (`--dump-model`, `--print-summary`,
  `--write-template`, `--materialize`).
* **Phase 2** turns that into a scriptable configuration *workflow*: look up and edit individual options
  from the command line, preview the effect of edits, validate that required options are set, materialize
  an edited config, ask the target to write its own canonical config file, and optionally run the target
  with a chosen configuration.
* **Phase 3** (this phase) adds `--ui`: an interactive, `make menuconfig`-style terminal UI (Curses-based)
  that presents the same configuration as a navigable hierarchy, on top of exactly the same discovery,
  editing, save, and run machinery Phase 2 already implements (see "Interactive UI (`--ui`)" below).

## What snodec-control does

Every SNode.C application built on `utils::Config`/`utils::SubCommand` accepts a `-s`/`--show-config`
flag that prints an INI-style configuration template describing all of its configurable options,
grouped by instance/section, together with descriptions, default values, and (if any) currently active
values. It also accepts `--config-file <path>` to read a config file and `--write-config <path>` (`-w`)
to have it write its own canonical config file and exit.

`snodec-control` treats a target application as an **opaque, separate executable** throughout:

1. **Discovery**: it runs `<target> [target-args...] -s` as a child process (via `posix_spawn`),
   capturing stdout and stderr separately, and parses the captured stdout as the SNode.C `-s`
   configuration template into an in-memory model (sections, options, descriptions, default/active
   values, required markers).
2. **Editing** (Phase 2): `--set`/`--unset` mutate that in-memory model directly; nothing is sent back
   to the target during editing itself.
3. **Materializing**: the (possibly edited) model can be rendered to a clean, readable INI file, printed
   as JSON, or summarized.
4. **Saving** (Phase 2): to get a *canonical* config file (in the target's own exact format), the edited
   model is materialized to a temporary file and the target is invoked out-of-process with
   `--config-file <temp> --write-config <output>`; the target performs the actual, authoritative write.
5. **Running** (Phase 2): the target can be launched out-of-process with a chosen config file, inheriting
   the terminal's stdin/stdout/stderr.
6. **Interactive editing** (Phase 3): `--ui` opens a Curses-based tree view of the same in-memory model,
   letting you navigate, edit, save, run, diff, and check-required interactively instead of one shot at a
   time from the command line. It calls the exact same underlying functions as the plain CLI (see
   `src/ConfigActions.h`); nothing about discovery, saving, or running is reimplemented for the UI.

**The target's own `-s`/`--show-config` output remains the sole discovery mechanism.** No other
introspection (headers, human `--help` text, source code, etc.) is used, and the target always performs
its own final validation — snodec-control's checks are local preflight conveniences only.

## Explicitly out of scope / architectural constraints

* **Out-of-tree.** Not built as part of the SNode.C source tree, not wired into its CMake, no
  dependency on its libraries.
* **No `dlopen()`.** The target is never loaded into `snodec-control`'s own address space.
* **No calling the target's `main()`.** The target always runs as its own process.
* **No parsing of human `--help` output.** Only `-s`/`--show-config` output is parsed.
* **No vendored/third-party UI toolkit.** The interactive UI is built exclusively on Curses via CMake's
  `find_package(Curses)`; nothing like FTXUI is fetched or bundled.

## Building

```sh
cd snodec-control
mkdir build && cd build
cmake -DSNODEC_CONTROL_BUILD_TESTS=ON ..
cmake --build .
```

This produces `build/src/snodec-control`. Relevant CMake options:

| Option | Default | Effect |
| --- | --- | --- |
| `SNODEC_CONTROL_BUILD_TESTS` | `OFF` | Also build the unit tests (`ctest` in the build directory) |
| `SNODEC_CONTROL_BUILD_TUI` | `ON` | Build interactive UI support (`--ui`) if Curses (`find_package(Curses)`) is found |

If `SNODEC_CONTROL_BUILD_TUI` is `OFF`, or it is `ON` but Curses isn't found on the system, the build
still succeeds and every non-UI feature works exactly as before — only `--ui` itself fails at runtime
with a clear message:

```text
Error: Interactive UI support was not built because Curses was not found.
```

CMake also prints which case applies at configure time, e.g.:

```text
-- snodec-control: building with interactive UI support (--ui), Curses found
```

To install Curses development headers if they are missing: `apt install libncurses-dev` (Debian/Ubuntu)
or the equivalent for your distribution.

## Usage

```sh
snodec-control --target <path> [options]
```

| Option | Description |
| --- | --- |
| `--target <path>` | Path to the target SNode.C application executable (required) |
| `--target-args "<args>"` | Extra arguments passed to the target before `-s` (discovery), `--config-file`/`--write-config` (save), or its own args (run) |
| `--dump-model` | Print the parsed configuration model as JSON |
| `--print-summary` | Print a human-readable summary (target, section/option counts, per-section breakdown) |
| `--list-options` | Print a stable, readable list of every parsed option |
| `--get <key>` | Print one option, looked up by full key (`section.key`) or bare key |
| `--write-template <file>` | Write the target's raw `-s` output verbatim to `<file>` |
| `--set <key=value>` | Set an option's active value (repeatable) |
| `--unset <key>` | Clear an option's active value, falling back to its default (repeatable) |
| `--allow-new-options` | Let `--set`/`--unset` create unknown keys instead of erroring |
| `--diff` | Print a summary of the changes made by `--set`/`--unset` (and by `--ui`, if used) |
| `--ui` | Open the interactive Curses UI after applying any `--set`/`--unset`; other requested actions run afterwards using the edited model (see below) |
| `--check-required` | Fail if any required option still has no usable value |
| `--materialize <file>` | Write a clean, editable INI config file derived from the (edited) model; `-` writes to stdout |
| `--save-config <file>` | Ask the target to write its canonical config file, from the (edited) model |
| `--run-config <file>` | Use an existing config file for `--run`/`--print-run-command` |
| `--run` | Run the target, with config file resolution described below |
| `--print-run-command` | Print the exact command `--run` would execute, shell-escaped, without running it |
| `--keep-temp` | Keep and print the path of any temporary materialized config file |
| `--dry-run` | Discover, edit, and check, but do not save, run, or write output files (`--materialize -` still prints to stdout) |
| `-h`, `--help` | Show usage and exit |

Multiple actions may be combined in a single invocation.

### Editing (`--set` / `--unset`)

```sh
snodec-control --target ./app --set log-level=5 --set echoserver.local.port=8080
```

* Each `--set key=value` splits only at the **first** `=`; everything after it, including further `=`
  characters, is the value verbatim (no shell quoting is required or interpreted — argv already gives
  each `--set` its own token).
* An empty value after `=` is valid: `--set name=` sets the option to the empty string, distinct from
  leaving it unset.
* `--unset key` clears the active value, falling back to the option's default (or an empty value if it
  has none) wherever the option is subsequently materialized or saved.
* Unknown keys are errors by default; pass `--allow-new-options` to let `--set`/`--unset` create a new
  option instead (section/key are derived from the dotted name the same way discovery does).
* A key may be given as its full, dotted name (`echoserver.local.port`) or as a bare key (`port`) if that
  bare key is unambiguous across all sections; an ambiguous bare key is reported as an error listing all
  matching full keys.
* If **any** `--set`/`--unset` is malformed, unknown (without `--allow-new-options`), or ambiguous,
  snodec-control reports all such errors and exits without performing any other requested action —
  editing errors are treated as fatal input mistakes rather than silently producing a half-edited model.

### Diffing (`--diff`)

```text
Changed options:
  echoserver.local.port: <REQUIRED> -> 8080
  log-level: 4 -> 5
  log-file: /var/log/snode.c/app.log -> <unset>

No option changes.
```

Only edits that actually changed an option's effective value are reported (setting a value to what it
already effectively was, or unsetting an option that had no active value, produce nothing).

### Validation (`--check-required`)

Reports every required option that, after any edits, still has no usable active value and no usable
default (including the case where the default is itself literally `<REQUIRED>`). This is a **local**
preflight check only — the target application always performs its own, final validation when it is
actually run or asked to save a config file.

### Saving through the target (`--save-config <file>`)

```sh
snodec-control --target ./app --set echoserver.local.port=8080 --save-config /tmp/app.conf
```

snodec-control never claims its own materialized file *is* the canonical config. Instead it:

1. Materializes the (edited) model to a temporary file.
2. Invokes the target out-of-process as `<target> [target-args] --config-file <temp> --write-config
   <file>` — the target itself performs the canonical write.
3. Captures the target's stdout/stderr and inspects whether `<file>` was actually written (created, or
   its modification time changed), since SNode.C applications conventionally exit non-zero even on a
   successful `--write-config` (the same behavior Phase 1 documented for `-s`).
4. Removes the temporary file (unless `--keep-temp`, which also prints its path).

If `--target-args` already specifies `--config-file`/`-c`, snodec-control refuses to append its own and
reports the conflict clearly, rather than silently producing a confusing double-specified command line.

### Running (`--run`, `--run-config`, `--print-run-command`)

Config file resolution for `--run` (and for the preview shown by `--print-run-command`), in priority
order:

1. `--run-config <file>` if given (the file must exist).
2. Otherwise, the file just written by `--save-config <file>` in the same invocation, if that save
   succeeded.
3. Otherwise, if any `--set`/`--unset` edits were given, a temporary materialized config (cleaned up
   afterwards unless `--keep-temp`).
4. Otherwise, no config file at all — the target runs with just `--target-args`.

`--run` uses an "attached" process mode: the target inherits snodec-control's own stdin/stdout/stderr
directly (nothing is buffered or captured), snodec-control waits for it, and the target's own exit code
becomes snodec-control's exit code. `--print-run-command` shows the exact, shell-escaped command line
that `--run` would execute, without executing it; combine both flags to see the command and then run it.

### Dry run (`--dry-run`)

Performs discovery, editing, and `--check-required`/`--diff`/`--get`/`--list-options` reporting exactly
as normal, but never actually writes an output file, invokes `--save-config`, or executes `--run` —
each of those instead prints a `[dry-run]` line describing what it would have done. The one exception is
`--materialize -`, which explicitly asks for stdout output and is not suppressed.

## Interactive UI (`--ui`)

`--ui` opens a Curses-based terminal UI presenting the configuration as a hierarchy that mirrors
SNode.C's own vocabulary — **not** a flat list of sections and options.

### Hierarchy derivation

Since Phase 4, `--ui` no longer guesses the hierarchy from dotted option names. SNode.C's `-s`/
`--show-config` output carries structured `"#@ snodec.meta ..."` comment metadata describing the
real CLI11/SNode.C tree — every application/subcommand `node` (with its `kind`, e.g.
`application`/`instance`/`section`/`anonymous`/`category`/`subcommand`, and its `path`, an array of
arbitrary length), every option `group`, and every `option`'s own placement. When a target emits
this metadata (any SNode.C build from this Phase 4 work onward), snodec-control builds the tree
directly from it:

```text
Application                        (kind: application; the tree's sole top-level node)
  <option>                         (every option shown here is already persistent/configurable, so
  <option>                          the implicit "Options (persistent)"/"default" group it belongs
                                     to is never shown as its own node - it would carry no
                                     information the UI needs; see "Groups" below)
  <node>                           (kind: instance/section/category/anonymous/subcommand/... -
    <node>                          whatever the target's own CLI11 tree actually contains, at
      <node>                        whatever depth it actually occurs - not capped at 3 levels)
        <option>
```

This is a genuine N-ary tree keyed by each node's `path`, so a target with arbitrarily nested,
arbitrarily shaped user-defined `SubCommand`s (not just the built-in `Instances`/`Sections`
pattern) is represented correctly, however deep or irregular its structure is. `kind`/`kindSource`
from the metadata are shown as-is rather than re-derived; where `kindSource` says the target's own
classification was itself a heuristic (e.g. `heuristic-cli11-group`), the UI says so rather than
presenting it as certain. A disabled node (metadata's `disabled` field) is marked `(disabled)` and
still fully navigable — SNode.C's disable/enable machinery only suppresses required/needs state, it
does not remove the node from the tree. An option present in the metadata but never matched back to
a discovered `ConfigOption` (or vice versa — not expected given both come from the same capture, but
handled defensively) is collected into an **"Unmatched Options"** node so nothing is ever silently
dropped — except a parsed option that only ever came from a commented "#key=value"-shaped line with
no matching metadata entry at all (see "Parser robustness" below): that one is discarded outright as
a parsing artifact, not shown as unmatched, since it was never a real option to begin with.

#### Groups

A metadata `group` becomes its own visible node **only** when it is semantically meaningful — a real,
named, non-default group (a custom group name a target explicitly assigns). The implicit groups every
option ends up in purely by virtue of being shown at all are flattened, i.e. their options attach
directly to the owning node instead of nesting one level deeper under a redundant group node:

* `kind == "default"` or `kind == "persistent"` (including the literal names `"Options"`/`"Options
  (persistent)"`, or no group name at all) — every option in the config UI is already
  persistent/configurable, so this layer is pure noise.
* `kind == "nonpersistent"` (`"Options (nonpersistent)"`) — excluded from the tree entirely, not
  merely flattened. In practice this cannot currently occur (SNode.C's formatter never emits a
  non-configurable option into `-s` output at all), but the config editor is deliberately scoped to
  writable/persistent options, so this is handled defensively rather than assumed impossible.

For a target built against an older SNode.C that predates this metadata format, `--ui` falls back
automatically to the original dotted-name heuristic, unchanged from Phase 1–3:

```text
Application
  Application Options            (dot-less, persistent options: log-level, verbose-level, daemonize, ...)
  Instances                      (runtime instances, e.g. "echoserver", "mqttbridge")
    <instance>
      Sections                   (subsections of the instance; a Section is flat, e.g.
        <section>                 "broker.connection" is one Section, never nested)
          <option>
  Other Options                  (fallback: anything that can't be confidently attributed to an
                                   instance; always shown, never silently dropped)
```

1. An option with **no dot at all** (e.g. `log-level`) is an **Application Option**.
2. An option whose section prefix **itself contains a dot** (3+ total components, e.g.
   `echoserver.local.port` or `mqttbridge.broker.connection.host`) becomes: Instance = the prefix's first
   component (`echoserver`, `mqttbridge`), Section = the rest of the prefix taken as **one flat name**
   (`local`, `broker.connection` — never nested further), Option = the trailing key (`port`, `host`).
3. An option whose section prefix has **exactly one component** (2 total components, e.g.
   `echoserver.enabled`) is ambiguous on its own. It resolves to a **direct child of that Instance** (no
   Section level) if that same prefix is also seen as a confirmed instance via rule 2 elsewhere in the
   model; otherwise it is treated as not-clearly-instance-owned.
4. Anything from rule 3's "otherwise" case is collected into the **"Other Options"** fallback node.
5. Instances, their sections, and their options preserve the order the target's own `-s` output listed
   them in.

Both paths are exercised by the same call site (`ui::runInteractiveUi`/`ui::UiState`), driven by
whether the target's captured `-s` output carried usable metadata — a fleet of targets built against
different SNode.C versions is handled transparently without any flag or configuration.

### Type-aware editing and relations

When metadata is available, each option also carries a `type.kind` (`boolean`/`integer`/`number`/
`string`) and `type.items` (`single`/`list`). `type.kind` is explicitly a heuristic on the target's
own side (SNode.C's formatter derives it from the option's CLI11 flag-ness and its declared type
name, not from any real type system), so snodec-control does not trust it to *drive* editing
behavior — the existing literal-value detection (an active/default value of exactly `"true"`/
`"false"`, or `"default"` for tristate) remains the sole signal for the `Space` boolean/tristate
cycle, since it is actually more precise than `type.kind` (which labels every flag-like option
"boolean", tristates included). What metadata *does* add: `type.items == "list"` now excludes a
multi-value option from `Space` cycling entirely, even if one of its several values happens to read
`"true"`/`"false"`/`"default"` — previously such an option could be misidentified as a scalar
toggle.

`relations.needs`/`relations.excludes` (option names this option needs/excludes) and the type/
required-state fields are shown in the option detail view (the `D`/status-line detail text also used
by `formatListOptions`/`--check-required`), each labeled with its provenance rather than presented
as certain. Per SNode.C's own documented limitation, this only reports *available* relation names —
it does not restore or infer any suppressed relation state.

### Key bindings

| Key(s) | Action |
| --- | --- |
| `Up`/`Down`, `PageUp`/`PageDown`, `Home`/`End` | Move the selection |
| `Right` | On a collapsed container: expand it. On an already-expanded container: move to its first child. On an option: nothing (options don't expand) — a status message points you to `Enter` instead |
| `Left` | On an expanded container: collapse it (selection stays on that same node). Otherwise (a leaf, or a collapsed/top-level node): move to the parent, if any |
| `Enter` | On a container: expand/collapse (same as `Right`/`Left`'s expand/collapse action). On an option: edit its value (text prompt) |
| `Space` | **Only** cycles a recognized boolean/tristate option's value (see below); does nothing else — not on containers, not on non-boolean options. A status message explains why when it doesn't apply |
| `S` | Save: materialize and ask the target to write its canonical config (`--save-config` semantics; see "Save path behavior" below) |
| `R` | Run: hands the terminal over to the target until it exits (see "Run and the terminal" below) |
| `D` | Diff: show all changes made so far (pre-`--ui` `--set`/`--unset` combined with in-UI edits) |
| `C` | Check required: list any required option still without a usable value |
| `M` | Materialize: write the edited model to a chosen file (`--materialize` semantics) |
| `H`, `F1` | Show the key-binding help screen |
| `Q` | Quit immediately if there are no unsaved changes; otherwise prompts to keep, discard, or cancel |

Left/Right are true tree navigation, not just "collapse if expanded, expand if collapsed": `Right` on an
already-expanded container moves *into* it (to its first child) instead of doing nothing, and `Left` on
anything that isn't itself expanded moves *up* to its parent instead of doing nothing. Every key that
doesn't apply to the current selection (`Space` on a non-boolean option, `Right` on an option, an
`Escape`d or otherwise canceled Save/Run/Materialize) leaves a short explanation in the footer rather than
silently doing nothing.

Editing a text value shows the current effective value prefilled with the cursor at the end; `Left`/
`Right` move the cursor, `Home`/`End` jump to the start/end, `Backspace`/`Delete` remove the character
before/at the cursor, and typing inserts at the cursor (not just at the end). An empty value is allowed;
`Escape` cancels without changing anything; `Enter` accepts.

#### Boolean and tristate cycling (`Space`)

`Space` only ever touches an option recognized as boolean or tristate, and always acts on its *current
effective value* (active value if set, else its default), so the visible value always changes on every
press — including when the option currently has no active value at all and is only showing its default:

* **Plain boolean** (effective value is literally `true` or `false`, and the option's default — if any —
  is not the literal string `default`): `Space` flips it, `false → true → false → ...`, always by setting
  an explicit active value.
* **Tristate** (the option's parsed default is literally the string `default` — SNode.C's own tristate
  marker, meaning "let the target decide"): `Space` cycles `false → true → default → false → ...`,
  always setting one of these three literal strings explicitly (never merely clearing the active value),
  so every step is visibly distinct regardless of whether the option started out unset, or with any of
  the three values already set explicitly.

Any other option (a string, a number, anything not built from exactly `true`/`false`/`default`) is left
alone by `Space`; press `Enter` to edit it as text instead.

A required option with no usable value is marked with a leading `!` in the tree (e.g. `! port =
<REQUIRED>`). This never blocks navigating away from it or quitting the UI, but Save and Run both warn
and ask for confirmation first if any required option is still unmet, mirroring `--check-required`.

#### Save path behavior (`S`)

`S` prefills its path prompt rather than always starting blank or silently reusing a path without
confirmation:

1. If a path was already used to save successfully earlier in this same `--ui` session, that path is
   the prefill.
2. Otherwise, if `--save-config <file>` was given on the command line before `--ui` opened, that path is
   the prefill.
3. Otherwise, the prompt starts empty.

In every case the path is shown as an editable, already-accepted prompt (`Enter` confirms it as-is,
`Escape` cancels the save entirely, or edit it first) — never a blind, unconfirmed write. A successful
save clears the `[Modified]` indicator (see below); it does not, by itself, affect what a later `--diff`
after `--ui` closes reports (that always reflects the full session's edits, saved or not).

#### Quit and the Modified indicator

The title bar shows `[Modified]` whenever there are changes since the UI opened (or since the last
successful Save) that haven't been saved. A successful Save clears it immediately. Quitting (`Q`):

* If nothing is unsaved (`[Modified]` is not shown): exits immediately, no prompt.
* Otherwise: shows the diff of the unsaved changes and asks **(K)eep** changes and quit (they still feed
  into any `--diff`/`--save-config`/`--run`/etc. requested on the same command line, after the UI
  closes), **(D)iscard** them and quit (restores the model to exactly how it looked when the UI opened,
  discarding *all* edits made in the UI, saved or not), or **(C)ancel**/`Escape` (stay in the UI).

#### Run and the terminal (`R`)

`R` needs a config file exactly like `--run` does: if nothing has been saved yet in this session, it asks
whether to **(S)ave now and run**, run with a **(T)emporary** materialized config, or **(C)ancel**;
otherwise it reuses the path last saved to.

Once a config file is settled, `snodec-control` hands the *real terminal* over to the target for the
duration of the run — this is not a captured/log-pane view:

1. Curses is suspended (`def_prog_mode()`/`endwin()`), returning the terminal to normal mode.
2. The target runs attached and becomes the terminal's **foreground process group**: it inherits stdin/
   stdout/stderr directly (its own output appears directly in the terminal, and it can read from stdin
   itself if it wants to), *and* it is placed in its own process group which is then given the terminal's
   foreground job via `tcsetpgrp()`. This means **Ctrl-C (and Ctrl-\/Ctrl-Z) stop the target, not
   `snodec-control`** — the kernel delivers terminal-generated signals only to whichever process group
   currently owns the terminal's foreground job, so once the target owns it, `snodec-control` (still
   running, in its own, now-background process group) is unaffected.
3. `snodec-control` waits for the target to exit (however it exits — normally, or via a signal like
   Ctrl-C's `SIGINT`), reclaims the terminal's foreground job for itself, then prints the target's exit
   status and `Press any key to return to snodec-control...`, and waits for exactly one keypress (not a
   full `Enter`-terminated line).
4. Curses is resumed (`reset_prog_mode()` + a redraw) and the full tree UI reappears, fully interactive
   again.

`snodec-control` itself stays running throughout — the target is a child process, never `exec()`'d in
its place — which is what makes surviving a Ctrl-C'd target and resuming the UI afterward possible. This
foreground-job handover is specific to the UI's `R`; the plain CLI's `--run` deliberately keeps its
simpler behavior (sharing `snodec-control`'s own foreground process group with the target), since there
`snodec-control` ending together with the target on Ctrl-C is normal, expected shell behavior — there is
no UI session to preserve.

### `--ui` combined with other flags

```sh
snodec-control --target ./app --set log-level=5 --ui --diff --save-config /tmp/app.conf --run
```

Processing order for a single invocation:

1. Discovery runs once, as always.
2. Any `--set`/`--unset` given on the command line are applied first.
3. `--ui` opens next, starting from that already-edited model; further edits made inside the UI are
   layered on top of it.
4. After the UI is closed (whether by quitting with changes kept or discarded), any other explicitly
   requested actions (`--diff`, `--check-required`, `--materialize`, `--save-config`, `--run`,
   `--print-run-command`) run exactly as in Phase 2, using the model as the UI left it. Discarding changes
   inside the UI does **not** cancel these other explicitly requested actions — it only reverts the
   model's edits before they run.

If this build has no Curses support, `--ui` alone fails clearly (see "Building" above) while every other
flag on the same command line still runs normally.

### Examples

```sh
# Phase 1: inspect
snodec-control --target /path/to/mqttbroker --print-summary
snodec-control --target /path/to/mqttbroker --dump-model
snodec-control --target /path/to/mqttbroker --write-template /tmp/mqttbroker.conf

# Phase 2: browse and query
snodec-control --target /path/to/mqttbroker --list-options
snodec-control --target /path/to/mqttbroker --get echoserver.local.port

# Phase 2: edit and preview
snodec-control --target /path/to/mqttbroker \
    --set echoserver.local.port=8080 --set log-level=5 --diff

# Phase 2: edit and materialize
snodec-control --target /path/to/mqttbroker \
    --set echoserver.local.port=8080 --materialize /tmp/edited.conf

# Phase 2: validate before doing anything else
snodec-control --target /path/to/mqttbroker --check-required

# Phase 2: save the canonical config through the target itself
snodec-control --target /path/to/mqttbroker \
    --set echoserver.local.port=8080 --save-config /tmp/app.conf

# Phase 2: run with an existing config
snodec-control --target /path/to/mqttbroker --run-config /tmp/app.conf --run

# Phase 2: edit, save, then run with the saved config
snodec-control --target /path/to/mqttbroker \
    --set echoserver.local.port=8080 --save-config /tmp/app.conf --run

# Phase 2: preview the exact run command without executing it
snodec-control --target /path/to/mqttbroker --run-config /tmp/app.conf --print-run-command

# Phase 3: open the interactive UI
snodec-control --target /path/to/mqttbroker --ui

# Phase 3: pre-apply an edit, then let the UI take over, then diff and run afterwards
snodec-control --target /path/to/mqttbroker \
    --set log-level=5 --ui --diff --save-config /tmp/app.conf --run
```

## The configuration model

```cpp
struct ConfigOption {
    std::string section;
    std::string key;
    std::string description;
    std::optional<std::string> defaultValue;
    std::optional<std::string> activeValue;
    bool required = false;
    bool hasActiveValue = false;
};

struct ConfigSection {
    std::string name;
    std::vector<ConfigOption> options;
};
```

Sections are derived from the dotted prefix of each option's fully qualified name (e.g.
`echoserver.local.port` becomes section `echoserver.local`, key `port`); options with no dot belong to
the unnamed root/global section. Explicit `[section]` bracket headers, if a target ever emits them, are
also honored and take precedence.

An option whose commented default is the literal placeholder `<REQUIRED>` is marked `required = true`
with no usable `defaultValue`. Empty string defaults (`key=""`) are preserved as a present-but-empty
value, distinct from an option that has no default at all. Note that `required` is persistent metadata
about the option — it stays `true` even after an active value has been set, so `--materialize`/`--get`
continue to show the "required" marker as documentation of the field's nature, not as a "still missing"
indicator (`--check-required` is the authoritative check for that).

## Code layout

* `src/ConfigModel.*`, `src/ConfigParser.*`, `src/ConfigEditor.*`, `src/CommandBuilder.*`,
  `src/Materializer.*`, `src/JsonWriter.*` — the process-agnostic model, parsing, editing, and rendering
  primitives (Phase 1/2).
* `src/Json.*` — a small, hand-rolled JSON value tree/parser (Phase 4), deliberately scoped to what
  SNode.C's `"#@ snodec.meta ..."` comment-metadata schema actually emits, not a general-purpose JSON
  library. No external dependency, matching SNode.C's own formatter-side choice.
* `src/Metadata.*` — decodes `"#@ snodec.meta begin/end <entity>"` blocks (`document`/`node`/`group`/
  `option`) into `ParsedMetadata`, checking schema/version and degrading gracefully per block (Phase 4).
  `ConfigModel.h`'s `applyMetadataToModel()` merges the decoded option fields (type/relations/required
  state) back onto the already-parsed `ConfigOption`s by matching on `key`.
* `src/ProcessRunner.*` — process execution: `runProcess()` (captured output, for discovery/save),
  `runProcessAttached()` (inherited stdio, for the plain CLI's `--run`), and
  `runProcessAttachedAsForegroundJob()` (inherited stdio *and* POSIX job control — its own process group,
  made the terminal's foreground job for the duration of the run — used only by the interactive UI's `R`,
  so Ctrl-C stops the target without also stopping `snodec-control`).
* `src/ConfigActions.*` — presentation-agnostic business logic (discovery, formatting, save/run
  resolution) shared verbatim between the plain CLI (`src/Cli.cpp`) and the interactive UI, returning
  strings/structs to print or display rather than writing to `stdout`/`stderr` directly — the plain CLI
  prints them immediately, the UI shows them in its own message screens.
* `src/ui/UiTree.*`, `src/ui/UiState.*` — the hierarchy derivation and interactive session state
  (selection/parent-child navigation, expand/collapse, dirty tracking, boolean/tristate cycling).
  Deliberately Curses-free, built into the always-available `snodec-control-ui-logic` library, and
  unit-tested without a real terminal.
* `src/ui/LineEditor.*` — `LineEditorBuffer`, the cursor-aware text-editing buffer behind the UI's value-
  and path-editing prompts (insert/delete/move-at-cursor). Curses-free and unit-tested on its own.
* `src/ui/RenderUtil.*` — `fitToWidth()`, the small helper that clips/pads a line to the terminal width
  so long option values or target paths never wrap into the next row or corrupt the layout.
* `src/ui/Ui.h` — the small public interface (`isUiAvailable()`, `runInteractiveUi()`) the CLI depends on.
* `src/ui/CursesUi.cpp` **or** `src/ui/UiUnavailable.cpp` — exactly one is compiled into the
  `snodec-control-ui` library depending on `SNODEC_CONTROL_BUILD_TUI`/Curses availability (see
  "Building"). `CursesUi.cpp` owns only rendering/input/prompting; it calls `ConfigActions.h` and
  `ConfigEditor.h` for everything else, the same as `Cli.cpp` does.

## Parser robustness

Two parsers run over the same captured `-s` output, independently:

* `parseShowConfigOutput()` (`ConfigParser.*`) is the original line-oriented INI parser (Phase 1),
  deliberately lenient: unrecognized lines are recorded as warnings (printed to stderr) and do not
  abort parsing. Recognized line forms include:

  * `#` comment/description lines
  * `##`/`###` group and section banner comments (SNode.C emits these; they are informational and do
    not affect the derived section structure)
  * `"#@ ..."` structured-metadata lines (Phase 4) — recognized and skipped outright by this parser
    (never folded into a description; see `parseMetaBlocks()` below for actually decoding them)
  * `#key=value` commented default/template lines, including quoted strings, empty string defaults
    (`""`), and the `<REQUIRED>` placeholder
  * `key=value` active (uncommented) lines
  * `[section]` bracket headers
  * blank lines

* `parseMetaBlocks()` (`Metadata.*`, Phase 4) separately scans the same text for `"#@ snodec.meta
  begin/end <entity>"` blocks and decodes each one's JSON body. `ParsedMetadata::usable()` is true only
  when metadata was present, its schema/version were recognized, and a root node decoded; a single
  malformed or unrecognized block is dropped with a warning without aborting the rest of the scan, and
  an unrecognized schema/version makes the whole result fall back to the legacy parser's output alone
  for hierarchy/typing purposes (the plain INI values themselves are unaffected either way, since
  `parseShowConfigOutput()` never depended on metadata being present).

### Reconciling the two parsers: real options vs. description-line artifacts

`parseShowConfigOutput()`'s `#key=value` recognition is deliberately broad (any comment line shaped
like `key = value`), which is exactly right for real commented default lines but also means an
option's own **multi-line description** can accidentally trigger it: SNode.C legitimately emits
descriptions containing explanatory `key = value`-shaped lines (e.g. `--sni-cert`'s description
includes a literal `sni = SNI of the virtual server` line documenting one of its sub-keys). Read in
isolation, that line is indistinguishable from a real commented default for a nonexistent option
`sni`.

When metadata is `usable()`, `applyMetadataToModel()` (`ConfigModel.*`) resolves this by treating
metadata as authoritative for option *identity*: each parsed `ConfigOption` tracks whether it came
from an active/uncommented line (`fromActiveLine`) and/or a commented default-looking line
(`fromCommentedDefaultLine`). A `ConfigOption` that is `fromCommentedDefaultLine` **and never**
`fromActiveLine`, with no metadata entry sharing its key, is discarded — it was never a real option,
just a comment line that happened to parse as one. An option backed by at least one real
active/uncommented line is always kept, metadata match or not, since it may be a genuine option (e.g.
one a user hand-set in a config file) that this particular metadata simply doesn't describe. This
runs only when metadata is present and usable; a legacy target with no metadata at all is not affected
by this reconciliation (the same false-positive parsing risk that existed before Phase 4 remains for
that case, since there is nothing to reconcile against).

## Known limitations

* The interactive UI's Run action hands the real terminal over to the target (Curses is fully suspended
  around it, then resumed; see "Run and the terminal" above), but there is no support for detaching,
  daemonizing, or monitoring a long-running target beyond waiting for it to exit and showing its status.
* Text editing in the UI supports cursor movement/insert/delete but not multi-line input, undo/redo, or
  clipboard operations; there is no in-UI incremental search/filter over the tree (`/`) — both remain
  reasonable follow-ups left out of this phase's scope.
* Boolean recognition is based purely on the literal strings `"true"`/`"false"`, and tristate recognition
  on the option's default being literally `"default"`; an option using any other convention (e.g. `1`/`0`,
  `yes`/`no`) is edited via the plain text prompt instead of cycling.
* `--materialize` produces a clean, readable, parseable INI file; it is **not** guaranteed to be
  byte-identical to what the target's own `--write-config` would produce (that is exactly why
  `--save-config` exists: it lets the target itself produce the canonical file).
* `--write-template` does not validate or reformat the captured output; it is a verbatim capture of the
  target's stdout for `-s`, and it is not affected by `--set`/`--unset`.
* Section detection in the *legacy fallback* tree (a target without `"#@"` metadata) is name-based
  (dotted prefixes / bracket headers), not based on the target's actual internal option types. The
  metadata-native tree (Phase 4, used whenever the target emits metadata) uses the target's own
  `node.kind`/`node.path` instead and does not have this limitation.
* Metadata's `required.effective` reflects only the target's *current* CLI11 state at the moment `-s`
  ran, not a canonical, registration-time, or suppression-aware value (SNode.C tracks that internally
  as of the "canonical state preservation model" work, but does not yet expose it through `"#@"`
  metadata) — `--check-required`/the UI's `!` marker remain an explicit local preflight, not an
  authoritative one, exactly as before Phase 4.
* Metadata's `type.kind` is a heuristic on SNode.C's own formatter side (derived from CLI11
  flag-ness and declared type name, not a real type system) and, notably, cannot distinguish a plain
  boolean option from a tristate one — both get `type.kind == "boolean"`. snodec-control does not use
  it to drive `Space` cycling for this reason; see "Type-aware editing and relations" above.
* `registrationDefault` (the option's original, pre-C++-API-mutation default) and `constraints`
  (decomposed validator metadata) are always absent/empty in the metadata schema version this build
  understands (v1) — snodec-control decodes their JSON shape but does not build any feature around
  them, so a future schema version populating them will not misbehave, it will simply still be unused
  until snodec-control is updated to consume them.
* `configFile.section` is always absent in schema v1 (every option is addressed by its full dotted
  `key`, even ones that would appear under a `[section]` bracket header), so snodec-control's own
  bracket-header handling in the legacy line parser remains the only source of section information for
  that case; metadata does not currently add anything here.
* `--save-config` and `--run` infer success/failure of a `--write-config` invocation from whether the
  output file's existence/modification time changed, since SNode.C applications conventionally exit
  non-zero even on success for both `-s` and `-w`. This is pragmatic but not foolproof (e.g. a target
  that fails after truncating-but-not-finishing the file could look like a false success).
* `--run`'s attached mode is intentionally simple: it inherits stdio and waits for one process; there is
  no support yet for detaching, daemonizing, or monitoring a long-running target beyond its exit code.
* Bare-key lookups (`--get`/`--set`/`--unset` without the section prefix) only disambiguate on the
  option's own trailing key name, not on partial section suffixes.

## Possible future work

* Optional round-tripping against the target's own `--write-config` for stricter fidelity.
* Richer success detection for `--save-config` (e.g. parsing the target's own success/error markers).
* In-UI incremental search/filter over the tree, and full cursor movement while editing a value.
* Recognizing additional boolean conventions (`1`/`0`, `yes`/`no`) for Space-cycling in the UI.
* Consuming canonical/suppression-aware required and needs state, and original registration-time
  defaults, if/when a future SNode.C metadata schema version exposes them (see "Known limitations").
* A dedicated editing affordance for `type.items == "list"` options (currently edited as one
  space-joined text string via the plain prompt, just never mistaken for a boolean/tristate).
