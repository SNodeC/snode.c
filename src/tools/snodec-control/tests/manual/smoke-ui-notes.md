# Manual UI smoke test notes

`fake-snodec-target.sh` in this directory is a tiny, dependency-free stand-in for a real SNode.C
application, used only for manually exercising `--ui` interactively. It is **not** built, packaged, or
run by CTest — it is a developer aid, not part of the automated test suite.

Its `-s` output is chosen to cover every interaction this repair pass targets in one session:
Application Options (`log-level`), one Instance (`echoserver`) with a direct option (`enabled`) and one
Section (`local`) containing a required, initially-unset option (`port`), plus two boolean-shaped
options under the "Other Options" fallback: a plain boolean with no active value yet (`feature.enabled`,
default `false`) and a genuine tristate option (`feature.mode`: `false`/`true`/`default`).

## Running it

```sh
cd snodec-control/build
./src/snodec-control --target ../tests/manual/fake-snodec-target.sh --ui
```

## Suggested key sequence

1. Start the UI as above. Confirm the tree shows **Application Options**, **Instances**, and **Other
   Options** at the top level (not a flat list).
2. `Down`/`Up` — move the selection through the top-level rows.
3. `Right` on **Instances** (already expanded by default) — since it's already expanded, `Right` moves
   into its first child, `echoserver`, instead of doing nothing.
4. `Right` on `echoserver` (collapsed) — expands it, revealing `enabled` (a direct option) and a
   **Sections** node.
5. `Right` on **Sections** (already expanded) — moves to its first child, `local`.
6. `Right` on `local` (collapsed) — expands it, revealing `port` (marked `!`, required and unset) and
   `address`.
7. `Down` to `port`, `Enter` — edit its value (e.g. type `8080`), `Enter` to accept. Confirm the `!`
   marker disappears immediately and the title bar shows `[Modified]`.
8. `Left` on `port` (a leaf, nothing to collapse) — moves the selection up to its parent, `local`.
9. `Left` on `local` (now expanded) — collapses it back in place; the selection stays on `local` itself.
10. Navigate to `feature.enabled` (under **Other Options**). Press `Space` — confirm it flips from
    `false` to `true` (this is the exact bug this pass fixed: a default-`false`, still-unset option must
    visibly flip, not silently stay `false`).
11. Navigate to `feature.mode` (the tristate option). Press `Space` three times — confirm the sequence
    `default -> false -> true -> default`.
12. Navigate to `log-level` (a plain string/number option) and press `Space` — confirm it does **not**
    open the text editor, and the footer shows a message that Space only toggles true/false/default
    options.
13. Press `D` — confirm the diff screen lists exactly the edits made so far (e.g. `echoserver.local.port`
    and `feature.enabled`/`feature.mode`, depending on what was changed).
14. Press `C` — confirm "check required" now reports all required options satisfied (once `port` is set).
15. Press `S` — since no `--save-config` was given, this prompts for a path; enter something like
    `/tmp/smoke.conf`. Confirm the save succeeds, the message box reports success, and once dismissed the
    title bar **no longer shows `[Modified]`** (Save clears the dirty flag).
16. Make another edit (so the session is dirty again), then press `S` a second time — confirm the save
    prompt is now **prefilled** with the path used last time (`/tmp/smoke.conf`) within this same running
    session, rather than a blank prompt. (This remembered-path behavior is per-session only: it does not
    persist across separate `snodec-control` invocations unless `--save-config <file>` is passed again.)
17. Make another edit (so the session is dirty again) and press `R`. Since a save already succeeded
    earlier in this session, `R` uses that same path directly (no Save-now/Temp-materialize/Cancel
    choice this time) — confirm this by starting a **fresh** `--ui` session instead and pressing `R`
    before ever saving: confirm the Save-now/Temp-materialize/Cancel choice appears first in that case.
    Either way, once a config path is settled:
    - Confirm the Curses UI **disappears entirely** and the terminal shows the fake target's own
      `tick 0`..`tick 4` output directly, once per second.
    - Let the fake target run to completion (about 5 seconds).
    - Confirm snodec-control prints `Target exited with status 0. Press any key to return to
      snodec-control...` and waits — press any single key (no Enter needed).
    - Confirm the Curses UI is restored cleanly (no corrupted screen) and all keys still work afterward.
18. Press `Q` and confirm clean exit (prompting only if still dirty).

## Ctrl-C during Run (job control / terminal foreground handover)

This exercises the fix for a real bug: pressing Ctrl-C while the target ran under `R` used to kill
`snodec-control` itself, not just the target, because both processes shared the same foreground process
group. `runProcessAttachedAsForegroundJob()` (used only by the UI's `R`, not by the plain CLI's `--run`)
now puts the target in its own process group and makes that group the terminal's foreground job for the
duration of the run, so Ctrl-C/Ctrl-\/Ctrl-Z are delivered to the target only.

`fake-snodec-target.sh` normally loops forever (one log line per second) so there is time to press
Ctrl-C against it; set `FAKE_TARGET_TICKS=<n>` in the environment to make it exit on its own after `n`
seconds instead, for testing the plain "target exits by itself" path without Ctrl-C at all.

```sh
cd snodec-control/build
./src/snodec-control --target ../tests/manual/fake-snodec-target.sh --ui
```

1. Press `R`. If prompted, choose `(T)` (temporary materialized config) or `(S)`/proceed past the
   required-options warning — either is fine for this test.
2. Confirm the Curses UI disappears and `fake-snodec-target: tick 0`, `tick 1`, ... appears directly in
   the terminal, once per second, along with its own PID (`fake-snodec-target: pid <N>, press Ctrl-C to
   stop me`).
3. Press **Ctrl-C**.
4. Confirm, in order:
   - The fake target prints `fake-snodec-target: got SIGINT (Ctrl-C) - stopping` and stops (it traps
     `SIGINT` and exits with status 130, the conventional 128+signal shell exit code).
   - `snodec-control` is **still alive** — it prints `Target exited with status 130. Press any key to
     return to snodec-control...` and waits. (You can double check independently with
     `ps aux | grep snodec-control` in another terminal/session: the process is still there.)
5. Press any single key (not Enter).
6. Confirm the Curses UI is restored cleanly (no corrupted screen), arrow-key navigation still works, and
   `Q` still quits cleanly.
7. Repeat once more letting the target exit on its own (`FAKE_TARGET_TICKS=3` and no Ctrl-C) to confirm
   the plain "target exits by itself" path (exit status 0) still works exactly as before this fix.

## What to look for

- The hierarchy is never flat: Application Options / Instances / Sections / Options (plus the "Other
  Options" fallback) are always visually distinct levels.
- `Left`/`Right` behave like real tree navigation (collapse/expand vs. move to parent/first child), not
  just "collapse if expanded, expand if collapsed" with no parent/child movement.
- `Space` only ever touches boolean/tristate options; every other key press on a non-boolean option (or a
  container) leaves a short explanatory message in the footer instead of silently doing nothing.
- Long lines (a long target path, a long option value) never wrap into the next row or corrupt the
  layout — they are clipped to the terminal width.
- The Modified indicator and the Quit prompt are always consistent with each other: Modified showing
  means Quit will ask; Modified not showing means Quit exits immediately.
- Run truly hands the terminal over: while the target runs, there is no Curses window at all, just the
  target's own output, exactly as if it had been started directly from a shell.
- Ctrl-C (and Ctrl-\/Ctrl-Z) during a Run stop the target only; `snodec-control` always survives to
  restore the UI afterward.
