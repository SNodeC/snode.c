# Optional-feature qualification

This record describes the initial optional-feature series at `0292f96bc`; subsequent review fixes add their own regression tests and qualification evidence.

The optional-feature implementation targets the official 2025-11-25 and 2026-07-28 schemas. The [API addendum](API-PROPOSAL.md#optional-feature-implementation-addendum) records the revision-specific wire decisions and normative references; the [guide](README.md) describes the APIs, bounds and transport ownership. Sampling, roots and modern logging remain excluded.

## Reproducible build discipline

Use separate external GCC and Clang build directories. Configure, build and test each directory sequentially: Doxygen configuration writes into the source tree. Enable `SNODEC_CONTROL_BUILD_TESTS`, applications and `CHECK_INCLUDES`; set `SNODEC_MCP_SDK_ROOT` to the installed official client package. The SDK regression also needs the adjacent official server package. All npm qualification dependencies belong in temporary storage, never the checkout. Run CTest with a short `TMPDIR` and `-j 1`.

The strict Clang configuration uses the repository warning baseline with `-Weverything -Werror`, AddressSanitizer and frame pointers. Run with `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:abort_on_error=1`. Existing third-party warning and include-analysis exclusions remain unchanged. Formatting checks use the committed configurations and exclude the schema-validator submodule.

## Coverage

| Feature | SNode.C client/server, both eras and transports | Official SDK client against SNode.C server | SNode.C client against official SDK server |
| --- | --- | --- | --- |
| Tools, resources, prompts | Passed, including typed results and input handling | Passed | Passed: 24 list/call/read/get cases |
| Runtime inventories / list changes | Passed, burst coalescing and modern subscription ids | Passed, all three notifications coalesced once | Not exercised by the SDK server fixture |
| Pagination | Passed, page size one; revision invalidation also unit-tested | Passed, twelve tool pages | Basic lists passed; multi-page SDK fixture not exercised |
| Legacy logging | Passed, thresholds/bounds plus delivery | Passed, one admitted mutation log | Not exercised by the SDK server fixture |
| Modern logging exclusion | Passed, omitted capability and no emission | Passed, no log notification | Not applicable |
| Task completion / polling | Passed | Passed with the official tasks extension | Not exercised by the SDK server fixture |
| Task cancellation / expiry | Passed over both bindings; isolation, limits and input also unit-tested | Passed, retained handles expire with -32602 | Not exercised by the SDK server fixture |
| Legacy HTTP replay | Passed with forced disconnect; no repeated operation | Existing SDK transport/read checks passed; replay fault injection uses SNode.C client | Not exercised by the SDK server fixture |
| Modern replay exclusion | Passed: no Last-Event-ID replay | Modern HTTP checks passed | Modern HTTP checks passed |
| Progress / cancellation / MRTR / elicitation | Client unit and component coverage; safe decline defaults | Existing feature/input tests plus ordinary SDK calls | Ordinary SDK calls; advanced interactions not exercised by this fixture |

Official package versions used: `@modelcontextprotocol/client` 2.4.0, `@modelcontextprotocol/server` 2.4.0 and `@modelcontextprotocol/ext-tasks` 0.2.3. The modern tasks adapter uses the extension's documented raw-dispatch and request-framing interface. The host coordinator allocates disjoint request ids and converts JSON-RPC envelopes to the adapter's result/error union. No SDK package source was patched.

Inspector 2.13.0 passed 42 supported command checks across both eras and transports: strict tool inventories, ordinary/task-backed tool calls, custom Unicode parameter headers, resource/template lists, resource reads, prompts and legacy logging levels. Its one-shot CLI rejects `tasks/list` before sending a request; two such attempts are recorded as unsupported, not server passes. Task listing and lifecycle were independently checked with the official tasks extension. One-shot Inspector commands do not establish notification UI or reconnect behavior.

## Native clients

Native gates use normal user settings/authentication with per-invocation endpoint overrides to the uninstalled build. Persisted MCP server definitions remain unchanged. Claude may update its normal runtime metadata. Each assertion below is based on observed MCP wire traffic rather than only the CLI exit status or generated answer.

| Feature | Codex CLI 0.162.1, stdio and HTTP | Claude Code 2.1.331, stdio and HTTP |
| --- | --- | --- |
| Negotiated era | 2025 initialization/initialized | 2026 discovery and subscriptions/listen |
| Tool calls | Passed: development lookup; background runs synchronously; mutation | Passed: development lookup and mutation |
| Resources | Native list/read passed | Native list/read passed |
| Prompts | Retrieval not exposed by its native tool interface | Native MCP slash-command retrieval passed, observed prompts/get |
| List changes | All three legacy notifications delivered; no explicit subscription control exposed | All three opt-in notifications delivered with subscription ids; subsequent tools/list observed |
| Resource-update subscription | Not requested by the native interface | Not requested by the native interface |
| Logging | Legacy mutation log delivered; no level-setting interface exposed | Excluded in this era |
| Tasks | No augmentation/control exposed; ordinary tool result remains synchronous | Tasks extension not negotiated; task-backed call correctly rejected with -32021 |
| Pagination | Resource cursor parameter exposed; no multi-page native flow claimed | No explicit pagination control exposed; no multi-page native flow claimed |
| Replay controls | No forced native reconnect claimed | Excluded in this era |

Neither native CLI was forced into an era it did not negotiate. The complementary era/transport combinations are covered by the SNode.C and official SDK tests. An early observer run let the HTTP fixture disappear and Claude returned exit zero with a connection error; those HTTP invocations were rejected as evidence and repeated with isolated server/observer processes. The repeated requests and prompt retrieval were verified on the wire. An exit-zero native invocation alone is not a gate.

## Bounds and deliberate differences

The task backend is bounded process-local storage, retaining work across transport disconnects within the process lifetime; no disk or multi-process durability is claimed. The spawned-stdio binding requires Linux pidfd support, using a read receiver and waitpid without taking over global SIGCHLD policy. HTTP exposes separate Http/Https template bindings; the demonstration CLI uses plain HTTP or spawned stdio. OAuth acquisition remains the embedding application's responsibility; the HTTP token supplier is called per exchange.

Modern HTTP resumability is deliberately absent because the 2026 transport excludes it. The legacy task metadata/error-schema conflict is documented in the guide; exact underlying error values are preserved. The unreleased MCP SOVERSION remains 4. No tag or release is created.

## Recorded suite results

| Check | Result |
| --- | --- |
| GCC 16.2 Debug, full suite, control tests and SDK enabled | 370/370 passed, 206.76 seconds |
| Clang 21.1.8 strict + ASan, full suite with leak detection | 370/370 passed, 232.28 seconds |
| Previous test names | All 357 retained; thirteen added; GCC and Clang lists identical |
| Installed consumer | Passed in both full suites, including shared/client components, private dependency loading and the installed client executable |
| Include analysis | CHECK_INCLUDES enabled; zero add/remove diagnostics in the qualifying build logs |
| Formatting | 71 changed C++ files and sixteen changed CMake files passed their committed formatters' check modes |
| Whitespace | git diff --check passed |

New test names: EventSourceParserTest, McpClientHandshakeTest, McpClientProcessTest, McpClientSdkTest, McpClientTest, McpClientTransportTest, McpHeadersTest, McpHttpReplayTest, McpLoggingTest, McpPaginationTest, McpReplayTest, McpRuntimeTest and McpTaskTest. Existing tests were not removed, weakened or disabled.

Qualification found and fixed child-exit/queued-attachment ownership, legacy HTTP setup ordering, default timeout construction and strict private-vtable issues. One intermediate GCC run timed out in the unchanged AddressResolutionPolicy-retry test after reporting a successful connection; its cause was not established. It subsequently passed twenty consecutive unchanged runs and the complete qualifying GCC and Clang suites. No test timeout was raised to obtain a pass.
