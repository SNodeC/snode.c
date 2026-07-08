# Root semantic CLI integration report

## Problem summary

SNode.C had an existing semantic logging backend with startup policy APIs, but the root configuration surface still exposed only the legacy numeric logging controls. This change bridges the root command-line/config layer to the semantic policy without touching production logging call sites or broadening the policy model.

## Existing root CLI logging surface

The pre-existing root surface included `--log-level`, `--verbose-level`, `--log-file`, `--monochrom`, `--quiet`, `--enforce-log-file`, and `--daemonize`. Those options remain available.

## Semantic policy capabilities

The already-existing semantic backend supports global, origin, boundary, component, and instance thresholds; text/json formatter selection; effective-level precedence; and freeze-on-startup behavior.

## Backward compatibility decision

Numeric root `--log-level` values `0..6` remain accepted. The default remains equivalent to the old `--log-level 4`, which maps to semantic `info`. The root-level legacy logger level continues to be set from the same option so existing scripts and config files keep their old numeric behavior for legacy macros.

## Numeric/name level mapping

| CLI value | Legacy numeric level | Semantic level |
| --- | ---: | --- |
| `0`, `off` | 0 | `off` |
| `1`, `critical` | 1 | `critical` |
| `2`, `error` | 2 | `error` |
| `3`, `warn`, `warning` | 3 | `warn` |
| `4`, `info` | 4 | `info` |
| `5`, `debug` | 5 | `debug` |
| `6`, `trace` | 6 | `trace` |

## CLI11 validator/transform decision for root `--log-level`

The old integer-only `CLI::Range(0, 6)` validator was replaced with a CLI11-compatible validator/transform that accepts numeric and named levels and normalizes the option value back to the legacy numeric representation. The reusable parsing entry point is declared in `utils::config::parseLogLevel`, making the same mapping available to the mandatory PR G.2 instance-level shorthand. The supported documented semantic override form is `--log-*-level=key=level`; the framework does not rewrite `argv` before CLI11 parsing.

## New root semantic options

The root command now exposes repeatable semantic policy overrides:

- `--log-origin-level=origin=level` for `framework` and `application` origins.
- `--log-boundary-level=boundary=level` for `application`, `configuration`, `instance`, `connection`, `context`, and `system` boundaries.
- `--log-component-level=component=level` for named component overrides.
- `--log-instance-level=instance=level` for named instance overrides.

Component and instance keys must be non-empty. Comma-separated `key=level` lists are supported, for example `--log-component-level=core.mux=debug,core.xyz=info`, and repeated options are supported. Comma-separated and repeated forms may be combined; duplicate keys are applied in order, so the last duplicate wins. Invalid `key=level` syntax, empty list items, unknown levels, unknown origins, and unknown boundaries are rejected by CLI11 validation and by the reusable parser helpers.

## Output format option

The root command now exposes `--log-format text|json`. `text` selects `LogManager::Format::Text`, and `json` selects `LogManager::Format::Json`. The JSON formatter/schema itself was not changed.

## `--verbose-level` compatibility note

`--verbose-level 0..10` remains a legacy compatibility option. Parsing, validation, and `Logger::setVerboseLevel(...)` behavior remain unchanged. It is deliberately not mapped into semantic logging policy because the semantic API has no numeric verbose axis and mapping it would invent new policy semantics.

## Semantic-vs-legacy filtering decision

Root `--log-level` configures both the legacy numeric logger level and the semantic global threshold. Semantic-specific overrides configure only `LogManager`. Legacy `LOG`, `PLOG`, and `VLOG` behavior remains governed by the legacy `Logger::shouldLog` / `Logger::shouldVerbose` paths.

## Double-gate issue and resolution

Before this PR, `Logger::emitSemantic(...)` first asked `LogManager::shouldEmit(record)` and then mapped the semantic level back to a legacy numeric level for `Logger::shouldLog(...)`. That double-gated semantic records and prevented a command like `--log-level=info --log-component-level=core.mux=debug` from emitting debug records for `core.mux`. This PR removes the legacy numeric gate from semantic emission after `LogManager` accepts a record. Semantic output is now filtered by `LogManager`; legacy macro output remains filtered by `Logger`.

## Policy precedence

The existing effective-level precedence remains unchanged:

1. instance
2. component
3. boundary
4. origin
5. global

The CLI integration applies root overrides into those existing policy axes and does not add role, connection, event, section, or subcommand policy axes.

## Freeze point

`LogManager::init()` runs with root logger initialization. Root semantic options are applied during normal config application and `LogManager::freeze()` is called after normal CLI/config policy has been applied, before application runtime. Help/show-config/command-line introspection paths do not freeze runtime policy.

## Help/show-config/command-line output verification

Smoke checks were run against `echoserver-legacy-in`. `--help` shows the new root options and existing logging options. `--show-config` renders configurable root logging options. `--command-line` renders configured root options. Smoke checks use the supported documented equals form, including `--log-component-level=core.mux=debug` and `--log-component-level=core.mux=debug,core.xyz=info`; command-line rendering preserves CLI11's escaped value form for values containing `=`.

## Explicit G.2 follow-up for `ConfigInstance --log-level`

PR G.1 intentionally does not add `ConfigInstance --log-level`. PR G.2 must add the instance-scoped shorthand and reuse the centralized parsing helpers to map a per-instance level to `LogManager::setInstanceLevel(instanceName, parsedLevel)`.

## Decision not to add `ConfigSection` logging options

No `ConfigSection` logging options were added. `LogManager` has no section-level policy axis, so adding section-level `--log-level` or scoped overrides would be misleading and out of scope.

## Files inspected

- `src/log/Logger.h`
- `src/log/Logger.cpp`
- `src/log/SemanticLogger.h`
- `src/log/SemanticLogger.cpp`
- `src/utils/Config.cpp`
- `src/utils/Config.h`
- `src/utils/SubCommand.cpp`
- `src/utils/SubCommand.h`
- `src/net/config/ConfigInstance.h`
- `src/net/config/ConfigInstance.cpp`
- `src/net/config/ConfigSection.h`
- `src/net/config/ConfigSection.hpp`
- `tests/unit/log`
- SNode.C and MQTT-related logging/test surfaces found by the required ripgrep searches.

## Implementation summary

- Added reusable root logging parse helpers in `utils::config`.
- Added root CLI/config options for semantic format, origin, boundary, component, and instance thresholds.
- Added reusable comma-separated `key=level` list parsing while preserving repeated option support and normal CLI11 `argv` parsing.
- Extended root `--log-level` to accept names while preserving numeric values.
- Applied semantic policy at root config application and froze it before normal runtime.
- Removed legacy numeric double-gating from semantic emission.
- Updated tests that previously asserted the old semantic backend gate behavior.

## Tests added/updated

- Added `SemanticCliIntegrationTest` for parser mappings, comma-separated lists, invalid values, semantic override behavior, duplicate-key ordering, verbose compatibility, freeze behavior, and double-gate regression coverage.
- Added `SemanticCliSourcePolicyTest` for root option presence and to guard against accidental ConfigInstance/ConfigSection logging options in PR G.1.
- Updated migration/round tests that intentionally covered the previous semantic backend double gate.

## Search commands run

The required pre-change and post-change `rg` searches were run for root CLI logging options, semantic manager APIs, semantic emission, `VLOG`, ConfigInstance/ConfigSection scope, help/config/command-line machinery, sensitive logging patterns, high-frequency logging patterns, SocketContext detach diagnostics, epoll/syscall diagnostics, legacy macro call sites, PR G.1/G.2 separation, and output-surface verification.

## Tests run

- `test -f` prerequisite checks for the PR #168-#173 logging reports and `SourcePolicyTestRoot.h`.
- `cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON`.
- `cmake --build cmake-build --parallel 2`.
- `ctest --test-dir cmake-build --output-on-failure`.
- Focused semantic/logging/source-policy `ctest` regex from the PR instructions.
- Nonstandard working-directory source-policy `ctest` run from `/tmp`.
- ASan focused configure/build and `ctest` for `SemanticCliIntegrationTest|SemanticEndToEndOutputTest`.
- Root output smoke checks with `echoserver-legacy-in --help`, `--show-config`, `--command-line`, and configured semantic options.

## Known follow-ups

1. PR G.2 — add instance-scoped semantic `--log-level` to `ConfigInstance`.
2. Final macro removal/source gate.
3. Round 10 — book integration.
