<a id="banner"></a>

<!-- snodec:begin page-header -->
<a id="banner"></a><a id="page-overview"></a>
<p><picture><source media="(max-width: 640px)" srcset="../media/nav/banner-design-compact.svg"><img src="../media/nav/banner-design.svg" alt="SNode.C — Design" width="100%"></picture></p>
<p><a href="README.md#banner" title="Up to Design"><picture><source media="(prefers-color-scheme: dark)" srcset="../media/nav/up-design-dark.svg"><img src="../media/nav/up-design-light.svg" alt="Up to Design" title="Up to Design" width="160" height="28"></picture></a><a href="https://github.com/VolkerChristian#banner" title="Volker Christian account"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../media/nav/account-icon-dark.svg"><source media="(max-width: 640px)" srcset="../media/nav/account-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../media/nav/account-dark.svg"><img align="right" src="../media/nav/account-light.svg" alt="Volker Christian account" title="Volker Christian account"></picture></a><a href="https://github.com/SNodeC#banner" title="SNode.C organization"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../media/nav/organization-icon-dark.svg"><source media="(max-width: 640px)" srcset="../media/nav/organization-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../media/nav/organization-dark.svg"><img align="right" src="../media/nav/organization-light.svg" alt="SNode.C organization" title="SNode.C organization"></picture></a><a href="../../README.md#banner" title="SNode.C repository"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../media/nav/repository-icon-dark.svg"><source media="(max-width: 640px)" srcset="../media/nav/repository-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../media/nav/repository-dark.svg"><img align="right" src="../media/nav/repository-light.svg" alt="SNode.C repository" title="SNode.C repository"></picture></a><br clear="all"></p>
<!-- snodec:end page-header -->

# Configuration comment metadata

`-s` / `--show-config` emits the SNode.C configuration as INI-compatible text with descriptions and commented defaults. `-S` / `--show-config=extensive` adds the metadata consumed by `snodec-control`. The real configuration lines are identical. `-w` / `--write-config` and `Config::writeConfig()` save the ordinary form without metadata.

Metadata is JSON inside comments delimited by `#@ snodec.meta begin <entity>` and `#@ snodec.meta end <entity>`. The marker identifies the entity; there is no duplicate JSON entity field. Normal config parsing ignores these comments. Metadata is emitted only for extensive output with descriptions enabled (`write_description=true`).

## Shared fields

Every block contains `schema = "snodec.config.comment-meta"` and `version = 1`. A consumer checks both before interpreting the fields. The reduced format retains these identifiers because the retained fields have unchanged meanings.

## Document

The `document` block contains only the shared fields. Its presence identifies a recognized metadata document. It appears once at the beginning of a full application dump.

## Nodes

Each `node` block contains:

- `kind`: `application`, `instance`, `section`, `anonymous`, `category` or `subcommand`, classified from the current CLI11 tree.
- `kindSource`: how the classification was obtained, such as `root`, `cli11-group`, `cli11-name`, `heuristic-cli11-group` or `fallback`.
- `displayName`: a nonempty label, chosen from the node name, group, description, or finally `<anonymous>`.
- `path`: the node's structural path relative to the dump root. The root has `[]`.
- `disabled`: CLI11's current disabled flag, displayed as a node annotation. It is not the framework instance's separate `--disabled` option.

Anonymous siblings receive generated path segments such as `<anonymous-0>`. These distinguish their structural positions without changing real config keys. Their numbering depends on sibling order.

## Groups

Each `group` block contains:

- `name`: the option group label.
- `kind`: `default`, `persistent`, `nonpersistent` or `custom`, classified from the group name.
- `nodePath`: the owning node's path.

The control UI flattens default and persistent groups, preserves custom groups, and excludes nonpersistent groups.

## Options

Each `option` block contains:

- `key`: the real emitted INI key, including the current dotted prefix where applicable.
- `nodePath`: the owning node's path.
- `group`: the owning option group.
- `description`: complete help text, without reconstructing it from surrounding comments.
- `required.source`: `cli11-current-state`, displayed as provenance alongside required-state information parsed from INI.
- `type.kind`: `boolean`, `integer`, `number` or `string`, inferred from CLI11 arity and the option/type name.
- `type.kindSource`: `heuristic-name`, identifying that classification as uncertain.
- `type.items`: `single` or `list`, derived from CLI11's expected item count.
- `relations.needs`: current CLI11 dependencies, expressed as local option names.
- `relations.excludes`: current CLI11 exclusions, also expressed as local names.

Only configurable options are emitted. A real INI section header may make `key` local to that section; it is not universally a fully qualified dotted address. The control tool matches options through their emitted keys and ordinary INI placement, and uses `nodePath` and `group` for the UI tree.

## Authority and limits

Ordinary INI assignments, commented defaults and `<REQUIRED>` placeholders remain the authority for editable values, defaults, active state and required-state decisions. Metadata adds structure, complete descriptions, type hints and relationship information. It does not duplicate values or preserve their origin.

The type classification is a hint, not a validator description. In particular, flag-like arity can classify a tristate as `boolean`. List metadata prevents the control UI from cycling a list as a scalar boolean/tristate. Dependencies and exclusions are displayed, not enforced by the control tool. Preserved original requirements and suppressed relationships are not exported.

The consumer drops malformed individual blocks with a warning. An unknown schema/version or malformed document disables metadata use; ordinary INI parsing remains available. Tree construction also requires a decoded root node.
