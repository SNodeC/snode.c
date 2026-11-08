<a id="banner"></a>

<!-- snodec:begin page-header -->
<a id="banner"></a><a id="page-overview"></a>
<p><picture><source media="(max-width: 640px)" srcset="../../../../../docs/media/nav/banner-tools-compact.svg"><img src="../../../../../docs/media/nav/banner-tools.svg" alt="SNode.C — Tools" width="100%"></picture></p>
<p><a href="../../README.md#banner" title="Up to snodec-control"><picture><source media="(prefers-color-scheme: dark)" srcset="../../../../../docs/media/nav/up-snodec-control-dark.svg"><img src="../../../../../docs/media/nav/up-snodec-control-light.svg" alt="Up to snodec-control" title="Up to snodec-control" width="160" height="28"></picture></a><a href="https://github.com/VolkerChristian#banner" title="Volker Christian account"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../../../../../docs/media/nav/account-icon-dark.svg"><source media="(max-width: 640px)" srcset="../../../../../docs/media/nav/account-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../../../../../docs/media/nav/account-dark.svg"><img align="right" src="../../../../../docs/media/nav/account-light.svg" alt="Volker Christian account" title="Volker Christian account"></picture></a><a href="https://github.com/SNodeC#banner" title="SNode.C organization"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../../../../../docs/media/nav/organization-icon-dark.svg"><source media="(max-width: 640px)" srcset="../../../../../docs/media/nav/organization-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../../../../../docs/media/nav/organization-dark.svg"><img align="right" src="../../../../../docs/media/nav/organization-light.svg" alt="SNode.C organization" title="SNode.C organization"></picture></a><a href="../../../../../README.md#banner" title="SNode.C repository"><picture><source media="(max-width: 640px) and (prefers-color-scheme: dark)" srcset="../../../../../docs/media/nav/repository-icon-dark.svg"><source media="(max-width: 640px)" srcset="../../../../../docs/media/nav/repository-icon-light.svg"><source media="(prefers-color-scheme: dark)" srcset="../../../../../docs/media/nav/repository-dark.svg"><img align="right" src="../../../../../docs/media/nav/repository-light.svg" alt="SNode.C repository" title="SNode.C repository"></picture></a><br clear="all"></p>
<!-- snodec:end page-header -->

# Test fixtures

## `NestedSubCommandFixture.h`

Real `--show-config=extensive` output, captured verbatim from a throwaway program built against the repository sources. It exercises genuinely nested custom `SubCommand`s rather than hand-written metadata.

`nested_subcommand_fixture_source.cpp` is the exact source used to produce it. It links only
against `utils::SubCommand`/`utils::Config` (no network stack), and registers:

- a custom subcommand (`tool`) at the application's top level, alongside where an `Instances` node
  would sit;
- a custom subcommand (`echoserver`, `group("Instances")`) standing in for a real
  `net::config::ConfigInstance` (which requires the full network stack to instantiate) containing
  another custom subcommand (`outer`), which itself contains a third (`inner`) — three levels of
  genuine nesting below the root;
- a disabled custom subcommand (`legacy`, `getAppWithPtr()->disabled()`).

To reproduce against a newer SNode.C checkout:

```sh
SNODEC_FIXTURE_BUILD=/tmp/codex/build/snodec/config-metadata-fixture
cmake -S . -B "$SNODEC_FIXTURE_BUILD" -DSNODEC_BUILD_APPS=OFF
cmake --build "$SNODEC_FIXTURE_BUILD" --target utils
c++ -std=c++20 -I src \
    src/tools/snodec-control/tests/fixtures/nested_subcommand_fixture_source.cpp \
    -L "$SNODEC_FIXTURE_BUILD/src/utils" -L "$SNODEC_FIXTURE_BUILD/src/log" \
    -lsnodec-utils -lsnodec-logger \
    -Wl,-rpath,"$SNODEC_FIXTURE_BUILD/src/utils" -Wl,-rpath,"$SNODEC_FIXTURE_BUILD/src/log" \
    -o "$SNODEC_FIXTURE_BUILD/nested_fixture"
XDG_CONFIG_HOME="$SNODEC_FIXTURE_BUILD/xdg-config" \
XDG_STATE_HOME="$SNODEC_FIXTURE_BUILD/xdg-state" \
XDG_RUNTIME_DIR="$SNODEC_FIXTURE_BUILD/xdg-run" \
    "$SNODEC_FIXTURE_BUILD/nested_fixture" -S > "$SNODEC_FIXTURE_BUILD/nested_fixture_output.txt"
```

Then re-embed `$SNODEC_FIXTURE_BUILD/nested_fixture_output.txt` as the raw string literal in `NestedSubCommandFixture.h`.
