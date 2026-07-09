# Configuration comment metadata

`-s` / `--show-config` emits the canonical SNode.C configuration template as INI-compatible text. Version 1 now augments that template with structured metadata comments for tools such as `snodec-control` while keeping the real configuration lines unchanged.

Metadata is carried only in comments with the reserved `#@` prefix:

```ini
#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "option",
#@   "key": "echoserver.local.port"
#@ }
#@ snodec.meta end option
# Port number
#echoserver.local.port="<REQUIRED>"
```

Because each metadata line is a comment, CLI11/SNode.C config parsing ignores it. Existing config files without metadata remain valid, and generated templates can still be edited by changing only normal INI key/value lines. No separate JSON output mode exists; JSON appears only as comment metadata inside the INI output. Metadata is emitted only when description/comment output is enabled (`write_description=true`, as used by `-s` / `--show-config`).

## Entities

Version 1 emits four entity types:

* `document` appears once at the beginning and describes the schema, application, output syntax (`ini-with-comment-metadata`), and scope (`configurable-options-only`).
* `node` appears before each CLI11/SNode.C application or subcommand node represented by the output. Node kinds are inferred from the current CLI11 tree: root applications, `Instances`, `Sections`, anonymous empty-name nodes, other grouped categories, and fallback subcommands.
* `group` appears before each CLI11 option group that contains configurable options. Group kind is inferred from the group name (`default`, `persistent`, `nonpersistent`, or `custom`).
* `option` appears immediately before the option's human description and config/default line.

The root application has `path: []`. Child subcommands use paths relative to that root, for example `["outer"]` and `["outer", "inner"]`, so the application name does not look like part of the config-key hierarchy. Anonymous / empty-name subcommands are not collapsed into their parent. Their metadata path receives deterministic sibling-local generated segments such as `<anonymous-0>` while the `name` field preserves the actual CLI11 name, including the empty string. Option `key` remains the real config key; option `id` uses generated anonymous path segments where needed so sibling anonymous options can be distinguished, for example `<anonymous-0>.value`.

## Value fields

Option metadata separates semantic values from INI literals:

* `cppDefault`, `configured`, and `effective` are semantic values.
* `cppDefaultLiteral`, `configuredLiteral`, and `effectiveLiteral` are the INI/config-file literal representations.
* `source` reports `command-line-or-config`, `cpp-default`, `required-placeholder`, or `empty-default` according to current formatter-visible state.

Version 1 exposes the current C++/CLI11 default as `cppDefault`. It does not expose the original registration-time default. `registrationDefault` is emitted as `null` with `registrationDefaultSource = "not-tracked"`.

For multi-value options, version 1 may represent semantic values as deterministic space-joined strings and marks list-like options with `type.items = "list"`.

## Required state

This formatter-only metadata exposes only current CLI11 required state:

```json
"required": {
  "effective": true,
  "source": "cli11-current-state"
}
```

Canonical required/needs state and disable/enable suppression semantics are not part of this formatter-only PR. Registration-default tracking is also deliberately left to a future option-metadata design.

## Known limitations

* The schema scope is `configurable-options-only`; non-configurable options are omitted from config output and metadata.
* `registrationDefault` is not tracked in version 1.
* `required.effective` reflects current CLI11 state only and is not a canonical registration-time value.
* Relation metadata reports available CLI11 `needs` / `excludes` option names but does not restore or infer suppressed state.
* `constraints` is emitted as an empty array in version 1; validator and constraint metadata is not decomposed yet.
* Metadata is suppressed when description/comment output is disabled.
* No separate JSON config output mode is provided; JSON appears only inside INI comments.
