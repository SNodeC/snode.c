# SNode.C MCP server

The in-tree MCP module provides one registry and dispatcher with stdio and Streamable HTTP adapters. Its first application is a read-only SNode.C development assistant: `api.find`, `api.read`, `skeleton.preview`, the `snodec://development/house-style` resource, and the `snodec-review` prompt. It does not execute commands, inspect app configurations, build examples, or write files.

## Stability

The MCP libraries (`mcp-server`, `mcp-server-stream`, and `mcp-server-http`), their installed headers under `mcp/server`, and the `snodec-dev` applications are EXPERIMENTAL. Their API and ABI may change in a minor SNode.C release. The MCP libraries have their own SOVERSION, incremented on every MCP ABI break independently of the SNode.C core; their VERSION continues to follow the SNode.C release version. MCP changes alone never require a major SNode.C release. The compatibility policy for all other SNode.C libraries is unchanged.

## Build and consume

Configure the canonical checkout with an external build directory. Set `BUILD_DIR` and `HEADER_ROOT` for your environment; `HEADER_ROOT` must name the actual installed SNode.C header directory, whose children include `core/` and `express/`.

```sh
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug \
  -DSNODEC_BUILD_APPS=ON -DSNODEC_BUILD_TESTS=ON
cmake --build "$BUILD_DIR" --target snodec-dev-stdio snodec-dev-http
```

The module follows the MQTT layout under `src/mcp/server`; the applications live in `src/apps/mcp`. The transport-independent newline protocol is `mcp-server-stream`; attach its original stream factory to a stdio endpoint or another stream connection. Installed components and exported targets are `mcp-server`, `mcp-server-stream`, and `mcp-server-http`, with `snodec::` target names. The existing package installation procedure installs these components; no additional JSON or MCP SDK dependency is introduced. Consumers can use:

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

The compatibility era is [2025-11-25](https://modelcontextprotocol.io/specification/2025-11-25/basic/lifecycle). Stdio retains initialization context per transport and honors `notifications/cancelled`. HTTP accepts the legacy initialization exchange but remains stateless: it never creates or honors `Mcp-Session-Id`, accepts calls without a session record, sends JSON responses only, and returns 405 to GET/DELETE. Legacy HTTP cancellation receives 202 and is ignored; its bounded short requests use the cancellation clause permitting the receiver to ignore a notification when [the request cannot be cancelled](https://modelcontextprotocol.io/specification/2025-11-25/basic/utilities/cancellation). Modern metadata or headers continue to require modern validation after any legacy traffic.

Modern HTTP uses source-backed SSE for HTTP/1.1 `tools/call` and `subscriptions/listen`, and JSON for the other methods. Every SSE event contains a complete JSON message and ends with a blank line. The source completes through EOF, producing the final chunk terminator; streamed responses never use `json()` or `send()`. Closing an HTTP stream cancels its pending modern work. Stdio cancellation is request-ID based. During shutdown the server sends subscription cancellation before ending the stream while the transport can still write.

`subscriptions/listen` acknowledges an empty honored filter set, including when the requested filters are nonempty. No watchers are installed. **Known qualification:** the [subscription](https://modelcontextprotocol.io/specification/2026-07-28/basic/utilities/subscriptions) and [cancellation](https://modelcontextprotocol.io/specification/2026-07-28/basic/utilities/cancellation) wording combines cancellation requirements with a graceful final-result recommendation. This implementation follows the cancellation requirement: it emits a cancellation notification and closes through Source EOF on server shutdown, without a final JSON-RPC result; a disconnected stream cannot receive the notification. This qualification is retained pending clarification of the specification.

## Supported schema dialect

The validator implements a deliberately restricted JSON Schema 2020-12 dialect. Tool input schemas must be object schemas with `type: "object"`. `$schema`, when present, must identify `https://json-schema.org/draft/2020-12/schema` (an empty `#` fragment is accepted). Nested boolean schemas are supported. Supported keywords are `type`, `properties`, `required`, boolean `additionalProperties`, `items`, `enum`, `const`, `minimum`, `maximum`, `minLength`, `maxLength`, `minItems`, and `maxItems`; `title`, `description`, and `default` are annotations. Defaults are not inserted into arguments. String lengths count Unicode code points.

Every registered input/output schema is checked at startup, including its keyword shapes. Any other keyword, including `$ref`, `$defs`, combinators, conditionals, formats, patterns, and schema-valued `additionalProperties`, is rejected. Registration checks also bound schema size (16 KiB), depth (16), and nodes (256); argument validation bounds nesting to 64. General 2020-12 schemas are not accepted. Each advertised supported assertion is enforced before its handler; declared output schemas are checked against `structuredContent` before a result is emitted.

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

The router accepts an optional `Authorize` callback taking the express request and returning `Authorization`: `Authorized`, `Unauthorized` with a challenge, or `Forbidden`. An embedding application can validate bearer tokens there; a future SNode.C identity-provider integration can call its token-validation interface through this hook. No identity-provider integration or built-in token store is supplied. Use TLS and an appropriate authorization callback before exposing HTTP beyond loopback. The shipped HTTP development application does not expose a bearer-token configuration option.

The shipped tools never start child processes. A future command-running tool would require a separately approved command allowlist and confined working-directory policy; this release provides neither command execution nor a generic shell escape. Header text and review input are untrusted data, not instructions granting additional permissions.

Config setters follow the existing SubCommand defaults: `setMaximumMessageBytes`, `setMaximumOutputBytes`, `setDeadline(const utils::Timeval&)` and `setAllowedOrigins` return `Config*`. Their matching getters read effective values; `getAllowedOrigins` borrows the current list for one event-loop turn. `--deadline-ms` remains milliseconds with default 30000 and range 1–300000. The timeout-type precedent is `ConfigConnection::getReadTimeout`/`setReadTimeout` and its write-timeout pair (`src/net/config/ConfigConnection.h:72`, `:75`; conversions in `.cpp:139–156`). Derive an application-local Config and retain CLI11 option handles, as DevelopmentConfig does; no development-only options are installed in MCP Config.

HTTP `Router::Authorize` returns `Authorized{principal}`, `Unauthorized{wwwAuthenticate}` or `Forbidden{}`. An empty principal permits anonymous access; identity and claims reach `RequestContext::getPrincipal`. Unauthorized emits 401 with a validated challenge, Forbidden emits 403 without one, and callback failures emit 500 without logging credentials. Origin validation runs independently before authorization.

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
