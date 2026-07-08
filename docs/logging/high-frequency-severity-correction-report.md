# High-frequency severity correction report

## Problem summary

High-frequency MQTT packet diagnostics and HTTP request/response lifecycle diagnostics were emitted at `info`, which made production logs noisy during normal protocol traffic. This correction keeps operator-visible lifecycle events at `info`, preserves warning/error paths, and moves normal per-packet/per-request diagnostics to `debug` or `trace`.

## Canonical recipe rule applied

The canonical logging recipe applied here is:

- `trace`: deep diagnostic detail, including serialized protocol data, full request formatting, headers, cookies, trailers, bodies, and queue micro-detail.
- `debug`: developer explanation of normal behavior, including normal per-packet and per-request/request-response lifecycle diagnostics.
- `info`: externally meaningful lifecycle/event.
- `warn`: recoverable abnormal situation.
- `error`: failed operation.
- `critical`: unrecoverable/fatal condition.

Normal high-volume HTTP/API requests now avoid `info`; per-packet MQTT diagnostics now avoid `info`; full request/protocol formatting remains trace-only or trace-guarded where touched.

## Files inspected

- `src/iot/mqtt/Mqtt.cpp`
- `src/web/http/server/SocketContext.cpp`
- `src/web/http/client/SocketContext.cpp`
- `src/iot/mqtt` and `src/web/http` via source searches for remaining `info()` and targeted lifecycle patterns
- `src` via sensitive logging and legacy macro regression searches

## MQTT changes

- Changed the per-packet `packet.getName()` receive summary in `Mqtt::printVP` from `info` to `debug`.
- Removed the separator-only `MQTT: ====================================` `info` line from `Mqtt::printFixedHeader`.
- Left serialized variable-header/payload and fixed-header hex dumps at `trace` behind existing `enabled(logger::LogLevel::Trace)` guards.
- Did not change packet parsing, session behavior, callbacks, stored state, validation, or redaction behavior.

## HTTP server changes

- Changed normal request parser start/success diagnostics from `info` to `debug`.
- Changed normal request delivery diagnostics from `info` to `debug`.
- Changed no-pending-request queue mechanics from `info` to `trace`.
- Changed normal response start, status line, and completion diagnostics from `info` to `debug`.
- Changed per-connection `HTTP: Connected` from `info` to `debug`.
- Preserved request parse errors at `error` and response-completed-with-error at `warn`.

## HTTP client changes

- Changed normal request accepted diagnostics from `info` to `debug`.
- Changed normal response received, status line, and request completed diagnostics from `info` to `debug`.
- Changed per-connection `HTTP: Connected` from `info` to `debug`.
- Split rejected request logging so the `warn` log remains a concise request-line summary and full request formatting is emitted only at `trace` behind an explicit trace guard.
- Preserved request rejection and delivery failure at `warn`, response parse error at `warn`, and response-without-delivered-request at `error`.

## Warn/error paths preserved

Warn/error intent was preserved for abnormal or failed operations:

- HTTP server request parse errors remain `error`.
- HTTP server response completion failures remain `warn`.
- HTTP client request rejection remains `warn`.
- HTTP client request delivery failure remains `warn`.
- HTTP client response parse errors remain `warn`.
- HTTP client response-without-delivered-request remains `error`.
- MQTT protocol validation/failure paths were not changed.

## Sensitive logging regression note

The sensitive logging regression search was rerun after the changes. No forbidden active or commented-out sensitive logging fragments were introduced. The rejected HTTP client request path now avoids dumping full headers/cookies/trailers at `warn`; full formatting is trace-only and guarded.

## Search commands run

```sh
rg -n "\\.info\\(\\)|\\.info\\s*\\(\\)|\\.info\\s*<<" \
  src/iot/mqtt src/web/http \
  -g '*.h' -g '*.hpp' -g '*.cpp'

rg -n "Request deliver|Response start|Response completed|Response received|Request accepted|HTTP: Connected|No more pending request|packet.getName\\(\\)|printVP|printFixedHeader|toString\\(" \
  src/iot/mqtt src/web/http \
  -g '*.h' -g '*.hpp' -g '*.cpp'

rg -n "getPassword\\(|getWillMessage\\(|RefreshToken:|AccessToken:|Invalid access token '|Valid access token '|Password:|WillMessage:|Recieving auth code|Receiving auth code|req\\.query\\(\"code\"\\)|authorization code.*<<" \
  src \
  -g '*.h' -g '*.hpp' -g '*.cpp'

rg -n "\\b(LOG|PLOG|VLOG)\\s*\\(" \
  src \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

Remaining `info()` occurrences in the inspected MQTT/HTTP tree are service, session, or broader MQTT broker/client application diagnostics outside this PR's focused per-packet/per-request severity correction scope.

## Tests run

- `git diff --check`
- `cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON`
- `cmake --build cmake-build --parallel 2`
- `ctest --test-dir cmake-build -R "HighFrequencyLoggingSeverityTest|SensitiveLoggingRedactionTest|MqttMigration08Test|SemanticEndToEndOutputTest|FinalCleanupMigration09Test" --output-on-failure`
- `ctest --test-dir cmake-build --output-on-failure`
- `cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON`
- `cmake --build cmake-build-asan --parallel 2 --target MqttMigration08Test HighFrequencyLoggingSeverityTest`
- `ASAN=$(gcc -print-file-name=libasan.so); LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan -R "MqttMigration08Test|HighFrequencyLoggingSeverityTest" --output-on-failure`

## Known follow-ups

1. PR C — duplicate connection-close statistics cleanup.
2. PR D — inner epoll robustness and syscall diagnostics.
3. PR E — broader syscall discipline audit.
4. Final macro removal/source gate.
5. Round 10 — book integration.
