# MCP showcase API inventory

Baseline: `69b16419a`. Source inventory; qualification is **UNVERIFIED** until the recorded tests execute. Sampling and roots are deliberately excluded. All showcase registrations belong to separate applications, not `snodec-dev-*`.

The invariant is one protocol implementation: the showcase consumes public MCP APIs and existing transport bindings; it must not implement a second MCP engine. Application state and timers live on the event loop and terminate with their request or server owner.

| Public feature | API entry points and source | Showcase item | Qualification |
|---|---|---|---|
| Shared types and negotiation | `identity, icons, annotations, principals, error codes, progress and task types` — `src/mcp/Types.h:178` | identity/content/work/tasks/input/auth | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Era selection and metadata keys | `both protocol revisions and metadata constants` — `src/mcp/Protocol.h:45` | negotiation | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Schema registration/value validation | `Schema::check, Schema::validate` — `src/mcp/Schema.h:50` | schema | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Result factories and serialization | `text/json/content/toolError, setMeta, toJson/fromJson; all content variants and result structures` — `src/mcp/Results.h:156` | content/errors/schema/resources/prompts | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Server lifecycle and identity | `constructors, ownership and lifetime` — `src/mcp/server/Server.h:84` | identity | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Registration and removal | `tool/resource/resourceTemplate/prompt overloads; remove; RegistrationError` — `src/mcp/server/Server.h:118` | dynamic | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Notifications and logging | `log, resourceUpdated` — `src/mcp/server/Server.h:106` | logging/updates | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Task policy | `setTasksPerRequestor, setMaximumTaskTtlMs` — `src/mcp/server/Server.h:109` | tasks/limits | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Typed handlers and definitions | `Tool/Resource/ResourceTemplate/Prompt; definitions and handler methods` — `src/mcp/server/Tool.h:63` | all registration groups | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Reply completion | `result, error, inputRequired, progress, startTask; Completion status` — `src/mcp/server/Reply.h:72` | errors/input/work/tasks | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Request context | `getPrincipal/getProgressToken/getInputResponses/getInputResponse/getRequestState/getTaskId/onCancel/log` — `src/mcp/server/Request.h:64` | auth/work/input/tasks/logging | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Completion inputs | `context/name/value/arguments; ResourceRead::variables; PromptGet::arguments` — `src/mcp/server/Request.h:96` | completion/templates/prompts | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Protocol and transport limits | `message/output/page/deadline/origin setters and getters` — `src/mcp/server/Config.h:60` | limits/pagination/origin | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Stream attachment | `factory/context on ordinary stream endpoint` — `src/mcp/server/stream/SocketContextFactory.h:59` | stdio | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Low-level JSON-RPC peer integration | `onConnected/onDisconnected/onSignal/receive/deliver/sendTaskMessage/wire/respond/forget/validRequest/errorResponse` — `src/mcp/server/Mcp.h:67` | wire/errors/lifecycle | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| HTTP authorization and metadata | `Authorized/Unauthorized/Forbidden/InsufficientScope, Authorize, ProtectedResource, mountMetadata` — `src/mcp/server/http/Router.h:68` | auth | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| HTTP sessions and replay | `LegacySubscriptions, ReplayOptions, RouterOptions` — `src/mcp/server/http/Router.h:101` | replay | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Client lifecycle and policy | `Options, RequestOptions, negotiation, onConnected/onDisconnected/onSignal/getProtocolEra/getServerCapabilities/disconnect` — `src/mcp/client/Mcp.h:85` | tour/lifecycle | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Client discovery and pagination | `listTools/listResources/listResourceTemplates/listPrompts and typed result hooks` — `src/mcp/client/Mcp.h:119` | catalogue | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Client operations | `callTool/readResource/getPrompt/complete/request; result/error/timeout hooks` — `src/mcp/client/Mcp.h:123` | tour | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Client subscriptions | `subscribe/unsubscribe/listen; listChanged/resourceUpdated/subscriptionAcknowledged hooks` — `src/mcp/client/Mcp.h:127` | updates/dynamic | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Client logging and progress | `setLoggingLevel, onLogMessage, onProgress, cancel/onCancelled` — `src/mcp/client/Mcp.h:130` | logging/work | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Client tasks | `getTask/getTaskResult/listTasks/cancelTask/updateTask, onTaskCreated/onTaskStatus/onTasks; automatic polling` — `src/mcp/client/Mcp.h:131` | tasks | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Client elicitation | `onElicitation, ElicitationReply::result, timeout/destruction handling` — `src/mcp/client/Mcp.h:68` | input | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Client subclass configuration | `setClientCapabilities/setNegotiation; extension result hook` — `src/mcp/client/Mcp.h:141` | tour/negotiation | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Client subprocess binding | `Creator, ExitHandler, StdioOptions, start/stop and graceful shutdown` — `src/mcp/client/Stdio.h:59` | stdio | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |
| Client HTTP/TLS binding | `Http/Https constructors/getConfig/setPath/start/stop; path/bearerToken/onError` — `src/mcp/client/http/Client.h:54` | http/https/auth | Planned: `McpShowcaseTour_*`, `McpShowcaseWire_*`; native support UNVERIFIED |

Wire references: [2025 schema](https://modelcontextprotocol.io/specification/2025-11-25/schema), [2026 schema](https://modelcontextprotocol.io/specification/2026-07-28/schema), [MRTR](https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/mrtr), [HTTP authorization](https://modelcontextprotocol.io/specification/2026-07-28/basic/authorization).
