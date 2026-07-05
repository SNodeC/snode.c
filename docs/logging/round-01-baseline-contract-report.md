# Logging Roadmap Round 1 Report: Baseline + Frozen Contract

Branch: `logging/round-01-baseline-contract`

## Files inspected

Logging implementation and configuration:

- `src/log/Logger.h`
- `src/log/Logger.cpp`
- `src/log/CMakeLists.txt`
- `src/utils/Config.cpp`
- `src/CMakeLists.txt`
- `CMakeLists.txt`

Socket/core identity and context model:

- `src/core/socket/SocketContext.h`
- `src/core/socket/stream/SocketConnection.h`
- `src/core/socket/stream/SocketConnection.cpp`
- `src/core/socket/stream/SocketConnection.hpp`
- `src/core/socket/stream/SocketContext.h`
- `src/core/socket/stream/SocketContext.cpp`
- `src/core/socket/stream/SocketContextFactory.h`
- `src/core/socket/stream/SocketContextFactory.cpp`
- `src/core/socket/stream/ClientFlowController.h`
- `src/core/socket/stream/ClientFlowController.cpp`
- `src/core/socket/stream/ServerFlowController.h`
- `src/core/socket/stream/ServerFlowController.cpp`

Representative current macro call sites:

- `src/core/DescriptorEventReceiver.cpp`
- `src/core/DynamicLoader.cpp`
- `src/core/TimerEventReceiver.cpp`
- `src/core/multiplexer/epoll/EventMultiplexer.cpp`
- `src/core/multiplexer/poll/EventMultiplexer.cpp`
- `src/core/multiplexer/select/EventMultiplexer.cpp`
- `src/core/socket/stream/SocketConnection.cpp`
- `src/iot/mqtt/Mqtt.cpp`
- `src/iot/mqtt/SubProtocol.hpp`
- `src/iot/mqtt/client/Mqtt.cpp`
- `src/iot/mqtt/server/Mqtt.cpp`
- `src/iot/mqtt/server/broker/Broker.cpp`
- `src/apps/testpost.cpp`
- `src/apps/express_compat_server.cpp`

MQTT/mqttsuite-oriented review inputs:

- `src/iot/mqtt/CMakeLists.txt`
- `src/iot/mqtt/Mqtt.h`
- `src/iot/mqtt/Mqtt.cpp`
- `src/iot/mqtt/SocketContext.h`
- `src/iot/mqtt/SocketContext.cpp`
- `src/iot/mqtt/MqttContext.h`
- `src/iot/mqtt/MqttContext.cpp`
- `src/iot/mqtt/client/Mqtt.h`
- `src/iot/mqtt/client/Mqtt.cpp`
- `src/iot/mqtt/server/Mqtt.h`
- `src/iot/mqtt/server/Mqtt.cpp`
- `src/iot/mqtt/server/broker/Broker.h`
- `src/iot/mqtt/server/broker/Broker.cpp`
- `docs/landing-pages/github-landing-openai-codex-pullreq/09-extended-review-snodec-and-mqttsuite.md`

Searches used during inspection:

- `rg --files`
- `rg -n "#define (LOG|PLOG|VLOG)|LOG\\(|PLOG\\(|VLOG\\(|instanceName|connectionName|getConnectionName|getInstanceName|class SocketConnection|class SocketContext" src core mqttsuite .github CMakeLists.txt`
- `rg -n "sanitize|SANIT|fsanitize|AddressSanitizer|UBSan|TSan" .github CMakeLists.txt cmake src tests`

## Files changed

- `docs/logging/semantic-logging-contract.md`
- `docs/logging/round-01-baseline-contract-report.md`

No production source files were changed.

## Baseline observations

- Current `LOG` and `PLOG` macros are defined in `src/log/Logger.h`; `PLOG` constructs `LogMessage` with `withErrno=true`.
- Current `VLOG` is also defined in `src/log/Logger.h` and uses `logger::Level::VERBOSE` plus an integer verbose threshold.
- `Logger.cpp` initializes stdout/file spdlog loggers with pattern `%Y-%m-%d %H:%M:%S %* %v`, where `%*` is a custom steady-clock elapsed tick.
- `Logger::setTickResolver` already exists and can make the elapsed tick deterministic inside a future harness.
- `Logger::setDisableColor` already exists and can remove ANSI color from future comparisons.
- `SocketConnection` stores both `instanceName` and `connectionName`, and exposes `getInstanceName()` and `getConnectionName()`.
- Current MQTT logging relies heavily on manually prefixed `connectionName` plus protocol text such as `MQTT`, `MQTT Client`, `MQTT Broker`, and `WsMqtt`.
- No top-level `mqttsuite` source tree was present in this checkout. The repository does contain MQTT implementation files under `src/iot/mqtt` and an existing extended review document under `docs/landing-pages/github-landing-openai-codex-pullreq/09-extended-review-snodec-and-mqttsuite.md`.

## Build command and result

Compiler detected by CMake:

```text
C compiler: GNU 13.3.0 (/usr/bin/cc)
C++ compiler: GNU 13.3.0 (/usr/bin/c++)
```

Build directory:

```text
cmake-build
```

Commands attempted:

```sh
cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON
cmake --build cmake-build --parallel 2
ctest --test-dir cmake-build --show-only=json-v1
```

Result: configure failed before generating build files because the environment is missing `nlohmann-json3-dev` / pkg-config module `nlohmann_json>=3.11`:

```text
-- Checking for module 'nlohmann_json>=3.11'
--   Package 'nlohmann_json', required by 'virtual:world', not found
CMake Error at src/express/CMakeLists.txt:50 (message):
   nlohmann-json3-dev not found:
     nlohmann_json is used for http. Please install it by executing
         sudo apt install nlohmann-json3-dev
```

Because configure did not complete, the build command failed with:

```text
gmake: Makefile: No such file or directory
gmake: *** No rule to make target 'Makefile'.  Stop.
```

Warnings observed before the configure error:

- Doxygen was not found; CMake says this can be ignored if documentation is not being built.
- Include-what-you-use was not found.
- `libbluetooth-dev` / `bluez` was not found; Bluetooth support is optional according to the CMake warning.
- `libmagic-dev` / `libmagic` was not found; HTTP MIME detection falls back to built-in knowledge according to the CMake warning.

Test commands available:

- `ctest --test-dir cmake-build` after a successful configure with `SNODEC_BUILD_TESTS=ON`.
- Component tests are declared under `tests/component/*/CMakeLists.txt` via `snodec_add_test(...)`.

Sanitizers:

- AddressSanitizer is available as an opt-in CMake flag: `SNODEC_ENABLE_ASAN=ON`.
- The normal configure attempted for this baseline did not enable sanitizers.
- No UBSan or TSan CMake option was found during the baseline search.

## Macro baseline method/result

No production macro implementation was changed in Round 1.

Deterministic byte-for-byte comparison across separate process runs is not currently a reliable baseline because current output includes:

- wall-clock date/time from the spdlog pattern,
- an elapsed steady-clock tick custom formatter,
- optional ANSI color depending on TTY/color configuration,
- `PLOG` text derived from captured `errno` and the platform C library's `strerror` text.

Round 1 therefore documents the compatibility gate strategy instead of adding a production harness. Later rounds should either:

1. run a same-process golden capture using `Logger::setTickResolver(...)`, `Logger::setDisableColor(true)`, fixed log/verbose levels, fixed `errno` values, and controlled sinks; or
2. run a normalized process capture that strips/normalizes the leading timestamp and elapsed tick fields and strips ANSI escape sequences before comparing level labels, message text, `PLOG` strerror suffixes, and `VLOG` threshold behavior.

The gate must include enabled and disabled `LOG` levels, enabled and disabled `VLOG` levels, and `PLOG` errno capture at macro construction time.

## What was deliberately not changed

- Did not implement `SemanticLog.h`.
- Did not implement `SemanticLog.cpp`.
- Did not add `LogScopeOwner`.
- Did not add `.log()` or `frameworkLog()` methods to production classes.
- Did not migrate production call sites.
- Did not change `LOG`, `PLOG`, or `VLOG` definitions.
- Did not reformat unrelated files.
- Did not add a test target because the repository did not configure successfully in this environment.

## Known risks or open questions

- The normal build cannot complete in this Codex environment until `nlohmann-json3-dev` is installed or Express is made optional for baseline builds.
- The current logger has a useful `setTickResolver` hook for deterministic elapsed ticks, but the wall-clock date/time remains part of the spdlog pattern; a future harness may need a process-local sink or normalization to avoid wall-clock instability.
- `PLOG` compatibility should check that `errno` is captured at `LogMessage` construction time, not destructor time.
- The semantic contract maps `WARNING` to intended `Warn` and `FATAL` to intended `Critical`; later implementation should state this mapping explicitly when bridging legacy macros.
- MQTT currently duplicates identity in strings (`connectionName`, `MQTT Client`, `MQTT Broker`, `WsMqtt`). Round 2 should avoid broad migration and first introduce the semantic model around object boundaries.

## Recommendation for Round 2

- Install or otherwise make available the missing `nlohmann_json>=3.11` dependency so the normal build and tests can run.
- Add a focused legacy macro compatibility harness before changing macro internals. Prefer using `Logger::setTickResolver(...)`, `Logger::setDisableColor(true)`, and fixed thresholds to produce normalized comparisons.
- Introduce semantic logging types behind new names only after the macro gate exists.
- Start with scope metadata definitions and construction points for `SocketConnection`/`SocketContext`, but do not migrate production call sites until the compatibility gate passes.
