# Canonical JSON configuration export

SNode.C can export its live CLI11/SNode.C configuration tree as a deterministic JSON document for tools such as `snodec-control`.
The export is generated from the in-memory CLI11 application tree and does not parse the legacy INI output.
No external JSON library is used; SNode.C emits this document with its own small JSON writer.

## Command-line behavior

The legacy INI behavior remains the default for compatibility:

```console
target-app -s
target-app --show-config
target-app --show-config=ini
```

The canonical JSON export is selected explicitly:

```console
target-app --show-config=json
```

Invalid formats such as `--show-config=xml` are rejected by CLI validation.
A future release may choose to make bare `--show-config` default to JSON, but this PR intentionally preserves the INI default to avoid breaking existing scripts.

## Top-level schema

The top-level object uses deterministic field order:

```json
{
  "format": {
    "name": "snodec.config",
    "version": 1,
    "scope": "configurable-options-only"
  },
  "application": {},
  "tree": {}
}
```

`format.name` is `snodec.config` and `format.version` is `1`.
The `scope` field is important: this is a configuration export, not a complete command-line schema, so only CLI11 options marked configurable are included.
Non-configurable operational options such as help and write-config are excluded in the same spirit as the legacy config export.

## Application object

The `application` object contains metadata from the root CLI11 application:

- `name`
- `description`
- `version`

## Tree nodes

The `tree` object recursively represents the actual CLI11/SNode.C subcommand hierarchy.
The exporter does not hard-code an Application -> Instances -> Sections shape.
Instances and sections are common SNode.C group names, but arbitrary categories, nested sections, and future custom subcommands are preserved as generic child nodes.

Node fields include:

- `id`: `app` for the root node or the stable dotted path for children, including generated anonymous segments when needed.
- `kind`: best-effort node kind.
- `kindSource`: source of the kind classification, for example `root`, `cli11-group`, or `heuristic-cli11-group`.
- `name`
- `displayName`
- `group`
- `description`
- `configurable` and `configurableSource`
- `required` and `requiredSource`
- `disabled` and `disabledSource`
- `path`
- `optionGroups`
- `children`

Kind inference is conservative. Only the root node is authoritative as `application`.
`Instances` and `Sections` group names produce `instance` and `section` with `kindSource` set to `cli11-group`.
Other grouped nodes are marked as `category` with a heuristic source, and ungrouped nodes fall back to `subcommand` or `anonymous`.
Anonymous or empty-name child subcommands are not collapsed into their parent. They receive deterministic generated path segments such as `<anonymous-0>` and `<anonymous-1>` within their sibling set, so IDs can look like `<anonymous-0>` or `named.<anonymous-0>`. The actual CLI11 `name` field remains empty, while `displayName` falls back to the group, description, or `<anonymous>`.

## Option groups

CLI11 option groups are emitted as structured JSON objects instead of INI comments.
Each group contains:

- `name`: the original CLI11 group name.
- `kind`: `default`, `persistent`, `nonpersistent`, or `custom`.
- `kindSource`: whether the classification came from CLI11 default-group handling or from a group-name heuristic.
- `options`: configurable options in that group.

Persistence is currently classified from the group name. Downstream tools must treat `persistentSource` / `kindSource` as part of the contract and should not assume heuristic values are authoritative.

## Options

Each option includes stable identity and best-effort CLI/config metadata:

- `id`, `key`, `displayName`, `description`
- `configurable` and `configurableSource`
- `persistent` and `persistentSource`
- `required` and `requiredSource`
- `disabled`
- `commandLine`
- `configFile`
- `type`
- `constraints`
- `relations`
- `value`

`id` is the unique identity of the option inside the exported JSON tree. It is derived from the current node ID plus the option name, so options below anonymous nodes include generated segments such as `<anonymous-0>.port`.
`key` remains the flattened SNode.C/CLI11 config key used for config-file and command-line mapping, and it is intentionally kept separate from `id` so config behavior does not change.
`commandLine` includes the visible long and short names where CLI11 exposes them, whether values are expected, the value separator, and repeatability.
`configFile` contains the flattened key used by the current config formatter, an optional section field, and writability derived from configurability.

## Type and constraint metadata

Type metadata is best effort:

- `type.kind`
- `type.kindSource`
- `type.name`
- `type.cpp`
- `type.items`

When CLI11 exposes direct information, such as zero expected items for a flag, the source reflects that.
Name-based classifications such as `integer` for `port` or `path` for `file` are marked with `kindSource: "heuristic-name"` so tools can distinguish hints from authoritative metadata.
Validators are emitted as opaque constraints when CLI11 exposes only a validator description.
The exporter does not invent numeric ranges or enum members when CLI11 does not provide them in decomposable form.

## Value semantics

The `value` object separates semantic values from INI/config-file literals:

- `apiDefault`: semantic CLI11 default string, `<REQUIRED>` for required options without defaults, or an empty string if that is the effective API default.
- `configured`: semantic command-line or config-file value when active; otherwise `null`. This is not INI quoted.
- `effective`: semantic configured value if present, otherwise the semantic API default or required placeholder.
- `source`: `command-line-or-config`, `api-default`, `required-placeholder`, or `empty-default`.
- `isEffectiveDefault`: true when the semantic effective value equals the semantic API default, even if the value was explicitly configured.
- `isExplicitlyConfigured`: true when CLI11 reports an active configured value, regardless of whether it equals the API default.
- `isMissingRequired`: true when a required option has no usable effective value or the effective value is `<REQUIRED>`.
- `apiDefaultLiteral`: INI/config-file literal form of `apiDefault`.
- `configuredLiteral`: INI/config-file literal form of `configured`, or `null` when no configured value is active.
- `effectiveLiteral`: INI/config-file literal form of `effective`.

For version 1, semantic multi-value options are represented as a deterministic space-joined string and the `type.items` metadata identifies list-like options. The `*Literal` fields preserve CLI11's config-file representation for tools that need to round-trip textual config snippets.
CLI11 does not currently expose enough origin metadata here to reliably distinguish command-line values from config-file values, so both are intentionally reported as `command-line-or-config`.

## Output purity

Successful `--show-config=json` output writes JSON to stdout only.
Diagnostics for JSON export failures are written to stderr and stdout is left empty instead of being mixed with colored plaintext.

## Known limitations

- This is a configurable-options export, not a complete CLI schema.
- Command-line and config-file value origins are merged as `command-line-or-config`.
- Validator metadata is opaque unless CLI11 exposes a decomposable description.
- Config-file section is currently `null` for flattened dotted keys.
- Semantic multi-value options are currently space-joined strings; consumers should inspect `type.items` and the literal fields when round-tripping.
- Some node, group, persistence, and type classifications are heuristic; every such field includes source metadata.
