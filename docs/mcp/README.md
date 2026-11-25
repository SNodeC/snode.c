# SNode.C MCP server

The in-tree MCP module provides one registry and dispatcher with stdio and Streamable HTTP adapters. Its first application is a SNode.C development assistant with read-only source access: `api.find`, `api.read`, `skeleton.preview`, the `snodec://development/house-style` resource, and the `snodec-review` prompt. It does not execute commands, inspect app configurations, build examples, or write files.

## Stability

The MCP libraries (`mcp-server` and `mcp-server-http`), their installed headers under `mcp/server`, and the `snodec-dev` applications are EXPERIMENTAL. Their API and ABI may change in a minor SNode.C release. The MCP libraries have their own SOVERSION, incremented on every MCP ABI break independently of the SNode.C core; their VERSION continues to follow the SNode.C release version. MCP changes alone never require a major SNode.C release. The compatibility policy for all other SNode.C libraries is unchanged.

## Build and consume

Initialize dependencies with `git submodule update --init --recursive` (or clone with `--recurse-submodules`) and install the PCRE2 development package providing pkg-config `libpcre2-8` (for example, `libpcre2-dev` on Debian/Ubuntu). MCP configuration fails explicitly if PCRE2 is unavailable. Configure the canonical checkout with an external build directory. Set `BUILD_DIR` and `HEADER_ROOT` for your environment; `HEADER_ROOT` must name the actual installed SNode.C header directory, whose children include `core/` and `express/`.

```sh
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug \
  -DSNODEC_BUILD_APPS=ON -DSNODEC_BUILD_TESTS=ON
cmake --build "$BUILD_DIR" --target snodec-dev-stdio snodec-dev-http
```

The module follows the MQTT layout under `src/mcp/server`; the applications live in `src/apps/mcp`. The internal `mcp::server::Mcp` protocol derives from the reusable [JSON-RPC peer](../jsonrpc/README.md). JSON-RPC owns envelopes, response correlation, timers and stream framing; MCP owns eras, metadata, registry dispatch, continuation semantics and output policy. The newline binding remains part of `mcp-server`, which links `jsonrpc` and `core-socket-stream` publicly; attach its `mcp::server::stream::SocketContextFactory` to a stdio endpoint or another stream connection. Its headers are installed under `snode.c/mcp/server/stream`. Installed components and exported targets are `mcp-server` and `mcp-server-http`, with `snodec::` target names; only the web-dependent HTTP binding has a separate component. The existing package installation procedure installs these components; no additional JSON or MCP SDK dependency is introduced. Consumers can use:

```cmake
find_package(snodec CONFIG REQUIRED COMPONENTS mcp-server)
target_link_libraries(my-stream-protocol PRIVATE snodec::mcp-server)
```

For the HTTP binding:

```cmake
find_package(snodec CONFIG REQUIRED COMPONENTS mcp-server-http)
target_link_libraries(my-server PRIVATE snodec::mcp-server-http)
```

## Run and configure

For a stdio client, configure the executable and arguments separately:

```sh
snodec-dev-stdio --config-file /dev/null --log-console stderr \
  mcp --header-root "$HEADER_ROOT"
```

Stdio preserves the original stdout descriptor for newline-delimited JSON-RPC and redirects ordinary stdout to stderr before initialization. The executable retains stdout before `core::SNodeC::init`, refuses daemon mode, attaches a `net::stdio::stream::SocketEndpoint` and runs the ordinary event loop. Shared protocol/diagnostic descriptions require `--quiet`. Even CLI help/errors are separated from protocol output. `--quiet` suppresses ordinary logging; it does not change the protocol descriptor. Client stdin EOF drains accepted requests and shuts down. Signals follow the normal SNode.C shutdown path.

For HTTP, the default instance is `mcp-http`, listening on `127.0.0.1:3337`, with the router mounted at `/mcp`:

```sh
snodec-dev-http --config-file /dev/null --daemonize=false \
  mcp-http local --host 127.0.0.1 --port 3337 \
  mcp --header-root "$HEADER_ROOT"
```

Options use the existing CLI11 instance/config-file mechanism. Stdio's section is `mcp`; HTTP's section is `mcp-http mcp`. Use `snodec-dev-http mcp-http mcp --help` or the corresponding stdio section help. Use `--config-file FILE` for a persistent configuration; `/dev/null` above selects an isolated empty configuration. Root log options precede instance sections.

| MCP option | Default | Bound / meaning |
| --- | --- | --- |
| `--header-root` | Configured installation include directory plus `snode.c` | Confined header tree; selected at startup |
| `--search-files` | 4096 | 1–8192 headers per search |
| `--search-bytes` | 8388608 | 1–67108864 bytes per search |
| `--maximum-message-bytes` | 1048576 | 1–16777216 bytes per message |
| `--maximum-output-bytes` | 1048576 | 1024–16777216 queued MCP output bytes |
| `--deadline-ms` | 30000 | 1–300000 ms for ordinary requests |
| `--allowed-origin` | Empty | Repeatable exact browser Origin allowlist |

Restart to change the selected header root and search budgets. Header traversal uses descriptor-relative opens, rejects symlinks and `..`, reads regular `.h`/`.hpp` files only, and bounds directory entries to 8192, queued directories to 512, and path depth to 16. A slice processes at most eight directory entries or one 16 KiB read, then yields through `core::timer::Timer::singleshotTimer`. Cancellation and deadlines are checked between slices. This bounds synchronous work; regular-file reads can still wait on the underlying filesystem.

`api.find` is a case-sensitive literal search with a required `query` of 1–128 Unicode code points and optional `limit` of 1–100 (default 20). It returns relative paths, one-based lines, byte offsets, and a conservative `truncated` flag. `api.read` requires a relative `path`, with optional byte `offset` (0–67108864) and `bytes` (1–65536, default 16384). Reads must form valid UTF-8. `skeleton.preview` accepts `kind` equal to `application` or `module` and returns proposed file text; it performs no writes. The review prompt accepts `code` as a string of at most 16384 bytes.

HTTP sets `ConfigHttpServer::setAllowPipelining(false)` and `setMaximumPendingRequests(1)` before initialization. Its explicit connection default is `--maximum-write-queue-bytes 1048576`, under `mcp-http connection`; this bounds the framework's write-queue high watermark, not a hard allocation cap. The MCP source queue has its own output-byte limit. HTTP/1.x responses remain sequential. Parser defaults are 1 MiB body, 64 KiB headers, and 100 header fields; the router also checks its own message limit. At most 64 live HTTP operations or stdio requests are accepted.

## Protocol eras and wire behavior

The modern era is [2026-07-28](https://modelcontextprotocol.io/specification/2026-07-28/basic/index). Every request carries `_meta` with `io.modelcontextprotocol/protocolVersion` and `io.modelcontextprotocol/clientCapabilities`. `server/discover` supplies server information and capabilities; responses use the modern result envelopes and caching hints (`ttlMs: 0`, private scope where required). This server advertises no list-change or subscription capabilities.

Modern HTTP requires `Content-Type: application/json`, an `Accept` containing both `application/json` and `text/event-stream`, and matching `MCP-Protocol-Version`, `Mcp-Method`, and, for named calls, `Mcp-Name` headers. Header names are case-insensitive. Names requiring encoding use the canonical `=?base64?BASE64?=` sentinel; names literally matching that sentinel are encoded too. Notifications receive HTTP 202 with an empty body. GET and DELETE return 405. There is no GET stream, session ID, replay store, or session state. See the [Streamable HTTP specification](https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http).

The router checks Content-Type itself (415) and checks the JSON middleware's attribute itself (parse error `-32700`, HTTP 400). Invalid JSON-RPC envelopes use `-32600`; invalid parameters use `-32602`; unsupported methods use `-32601`/404; internal errors use `-32603`/500. Modern header mismatch is `-32020`/400 and unsupported protocol version is `-32022`/400. Missing acceptable response media types produce 406; request-size limits produce 413, and the live-operation limit produces 429.

As required by the [modern tool error rules](https://modelcontextprotocol.io/specification/2026-07-28/server/tools#error-handling), malformed call envelopes and unknown tools produce JSON-RPC errors; schema input-value failures and tool execution/deadline failures produce a tool result with `isError: true`. Resource/prompt deadlines produce JSON-RPC internal errors. Unsupported registration schemas fail startup with a semantic error log and nonzero exit.

The compatibility era is [2025-11-25](https://modelcontextprotocol.io/specification/2025-11-25/basic/lifecycle). Stdio retains initialization context per transport and honors `notifications/cancelled`. HTTP legacy initialization creates a bounded `Mcp-Session-Id` notification session; clients open GET with that ID and the same Authorization credentials to receive resource updates, and DELETE closes the session. Unknown sessions return 404. Ordinary legacy calls can still be processed without session state; subscriptions require a session. `http::RouterOptions::legacySubscriptions` can explicitly disable this legacy binding. Modern requests remain stateless and never rely on legacy sessions. Legacy HTTP cancellation receives 202 and remains ignorable for bounded requests under the [cancellation rules](https://modelcontextprotocol.io/specification/2025-11-25/basic/utilities/cancellation).

Modern HTTP uses source-backed SSE for HTTP/1.1 `tools/call` and `subscriptions/listen`, and JSON for the other methods. Every SSE event contains a complete JSON message and ends with a blank line. The source completes through EOF, producing the final chunk terminator; streamed responses never use `json()` or `send()`. Closing an HTTP stream cancels its pending modern work. Stdio cancellation is request-ID based. During shutdown the server completes HTTP subscription streams with a correlated successful result before EOF; stdio uses its permitted subscription cancellation notification while the transport can still write.

`subscriptions/listen` honors `notifications.resourceSubscriptions` URI arrays, acknowledges the supported subset before updates, and tags messages with the original subscription ID. `Server::resourceUpdated(uri)` reports application-owned changes; it does not install filesystem watchers. Each subscription/channel accepts at most 128 URIs; the registry bounds live request subscriptions and legacy channels separately to 1,024 each. HTTP legacy sessions are limited to 64 and expire after 30 minutes of inactivity when subsequent requests prune the session table. Disconnect/cancel releases interests. No list-change notifications are advertised because registration is sealed.

## Supported schema dialects

Both protocol eras default to JSON Schema 2020-12; an explicit draft-07 `$schema` is also supported. Other dialects produce an explicit registration error. Tool input schemas remain objects with `type: "object"`; the legacy wire keeps its existing object-only structured-output encoding. Registered input/output schemas are validated against their bundled metaschema before attachment. See the [2025 schema requirements](https://modelcontextprotocol.io/specification/2025-11-25/basic#implementation-requirements) and [2026 schema requirements](https://modelcontextprotocol.io/specification/2026-07-28/basic#implementation-requirements).

Local references, `$defs`, anchors, dynamic references, compositions, conditional schemas, evaluated-property/item tracking, Unicode patterns and the standard validation keywords are supported. Unknown extension keywords remain annotations; `format` and content keywords are annotation-only in both supported dialects. Defaults do not modify arguments. Tool inputs are checked before the handler; successful structured outputs are checked before emission. Invalid values produce explicit tool errors. PCRE2 is required; see the [qualification record](SCHEMA-VALIDATOR.md) for required-suite results and optional regex/format limitations.

Validation never fetches network references. References must resolve within the registered schema or the bundled metaschemas; unresolved external references are registration errors. Schema size is limited to 16 KiB, compilation to 1,024 subschemas and depth 256, and validation to 100,000 evaluations and depth 256. PCRE2 matching is bounded separately. These are implementation limits following the [reference and composition guidance](https://modelcontextprotocol.io/specification/2026-07-28/basic#ref-resolution), not protocol constants.

## Embedding and security

Subclass `mcp::server::Tool`, `Resource` or `Prompt` and override `call`, `read` or `get`. Each base owns its typed definition and exposes it through `getDefinition`; register a shared instance with `Server::tool`, `resource` or `prompt`. The server retains the instance for its lifetime, validates the definition transactionally and throws a named `RegistrationError` on failure. Registration is sealed on transport attachment. Constructors and handlers run on the event-loop thread; retain a shared reply for asynchronous work and keep the server alive until attached transports and requests drain.

Parse schema raw strings once during construction. Use fully expanded JSON in natural schema order (`type`, `properties`, `required`, then the remaining keys), exactly as `nlohmann::ordered_json::parse(text).dump(4)` writes it; malformed schema text must throw `RegistrationError` naming the registration and `inputSchema` or `outputSchema`. The constructor function-try-block below catches parsing failures during base initialization. Demo classes use the equivalent application-local `parseSchema` helper.

```cpp
#include <mcp/server/Server.h>
#include <mcp/server/Tool.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

class EchoTool final : public mcp::server::Tool {
public:
    EchoTool();
    void call(const mcp::server::ToolCall& request, const std::shared_ptr<mcp::server::ToolReply>& reply) override;
};
EchoTool::EchoTool() try
    : Tool({"echo", "Echo the supplied text object", nlohmann::ordered_json::parse(R"json(
{
    "type": "object",
    "properties": {
        "text": {
            "type": "string"
        }
    },
    "required": [
        "text"
    ],
    "additionalProperties": false
}
)json")}) {
} catch (const mcp::server::Json::parse_error& error) {
    throw mcp::server::RegistrationError(mcp::server::RegistrationKind::Tool, "echo", "inputSchema", error.what());
}
void EchoTool::call(const mcp::server::ToolCall& request, const std::shared_ptr<mcp::server::ToolReply>& reply) {
    reply->result(mcp::server::ToolResult::json(request.arguments));
}
```

```cpp
mcp::server::Server server("example", "1", "Example tools");
server.tool(std::make_shared<EchoTool>());
```

For an existing `ToolDefinition`, the lambda overload is the short form. It wraps the callback in a private `Tool` subclass and uses the same validation, retention and dispatch path. For example, use this instead of the class registration above:

```cpp
const auto definition = EchoTool().getDefinition();
server.tool(definition, [](const mcp::server::ToolCall& request, const std::shared_ptr<mcp::server::ToolReply>& reply) {
    reply->result(mcp::server::ToolResult::json(request.arguments));
});
```

The handlers receive `ToolCall`, `ResourceRead` or `PromptGet` plus the matching shared `ToolReply`, `ResourceReply` or `PromptReply`; a mismatched result kind does not compile. Use `ToolResult::text`, `ToolResult::json`, `ToolResult::toolError`, `ResourceResult` and `PromptResult` instead of constructing wire envelopes. `ToolResult::json` derives text and structured content from one value; non-object JSON is text-only in the 2025 era. Use `RequestContext::onCancel` to stop bounded asynchronous work; the module owns the deadline timer and `Reply::progress` preserves the request token's string/integer type. The complete examples are `src/apps/mcp/main.cpp` and the five class files `FindTool`, `ReadTool`, `SkeletonPreviewTool`, `HouseStyleResource` and `ReviewPrompt`; `Tools.cpp` retains shared confinement and slicing. Link the transport component, create `mcp::server::Config` before initialization, and ensure the server and config outlive the transport/router. For newline-framed MCP use `mcp::server::stream::SocketContextFactory` with the original stream transports, including `net::stdio::stream::SocketEndpoint`. The application owns initialization, protocol-stdout retention and `core::SNodeC::start`; register handlers after parsing, then queue `attach`. Set the MCP connection defaults with `setReadTimeout(0)->setWriteTimeout(0)` before parsing; typed connection options can override them. See `src/apps/mcp/main.cpp` for the foreground/daemon-reparse guards. For HTTP construct and mount `mcp::server::http::Router` before calling `listen`; its shutdown guard preserves cancellation ordering. The same router is exercised with both legacy and TLS WebApp variants in the component tests.

The development HTTP app binds to localhost by default. A present Origin must be well-formed and exactly allowed; invalid or unlisted Origins receive 403. An absent Origin is allowed for native clients. The default empty allowlist therefore rejects browser Origins. Supply repeatable `--allowed-origin` values only for trusted browser callers.

The router accepts an optional `Authorize` callback taking the express request and returning `Authorization`: `Authorized`, `Unauthorized` with a challenge, or `Forbidden`. An embedding application can validate bearer tokens there; a future SNode.C identity-provider integration can call its token-validation interface through this hook. No identity-provider integration or built-in token store is supplied. Optional `http::ProtectedResource` metadata is configured through `http::RouterOptions::protectedResource`; call `Router::mountMetadata(app)` on the root router before listening to publish the RFC 9728 paths. This adds discovery and Bearer challenges, not an authorization server or token validator. Use TLS and an appropriate authorization callback before exposing HTTP beyond loopback. The shipped HTTP development application does not expose a bearer-token configuration option.

The shipped tools never start child processes. A future command-running tool would require a separately approved command allowlist and confined working-directory policy; this release provides neither command execution nor a generic shell escape. Header text and review input are untrusted data, not instructions granting additional permissions.

Config setters follow the existing SubCommand defaults: `setMaximumMessageBytes`, `setMaximumOutputBytes`, `setDeadline(const utils::Timeval&)` and `setAllowedOrigins` return `Config*`. Their matching getters read effective values; `getAllowedOrigins` borrows the current list for one event-loop turn. `--deadline-ms` remains milliseconds with default 30000 and range 1–300000. The timeout-type precedent is `ConfigConnection::getReadTimeout`/`setReadTimeout` and its write-timeout pair (`src/net/config/ConfigConnection.h:72`, `:75`; conversions in `.cpp:139–156`). Derive an application-local Config and retain CLI11 option handles, as DevelopmentConfig does; no development-only options are installed in MCP Config.

HTTP `Router::Authorize` returns `Authorized{principal}`, `Unauthorized{wwwAuthenticate}` or `Forbidden{}`. An empty principal permits anonymous access; identity and claims reach `RequestContext::getPrincipal`. Unauthorized emits 401 with a validated challenge (and resource metadata/scopes when configured), Forbidden emits 403 without one, `InsufficientScope` emits 403 with a Bearer insufficient_scope challenge, and callback failures emit 500 without logging credentials. Origin validation runs independently before authorization.

## Agent skill and tests

The companion skill is part of the `apps` component, sourced from `src/apps/mcp/skill/snodec-development`. Installation places `SKILL.md` and its two references under `<datadir>/snode.c/skills/snodec-development`, where `<datadir>` is the absolute installed data directory (`<prefix>/share` by default, governed by `CMAKE_INSTALL_DATADIR`). To enable the installed skill for your user, substitute that absolute directory below; these commands leave existing links or directories intact if the name is already occupied.

```sh
mkdir -p ~/.agents/skills ~/.claude/skills
ln -s "<datadir>/snode.c/skills/snodec-development" ~/.agents/skills/snodec-development
ln -s "<datadir>/snode.c/skills/snodec-development" ~/.claude/skills/snodec-development
```

[Codex loads personal skills](https://learn.chatgpt.com/docs/build-skills) from `~/.agents/skills`; [Claude Code loads personal skills](https://code.claude.com/docs/en/skills) from `~/.claude/skills` and supports symlinked skill directories. The installation does not modify either user directory.

CTest uses the existing repository mechanism: `McpServerTest` covers dispatch and schema enforcement; `McpStdioTest` covers framing, cancellation, EOF, pressure, logging, quiet mode, and CLI stdout purity; `McpHttpTest` and `McpTlsTest` use curl plus wire-level checks for headers, SSE framing/chunk termination, authentication, cancellation, and era isolation; `McpDevelopmentTest` covers confinement, slicing, cancellation, budgets, previews compiled against the staged installation, and both applications. The independent four-mode `InetHttpServerSseModeTest` verifies the framework's response-based SSE lifetime.

```sh
cmake --build "$BUILD_DIR"
mkdir -p /tmp/snc
TMPDIR=/tmp/snc ctest --test-dir "$BUILD_DIR" -L mcp --output-on-failure
```

Use a short external temporary directory for all CTest runs because database fixtures reject long Unix-socket paths. The installed-consumer test must run before preview compilation in a fresh build. Official Inspector/SDK and real-client qualification are testing-only operations; neither npm packages nor MCP clients are build dependencies. Exact tool versions, commands, transcripts, and any skipped checks belong in the local qualification report rather than a claim that all future client versions interoperate.


## Additional server features

`ResourceTemplate` is the primary class-based form for parameterized resources, with `Server::resourceTemplate(definition, handler)` as the short form. Scalar RFC 6570 `{variable}` and `{+variable}` expressions are supported; other operators, modifiers and adjacent ambiguous expressions fail registration with a named `RegistrationError`. Exact resources win before templates, which match in registration order. Percent escapes are decoded once; `ResourceRead::variables` owns the resulting values. Application confinement still applies after decoding. `resources/templates/list` works in both eras.

`PromptArgument::complete` and `ResourceTemplateDefinition::variables` carry `CompletionHandler` callbacks. They receive `CompletionRequest` (including previously resolved arguments) and `CompletionReply`. Complete with `CompletionResult`; the library caps wire values at 100, preserves optional totals and marks omitted extra values through `hasMore`. Only registries with completers advertise `completions`. The server limits completion requests to 128 per second and reports excess work with an implementation-specific error (`-32000`, HTTP 429). Keep completers bounded and protect suggestions using `request.context.getPrincipal()`.

Request input from a tool/resource/prompt with `reply->inputRequired(InputRequired{...})`. The requests map contains `FormInput` or `UrlInput`. Return after requesting input; do not also complete that reply. The handler can run more than once, so defer irreversible side effects until it has the required input.

For 2026-07-28, the module returns `input_required` after checking per-request client capabilities. The client retries independently; `RequestContext::getInputResponses()` and `getRequestState()` expose its untrusted inputs and state. Protect state integrity when it affects access or business logic, binding it to the principal, operation and a short expiry. See [MRTR](https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/mrtr).

For 2025-11-25, the same API sends nested `elicitation/create` requests using capabilities retained from initialization. Once all replies arrive, the same handler runs again with the input responses and server-held state; its original arguments, deadline, principal and cancellation remain in force. Empty `elicitation: {}` means form-only support. Clients without the requested mode receive an ordinary tool error (`isError: true`), or InvalidParams for resources/prompts. Legacy state-only continuations are invalid: provide at least one input request. See [2025 elicitation](https://modelcontextprotocol.io/specification/2025-11-25/client/elicitation).

Each legacy channel permits 1,024 outstanding inputs, at most 16 per request. Completion, cancellation, deadline, disconnect and shutdown release their correlations; late replies never resume cancelled work. Legacy HTTP uses the originating POST's SSE stream. Clients POST replies with the initialized session and credentials and receive empty `202` acknowledgements. Unknown, duplicate, malformed or cross-session replies receive `400`; stdio ignores them. Elicitation does not need a subscription GET stream. See [2025 transport](https://modelcontextprotocol.io/specification/2025-11-25/basic/transports#sending-messages-to-the-server).

Form mode is for non-sensitive data only. The 2025 channel validates accepted content against its pending form before resuming. In a stateless 2026 retry, use `RequestContext::getInputResponse(key, requestedSchema)` with the handler's server-owned schema: it returns a borrowed pointer (null if absent), validates accepted content, and throws `std::invalid_argument` on failure. The dispatcher reports invalid retry content as InvalidParams. The raw `getInputResponses()` remains untrusted; application-specific business checks still belong in the handler. URL applications must verify the initiating and completing user are the same; consent to navigation is not proof of external completion. Legacy URL requests receive a generated `elicitationId`. See [2025 elicitation security](https://modelcontextprotocol.io/specification/2025-11-25/client/elicitation#security-considerations) and [2026 elicitation](https://modelcontextprotocol.io/specification/2026-07-28/client/elicitation).

The demo adds `snodec://development/style/{section}` with completion for `overview`, plus the `review.label` elicitation tool and `snodec://development/review-label` resource. Choosing a new label changes only in-memory demonstration data and emits an update for that URI; it never writes source files or configuration. The label resource is the update source; there is no background polling timer.

Definitions expose optional icons and metadata; resources/templates expose usage annotations. `Implementation` supplies optional title, description, website and icons. Content and results carry optional metadata; resource results can supply `CacheHints`. Modern cache scope defaults to private with zero freshness; public caching requires an explicit choice. The 2025 wire omits modern result/cache fields and keeps structured content object-only.

The required draft-07 and 2020-12 validation suites pass with PCRE2; optional tests and format-assertion limitations are recorded in the [schema validation design record](SCHEMA-VALIDATOR.md). Sampling, roots, protocol logging, tasks, dynamic registration and an MCP client role remain deliberately outside this module's scope.

For protected HTTP deployments, configure the canonical resource URI and HTTPS authorization-server issuers explicitly, mount metadata at the application root, then mount the MCP router at its endpoint. Validate token signature, expiry, audience/resource (RFC 8707) and operation scopes in `Authorize`; discovery metadata never establishes token validity. `Unauthorized::scope` can guide authorization, and `InsufficientScope{scope}` distinguishes a valid token lacking permissions from unconditional `Forbidden{}`. Metadata routes are public; do not mount them behind endpoint authentication.

Role-neutral owned types, content/results, protocol eras and schema validation are provided by `snodec::mcp` under `mcp/`. The former `mcp::server` type names remain aliases; server registries, request contexts and replies remain in `snodec::mcp-server`. Both components share one schema-validation implementation.

Runtime registration and `Server::remove(RegistrationKind, identifier)` run exclusively on the SNode.C event-loop thread, including after attachment. Worker threads must marshal work through the application's existing event-loop mechanism. Removal affects subsequent lookup; replies already in progress retain their handler. Both eras advertise `listChanged: true` for tools, resources and prompts, including initially empty lists. Bursts within one event-loop turn coalesce by category; resource templates share the resources category. Legacy notifications use the initialized session channel; modern subscriptions must opt in and receive their subscription id. See [2025 tools/list-change](https://modelcontextprotocol.io/specification/2025-11-25/server/tools#list-changed-notification), [2026 subscriptions](https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/subscriptions) and [cache invalidation](https://modelcontextprotocol.io/specification/2026-07-28/server/utilities/caching#interaction-with-notifications). List pages retain private, immediately stale cache hints; a notification invalidates any client cache even before a positive TTL expires.

Pagination uses `Config::setPageSize(1..128)` or `mcp --page-size N`; the default is 128. All four list methods use lexical identifier ordering in both eras. Cursors are opaque to clients and validated against method, registry revision, page size and a valid page boundary. Any successful registry mutation invalidates existing cursors with `-32602`; restart without a cursor to obtain the current list. Modern pages use consistent `cacheScope: "private"` and `ttlMs: 0`. See [2025 pagination](https://modelcontextprotocol.io/specification/2025-11-25/server/utilities/pagination), [2026 pagination](https://modelcontextprotocol.io/specification/2026-07-28/server/utilities/pagination) and [cache interaction with pagination](https://modelcontextprotocol.io/specification/2026-07-28/server/utilities/caching#interaction-with-pagination).

Legacy logging follows [2025 logging](https://modelcontextprotocol.io/specification/2025-11-25/server/utilities/logging): initialized sessions advertise `logging`, `logging/setLevel` selects the minimum RFC 5424 severity, and `Server::log` / `RequestContext::log` emit structured `notifications/message` values. The default minimum is `info`; limits are 128 accepted messages per second per session, 16 KiB JSON data and 128 logger-name bytes. Request-context logging returns whether a message was admitted. HTTP clients open the session GET notification channel to receive these messages; stdio uses its duplex channel. Applications must remove credentials and private information from payloads. [2026 logging is deprecated](https://modelcontextprotocol.io/specification/2026-07-28/server/utilities/logging): modern discovery omits it, modern logging/setLevel returns Method not found, and modern request contexts emit nothing.

Tasks are opt-in per tool through `ToolDefinition::taskSupport` (`Forbidden`, `Optional`, `Required`). [2025 tasks](https://modelcontextprotocol.io/specification/2025-11-25/basic/utilities/tasks) require `params.task` and use nested handles, `ttl` / `pollInterval`, `tasks/result`, `tasks/list` and optional `notifications/tasks/status`. The [2026 tasks extension](https://github.com/modelcontextprotocol/ext-tasks/blob/main/specification/2026-07-28/tasks.md) requires `extensions["io.modelcontextprotocol/tasks"]` on every request; handles are flat with `resultType: "task"`, `ttlMs` / `pollIntervalMs`, and final payloads come from `tasks/get`. Modern handlers call `ToolReply::startTask(TaskOptions)` to choose task execution; required-task tools start automatically. Keep using the same asynchronous reply to complete, fail, request input or observe cancellation. `RequestContext::getTaskId()` identifies the task. Modern input uses `tasks/update`; legacy input uses the initialized elicitation channel. Legacy results remain retrievable until expiry; modern task-status subscriptions opt in with `taskIds` and receive subscription metadata.

The 2025 task specification requires related-task `_meta` on task-associated responses while also requiring `tasks/result` to return the exact underlying JSON-RPC error. Its JSON-RPC error schema has no `_meta` field. For this conflict, failed task retrieval preserves the original code, message and optional data without inventing error fields; successful task results carry related-task metadata. See [task metadata and result retrieval](https://modelcontextprotocol.io/specification/2025-11-25/basic/utilities/tasks#related-task-metadata).

The process-local task store accepts at most 128 tasks, retains each terminal payload up to 1 MiB, defaults to a 60-second lifetime, and permits lifetimes from 1 ms to 24 hours. Tasks survive transport disconnection within the lifetime of the server process; restart clears this in-memory store. Task ids use 256 bits from the OS random source. Legacy lookup requires the originating session and principal; modern lookup requires the originating principal, with unguessable ids acting as bearer handles for anonymous callers. Every HTTP operation still passes through the application's authorizer. Expiry removes the handle and notifies cancellation observers; explicit cancellation retains the terminal state until expiry. Server restart and multi-process durability require an application task backend and are not claimed by this in-memory implementation.

[2025 Streamable HTTP resumability](https://modelcontextprotocol.io/specification/2025-11-25/basic/transports#resumability-and-redelivery) assigns globally unique SSE ids within each retained logical stream. Reconnect with HTTP GET, the same `Mcp-Session-Id`, authorization and `Last-Event-ID`; only events after that exact id on that stream are replayed. A concurrent physical reader receives 409; missing, expired, evicted or cross-session ids receive 410 and require restarting the operation. `RouterOptions::replay` configures maximum events, bytes, retained streams and retention milliseconds; defaults are 128 events, 1 MiB, 64 streams and 60 seconds. `maximumEvents=0` disables replay. Both POST SSE responses and the legacy GET notification stream can resume. Disconnect detaches the physical connection while retained execution continues; expiry of a detached stream releases it. [2026 Receiving Messages](https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http#receiving-messages) explicitly excludes resumability, so modern responses do not acquire replay ids and retain disconnect-as-cancellation behavior.

The concrete `mcp::client::Mcp` role in `snodec::mcp-client` derives from `jsonrpc::JsonRpc`. Override safe, non-pure hooks such as `onReady`, `onTools`, `onToolResult`, `onResourceResult`, `onPromptResult`, `onRemoteError` and `onTimeout`; command methods return JSON-RPC request ids. There are no per-call continuation callbacks: semantic operation descriptors accompany the engine's single outstanding-request ledger, following the MQTT send/hook style. `Options::negotiation` selects automatic discovery/fallback or an explicit era. Automatic mode uses [2026 discovery](https://modelcontextprotocol.io/specification/2026-07-28/server/discover), falling back to [2025 initialization](https://modelcontextprotocol.io/specification/2025-11-25/basic/lifecycle) when discovery is unsupported. Sampling and roots are not advertised.

The client follows opaque `nextCursor` values, including empty strings, and aggregates list pages under the original request id; an invalidated cursor restarts once from the beginning. It retries [MRTR](https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/mrtr) with a fresh wire id, exact opaque requestState and only that operation's inputResponses. Elicitation declines by default. Typed shared result values own their content; typed list results retain descriptors as JSON to preserve schemas and extension fields. Task watches poll using the server's interval, route status/input notifications, and return final results through the original operation's hook. `RequestOptions::task` is the legacy task-augmentation flag; modern task creation remains the server's decision. Pagination, MRTR rounds, task watches and input-key histories are bounded. No result cache is maintained, so each explicit list/read fetch observes the server and list-change hooks have no hidden cache to invalidate.

### Client transports and CLI

`mcp/client/Stdio.h` belongs to `snodec::mcp-client`. Construct the named `Stdio` facade before `SNodeC::init`, then call `start(argv, onExit)` and enter `SNodeC::start`. Its creator returns a connection-owned `mcp::client::Mcp` subclass. The binding uses `posix_spawnp` without a shell, CLOEXEC pipes, the existing `net::stdio` endpoint with TAKE ownership, and newline JSON-RPC framing. stderr is inherited. Child signal masks/defaults are reset before execution. On Linux, a pidfd read receiver and `waitpid` integrate exit handling without replacing application SIGCHLD handlers. `stop` closes the child stdin first, waits two seconds for clean exit, then sends SIGTERM and escalates to SIGKILL after a further two seconds; teardown reaps the child. `onExit` receives the raw wait status after the child is reaped and the old pipe connection has detached, or negative errno when spawning fails. Destroying the facade suppresses late exit callbacks and cancels queued starts. Start/stop, creators, hooks and final release belong to the event-loop thread.

`mcp/client/http/Client.h` belongs to `snodec::mcp-client-http`. `http::Http` and `http::Https` are aliases of a template over the existing IPv4 HTTP/TLS clients, with explicit instantiations. Construct it before init with a newly allocated protocol; the binding owns it. `getConfig()` exposes the original transport configuration, including TLS settings. `start(SocketAddress)` begins negotiation; `Options::path` (or `setPath` before start) selects the MCP endpoint. `Options::bearerToken` supplies an access token per exchange; `Options::onError` reports transport failures. OAuth flows, consent, token storage and refresh remain application responsibilities. Credentials are not written to protocol logs.

The HTTP binding implements the [2025 transport](https://modelcontextprotocol.io/specification/2025-11-25/basic/transports) with session headers and its session GET notification stream, and the [2026 transport](https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http) with per-request protocol, method and name headers. Fetch `listTools` before calling tools with `x-mcp-header` annotations: the transport validates the descriptors, excludes invalid declarations, retains bounded header projections, extracts nested primitive arguments and applies the specified base64 sentinel encoding. List-change notifications invalidate these projections. A HeaderMismatch error is delivered to the protocol hook; applications can refresh the tool inventory and retry. The registry also rejects invalid header annotations at registration, and the HTTP router compares recognized custom headers against the current registry schema.

SSE parsing is shared with the HTTP EventSource implementation. HTTP transfer decoding remains in `web::http`, including chunked responses; UTF-8 BOM, CR/LF/CRLF delimiters, event ids, retry values and partial messages are handled by the shared bounded parser. The binding admits at most 64 simultaneous/queued exchanges and 8 MiB JSON response bodies; SSE lines are bounded to 256 KiB and event data to 1 MiB. Legacy interrupted SSE streams retry the same stream via GET and Last-Event-ID, up to eight times, without resending POST. Expired/evicted streams report failure. Modern disconnects cancel the exchange; applications may issue a fresh listen request. Modern streams never use Last-Event-ID.

The `snodec-mcp-client` demonstration lists tools by default and also supports `--command resources`, `templates`, `prompts`, `call`, `read`, `prompt`, `listen`, `log` or an extension method. Use `--name` for a tool/prompt name or resource URI and `--arguments` for a JSON object. `--legacy true` pins 2025; automatic discovery/fallback is the default. `--task true` requests legacy task augmentation. `--watch true` keeps receiving notifications. Typed result hooks print JSON; notifications, task status, logs and progress are displayed. `MCP_BEARER_TOKEN` supplies the demonstration's HTTP access token.

```sh
snodec-mcp-client --config-file /dev/null --quiet --server snodec-dev-stdio --command tools
snodec-mcp-client --config-file /dev/null --quiet --host 127.0.0.1 --port 8080 --command tools
snodec-mcp-client --config-file /dev/null --quiet --host 127.0.0.1 --port 8080 --command call --name api.find --arguments '{"query":"SNodeC","limit":1}'
snodec-mcp-client --config-file /dev/null --quiet --command listen --arguments '{"toolsListChanged":true,"resourceSubscriptions":["snodec://development/review-label"]}' --watch true
```

For embedding, derive once and select the binding independently:

```cpp
#include <core/SNodeC.h>
#include <mcp/client/Mcp.h>
#include <mcp/client/Stdio.h>

class Inventory final : public mcp::client::Mcp {
    void onReady(mcp::ProtocolEra, const mcp::Json&) final {
        listTools();
    }
    void onTools(const jsonrpc::messages::Id&, const mcp::ToolsListResult&) final {
        disconnect();
    }
};

int main(int argc, char* argv[]) {
    mcp::client::Stdio server("inventory", [] { return new Inventory(); });
    core::SNodeC::init(argc, argv);
    server.start({"snodec-dev-stdio", "--config-file", "/dev/null"});
    return core::SNodeC::start();
}
```

Link `snodec::mcp-client` for this example. Applications own their UI, elicitation decisions, retry policy after transport failures and authorization flow. There are no sampling or roots APIs and no modern logging capability. Server/client code is experimental; the unreleased MCP SOVERSION remains 4.

See the [qualification record](QUALIFICATION.md) for the SDK/Inspector/native feature matrices, supported eras and reproducible build discipline.

`StdioOptions::shutdownGrace` and `terminateGrace` configure the EOF-to-SIGTERM and SIGTERM-to-SIGKILL grace periods (both default to two seconds). The stdio client keeps reading stdout until child exit; the demo reports the decoded exit code or signal description.
