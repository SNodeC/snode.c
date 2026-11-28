# MCP qualification

The current completion work extends the separate showcase applications and the opt-in sampling/roots APIs. The [operation inventory](SHOWCASE-API.md#protocol-operation-inventory) distinguishes both protocol revisions, including operations removed or replaced in 2026. The [application guide](SHOWCASE.md) documents running the separate executables; [client inputs](CLIENT-INPUTS.md) documents application ownership and lifetime.

## Ownership-isolated integration gate

A GCC export of staged tree `7ae838b24862e9c683142fa9ceba9bd824a54236` built and passed all 65 enabled MCP-label cases in 230.50 seconds: 67 selected, two owner-disabled SDK stdio cases, zero failures or skips. It contained only the staged MCP changes and committed dependency contents; the unrelated include edits were excluded. The temporary export and build were removed after verification. This independently verifies the implementation without those working-tree changes.

The MCP-only records below predate the final integration gate. Local merging requires complete clean GCC and strict Clang/ASan builds followed by the entire CTest suite. The include changes outside that MCP integration were later committed in `f4dcde9f7`. Full-suite outcomes and commit ownership evidence are recorded in the local handover. Include-analysis tooling and the existing SDK test dependency bundle are kept in persistent build tooling outside the repository; no machine-specific tool paths are committed.

## MCP completion qualification (2026-11-28)

One shared task store now owns server-hosted tool tasks and client-hosted legacy sampling/elicitation tasks: tickets, status, retained results, expiry, cancellation and waiting result requests. The former server-only implementation is removed. Existing input correlation and handler resumption carry reverse task controls and ping. A completed or cancelled client task retains its task-owned expiry timer; cancelling the original, already answered augmentation RPC cannot cancel that separately owned task. Terminal payloads retain the child task's related-task metadata. Roots cannot accidentally inherit sampling/elicitation task support; augmentation requires the exact advertised method, and optional task list/cancel methods enforce their own capability declarations. Completion retains its task across application callbacks: an error observer may disconnect reentrantly without switching completion ownership or invoking an empty callback. The public-boundary regression failed before retaining that temporary strong reference and passed after the correction.

The showcase has 37 tools: `client.task` and `client.ping` extend the previous 35-tool sampling/roots inventory. Sampling scenarios cover text/image/audio, all context and tool-choice modes, model hints/priorities, system prompt, temperature, stop sequences, metadata and a complete tool-use/result loop. Additional scenarios cover client-owned sampling/elicitation task completion and cancellation, bidirectional legacy ping, URL completion and URL-required errors, reverse progress, complete negotiated server descriptions, task status messages and modern request-scoped logging. Modern task input uses the negotiated task extension; the removed legacy client-owned task RPCs are not advertised in that era. Modern task status uses task descriptions rather than progress/logging streams.

The final Clang client-input fixture initially started timed task work before synchronous capacity setup. ASan made that setup exceed the task TTL before the event loop started. Timed calls now start after setup; TTL values and all assertions are unchanged. The focused case passed on both compilers after this test-only correction.

Regression coverage checks pending result ownership, abandoned-waiter capacity, session isolation, expiry after completion/cancellation, late replies, unsupported augmentation, malformed input, progress monotonicity and reentrant cancellation. URL completion accepts only learned IDs once, clears connection-scoped correlation on disconnect and bounds retained identifiers; unknown/duplicate and capacity controls cover the notification receiver. Capacity is retained for pending accepted interactions and released immediately on decline/cancellation/failure. Repeated decline/cancel and client-hosted URL task regressions cover that separate lifetime; these regressions failed before the disposition correction. The unknown/duplicate regressions failed before correlation was added. Ordinary input framing checks result shape before correlation completion; the original input schema is enforced at completion. Legacy URL completion uses the live originating response when available, so ordinary HTTP/HTTPS input needs no independent GET stream. Legacy server-owned tool-task input uses the session GET SSE channel, maintained by SNode.C's HTTP client.

Application-owned model providers, consent interfaces, actual filesystem authorization and OAuth flows remain outside the synthetic showcase. No new native-client qualification was performed for these additions. Native sampling/roots, task controls, progress/log forwarding and host UI behavior remain unverified unless explicitly covered by earlier observations below; those observations do not establish support for the new APIs. Interactive cancellation remains a manual owner check in native hosts; automated C++ tests verify protocol cancellation. Missing model-facing controls do not establish missing protocol support in a desktop bridge.

| Completion gate | Result |
| --- | --- |
| GCC 16.2 Debug | All 65 enabled MCP cases passed on the final implementation; 67 selected, the same two SDK stdio cases disabled, zero failures/skips; 229.64 seconds. Separate canned decline/cancel tours also passed. |
| Clang 21.1.8 strict + ASan | All 65 enabled MCP cases have passing evidence: 64 passed in the full 281.66-second selection; the client-input case passed in a final 1.67-second focused rerun after a test-only setup-order correction. The full run had that one failure, two owner-disabled SDK stdio cases and zero skips; no failures remain after the rerun; leak detection, halt-on-error and abort-on-error enabled. Separate canned decline/cancel tours also passed. |
| Protocol / transport matrix | Six typed tours and six independent raw-wire cases passed on each toolchain: both eras over stdio, HTTP and HTTPS. |
| Installed consumption | Final staged installed consumers and both installed stdio tours passed on GCC (37.21 seconds) and strict Clang/ASan (43.12 seconds). Loader checks cover the common MCP and MCP HTTP libraries without private `LD_LIBRARY_PATH` entries. Normal install prerequisites were built after the persistent trees initially lacked the poll multiplexer library. |
| Development assistant | All four exact-inventory gates passed on both toolchains. Fresh stdio probes found the original four tools on one page and 37 showcase tools over 13 pages in each era. Development registrations, CLI, task settings, routes and skill are unchanged by this completion work. |
| Formatting / analysis | Twenty-one touched C++ files and four CMake files passed repository formatting; diff check passed. Strict MCP compilation emitted zero warnings/errors, and touched files have zero IWYU recommendations. Recommendations in six untouched files remain outside this change. |
| Documentation data | Twenty-six selected groups regenerated from one fresh typed stdio tour per era, with repetitive polling omitted; four-space JSON and synthetic data only. The advertised schema examples retain `$schema`, `$defs`, local references and `unevaluatedProperties`. |

The test inventory remains 400 registered and 398 enabled; this completion phase adds regression assertions without adding or removing test registrations. All 375 original registrations remain, with 25 additions from earlier work; all six SDK registrations remain and only the two owner-disabled stdio cases are disabled. All 80 Python CTest commands use `-B`, and control tests remain enabled. The whole non-MCP suite was not rerun for this change. Configure and test commands ran sequentially using the persistent toolchain builds and `TMPDIR=/tmp/snc`. A final fixture rebuild briefly overlapped another incremental build; the second was stopped, the dependency cache was audited and a serial rebuild completed before testing.

The combined production diff adds 1,658 lines and removes 644, including moved task-store code, license headers and three inherited include deletions in overlapping files. Excluding those inherited deletions, this task removes 641 lines (net +1,017), within the owner-approved bound of 2,000 added production lines plus tests/documentation. Existing public object layouts and virtual interfaces are unchanged, and MCP SOVERSION remains 4. The TypeScript SDK checkout was left unchanged; no SDK build, installation or SDK PR qualification was performed. System-installed applications were not updated.

## Historical sampling and roots qualification (2026-11-27)

At this historical revision, sampling and roots used opt-in public APIs and the existing client-input lifetime: [CLIENT-INPUTS.md](CLIENT-INPUTS.md). Both eras are covered, including sampling text, media validation, tool-use/result balance, mixed input rounds, tool-task input/update/result, application refusal, timeout, cancellation, disconnect and legacy roots changes. Unsupported client methods return Method not found. Malformed roots notification capabilities cannot escape the event loop. The 2026 client capability encoding omits the removed legacy `listChanged` field. Client-side task-augmented sampling RPCs remained unadvertised at that revision; the completion work above supersedes that limitation. The focused fallback regression verifies the mandatory 2025 behavior: unadvertised task-augmentation metadata is ignored and an ordinary sampling result is returned. That regression failed before the correction and passed afterward on GCC and strict Clang/ASan; only the focused case was rerun for this one-line correction.

| Gate | Result |
| --- | --- |
| GCC MCP tests | All 65 enabled MCP cases passed after targeted corrections and reruns; two owner-disabled SDK stdio cases retained |
| Strict Clang and ASan | All 65 enabled MCP cases passed, with final affected cases rerun after the capability-encoding correction; leak detection, halt-on-error and abort-on-error enabled |
| Protocol / transport matrix | Six typed tours and six independent raw-wire cases passed on each toolchain: both eras over stdio, HTTP and HTTPS |
| Installed consumption | Staged installed consumers and their tours passed on the final revision with each toolchain |
| ABI | Existing client virtual interface and object layouts retained; previous-header elicitation consumer passed against the new libraries; MCP SOVERSION 4 retained |
| Development assistant | All four original-inventory gates passed on both toolchains; registrations, configuration and skill remain unchanged |
| Formatting / includes | Fifteen touched C++ files checked using the repository style; two CMake files checked; zero IWYU diagnostics in final builds; diff check passed |
| Documentation data | Twenty-four selected exchanges regenerated from the typed stdio tours, including sampling and roots; four-space JSON, synthetic data only |

The repository inventory grew from 399 to 400 tests by adding `McpClientInputsTest`; no tests were removed. Of these, 398 are enabled and the same two SDK stdio cases remain disabled by owner decision. The whole repository suite was not rerun for these MCP changes. GCC's earlier failed cases were replaced by passing targeted results; the final affected selection contained 16 MCP cases plus the staged installed-consumer case on each toolchain.

Native model-provider integrations, permission UIs and native-client sampling/roots remain unqualified. The typed tour supplies a deterministic test model and informational root; it does not contact an LLM provider or access that directory. The TypeScript SDK checkout was unchanged. Production changes add 994 lines and remove 189 (net +805), within the owner-approved bound of 1,200 added lines; the shared input path replaces the elicitation-only path while preserving its public APIs. New demonstrations stay in the separate showcase executables.

## Reply result type checks

`McpReplyTypeTest` is a regular C++20 unit test whose compile-time assertions check that each reply accepts its matching result and rejects the six mismatched pairs. The test builds with the same MCP server target requirements as the neighboring unit tests. CTest runs the resulting executable. This replaces the nested CMake checks without changing the test inventory.

## Separate executable qualification

The development assistant (`snodec-dev-stdio`, `snodec-dev-http` and its installed skill) is restored to the `69b16419a` functional surface. The dedicated `snodec-mcp-showcase-stdio` and `snodec-mcp-showcase-http` executables own synthetic registrations, page size three, task limits, demo authorization, protected-resource metadata, `/showcase/approve`, replay sizing and independent named HTTP/HTTPS instances. The development assistant keeps page size 128 and the baseline task settings, `/mcp` route and CLI. All concrete registrations and transport code are shared without copying implementations.

The development registration set is four tools (`api.find`, `api.read`, `skeleton.preview`, `review.label`), two resources (`snodec://development/house-style`, `snodec://development/review-label`), prompt `snodec-review`, and template `snodec://development/style/{section}`. Four `McpDevelopmentInventory_*` gates check the exact sets, one page, absence of showcase entries, absence of synthetic HTTP routes and absence of showcase CLI options, for both eras and both dev transports.

The six typed C++ client tours, six raw-wire cases, SDK showcase cases and HTTP/HTTPS instance policy test target the dedicated executables. Staged installed-consumer tours use the installed showcase binary. Common router fixture tests remain library controls. The two SDK stdio progress cases remain disabled with their existing owner-decision comment; their assertions are preserved.

The development skill and all development handler definitions match `69b16419a`. The registration statements and stdio lifetime were extracted into shared functions without functional changes. The stdio include-analysis corrections, `SubCommand` perfect forwarding and replay-retirement library fix remain. No showcase identity, policy, inventory, task setting or small page default remains in the dev assistant.

The owner-requested schema-validator update retains exact integer and zero decisions without floating-point equality diagnostics. Its numeric regression covers adjacent fractions, signed zero and overflowing `multipleOf` quotients. Required draft-07 (972/972) and 2020-12 (1319/1319) conformance passed; the existing optional 2019-09 cross-draft limitation remains.

Native observations in the historical sections were recorded before this separation. They are not new native-client runs of the separate executables. Native decoding causes and unexposed controls retain their original UNVERIFIED status; automated protocol coverage is recorded independently.

A focused metadata check on 2026-11-26 confirmed that the installed showcase's stdio `text` response includes `_meta["showcase/result"] = "text"` in both protocol versions. Native calls through the connected `showcase-stdio` and `showcase-http` tools exposed only the text content, without result metadata. The active Codex backend was verified from its running executable as `0.171.0-alpha.13`; the older CLI on PATH does not identify this session. Its [release source](https://github.com/openai/codex/blob/rust-v0.171.0-alpha.13/codex-rs/tools/src/tool_output.rs#L239) explicitly removes private MCP result `_meta` before exposing results to Code Mode. This model-visible omission is therefore distinct from server delivery, which the raw-wire tests explicitly assert for both eras and all three transports.

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

Before separation, the combined `snodec-dev-stdio`, independent `mcp-http`/`mcp-https` instances in `snodec-dev-http`, and `snodec-mcp-client client --command tour` demonstrated the implemented module. See [SHOWCASE.md](SHOWCASE.md), the [API inventory](SHOWCASE-API.md) and the selected synthetic [wire examples](SHOWCASE-WIRE.json). At that historical revision sampling and roots were excluded; logging and replay apply only to 2025-11-25. MCP SOVERSION remains 4.

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
