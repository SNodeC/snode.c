# Using the development MCP server

Use the installed executables `snodec-dev-stdio` and `snodec-dev-http`. Select the installed header tree with `snodec-dev-stdio mcp --header-root DIRECTORY` or `snodec-dev-http mcp-http mcp --header-root DIRECTORY` when it differs from the configured installation prefix. Read the relevant section's `--help` for options and limits; transport setup and protocol qualifications are documented in `docs/mcp/README.md` when a source checkout is available.

## API evidence

1. Call `api.find` with `{"query":"SNodeC","limit":5}`. Search is literal and case-sensitive. Results contain paths relative to the confined header root, one-based lines, and byte offsets. A `truncated` result means the result or traversal budget stopped the search; absence is not proof that an API does not exist elsewhere.
2. Call `api.read` with a returned relative path, for example `{"path":"core/SNodeC.h","offset":0,"bytes":8192}`. Reads are bounded UTF-8 byte ranges; a range splitting an encoding is rejected. Absolute paths, `..`, symlinks, non-header files, and special files are rejected.
3. Compare the declaration with the current checkout's implementation and callers before using it. The MCP tools inspect headers; they do not establish runtime behavior or verify that an installed tree matches the checkout.

The server performs bounded slices between event-loop iterations. Cancel work that is no longer needed. Transport deadlines and configured byte/file limits also bound searches. Tool input-value validation and tool execution failures are returned with `isError`; JSON-RPC errors identify malformed calls or protocol failures. Inspect both layers.

## Previews and review

Call `skeleton.preview` with `{"kind":"application"}` or `{"kind":"module"}`. The result's `structuredContent.files` maps relative paths to proposed text. Review instance names, dependencies, central export registration, ownership, and lifecycle before applying a preview within an authorized editing task. The tool writes no files.

Read the resource `snodec://development/house-style` for the concise runtime conventions. Get prompt `snodec-review` with a string `code` argument of at most 16384 bytes to obtain a review instruction containing that code as data. The prompt does not execute or review code by itself.

The first application exposes exactly these three tools, one resource, and one prompt. It has no build/run, configuration-inspection, command-execution, or file-writing tool. Do not infer those capabilities from the broader MCP framework.

## Embedding the MCP module

These techniques belong to the MCP implementation; they are not general application conventions.

- Use the class-based form first: derive `mcp::server::Tool`, `Resource` or `Prompt`; override `call`, `read` or `get`, taking the matching request and shared reply. Construct the typed definition once, use `getDefinition` for immutable metadata and register the shared object through `Server::tool`, `resource` or `prompt`. The server retains it for its lifetime. Follow the separate `.h`/`.cpp` files `FindTool`, `ReadTool`, `SkeletonPreviewTool`, `HouseStyleResource` and `ReviewPrompt` under `src/apps/mcp/`; `main()` registers the five objects directly. Use `ToolResult::json`, `ResourceResult` and `PromptResult` for completion. The existing definition/callback overloads are short convenience adapters on this same class path.
- For stdio, call `mcp::server::stdio::Transport::run` once before core initialization. It owns protocol descriptor retention, foreground initialization, the registration callback, event-loop execution and cleanup; ordinary stdout is redirected to stderr. Follow `src/mcp/server/stdio/Transport.cpp` for event-receiver framing, pressure handling, and EOF/shutdown. Its private receiver destructors are defined out of line.
- For sliced tool work, follow `src/apps/mcp/Tools.cpp`: retain lookup state across `core::timer::Timer::singleshotTimer` callbacks, perform bounded reads/directory steps, and register `RequestContext::onCancel` to cancel the next slice and close descriptors. The module owns deadlines; applications do not poll reply state. Descriptor cleanup belongs to that lookup state.
- HTTP authorization returns `Authorized`, `Unauthorized` with a WWW-Authenticate challenge, or `Forbidden`; handlers read `RequestContext::getPrincipal`. Config offers chainable setters and matching getters; `setDeadline`/`getDeadline` use `utils::Timeval` while CLI `--deadline-ms` remains milliseconds.
- For HTTP, mount `mcp::server::http::Router` before listening. `src/mcp/server/http/Router.cpp` owns the Origin/authentication/Content-Type checks, invokes `JsonMiddleware`, checks JSON-attribute presence, then validates headers and dispatches. Reuse that chain rather than duplicating it in the application.
- MCP SSE uses `Response::pipe()` with `core::pipe::Source`, bounded output queues and blank-line event framing. Complete through Source EOF; never call `json()` or `send()` on an open streamed response. Follow the router's source for backpressure, disconnect cancellation, and shutdown cancellation while output can still drain.
- Copy the application's explicit failure cleanup from `src/apps/mcp/main.cpp`: failed listeners call `core::SNodeC::stop`, and initialization exceptions call `core::SNodeC::free`. Keep this MCP embedding guidance separate from the established examples in `structure.md`.

### Class and short-form examples

Own the definition in the base and parse fully expanded schema text exactly once during construction. Preserve `Json::dump(4)` layout in every schema raw string, including nested arrays. Convert parsing failures to a named `RegistrationError`; for output schemas use field `outputSchema`. The demo's `parseSchema` helper performs that conversion for its constructors. Keep callbacks and final release on the event-loop thread.

```cpp
#include <mcp/server/Server.h>
#include <mcp/server/Tool.h>
#include <memory>

class EchoTool final : public mcp::server::Tool {
public:
    EchoTool();
    void call(const mcp::server::ToolCall& call, const std::shared_ptr<mcp::server::ToolReply>& reply) override;
};
EchoTool::EchoTool() try
    : Tool({"echo", "Echo the supplied text object", mcp::server::Json::parse(R"json(
{
    "additionalProperties": false,
    "properties": {
        "text": {
            "type": "string"
        }
    },
    "required": [
        "text"
    ],
    "type": "object"
}
)json")}) {
} catch (const mcp::server::Json::parse_error& error) {
    throw mcp::server::RegistrationError(mcp::server::RegistrationKind::Tool, "echo", "inputSchema", error.what());
}
void EchoTool::call(const mcp::server::ToolCall& call, const std::shared_ptr<mcp::server::ToolReply>& reply) {
    reply->result(mcp::server::ToolResult::json(call.arguments));
}
```

```cpp
mcp::server::Server server("example", "1", "Example tools");
server.tool(std::make_shared<EchoTool>());
```

For an existing `ToolDefinition`, the lambda overload is the short form. It wraps the callback in a private `Tool` subclass and uses the same validation, retention and dispatch path. For example, use this instead of the class registration above:

```cpp
const auto definition = EchoTool().getDefinition();
server.tool(definition, [](const mcp::server::ToolCall& call, const std::shared_ptr<mcp::server::ToolReply>& reply) {
    reply->result(mcp::server::ToolResult::json(call.arguments));
});
```
