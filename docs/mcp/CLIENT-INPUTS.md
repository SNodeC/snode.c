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

In 2025-11-25, requests use the initialized session capabilities. Nested `sampling/createMessage` and `roots/list` RPCs travel over stdio or the originating HTTP/HTTPS POST SSE response. Correlation, cancellation, request deadline, principal and handler resumption share the existing input mechanism. Reply POSTs are session/credential bound; ordinary reverse input does not require an independent GET notification stream. Legacy server-owned tool tasks use the session GET SSE channel for their asynchronous input and notifications; the HTTP client maintains that channel. Clients advertising roots `listChanged` can call `notifyRootsChanged()`; servers can replace the current session's observer using `RequestContext::onRootsChanged(callback)`. Session retirement clears that observer. The observer should capture application state with an appropriate lifetime and invalidate cached roots; it must not assume roots confer access rights.

Client applications can opt in to hosting 2025 sampling and elicitation tasks with `setClientTaskSupport(sampling, elicitation)`, after configuring their sampling handler and before connection. The advertised `tasks.requests` identifies the supported operations, with list and cancellation controls. Client-hosted tasks are bounded to 16 retained tasks per connection, a default 60-second TTL capped at one day, 128 pending result waiters per task and a one-MiB retained result. Completed and cancelled entries occupy capacity until expiry; new work is refused when capacity is exhausted. The existing shared task store owns the ticket, state, result, TTL, waiting `tasks/result` requests and cancellation. Retain the ordinary `ClientReply` or `ElicitationReply` to answer asynchronously; `getTaskId()` identifies its client-owned task. The common task store adds the required related-task metadata to retained legacy payloads. `taskStatus(Working|InputRequired, message)` publishes application status, and `onCancel` stops application work. Completing the reply completes the task. Task expiry or disconnection invalidates retained replies. Task augmentation requires the exact advertised operation, not just its category; optional list/cancel methods require their own advertised controls. Unsupported task augmentation is still ignored and returns the ordinary result, as required by the [2025 specification](https://modelcontextprotocol.io/specification/2025-11-25/basic/utilities/tasks#task-support-and-handling).

Server applications use `Reply::clientRequestsRequired(requests, state)` for correlated legacy reverse requests: sampling/elicitation with `params.task`, `tasks/get`, `tasks/list`, `tasks/result`, `tasks/cancel` and `ping`. Each map entry contains `method` and object `params`. This reuses the existing input round, deadline, request ownership and handler resumption. Read the ticket or lifecycle result through `RequestContext::getClientResponse(key)`. A task ticket is not a sampled message: retrieve its final payload with `tasks/result`, then validate it as the original sampling/elicitation result before application use. The requestor explicitly controls task cancellation; cancelling an already answered augmentation request does not cancel its task. Use `getProtocolEra()` to distinguish legacy reverse operations from modern input continuations.

For legacy URL input, `getElicitationId(key)` returns the identifier generated for that input round; `notifyElicitationComplete(id)` emits the optional completion notification on the live session. The application owns successful out-of-band completion and its binding to the initiating identity. Client applications receive it through `setElicitationCompleteHandler`. The client correlates IDs received in URL input or URL-required error descriptions, consumes each completion before invoking the callback, ignores unknown/duplicate IDs and clears correlation on disconnect. Decline, cancellation and failure release pending URL correlation immediately; accepted interactions remain correlated after their input reply completes. Replies hold a weak reference to the connection-owned registry, so retiring a reply cannot affect a later connection. It retains at most 128 uncompleted identifiers of at most 256 bytes each; input beyond that capacity is rejected, and excess error descriptors are not retained. A client `ElicitationReply::getElicitationId()` exposes the opaque identifier for associating the URL interaction with that notification. `ErrorCode::UrlElicitationRequired` supports the legacy URL-required error with its `data.elicitations` descriptions. These operations do not exist in the modern era.

In 2026-07-28, the server emits keyed input requests in `input_required`, and the client retries the operation with collected `inputResponses` and opaque state. Capabilities are checked for each request, including sampling tool/context support. Sampling and roots inside tool tasks use the existing task input/update mechanism. Modern roots have no list-change notification; obtain roots through the operation's input continuation. `notifyRootsChanged()` and `onRootsChanged()` return inactive/false in that era. A client-side failure does not submit a partial retry; failing modern task input retires its watch and cancels the server task. Disconnect and cancellation invalidate outstanding client handles and prevent late continuations.

## Demonstration and qualification

The dedicated showcase adds `sampling` and `roots`; the development assistant registrations remain unchanged. `snodec-mcp-client client --command tour` configures an explicitly synthetic model and root, then checks sampling text/image/audio, all context and tool-choice modes, model options, a complete tool loop, roots and legacy client-owned task lifecycles through public APIs. The same operations have independent raw-wire assertions over stdio, HTTP and HTTPS in both supported eras. The tour's synthetic result is protocol evidence, not qualification of a model provider or native host. Native clients that do not expose sampling or roots remain unverified.

## Reverse progress

For legacy client work, call `RequestContext::trackClientProgress(key, observer)` before requesting the keyed input. The input mechanism assigns its existing unique correlation ID as the progress token. The parent context owns the observer and last accepted progress, accessible with `getClientProgress(key)`. Sampling/roots `ClientReply::progress` and `ElicitationReply::progress` emit finite, increasing values only when that token was requested; they preserve related-task metadata for client-hosted tasks. The shared numeric validation also governs ordinary server progress. Malformed, duplicate, decreasing and uncorrelated updates are ignored. Parent cancellation/deadline prevents further observation, and callbacks may retire the request safely. Modern input continuations do not create reverse JSON-RPC streams, so these progress senders are inactive there.

Task-producing tool handlers can set working/input-required status messages with `ToolReply::taskStatus`. Client applications may install `setTaskObserver` to receive complete validated task descriptions, including `statusMessage` and extension metadata, before the existing typed task hooks. The callback borrows the description for that invocation; the library adds no description cache and keeps the public `TaskInfo` layout unchanged. Use the typed task hooks for lifecycle control.

`Mcp::getServerDescription()` borrows the complete successful initialization/discovery result after readiness, including server identity, instructions, capabilities and metadata. `getServerCapabilities()` remains a view of that same result; there is one negotiated-description owner.
