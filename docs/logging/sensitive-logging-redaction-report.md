# Sensitive logging redaction report

## Problem summary

The logging audit found security-sensitive diagnostics that emitted plaintext MQTT CONNECT credential/payload fields and OAuth2 token values. This report covers the focused PR A remediation for redacting those values without changing authentication, MQTT protocol behavior, OAuth2 response behavior, database schema, JSON schema, LogManager behavior, or legacy macro definitions.

## Files inspected

- `src/iot/mqtt/server/Mqtt.cpp`
- `src/apps/oauth2/authorization_server/AuthorizationServer.cpp`
- `src/apps/oauth2/resource_server/ResourceServer.cpp`
- `src/apps/oauth2/client_app/ClientApp.cpp`
- Broader `src/**/*.h`, `src/**/*.hpp`, and `src/**/*.cpp` matches for password, token, Authorization/Bearer, MQTT Will fields, and related accessor names.

## Findings fixed

1. MQTT CONNECT diagnostics logged plaintext `connect.getPassword()` when the password flag was present.
2. MQTT CONNECT diagnostics logged plaintext `connect.getWillMessage()` at `info` severity.
3. MQTT CONNECT diagnostics logged plaintext `connect.getUsername()` at `info` severity.
4. OAuth2 refresh-token diagnostics logged the plaintext `refresh_token` query value.
5. OAuth2 resource-server diagnostics logged the plaintext `access_token` query value.
6. OAuth2 authorization-server validation diagnostics logged plaintext invalid/valid access-token values.
7. OAuth2 client-app source hygiene preserved a commented-out example that logged an authorization-code value and a token request URI containing `code=...`.

## MQTT redaction changes

- Kept protocol version, connect flags, keep-alive, client id, clean-session, Will QoS, and Will retain diagnostics unchanged where they do not expose secret values.
- Moved Will topic diagnostics from `info` to `debug` because topic strings can carry deployment/application detail.
- Replaced plaintext Will message logging with presence and size only.
- Replaced plaintext username logging with username presence only.
- Replaced plaintext password logging with password presence only.
- Left MQTT validation and stored session state unchanged; `willMessage`, `username`, and `password` are still assigned for existing protocol behavior, but their values are not emitted by CONNECT diagnostics.

## OAuth2 redaction changes

- Replaced plaintext refresh-token diagnostics with supplied/length metadata.
- Replaced plaintext resource-server access-token diagnostics with supplied/length metadata.
- Replaced authorization-server validation outcome logs with value-free success/failure messages.
- Left request parsing, SQL lookup strings, response bodies, status codes, and token generation unchanged.

## OAuth2 authorization-code diagnostics

`src/apps/oauth2/client_app/ClientApp.cpp` did not contain an active runtime authorization-code log, but it did contain a commented-out token-exchange block that appended `req.query("code")` to a token request URI and showed a debug log printing both the authorization-code value and full request URI. The dead commented-out block was removed so the source no longer preserves or suggests logging OAuth2 authorization-code values. The active client-app flow still checks whether a code query parameter was supplied for routing behavior, but it does not log the code value or a token request URI containing `code=...`.

## Search commands run

Before changes:

```sh
rg -n "password|Password|token|Token|access_token|refresh_token|authorization_code|auth.?code|Authorization|Bearer|WillMessage|WillTopic|getPassword|getUsername|getWillMessage|getWillTopic|req\.query\(\"code\"\)" \
  src \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

```sh
rg -n "(log\(\)|Log\(\)|appLog\(\)|mqtt.*Log\(\)|frameworkLog\(\)|webHttpLog\(\)|expressLog\(\)|debug\(\)|info\(\)|warn\(\)|error\(\)|trace\(\)).*(password|Password|token|Token|access_token|refresh_token|Authorization|Bearer|WillMessage|getPassword|getWillMessage)" \
  src \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

After changes:

```sh
rg -n "password|Password|token|Token|access_token|refresh_token|authorization_code|auth.?code|Authorization|Bearer|WillMessage|WillTopic|getPassword|getUsername|getWillMessage|getWillTopic|req\.query\(\"code\"\)" \
  src \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

```sh
rg -n "getPassword\(|getWillMessage\(|RefreshToken:|AccessToken:|Invalid access token '|Valid access token '|Password:|WillMessage:|Recieving auth code|Receiving auth code|req\.query\(\"code\"\)|authorization code.*<<" \
  src \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

```sh
rg -n "\b(LOG|PLOG|VLOG)\s*\(" \
  src \
  -g '*.h' -g '*.hpp' -g '*.cpp'
```

## Remaining allowed sensitive-word occurrences

- MQTT accessor declarations/definitions and protocol assignments remain because they are required for protocol state and validation.
- MQTT redacted diagnostics mention `WillMessage supplied` and `Password supplied` without logging values.
- OAuth2 SQL/query logic and JSON response keys still use `access_token` and `refresh_token` where required by protocol/API behavior.
- OAuth2 diagnostics still mention validation outcomes such as `Invalid access token` and `Valid access token` without appending token values.
- `src/log/Logger.h` still contains legacy `LOG`, `PLOG`, and `VLOG` macro definitions; this PR did not remove macro definitions and did not reintroduce call sites.
- OAuth2 client-app active routing still checks `req->query("code")` for presence, but no active or commented-out diagnostic logs the authorization-code value or a full token request URI containing `code=...`.

## Tests run

- `git diff --check`
- `cmake -S . -B cmake-build -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON`
- `cmake --build cmake-build --parallel 2`
- `ctest --test-dir cmake-build -R "SensitiveLoggingRedactionTest|MqttMigration08Test|SemanticEndToEndOutputTest|FinalCleanupMigration09Test" --output-on-failure`
- `ctest --test-dir cmake-build --output-on-failure`
- `cmake -S . -B cmake-build-asan -DSNODEC_BUILD_TESTS=ON -DSNODEC_BUILD_APPS=ON -DSNODEC_ENABLE_ASAN=ON`
- `cmake --build cmake-build-asan --parallel 2 --target MqttMigration08Test SensitiveLoggingRedactionTest`
- `ASAN=$(gcc -print-file-name=libasan.so) LD_PRELOAD=$ASAN ctest --test-dir cmake-build-asan -R "MqttMigration08Test|SensitiveLoggingRedactionTest" --output-on-failure`

## Known follow-ups

1. PR B — high-frequency severity correction for MQTT packet and HTTP request/response lifecycle logs.
2. PR C — duplicate connection-close statistics cleanup.
3. PR D — inner epoll robustness and syscall diagnostics.
4. PR E — broader syscall discipline audit.
5. Final macro removal/source gate.
6. Round 10 — book integration.
