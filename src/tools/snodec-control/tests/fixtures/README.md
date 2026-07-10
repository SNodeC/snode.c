# Test fixtures

## `NestedSubCommandFixture.h`

Real `--show-config` output, captured verbatim from a throwaway program built against SNode.C
master, used by tests that need genuinely nested, non-`Instances`/`Sections` custom `SubCommand`s
(the framework's own examples/apps don't nest custom subcommands more than one level deep, so this
was purpose-built per the Phase 4 task's Step 0 instructions rather than hand-written as a `#@`
snippet).

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
cmake --build build --target utils
g++ -std=c++20 -I src \
    snodec-control/tests/fixtures/nested_subcommand_fixture_source.cpp \
    -L build/src/utils -L build/src/log -lsnodec-utils -lsnodec-logger \
    -Wl,-rpath,$PWD/build/src/utils -Wl,-rpath,$PWD/build/src/log \
    -o /tmp/nested_fixture
HOME=/tmp/fakehome /tmp/nested_fixture -s > /tmp/nested_fixture_output.txt
```

then re-embed `/tmp/nested_fixture_output.txt` as the raw string literal in
`NestedSubCommandFixture.h`.
