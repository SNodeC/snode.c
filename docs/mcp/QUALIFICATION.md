# MCP qualification

The current separation qualification is described below; earlier application qualification is historical. The initial optional-feature record is retained at the end as historical evidence; it does not validate later revisions.

This record covers the current application tour and preserves the initial optional-feature series at `0292f96bc` as history. The application guide, including HTTPS and the public API inventory, is [SHOWCASE.md](SHOWCASE.md).

The optional-feature implementation targets the official 2025-11-25 and 2026-07-28 schemas. The [API addendum](API-PROPOSAL.md#optional-feature-implementation-addendum) records the revision-specific wire decisions and normative references; the [guide](README.md) describes the APIs, bounds and transport ownership. Sampling, roots and modern logging remain excluded.

## Reply compile-check placement

The reply/result compile contract is now registered as `McpReplyTypeTest` and runs only through CTest. Project configuration performs none of its compiler invocations. The existing three matching combinations must compile and six mismatches must fail; all nine checks run even when some fail, and CMake returns failure after processing them. The test uses the configured compiler and flags in its own build directory. This adds one CTest registration (399 total, 397 enabled, with the same two SDK stdio cases disabled), without production changes.

Focused verification passed all 24 MCP unit tests on GCC and strict Clang+ASan, including the relocated test. An intentionally broken include path made the test fail and still produced all nine compile logs, verifying the positive controls and complete execution. Formatting and `git diff --check` passed. The full-suite results below predate this test-only relocation and retain their original counts.

## Separate executable qualification

The development assistant (`snodec-dev-stdio`, `snodec-dev-http` and its installed skill) is restored to the `69b16419a` functional surface. The dedicated `snodec-mcp-showcase-stdio` and `snodec-mcp-showcase-http` executables own synthetic registrations, page size three, task limits, demo authorization, protected-resource metadata, `/showcase/approve`, replay sizing and independent named HTTP/HTTPS instances. The development assistant keeps page size 128 and the baseline task settings, `/mcp` route and CLI. All concrete registrations and transport code are shared without copying implementations.

The development registration set is four tools (`api.find`, `api.read`, `skeleton.preview`, `review.label`), two resources (`snodec://development/house-style`, `snodec://development/review-label`), prompt `snodec-review`, and template `snodec://development/style/{section}`. Four `McpDevelopmentInventory_*` gates check the exact sets, one page, absence of showcase entries, absence of synthetic HTTP routes and absence of showcase CLI options, for both eras and both dev transports.

The six typed C++ client tours, six raw-wire cases, SDK showcase cases and HTTP/HTTPS instance policy test target the dedicated executables. Staged installed-consumer tours use the installed showcase binary. Common router fixture tests remain library controls. The two SDK stdio progress cases remain disabled with their existing owner-decision comment; their assertions are preserved.

The development skill and all development handler definitions match `69b16419a`. The registration statements and stdio lifetime were extracted into shared functions without functional changes. The stdio include-analysis corrections, `SubCommand` perfect forwarding and replay-retirement library fix remain. No showcase identity, policy, inventory, task setting or small page default remains in the dev assistant.

The owner-requested schema-validator update retains exact integer and zero decisions without floating-point equality diagnostics. Its numeric regression covers adjacent fractions, signed zero and overflowing `multipleOf` quotients. Required draft-07 (972/972) and 2020-12 (1319/1319) conformance passed; the existing optional 2019-09 cross-draft limitation remains.

Native observations in the historical sections were recorded before this separation. They are not new native-client runs of the separate executables. Native decoding causes and unexposed controls retain their original UNVERIFIED status; automated protocol coverage is recorded independently.

| Check | Separation result |
| --- | --- |
| GCC 16.2 Debug, full clean build and full CTest | 396/396 enabled tests passed; 398 registered, two owner-disabled SDK stdio cases, zero skips; 301.13 seconds |
| Clang 21.1.8 strict + ASan, full clean build and full CTest | 396/396 enabled tests passed; identical inventory and disabled cases, zero skips; 357.31 seconds; leak detection, halt-on-error and abort-on-error enabled |
| Protocol / transport matrix | Six C++ tours and six raw-wire cases passed on both toolchains: both eras over stdio, HTTP and HTTPS |
| Dev surface | All four exact-inventory gates passed on both toolchains; manual stdio probe confirmed four dev tools on one page and 33 showcase tools over eleven pages |
| HTTP/HTTPS policy and retirement | Independent showcase instances and both shared-router retirement regressions passed on both toolchains |
| Installed consumption | Staged exported components, concrete examples and both installed showcase stdio tours passed on both toolchains |
| Include analysis and formatting | Zero IWYU corrections; twelve touched C++ files and five CMake files passed formatting checks; `git diff --check` passed |
| Wire examples | Twenty-two selected exchanges recaptured from both typed stdio tours; four-space JSON retained; no local paths or native credentials |

The starting local merge had 394 test registrations. This separation adds exactly four `McpDevelopmentInventory_*` registrations and removes none, yielding 398 registrations and 396 enabled tests. All 375 original baseline registrations remain, as do all six SDK tests. The two SDK stdio showcase cases retain their owner-disabled status and existing explanatory comment; SDK read, client and HTTP showcase cases passed. Control tests remain enabled and every Python test command uses `-B`.

The final GCC clean build has no compiler warnings. Strict Clang retains the existing third-party validator warning exception; its other pre-existing warnings remain visible, with no floating-point equality diagnostics or SNode.C application warnings. No warning or analysis setting was weakened for this change.

## Historical combined-application qualification

Before separation, the combined `snodec-dev-stdio`, independent `mcp-http`/`mcp-https` instances in `snodec-dev-http`, and `snodec-mcp-client client --command tour` demonstrated the implemented module. See [SHOWCASE.md](SHOWCASE.md), the [API inventory](SHOWCASE-API.md) and the selected synthetic [wire examples](SHOWCASE-WIRE.json). Sampling and roots remain excluded; logging and replay apply only to 2025-11-25. MCP SOVERSION remains 4.

Configure, build and test strictly sequentially in separate external GCC and strict Clang+ASan build directories. Enable applications, tests, control tests and `CHECK_INCLUDES`. Run every test with `TMPDIR=/tmp/snc`; Python tests use `python3 -B`. Strict Clang uses `-Weverything -Werror`, ASan and frame pointers, with `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:abort_on_error=1`. Existing third-party exclusions remain unchanged. The installed-consumer check stages its own prefix and loads the staged MCP and JSON-RPC libraries rather than a live installation.

SNode.C's own C++ client is the end-to-end authority: six `McpShowcaseTour_*` tests exercise both protocol versions over stdio, HTTP and HTTPS. Six corresponding `McpShowcaseWire_*` tests preserve independent raw-wire assertions. `McpShowcaseInstancePolicyTest` checks that HTTP and HTTPS policies, registries and state are independent. The staged installed consumer compiles the concrete examples against exported components and runs both stdio tours.

The owner retained the TypeScript SDK integration and disabled only `McpShowcaseSdk_2025-11-25_stdio` and `McpShowcaseSdk_2026-07-28_stdio`. SDK 2.4.0 loses the final progress callback when a stdio notification and result arrive together; earlier wire evidence contains all three ordered progress messages. The assertions remain intact. The two SDK HTTP showcase tests, `McpReadSdkTest` and `McpClientSdkTest` stay enabled using the already configured test dependency. No SDK checkout change, SDK build, SDK installation or SDK PR qualification is part of this work.

### Replay retirement fix

The invariant is that every operation retained by `State::replayStreams` retains valid replay state until `State` retires it. Previously, DELETE after GET SSE disconnection called `Operation::shutdown()`, which abandoned replay while its map entry remained. The later cleanup dereferenced cleared state in `Replay::prune`; the local before-fix ASan regression aborted. Removing that premature abandonment leaves retirement with the existing `State::cleanReplay` owner and its teardown. This deletes three production lines without introducing a timer, flag, second owner or null-check workaround.

`McpHttpReplayTest` and the new `McpTlsReplayTest` exercise the shared router through the local HTTP/TLS application: disconnect GET SSE, DELETE, wait 1.2 seconds through the one-second cleanup interval, reject the deleted session with 404, initialize a fresh session, successfully list tools, and require clean shutdown. Existing POST/GET replay, expiry, eviction, principal/session binding, no duplicate execution and modern replay exclusion assertions remain enabled. The common fixture helper places MCP options in their owning section before TLS options. The four existing database Python launcher registrations also use `-B`, so all 76 Python CTest commands meet the bytecode-suppression requirement without changing test assertions.

### Other reviewed fixes already on the branch

`utils::SubCommand::newSubCommand` forwards constructor arguments instead of dropping them; `SubCommandForwardingTest` verifies the public construction interface. The stdio endpoint headers export their inline dependencies for include analysis. The HTTP application constructs separate registries and configuration owners for its named plain/TLS transports; `McpShowcaseInstancePolicyTest` exercises different policies and state in one process. The C++ tour reports each request and checks complete removal from all four dynamic inventories. Search responsiveness starts after protocol readiness, separating process startup from the concurrent-read budget. The staged installed consumer includes private MCP and JSON-RPC library directories in its runtime search path; its compilation and both installed stdio tours verify exported consumption.

### Feature and native support inventory

All automated rows below apply to both eras over stdio/HTTP/HTTPS where the feature exists. Native observations refer only to the negotiated era and tested stdio/HTTP invocations. They establish observed interoperability, not support for every future client version.

| Feature / demonstration | Automated boundary | Claude Code 2.1.331 | Codex CLI 0.162.1 |
| --- | --- | --- | --- |
| Identity / negotiation / metadata | Tour + wire negotiation | Negotiated 2026-07-28 | Offered 2025-06-18; negotiated 2025-11-25 |
| Text, tool/protocol errors, structured validation, schema | Tour + wire valid/invalid cases | Ordinary calls and expected errors observed | Ordinary calls and expected errors observed |
| All content variants / `content` | Tour + wire owned values and envelopes | Succeeded | “Unexpected response type”; decoding cause **UNVERIFIED** |
| Principal / header mirroring / HTTP auth and Origin | Tour + wire; instance-policy test | Ordinary calls observed; exhaustive policy controls not exposed | Ordinary calls observed; exhaustive policy controls not exposed |
| Four paginated inventories / catalogue entries | Tour follows complete pages; wire cursors | Prompt slash-command discovery followed four pages; no broader native pagination claim | One resource inventory page reported “Unexpected response type”; cause **UNVERIFIED** |
| Progress / cancellation / deadlines / `work` | Tour hooks, cancel, timeout and continued use | No interactive cancellation check claimed | No interactive cancellation check claimed |
| Optional/required tasks and task input | Tour + wire creation, get/result/list/cancel/update and polling | Required calls omitted per-request tasks extension; -32021 | Required tasks not augmented; rejected |
| Form, URL, rounds, parallel, resource/prompt input | Tour + wire independent asynchronous replies | Headless elicitation returned cancellation without a form | Headless elicitation returned cancellation |
| Text/blob/changing resources, templates, completion | Tour + wire reads, RFC 6570, completion, updates | Resource read not exposed in these print-mode runs | Five requested resource reads succeeded; completion/subscription controls not exposed |
| Conversation and embedded-resource prompt | Tour + wire get and completion | MCP slash command succeeded over stdio and HTTP; observed `prompts/get`, text and embedded resource | Prompt retrieval not exposed |
| Dynamic registration/removal of all four kinds | Tour full inventories/removal + wire notifications | Refresh delayed; HTTP eventually called `dynamic.tool` | Dynamic refresh not exposed |
| Legacy logging / modern exclusion | Tour + wire logging; omitted/rejected in modern era | Excluded in negotiated modern era | Ordinary logging tool invoked; level control not exposed |
| Legacy sessions/replay / modern exclusion | C++ reconnect fault injection + wire + HTTP/TLS retirement regressions | Excluded in negotiated modern era | No native forced reconnect claimed |
| Transport lifecycle and installed consumption | Component lifecycle tests + staged installed consumer | Stdio/HTTP ordinary invocations completed | Stdio/HTTP ordinary invocations completed |

The separate Claude MCP slash-command checks used `/mcp__showcase__conversation alpha` over stdio and HTTP. Both followed four `prompts/list` pages and sent `prompts/get` with `name=conversation` and arguments `name=alpha`; the response contained `Hello alpha` and the embedded `showcase://text` resource with `SNode.C`. The first restricted-environment attempt failed before protocol traffic and is excluded from successful evidence.

Native settings and authentication were not changed for qualification. Per-invocation MCP definitions target the local application; Claude-managed runtime metadata updates are allowed. Protocol observations, rather than CLI exit status or generated prose alone, establish success. Captures committed as documentation contain demonstration data only; native credentials, authorization headers, account/session metadata and opaque network captures are excluded.

Interactive cancellation remains a manual owner check in the native clients. Automated C++ tests verify protocol cancellation and continued operation. Native task controls, completion, subscriptions, forced replay and prompt retrieval remain **UNVERIFIED** where the client's tested interface did not expose them. Headless cancellation is not evidence of a visible form or an interactive cancellation UI.

Inspector recorded 70 invocations: 66 supported invocations succeeded and four `tasks/list` invocations were rejected by the CLI before dispatch. Those four are unsupported controls, not server passes. Inspector one-shot calls do not establish notification UI or reconnect behavior.

### Final suite results

| Check | Result |
| --- | --- |
| GCC 16.2 Debug full suite | 392/392 enabled tests passed; 2 owner-disabled SDK stdio cases; zero skips; 307.10 seconds |
| Clang 21.1.8 strict + ASan full suite | 392/392 enabled tests passed; the same 2 disabled cases; zero skips; 363.82 seconds; leak detection and abort-on-error enabled |
| Era / transport matrix | All six C++ tours and all six raw-wire tests passed on both toolchains: 2025-11-25 and 2026-07-28 over stdio, HTTP and HTTPS |
| Replay retirement | HTTP and HTTPS regressions passed on both toolchains, including subsequent requests and clean shutdown |
| Installed consumption | Staged exported components, private runtime libraries, example compilation and both installed stdio tours passed in both full suites |
| Include analysis | `CHECK_INCLUDES=ON`; fresh strict router compilation and qualifying build logs reported zero add/remove or compiler diagnostics |
| Formatting | 1,037 C++ files and six changed CMake files passed the committed formatter checks |
| Whitespace | `git diff --check` passed |

The original baseline contained 375 registrations, including `McpReadSdkTest` and `McpClientSdkTest`. All 375 remain. Nineteen were added: six `McpShowcaseTour_*`, six `McpShowcaseWire_*`, four `McpShowcaseSdk_*`, `McpShowcaseInstancePolicyTest`, `SubCommandForwardingTest` and `McpTlsReplayTest`. Relative to the later 393-test inventory, only `McpTlsReplayTest` was added. GCC and Clang have identical 394-test inventories, with 392 enabled and the two SDK stdio showcase cases disabled. Under the owner's revised decision, **no SDK tests were removed**. The enabled SDK read, client and two HTTP showcase cases passed on both toolchains. All 76 Python test commands include `-B`; control tests remain enabled.

The earlier frozen GCC and strict Clang+ASan runs each passed 391/393 with zero skips; the two failures were the SDK stdio progress cases. They are baseline evidence only and do not validate the replay change. The current owner decision retains these tests as disabled rather than removing SDK coverage. No test assertion was weakened.

## Historical optional-feature record

The remainder of this section records `0292f96bc` and its original test environment. SDK package requirements and suite counts here are historical, not instructions for the current qualification.

### Historical build discipline

Use separate external GCC and Clang build directories. Configure, build and test each directory sequentially: Doxygen configuration writes into the source tree. Enable `SNODEC_CONTROL_BUILD_TESTS`, applications and `CHECK_INCLUDES`; set `SNODEC_MCP_SDK_ROOT` to the installed official client package. The SDK regression also needs the adjacent official server package. All npm qualification dependencies belong in temporary storage, never the checkout. Run CTest with a short `TMPDIR` and `-j 1`.

The strict Clang configuration uses the repository warning baseline with `-Weverything -Werror`, AddressSanitizer and frame pointers. Run with `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:abort_on_error=1`. Existing third-party warning and include-analysis exclusions remain unchanged. Formatting checks use the committed configurations and exclude the schema-validator submodule.

### Historical coverage

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

### Historical native clients

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

### Historical bounds and deliberate differences

The task backend is bounded process-local storage, retaining work across transport disconnects within the process lifetime; no disk or multi-process durability is claimed. The spawned-stdio binding requires Linux pidfd support, using a read receiver and waitpid without taking over global SIGCHLD policy. HTTP exposes separate Http/Https template bindings; the demonstration CLI uses plain HTTP or spawned stdio. OAuth acquisition remains the embedding application's responsibility; the HTTP token supplier is called per exchange.

Modern HTTP resumability is deliberately absent because the 2026 transport excludes it. The legacy task metadata/error-schema conflict is documented in the guide; exact underlying error values are preserved. The unreleased MCP SOVERSION remains 4. No tag or release is created.

### Historical recorded suite results

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
