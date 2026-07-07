# SNode.C canonical JSON config export report

## Summary
Implemented a canonical JSON config introspection path for `--show-config`, alongside the existing INI formatter. JSON is emitted directly from the live CLI11/SNode.C tree and preserves generic subcommand hierarchy, option groups, options, values, defaults, and best-effort metadata.

## Files changed
- `src/utils/Formatter.h` / `src/utils/Formatter.cpp`: added `JsonWriter` and `JsonConfigFormatter`.
- `src/utils/SubCommand.h` / `src/utils/SubCommand.cpp`: defines a CLI11 flag `-s{json},--show-config{json}` with `CLI::IsMember({"ini", "json"})`, defaults to JSON, and retains `-s` as compatibility alias without a separate static show-config format variable.
- `src/utils/Config.cpp`: selects JSON vs INI at runtime by reading the parsed `--show-config` CLI11 option pointer and keeps JSON stdout pure during show-config parse errors.
- `tests/unit/core/ConfigJsonFormatterTest.cpp` and `tests/unit/core/CMakeLists.txt`: added formatter unit coverage.
- `docs/landing-pages/github-landing-openai-codex-pullreq/03-net-and-config.md`: documented the new export.

## Command-line behavior
- `--show-config` prints JSON.
- `--show-config=json` prints JSON.
- `--show-config=ini` prints legacy INI output.
- `-s` remains a compatibility alias for bare `--show-config` and therefore prints JSON by default; explicit format switching is documented for the long `--show-config=...` form.
- Invalid formats such as `--show-config=xml` fail CLI validation.

## JSON schema explanation
Top-level fields are deterministic: `format`, `application`, and `tree`. Format name is `snodec.config`; version is `1`. The tree contains generic nodes, option groups, options, command-line/config-file mappings, type metadata, constraints, relations, and value/default/effective state.

## Hierarchy model explanation
The exporter walks `CLI::App` children recursively. It does not hard-code Application -> Instances -> Sections. Instance and section kinds are inferred from CLI11 group names when present; otherwise nodes are categorized conservatively as category, subcommand, or anonymous.

## Option group handling
CLI11 option groups become structured JSON objects with name, inferred kind, and an options array. Only configurable options are included, matching the previous `-s` behavior.

## No JSON library implementation note
No JSON library was added or used for the exporter. `JsonWriter` writes JSON directly and escapes quotes, backslashes, JSON control escapes, all control bytes below 0x20, and passes UTF-8 bytes through unchanged.

## Tests added
- JSON string escaping coverage for plain text, quotes, backslashes, newline, tab, carriage return, control bytes, and UTF-8.
- Synthetic CLI tree coverage for format metadata, application name, tree, arbitrary nested child nodes, option groups, defaults/effective values, required placeholders, and legacy INI availability.

## Manual validation commands
- `./build/src/apps/echo/echoserver-legacy-in --show-config > /tmp/config.json`
- `jq . /tmp/config.json`
- `./build/src/apps/echo/echoserver-legacy-in --show-config=json > /tmp/config2.json`
- `jq . /tmp/config2.json`
- `./build/src/apps/echo/echoserver-legacy-in --show-config=ini > /tmp/config.ini`
- `head -20 /tmp/config.ini`
- `./build/src/apps/echo/echoserver-legacy-in -s > /tmp/configs.json`
- `jq . /tmp/configs.json`

## Known limitations
- Command-line and config-file value origins are merged as `command-line-or-config` when CLI11 does not expose the exact origin.
- Validator metadata is opaque unless available from CLI11 as a description.
- Config-file section is `null` for flattened dotted keys.
- Show-config can still return a non-zero status when required subcommands are missing, but JSON stdout remains parseable.

## Final status
Implemented, built, unit-tested, manually validated with the echo server sample, documented, and packaged.
