# Sampling and roots

SNode.C exposes sampling and roots through its shared MCP client/server libraries for application-owned implementations. Both features are optional. They remain specified but are deprecated in [2026-07-28 sampling](https://modelcontextprotocol.io/specification/2026-07-28/client/sampling) and [roots](https://modelcontextprotocol.io/specification/2026-07-28/client/roots); new applications should consider the specification's direct-provider and explicit-path alternatives. Supporting these operations does not supply an LLM provider, permissions UI or filesystem sandbox.

## Server applications

Use `Reply<Result>::clientInputRequired(ClientInputRequired)` from a tool, resource or prompt handler. Its keyed map accepts `SamplingRequest`, `RootsRequest`, `FormInput` and `UrlInput` in the same round. `SamplingRequest(Json)` validates owned sampling parameters, including message content, model preferences and tool schemas. Arbitrary provider metadata and protocol metadata are retained. Existing `inputRequired(InputRequired)` remains available and uses the same mechanism.

```cpp
class SampleTool final : public mcp::server::Tool {
public:
    SampleTool()
        : Tool({"sample", "Ask the client's model", {{"type", "object"}}}) {
    }

    void call(const mcp::server::ToolCall& call,
              const std::shared_ptr<mcp::server::ToolReply>& reply) final {
        const mcp::SamplingRequest sampling({
            {"messages", {{{"role", "user"},
                           {"content", {{"type", "text"}, {"text", "Explain this operation."}}}}}},
            {"maxTokens", 128}
        });
        if (const auto result = call.context.getSamplingResponse("sample", sampling)) {
            reply->result(mcp::ToolResult::json(result->toJson()));
        } else {
            reply->clientInputRequired({{{"sample", sampling}}, "sample-round"});
        }
    }
};
```

Retrieve roots through `getRootsResponse(key)`. The returned `RootsResult` validates unique `file://` URIs and preserves names and metadata. Roots are informational: applications still authorize and validate every actual filesystem operation. Neither transport nor root disclosure grants file access. `getSamplingResponse(key, originalRequest)` validates the returned message and any tool use against the original tool definitions and choice. Treat all input and modern `requestState` as untrusted; verify state integrity before business or authorization decisions.

Sampling accepts text, image and audio messages, single blocks or arrays, tool-use/tool-result blocks, model preference hints/priorities, system prompt, maximum tokens, temperature, stop sequences, context selection and metadata. Tools and context require their respective client capabilities. When continuing a sampling tool loop, append each assistant tool-use message and a following user message containing exactly its matching tool results; then request another sampling round. Execute only application-authorized tools, validate arguments and bound iteration/work. The library validates message balance and does not execute tools automatically.

## Client applications

A `mcp::client::Mcp` subclass opts in before protocol negotiation with `setSamplingHandler(handler, tools, context)` and `setRootsHandler(handler, listChanged)`. Each setter advertises the corresponding capability only while a handler is configured. The existing `setClientCapabilities` remains authoritative for custom capability configuration; ensure any custom declaration agrees with installed handlers. Existing virtual hooks and defaults are unchanged. When handler setup depends on parsed application options, perform it before delegating to `Mcp::onConnected()`; HTTP bindings can construct their client engine before CLI parsing.

```cpp
setSamplingHandler(
    [](const std::string& key, const mcp::SamplingRequest& request,
       const std::shared_ptr<mcp::client::ClientReply>& reply) {
        // Inspect request.toJson(), obtain consent, and call an application-owned model.
        // Retain reply for asynchronous work on the event loop.
        // Complete with reply->result(mcp::SamplingResult(validatedModelResponse)),
        // or reply->error(mcp::ErrorCode::InvalidParams, "Sampling declined").
    },
    true, false);

setRootsHandler(
    [](const std::string&, const std::shared_ptr<mcp::client::ClientReply>& reply) {
        // Return only roots the application/user has approved for disclosure.
        reply->result(mcp::RootsResult({{"roots", {{{"uri", "file:///workspace"}}}}}));
    },
    true);
```

The client controls consent, model selection, provider credentials, prompt/context disclosure and response review. Respect `maxTokens`; preferences are advisory. The library forwards validated input without accessing a provider or inspecting roots. A reply is once-only; a wrong result kind, invalid tool response or duplicate completion returns `Completion::Inactive`. Retain the shared handle to answer asynchronously. `onCancel` lets application work stop when an input is invalidated. Last-handle destruction fails unanswered sampling/roots; `Options::elicitationTimeout` bounds all client inputs. Elicitation retains its existing decline-on-abandonment behavior.

## Protocol and lifetime

In 2025-11-25, requests use the initialized session capabilities. Nested `sampling/createMessage` and `roots/list` RPCs travel over stdio or the originating HTTP/HTTPS POST SSE response. Correlation, cancellation, request deadline, principal and handler resumption share the existing input mechanism. Reply POSTs are session/credential bound; independent GET notifications are not required. Clients advertising roots `listChanged` can call `notifyRootsChanged()`; servers can replace the current session's observer using `RequestContext::onRootsChanged(callback)`. Session retirement clears that observer. The observer should capture application state with an appropriate lifetime and invalidate cached roots; it must not assume roots confer access rights.

The client does not advertise the separate 2025 capability for task-augmented sampling RPCs. Following the [2025 task-handling requirement](https://modelcontextprotocol.io/specification/2025-11-25/basic/utilities/tasks#task-support-and-handling), unsupported task-augmentation metadata is ignored and sampling returns its ordinary result; `SamplingRequest` removes the `task` field rather than requesting a client-owned task.

In 2026-07-28, the server emits keyed input requests in `input_required`, and the client retries the operation with collected `inputResponses` and opaque state. Capabilities are checked for each request, including sampling tool/context support. Sampling and roots inside tool tasks use the existing task input/update mechanism. Modern roots have no list-change notification; obtain roots through the operation's input continuation. `notifyRootsChanged()` and `onRootsChanged()` return inactive/false in that era. A client-side failure does not submit a partial retry; failing modern task input retires its watch and cancels the server task. Disconnect and cancellation invalidate outstanding client handles and prevent late continuations.

## Demonstration and qualification

The dedicated showcase adds `sampling` and `roots`; the development assistant registrations remain unchanged. `snodec-mcp-client client --command tour` configures an explicitly synthetic model and root, then checks sampling text, tool use and roots through public APIs. The same operations have independent raw-wire assertions over stdio, HTTP and HTTPS in both supported eras. The tour's synthetic result is protocol evidence, not qualification of a model provider or native host. Native clients that do not expose sampling or roots remain unverified.
