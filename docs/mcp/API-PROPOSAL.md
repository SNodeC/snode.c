# MCP application API proposal

Status: approved for implementation with amendments A1–A8 below. Baseline: local `master` at `337065989`. No core change, implementation, branch, fetch, build, installation, or public action is proposed in this document-writing step. All new declarations and examples below are proposed APIs, not APIs already available in SNode.C.


## Approved amendments and execution contract

A1: Public ErrorCode contains only InvalidParams and InternalError. Unknown resource mapping is private: [2026-07-28 resources](https://modelcontextprotocol.io/specification/2026-07-28/server/resources#error-handling) requires -32602; [2025-11-25 resources](https://modelcontextprotocol.io/specification/2025-11-25/server/resources#error-handling) specifies -32002. Test both eras.

A2: ProgressToken is exactly variant<string, int64_t>; preserve the wire JSON type, reject floating-point/out-of-range tokens. Both [2026 progress](https://modelcontextprotocol.io/specification/2026-07-28/basic/patterns/progress) and [2025 progress](https://modelcontextprotocol.io/specification/2025-11-25/basic/utilities/progress) require string or integer.

A3: Public deadline methods use utils::Timeval: setDeadline(const utils::Timeval&) and getDeadline() const. This follows src/net/config/ConfigConnection.h:72 and :75 and ConfigConnection.cpp:153. CLI --deadline-ms stays 30000, range 1–300000 milliseconds; convert only at the option boundary.

A4: One Reply<Result> template, pimpl, shared private request implementation, explicit library instantiations; ToolReply/ResourceReply/PromptReply handlers enforce the result kind at compile time. Add negative compilation with a successful matching-kind control.

A5: Process-owning stdio::Transport::run, typed outer definitions with raw JSON Schema and no raw-description overload, and content-only non-object JSON for the 2025 era are approved.

A6: Every public declaration in every installed MCP header must carry a doc comment documenting its actual ownership, lifetime, call-once and event-loop contract as applicable. Value fields are owned values; handles document borrowing/copying; accessors document reference validity. Declaration comments below are supplemented by this contract and must be made individual in implementation.

A7: Optional result-field changes are omitted from this implementation to reserve the hard budget for the API changes. Existing demo result field names and shapes remain unchanged.

A8: Hard ceilings: production additions plus deletions <=1800, net production growth <=850, test additions plus deletions <=1200. Account separately for production, tests and documentation after every commit and before further production changes; do not compress code or hide production in tests/docs. If the completed design cannot fit, stop and report the accounting. The original estimates are not permission to exceed these ceilings.

Execution: first commit records this amended proposal. Then implement the seven planned commits in order. The first implementation commit raises the independent MCP SOVERSION 2 to 3 exactly once. Every implementation commit must build and pass ctest -L mcp plus relevant existing express/pipe/shutdown tests. Only approved MCP/application/test/docs paths and the one CMake SOVERSION line may change; the core diff against master must remain empty. Stage explicit paths only. Final GCC and strict Clang+ASan suites must have zero failures/skips; staged SONAMEs, SDK/Inspector checks, Claude Code and Codex stdio/HTTP calls and captured transcripts remain required. Use normal client configs and invocation-only MCP config. No push or public action.

## Boundary and governing invariant

Applications register domain operations and complete requests with typed MCP results. The MCP module alone owns protocol envelopes, schema enforcement, request completion, deadlines, cancellation, and transport output. A request has one terminal-state authority; structured tool output has one value authority. HTTP remains an express router, configuration remains SubCommand/CLI11 configuration, and all callbacks run on the existing SNode.C event loop. No thread, second dispatcher, generic application runner, schema DSL, filesystem service, or core lifecycle API is added.

Prefer reduction first: remove public dispatch/validation/envelope operations and the registration-validity latch; remove arbitrary result-envelope construction and repeated deadline polling. Change existing registry entries, Reply state and transport adapters next. Add only the requested typed vocabulary, cancellation callbacks, and ordered stdio entry where the existing public interface cannot express the boundary. This proposal does not authorize implementation or its estimated production growth.

## Verified review findings

References are to the baseline checkout, not to hypothetical future line numbers.

| Finding | Evidence and qualification |
| --- | --- |
| Installed protocol/plumbing exposure | Confirmed: `src/mcp/server/Server.h:16` exposes protocol versions and metadata keys; `:24` exposes Output and the boolean-heavy Reply constructor; `:57` exposes dispatch; `:61` exposes schema checking/validation/request checking/HTTP status mapping. `src/mcp/server/Server.h:38` begins nine Reply data members, and `:75` exposes Server storage. Neither class has a pimpl. `src/mcp/server/stdio/Transport.h:16` already hides State behind a shared pointer. Installation is explicit in `src/mcp/server/CMakeLists.txt:26`, `src/mcp/server/stdio/CMakeLists.txt:17`, and `src/mcp/server/http/CMakeLists.txt:17`. |
| Anonymous late registration failure | Confirmed, with a qualification: `src/mcp/server/Server.cpp:258` validates during registration, and `:276` already logs a reason, which can be empty. It neither names the registration nor reports failure to the caller; `:275` sets a global latch. `src/mcp/server/Server.h:51` supplies one Handler for all kinds; `:53` returns void. Transport rejection is later and generic at `src/mcp/server/stdio/Transport.cpp:282` and `src/mcp/server/http/Router.cpp:267`. Server version is literally `"1"` at `src/mcp/server/Server.cpp:343` and `:346`; no instructions field exists there. |
| Reply conflates responsibilities | Confirmed at `src/mcp/server/Server.h:26` through `:34`; `src/mcp/server/Server.cpp:182` makes expire a mutating timeout completion. Tool results are hand-built at `src/apps/mcp/Tools.cpp:229`, `:236`, `:240`, `:396`; resources at `:401`, prompts at `:410`. No progress helper or onCancel API exists in Server.h. Cancellation has concrete effects already: `src/mcp/server/Server.cpp:237` terminates output; it is not merely an application polling convention. |
| Polling-only deadline detection | Qualified: the application polls at `src/apps/mcp/Tools.cpp:119` and `:246`, but transports already use framework interval timers: HTTP at `src/mcp/server/http/Router.cpp:182`, stdio at `src/mcp/server/stdio/Transport.cpp:295`. Replace their deadline-detection polling with request-owned singleshot timers, not with a new clock or scheduling framework. Preserve transport scheduling still needed for input/queue pumping. |
| Config inconsistency | Confirmed: SubCommand inheritance at `src/mcp/server/Config.h:13`; option spellings at `src/mcp/server/Config.cpp:6`; mismatched getter names at `:12`; no setters in Config.h. Application string lookups at `src/apps/mcp/Tools.cpp:291`. HTTP copies the origins vector at `src/mcp/server/http/Router.cpp:274`. |
| HTTP authorization | Router inheritance is appropriate (`src/mcp/server/http/Router.h:9`). Authorize is boolean-only (`:11`). Disagreement with “no WWW-Authenticate” literally: rejection already sends a fixed `WWW-Authenticate: Bearer` at `src/mcp/server/http/Router.cpp:282`; the callback cannot choose a challenge, return 403, or supply a principal. Origin rejection is separately 403 at `:280`. |
| Stdio ordering | Confirmed: separate retain/construct calls at `src/mcp/server/stdio/Transport.h:11`; descriptor duplication/redirection at `src/mcp/server/stdio/Transport.cpp:268`; main must retain before init at `src/apps/mcp/main.cpp:28` and `:37`. No public signature ties these operations together. |
| Naming collisions | Confirmed: `src/mcp/server/Server.h:25`, `:29`, `:58` contain boolean modes. MCP “legacy” selects protocol 2025-11-25 (`src/mcp/server/Server.h:17`); elsewhere `src/net/in/stream/legacy/SocketServer.h:47` selects the plain stream implementation. These concepts must not share public terminology. |
| Demo workarounds and responsibilities | Confirmed: argv append at `src/apps/mcp/main.cpp:33`, root option lookup/default change at `:38`, listener result/state/free handling at `:71`. Headers confinement is `src/apps/mcp/Tools.cpp:36`, slicing is `:98` and `:197`. Disagreement: confinement and bounded filesystem work properly belong to the development application, not to the MCP library. Also, an asynchronous listener failure must still affect the process exit status; deleting that bookkeeping would be a behavior regression. |

The api.read divergence is already fixed on this baseline (`src/apps/mcp/Tools.cpp:231` includes text and byte metadata in the shared value). This redesign makes the same fix reusable and harder to undo; it does not claim the baseline bug is still present.

## Established SNode.C conventions

- Configure named instances and application subcommands before init, mount handlers after init, listen, then start: `src/apps/http/httpserver.cpp:65`, `:67`, `:69`, `:71`, `:96`, `:115`.
- Use pointer-returning chainable setters, `getX` getters, and existing option handles: `src/web/http/server/ConfigHttpServer.h:46`, `:52`; implementation uses `setDefaultValue` and returns `this` at `src/web/http/server/ConfigHttpServer.cpp:47` and `:65`.
- Applications own their options in a SubCommand: `src/apps/http/ConfigWWW.h:56`, `src/apps/http/ConfigWWW.cpp:50`. Existing typed variable bindings are available at `src/utils/SubCommand.h:195`; there is no need for a new MCP option framework.
- Mountable, copyable express routers are normal: `src/express/Router.h:98`, `:101`, `:109`. Preserve that behavior rather than introduce hand-written HTTP dispatch.
- Use semantic application/framework logging: `src/Log.h:286` and `:287`; the pre-MCP HTTP app uses it at `src/apps/http/httpserver.cpp:86` and `:100`. Opaque implementation ownership also has a pre-MCP precedent in `Log.h:280`.
- Use framework timers and cancellation: `src/core/timer/Timer.h:73`, `src/core/Timer.h:67`. `SNodeC::init/start/free` are existing entry points (`src/core/SNodeC.h:66`); `start()` already calls free (`src/core/EventLoop.cpp:356`). It does not turn an application listener failure into an application-specific exit status. No new core API is assumed.

## Complete proposed installed headers

These eight headers replace the four current installed headers. Code is declarations only, including value-type fields and ownership declarations; method bodies belong in the MCP libraries. Preserve the repository license and include-guard style. JSON remains the repository's nlohmann JSON. The intentionally small value records below are experimental API/ABI too; pimpl is not a claim of permanent ABI stability.

### `mcp/server/Types.h`

```cpp
// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_TYPES_H
#define MCP_SERVER_TYPES_H
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <variant>
#include <vector>
namespace mcp::server {
    using Json = nlohmann::json;
    /// Authenticated application identity; never inferred from request arguments.
    struct Principal {
        std::string subject;
        Json claims;
    };
    /// Preserve numeric/string tokens without an application parsing _meta.
    using ProgressToken = std::variant<std::string, std::int64_t>;
    enum class CancelReason { Client, Deadline, Disconnected, Shutdown };
    enum class Completion { Accepted, Inactive };
    /// Application-visible errors; transport/envelope errors stay internal.
    enum class ErrorCode : int { InvalidParams = -32602, InternalError = -32603 };
    enum class Role { User, Assistant };
    /// Optional MCP content annotations, not an authorization policy.
    struct Annotations {
        std::vector<Role> audience;
        std::optional<double> priority;
        std::optional<std::string> lastModified;
    };
    struct Progress {
        double value;
        std::optional<double> total;
        std::string message;
    };
} // namespace mcp::server
#endif
```

### `mcp/server/Results.h`

```cpp
// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_RESULTS_H
#define MCP_SERVER_RESULTS_H
#include "mcp/server/Types.h"

#include <optional>
#include <string>
#include <variant>
#include <vector>
namespace mcp::server {
    struct TextResource {
        std::string uri;
        std::string mimeType;
        std::string text;
    };
    struct BlobResource {
        std::string uri;
        std::string mimeType;
        std::string base64;
    };
    using ResourceContents = std::variant<TextResource, BlobResource>;
    struct TextContent {
        std::string text;
        Annotations annotations;
    };
    struct ImageContent {
        std::string base64;
        std::string mimeType;
        Annotations annotations;
    };
    struct AudioContent {
        std::string base64;
        std::string mimeType;
        Annotations annotations;
    };
    struct ResourceLink {
        std::string uri;
        std::string name;
        std::string description;
        std::string mimeType;
        std::optional<std::uint64_t> size;
        Annotations annotations;
    };
    struct EmbeddedResource {
        ResourceContents resource;
        Annotations annotations;
    };
    using Content = std::variant<TextContent, ImageContent, AudioContent, ResourceLink, EmbeddedResource>;
    /// Prompt messages accept content, not a prebuilt protocol envelope.
    struct PromptMessage {
        Role role;
        Content content;
    };
    template <class Result> class Reply;
    class ToolResult {
    public:
        /// Unstructured text; no invented structuredContent.
        static ToolResult text(std::string text);
        /// Serialize one JSON value; the encoder handles era-specific wire fields.
        static ToolResult json(Json value);
        /// Other tool content shapes, including mixed image/audio/resource blocks.
        static ToolResult content(std::vector<Content> content);
        /// Tool execution/schema failure, distinct from a JSON-RPC error.
        static ToolResult toolError(std::string message);
        static ToolResult toolError(std::vector<Content> content);

    private:
        ToolResult();
        template <class Result> friend class Reply;
        std::vector<Content> blocks;
        std::optional<Json> structured;
        bool failed;
    };
    struct ResourceResult {
        std::vector<ResourceContents> contents;
    };
    struct PromptResult {
        std::optional<std::string> description;
        std::vector<PromptMessage> messages;
    };
} // namespace mcp::server
#endif
```

### `mcp/server/Request.h`

```cpp
// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_REQUEST_H
#define MCP_SERVER_REQUEST_H
#include "mcp/server/Types.h"

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
namespace mcp::server {
    namespace detail {
        class RequestAccess;
    }
    /// Copyable handle to one request's shared state; event-loop use only.
    class RequestContext {
    public:
        using OnCancel = std::function<void(CancelReason)>;
        ~RequestContext();
        RequestContext(const RequestContext&);
        RequestContext& operator=(const RequestContext&);
        const std::optional<Principal>& getPrincipal() const;
        const std::optional<ProgressToken>& getProgressToken() const;
        /// Each registered callback runs once on cancellation, never on success.
        /// Late registration after cancellation invokes it before returning.
        void onCancel(OnCancel callback) const;

    private:
        struct Impl;
        explicit RequestContext(std::shared_ptr<Impl> impl);
        std::shared_ptr<Impl> impl;
        friend class detail::RequestAccess;
    };
    /// Arguments have already passed the registered input schema.
    struct ToolCall {
        RequestContext context;
        Json arguments;
    };
    /// Resources receive their URI, rather than a differently shaped Json args.
    struct ResourceRead {
        RequestContext context;
        std::string uri;
    };
    /// Prompt arguments have the protocol's string-valued argument shape.
    struct PromptGet {
        RequestContext context;
        std::map<std::string, std::string> arguments;
    };
} // namespace mcp::server
#endif
```

### `mcp/server/Reply.h`

```cpp
// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_REPLY_H
#define MCP_SERVER_REPLY_H
#include "mcp/server/Results.h"

#include <memory>
#include <string>
namespace mcp::server {
    namespace detail {
        class RequestAccess;
    }
    /// Share the handle for asynchronous work; construction is module-private.
    template <class Result> class Reply {
    public:
        ~Reply();
        Reply(const Reply&) = delete;
        Reply& operator=(const Reply&) = delete;
        Reply(Reply&&) = delete;
        Reply& operator=(Reply&&) = delete;
        /// Complete once with the matching result kind on the event-loop thread.
        Completion result(Result result);
        Completion error(ErrorCode code, std::string message, Json data = nullptr);
        /// Automatically attaches the request's token; absent token is a no-op.
        void progress(Progress progress);

    private:
        struct Impl;
        explicit Reply(std::unique_ptr<Impl> impl);
        std::unique_ptr<Impl> impl;
        friend class detail::RequestAccess;
    };
    /// Typed tool completion handle; ownership is shared by asynchronous handlers.
    using ToolReply = Reply<ToolResult>;
    /// Typed resource completion handle; ownership is shared by asynchronous handlers.
    using ResourceReply = Reply<ResourceResult>;
    /// Typed prompt completion handle; ownership is shared by asynchronous handlers.
    using PromptReply = Reply<PromptResult>;
    /// Instantiations are supplied by the MCP library; no consumer implementation.
    extern template class Reply<ToolResult>;
    /// Instantiation supplied by the MCP library.
    extern template class Reply<ResourceResult>;
    /// Instantiation supplied by the MCP library.
    extern template class Reply<PromptResult>;
} // namespace mcp::server
#endif
```

### `mcp/server/Server.h`

```cpp
// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_SERVER_H
#define MCP_SERVER_SERVER_H
#include "mcp/server/Reply.h"
#include "mcp/server/Request.h"

#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
namespace mcp::server {
    namespace detail {
        class ServerAccess;
    }
    enum class RegistrationKind { Tool, Resource, Prompt };
    enum class Requirement { Optional, Required };
    /// Typed outer description; schemas and annotation payloads remain JSON.
    struct ToolDefinition {
        std::string name;
        std::string description;
        Json inputSchema;
        std::optional<Json> outputSchema;
        std::optional<std::string> title;
        Json annotations;
    };
    struct ResourceDefinition {
        std::string uri;
        std::string name;
        std::string description;
        std::string mimeType;
        std::optional<std::string> title;
    };
    struct PromptArgument {
        std::string name;
        std::string description;
        Requirement requirement;
    };
    struct PromptDefinition {
        std::string name;
        std::string description;
        std::vector<PromptArgument> arguments;
        std::optional<std::string> title;
    };
    /// what() includes kind, identifier, field path, and rejection reason.
    class RegistrationError : public std::invalid_argument {
    public:
        RegistrationError(RegistrationKind kind, std::string identifier, std::string field, std::string reason);
    };
    class Server {
    public:
        using ToolHandler = std::function<void(const ToolCall&, const std::shared_ptr<ToolReply>&)>;
        using ResourceHandler = std::function<void(const ResourceRead&, const std::shared_ptr<ResourceReply>&)>;
        using PromptHandler = std::function<void(const PromptGet&, const std::shared_ptr<PromptReply>&)>;
        explicit Server(std::string name, std::string version, std::string instructions = {});
        ~Server();
        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;
        Server(Server&&) = delete;
        Server& operator=(Server&&) = delete;
        /// Transactional registration; throws RegistrationError immediately.
        Server* tool(ToolDefinition definition, ToolHandler handler);
        Server* resource(ResourceDefinition definition, ResourceHandler handler);
        Server* prompt(PromptDefinition definition, PromptHandler handler);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl;
        friend class detail::ServerAccess;
    };
} // namespace mcp::server
#endif
```

### `mcp/server/Config.h`

```cpp
// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_CONFIG_H
#define MCP_SERVER_CONFIG_H
#include "utils/SubCommand.h"
#include "utils/Timeval.h"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
namespace mcp::server {
    class Config : public utils::SubCommand {
    public:
        constexpr static std::string_view NAME{"mcp"};
        constexpr static std::string_view DESCRIPTION{"MCP limits and Origin policy"};
        explicit Config(utils::SubCommand* parent);
        ~Config() override;
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;
        Config(Config&&) = delete;
        Config& operator=(Config&&) = delete;
        Config* setMaximumMessageBytes(std::size_t bytes);
        std::size_t getMaximumMessageBytes() const;
        Config* setMaximumOutputBytes(std::size_t bytes);
        std::size_t getMaximumOutputBytes() const;
        Config* setDeadline(const utils::Timeval& deadline);
        utils::Timeval getDeadline() const;
        Config* setAllowedOrigins(std::vector<std::string> origins);
        /// Borrow only for the current event-loop turn; no request-time copy.
        const std::vector<std::string>& getAllowedOrigins() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl;
    };
} // namespace mcp::server
#endif
```

### `mcp/server/http/Router.h`

```cpp
// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_HTTP_ROUTER_H
#define MCP_SERVER_HTTP_ROUTER_H
#include "express/Router.h"
#include "mcp/server/Types.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <variant>
namespace mcp::server {
    class Server;
    class Config;
    namespace http {
        /// nullopt is deliberate anonymous access, not failed authentication.
        struct Authorized {
            std::optional<Principal> principal;
        };
        /// Must supply a nonempty, header-safe authentication challenge.
        struct Unauthorized {
            std::string wwwAuthenticate;
        };
        struct Forbidden {};
        using Authorization = std::variant<Authorized, Unauthorized, Forbidden>;
        class Router : public express::Router {
        public:
            using Authorize = std::function<Authorization(const std::shared_ptr<express::Request>&)>;
            /// Borrow server/config; retain them through shutdown/drain.
            /// Empty authorize means anonymous access, as in the current demo.
            Router(Server& server, Config& config, Authorize authorize = {});
        };
    } // namespace http
} // namespace mcp::server
#endif
```

### `mcp/server/stdio/Transport.h`

```cpp
// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_STDIO_TRANSPORT_H
#define MCP_SERVER_STDIO_TRANSPORT_H
#include <functional>
#include <memory>
namespace mcp::server {
    class Server;
    class Config;
    namespace stdio {
        class Transport {
        public:
            using Ready = std::function<void()>;
            /// Sole public entry: retain stdout, init, Ready, attach, start/drain.
            /// Requires core state LOADED; rejects other states before FD changes.
            static int run(int argc, char* argv[], Server& server, Config& config, Ready ready);

        private:
            struct State;
            explicit Transport(std::shared_ptr<State> state);
            ~Transport();
            Transport(const Transport&) = delete;
            Transport& operator=(const Transport&) = delete;
            std::shared_ptr<State> state;
        };
    } // namespace stdio
} // namespace mcp::server
#endif
```

All definitions above are complete declaration sets, not snippets requiring an unspecified public base class. Normal standard-library-generated value-record operations remain available. Private pimpl and friend declarations are access boundaries, not installed protocol implementation headers.

## Non-installed detail and ownership

Keep all dispatch, protocol keys, supported-version strings, validators, HTTP status selection, wire encoding, output callbacks, subscriptions, and transport cancellation in non-installed detail headers/sources under `src/mcp/server/detail`. The old Server.h does not remain installed as a compatibility façade: its replacement contains only registration and identity. Proposed internal headers are `ServerAccess.h` (registration freeze/dispatch), `RequestAccess.h` (creation/terminal transitions), and `Protocol.h` (envelope codec/schema helpers and era selection). They may be shared by the three MCP targets through PRIVATE build include paths; they are not included transitively by installed headers or exported as a consumer dependency. Unit tests may deliberately use private build includes; installed-consumer tests may not.

Internally name the eras `ProtocolEra::Initialization20251125` and `ProtocolEra::PerRequest20260728`. Transport streaming, cache policy, and cancellation origin also use named enums internally. No public function takes a boolean mode. Ordinary JSON booleans and the private ToolResult failure bit are data, not positional mode switches. Do not rename the framework's unrelated `legacy` TCP namespace.

Server owns the registry. Transport state borrows Server/Config and owns accepted-operation references and output queues. Server/Config must outlive transport shutdown and its draining operations. Reply's pimpl references the same request state as RequestContext; it does not keep an independent done/cancelled/deadline authority. Context copies extend request metadata lifetime; they do not keep a transport alive indefinitely. Reply encodes into a weak output sink so an application retaining a late reply cannot dereference a destroyed router/stdio state.

At successful attachment, seal registration through ServerAccess; later registration throws a named RegistrationError. Reuse a sealed registry for both transports if an embedding application attaches both. Failed attachment must not leave a half-attached transport. Destruction cancels pending requests while output can still drain; no destructor reaches already freed core state. No process-wide registry of Server objects is introduced.

## Handler, completion and cancellation contract

Handlers have different input records, preserving three distinct meanings: ToolCall.arguments is schema-validated JSON; ResourceRead.uri identifies the registered resource; PromptGet.arguments is a validated string map, with required arguments enforced. RequestContext is shared infrastructure, not a raw JSON-RPC request. No handler receives an express request; authenticated claims enter only via the transport-created principal. Stdio supplies no principal by default and does not silently equate an OS account with application authorization.

One private RequestContext::Impl owns phase, optional cancellation reason, callbacks, and the framework singleshot deadline timer. Reply::Impl refers to it for completion. Arm the timer for every ordinary accepted request, including synchronous handlers; do not arm an ordinary deadline for protocol-owned subscription streams. A singleshot cannot interrupt a blocking handler; bounded event-loop work remains mandatory.

Completion, client cancellation, timeout, disconnect, output failure, and shutdown race through one event-loop terminal transition. Mark terminal, detach the deadline timer and callback list, then invoke callbacks with a temporary lifetime guard. No callback can create a second terminal message. A deadline emits the current toolError or JSON-RPC timeout behavior before notifying cancellation observers; client cancellation/disconnect/shutdown retain their current wire semantics. Before every terminal result/error attempt and at a handler return boundary, compare the stored deadline as well, so overdue synchronous work cannot win merely because its timer callback has not run. This comparison is internal, not a new public expire().

onCancel callbacks are for cancelling the application's own timer/I/O and releasing descriptors, not for completing the protocol again. They run once, on the event-loop thread; callback exceptions are logged and contained so other callbacks still run. Late registration after cancellation fires immediately. Capture a weak application-operation pointer to avoid a context/callback/operation cycle. Success clears cancellation callbacks without invoking them. Applications do not poll active()/expired()/expire(); bounded slices and their own operation-finished state still exist.

Reply::result returns Completion::Inactive after any terminal transition. Passing a ResourceResult to ToolReply is ill-formed at compile time. One explicitly instantiated Reply<Result> template supplies ToolReply, ResourceReply and PromptReply aliases; there are no three inheritance hierarchies. A shared private request implementation owns the common completion/error/progress behavior. A negative compile test must reject every wrong result-kind pairing and a positive control must compile. The accepted result is schema-validated and bounded before becoming terminal. Successful tool output that fails its declared output schema is converted to toolError as today.

ToolResult::json owns one canonical JSON value. For an object, both eras encode `content[0].text = value.dump()` and `structuredContent = value`; this is the default for api.read/find/preview. It exposes no independent setters for those channels. The modern era also permits non-object structured JSON values, whereas the 2025 era permits objects. For non-object values, emit the same serialized JSON text to both eras, add structuredContent only for the modern era, and omit a non-object-only outputSchema from the 2025 listing. Server-side validation still uses the registered schema and canonical value before encoding; do not wrap or otherwise change the domain value. This explicit older-client representation preserves data without emitting an invalid legacy structuredContent. [2025 tool result rules](https://modelcontextprotocol.io/specification/2025-11-25/server/tools#structured-content), [2026 tool result rules](https://modelcontextprotocol.io/specification/2026-07-28/server/tools#structured-content). ToolResult::text/content have no structuredContent and cannot satisfy a declared object outputSchema; this is diagnosed, not silently filled with an unrelated object. ToolResult::toolError creates `isError: true` content and bypasses successful-output schema validation. ResourceResult supports multiple text/blob entries; PromptResult supports description and multiple role/content messages. Text/image/audio/resource-link/embedded-resource blocks cover the application content union. Unsupported blocks for a negotiated era fail explicitly; no silent dropping or unadvertised capability is added.

Listings, initialize/discover, ping, empty resource-template lists, subscription acknowledgements/cancellation, and modern result/cache envelopes remain protocol-owned results, not application factories. Arbitrary extension notification methods and raw wire-result escape hatches are intentionally absent. Existing unsupported sampling, elicitation, completion, resource-template registration, and notification capabilities are not introduced by giving them speculative application APIs.

Reply::progress attaches the captured token automatically, validates finite nonnegative progress/total and monotonic progression, and ignores progress without a token or after termination. The encoder preserves the token's JSON number/string type; it does not change IDs into strings. Progress is not a terminal result. Public errors use ErrorCode; ordinary tool execution failures use toolError, not ErrorCode::InternalError. No raw integer overload is kept.

## Registration, configuration, authorization and startup

Recommend typed outer descriptions instead of a free-form description Json and instead of a fluent schema-builder DSL. The aggregates are small builders of registry entries; input/output schemas remain one JSON Schema authority using the existing restricted validator (`src/mcp/server/Server.cpp:36`). Tool annotations stay JSON because this proposal does not add a second annotation schema engine. No raw-description overload survives, so misspelled outer fields cannot be ignored. This is an intentional source break.

Every registration is transactional and synchronous: validate kind/name or URI, required fields, duplicate key, handler presence, limits, schemas, and prompt arguments before inserting. Throw RegistrationError containing e.g. `tool api.read: inputSchema.properties.bytes.maximum: unsupported value`. A missing identifier is reported as `<unnamed>` with the failing field. Replace registrationsValid/valid() and generic transport registration rejection. No “best effort” partially invalid startup. Caller-supplied name/version go into result.serverInfo for 2025 initialization and result._meta["io.modelcontextprotocol/serverInfo"] for modern discovery. Nonempty instructions go into result.instructions in both. These placements are confirmed by the [2025 lifecycle example](https://modelcontextprotocol.io/specification/2025-11-25/basic/lifecycle) and [2026 discovery definition](https://modelcontextprotocol.io/specification/2026-07-28/server/discover); the application never chooses the placement.

Config setter/getter names correspond to `--maximum-message-bytes`, `--maximum-output-bytes`, `--deadline-ms`, and repeatable `--allowed-origin`; only pluralizing the vector getter is intentional. Preserve current defaults and validators: 1 MiB message/output limits and 30,000 ms deadline (`src/mcp/server/Config.cpp:6`). Setters return Config*, reject invalid defaults, and use the SubCommand default mechanism. Store typed option values using existing addOptionVariable bindings, including one authoritative origins vector in Config::Impl; do not add a Router snapshot/cache. Borrow that vector during validation only. Existing framework reconfiguration remains the authority; no borrowed reference survives a callback/reconfiguration boundary. An accepted request retains its admission-time deadline; a later configuration change affects subsequent admissions.

For application options, derive an application-local DevelopmentConfig from Config, retain CLI::Option* handles, and expose typed getters just as ConfigWWW does. This keeps the existing `mcp` section and HTTP instance nesting; no nested `development` section or new command-line spellings. Retain the application header-root/search-files/search-bytes options and limits from `src/apps/mcp/Tools.cpp:284`. Do not put those development-only options into installed MCP Config.

The corresponding proposed, non-installed Tools.h declarations are:

```cpp
#include "mcp/server/Config.h"

#include <cstddef>
#include <string>
namespace mcp::server {
    class Server;
}
namespace apps::mcp {
    class DevelopmentConfig : public ::mcp::server::Config {
    public:
        explicit DevelopmentConfig(utils::SubCommand* parent);
        std::string getHeaderRoot() const;
        std::size_t getSearchFiles() const;
        std::size_t getSearchBytes() const;

    private:
        CLI::Option* headerRootOpt;
        CLI::Option* searchFilesOpt;
        CLI::Option* searchBytesOpt;
    };
    /// Return the same house-style authority used by the resource and prompt.
    std::string instructions();
    void registerTools(::mcp::server::Server& server, const DevelopmentConfig& config);
} // namespace apps::mcp
```

Authorize returns exactly Authorized(optional principal), Unauthorized(challenge), or Forbidden. These map to admitted request, HTTP 401 plus caller-supplied WWW-Authenticate, and HTTP 403. Validate nonempty challenge/header safety before writing headers. Exceptions log an internal diagnostic and produce 500, not anonymous access. Preserve Origin validation as an independent prior rejection. Authenticate before schema/handler dispatch, never derive principal from the request body, and do not log bearer credentials. The callback remains synchronous and bounded, like an express middleware callback; remote token introspection is not part of this proposal. It may consult identity supplied by earlier express middleware.

Stdio run is the only public attachment entry. Options and Server may be constructed before it; the Ready callback registers handlers after parsing and before transport acceptance. Inside run: verify LOADED; retain/redirect stdout; force foreground arguments using the existing root-option mechanism; init; call Ready; attach the private Transport; start and drain; close owned descriptors on each non-exit path. Do not leave a public retainProtocol/raw-fd constructor pair. This structurally orders operations inside the entry; C++ cannot prevent a caller separately invoking global SNodeC::init, so such a call is rejected by the LOADED check rather than claimed to be a compile-time impossibility.

Do not add a core foreground flag API just for MCP. The existing private argv normalization can remain inside the stdio adapter until the framework has a separately approved general API. stdout-to-stderr redirection already isolates help, parse diagnostics and logging; root --log-console default manipulation can be removed from the demo. Explicit daemonize=true must not fork a protocol client: preserve the adapter's foreground-only contract and test it. Quiet/logging behavior and stdin-EOF draining remain unchanged. Config/argv storage must live through start/free because `src/utils/Config.cpp:952` retains argv. This entry is deliberately process-owning; embedded applications wanting to run unrelated initialized services must use HTTP or request a separately designed descriptor-based stdio embedding API.

## Proposed snodec-dev main.cpp in full

`DevelopmentConfig`, `registerTools` and `instructions` below are proposed application-local declarations in Tools.h, not new framework APIs. DevelopmentConfig derives Config, retains the three existing CLI option handles, and provides getHeaderRoot/getSearchFiles/getSearchBytes. `SNODEC_DEV_VERSION` is a proposed additional substitution in the existing generated `mcp-config.h` (`src/apps/mcp/CMakeLists.txt:2`, `src/apps/mcp/config.h.in:3`), sourced from SNode.C_VERSION. No independent hand-maintained demo version constant is added.

```cpp
// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "Log.h"
#include "Tools.h"
#include "core/SNodeC.h"
#include "mcp-config.h"
#include "mcp/server/Server.h"

#include <exception>
#ifdef MCP_STDIO
#include "mcp/server/stdio/Transport.h"
#include "utils/Config.h"
#else
#include "express/legacy/in/WebApp.h"
#include "mcp/server/http/Router.h"
#include "web/http/ConfigHttpParser.h"
#include "web/http/server/ConfigHttpServer.h"
#endif

int main(int argc, char* argv[]) {
    int result = 1;
    mcp::server::Server server("snodec-dev", SNODEC_DEV_VERSION, apps::mcp::instructions());
#ifdef MCP_STDIO
    auto* config = utils::Config::configRoot.newSubCommand<apps::mcp::DevelopmentConfig>();
    result = mcp::server::stdio::Transport::run(argc, argv, server, *config, [&] {
        apps::mcp::registerTools(server, *config);
        snode::log::application("mcp.development").info() << "SNode.C development stdio ready";
    });
#else
    using WebApp = express::legacy::in::WebApp;
    const WebApp app("mcp-http");
    auto* config = app.getConfig()->Instance::newSubCommand<apps::mcp::DevelopmentConfig>();
    auto* http = app.getConfig()->Instance::getSubCommand<web::http::server::ConfigHttpServer>();
    http->setAllowPipelining(false)->setMaximumPendingRequests(1);
    http->getParserConfig()->setMaximumBodyBytes(1048576)->setMaximumHeaderBytes(65536)->setMaximumHeaderFields(100);
    app.getConfig()->Connection::setMaximumWriteQueueBytes(1048576);
    app.getConfig()->Local::setHost("127.0.0.1")->setPort(3337);
    bool listenFailed = false;
    try {
        WebApp::init(argc, argv);
        if (app.getConfig()->Instance::getDisabled()) {
            core::SNodeC::free();
            result = 0;
        } else {
            apps::mcp::registerTools(server, *config);
            app.use("/mcp", mcp::server::http::Router(server, *config));
            app.listen([&listenFailed](const auto& address, const auto& state) {
                if (state == core::socket::State::OK) {
                    snode::log::application("mcp.development").info() << "SNode.C development HTTP listening on " << address.toString();
                } else {
                    listenFailed = state != core::socket::State::DISABLED;
                    snode::log::application("mcp.development").error() << "HTTP listener unavailable: " << state.what();
                    core::SNodeC::stop();
                }
            });
            if (core::SNodeC::state() == core::State::STOPPING) {
                core::SNodeC::free();
                result = listenFailed ? 1 : 0;
            } else {
                const int loopResult = WebApp::start();
                result = listenFailed ? 1 : loopResult;
            }
        }
    } catch (const std::exception& error) {
        snode::log::application("mcp.development").error() << error.what();
        core::SNodeC::free();
    }
#endif
    return result;
}
```

Server, HTTP app and listener-status storage are alive when cleanup runs, including the exception path. The stdio entry catches registration/attachment/runtime exceptions, logs them, drains what is safe, and returns failure; it does not throw after it has freed the loop. Construction failures before entry have not initialized the core. A disabled HTTP instance is intentionally successful without opening the header root. The explicit STOPPING/free branch and asynchronous listenFailed result remain necessary: start invoked outside INITIALIZED sets a failure status (`src/core/EventLoop.cpp:347`), and start itself otherwise owns free (`:356`). In particular, a synchronously disabled listener must still exit successfully. Keeping this small app-specific branch is preferable to adding an MCP HTTP application runner merely to hide it.

## Proposed tool/resource/prompt registrations

This is the complete registration function. The existing schema objects, house-style text and preview file bodies remain application data, named `findSchema`, `readSchema`, `previewSchema`, `style` and `makePreview` below. `makePreview(arguments)` is an application-local extraction of the current preview branch (`src/apps/mcp/Tools.cpp:325` through `:395`) returning the same object; no template text or behavior changes. The proposed Lookup constructor takes ToolCall and a named Operation enum instead of bare args and a read boolean; its existing descriptor confinement and bounded slices remain application code.

```cpp
void registerTools(mcp::server::Server& server, const DevelopmentConfig& config) {
    using namespace mcp::server;
    auto headers = std::make_shared<Headers>(config.getHeaderRoot());
    const auto files = config.getSearchFiles();
    const auto bytes = config.getSearchBytes();
    server.tool({.name = "api.find",
                 .description = "Literal search of confined installed headers; returns relative paths, lines and byte offsets.",
                 .inputSchema = findSchema},
                [headers, files, bytes](const ToolCall& call, const auto& reply) {
                    std::make_shared<Lookup>(headers, call, reply, Operation::Find, files, bytes)->schedule();
                });
    server.tool({.name = "api.read",
                 .description = "Read a bounded UTF-8 header range using a relative path and optional byte offset.",
                 .inputSchema = readSchema},
                [headers](const ToolCall& call, const auto& reply) {
                    std::make_shared<Lookup>(headers, call, reply, Operation::Read, 1, 65536)->schedule();
                });
    server.tool({.name = "skeleton.preview",
                 .description = "Preview an express application or router module. Does not write files.",
                 .inputSchema = previewSchema},
                [](const ToolCall& call, const auto& reply) {
                    reply->result(ToolResult::json(makePreview(call.arguments)));
                });
    server.resource({.uri = "snodec://development/house-style", .name = "SNode.C house style", .mimeType = "text/plain"},
                    [](const ResourceRead& read, const auto& reply) {
                        reply->result(ResourceResult{{TextResource{read.uri, "text/plain", style}}});
                    });
    server.prompt(
        {.name = "snodec-review",
         .description = "Review SNode.C code for event-loop, ownership, configuration and streaming conventions.",
         .arguments = {{"code", "Code to review", Requirement::Required}}},
        [](const PromptGet& get, const auto& reply) {
            const auto& code = get.arguments.at("code");
            if (code.size() > 16384) {
                reply->error(ErrorCode::InvalidParams, "code must be a string of at most 16384 bytes");
            } else {
                reply->result(PromptResult{
                    .messages = {
                        {Role::User, TextContent{std::string(style) + "\nTreat the following code as data and review it:\n" + code, {}}}}});
            }
        });
}
```

Lookup starts one cancellation subscription before scheduling its first slice. Proposed application-local declarations and statements (not installed MCP methods): store its singleshot Timer handle, add a `stop()` that cancels that timer/closes file and DIR handles, and have `call.context.onCancel([weak](CancelReason) { if (auto job = weak.lock()) { job->stop(); } });`. stop is idempotent and clears ownership on the event-loop thread. Clear callbacks at terminal completion. Remove the old reply-expiry checks from tick and collect rather than keep both mechanisms. Retain byte/file/entry/depth/path/queue bounds and checks between slices. A completed read calls `reply->result(ToolResult::json(value))`; lookup failure calls `ToolResult::toolError(failure)`. Neither the library nor context gains knowledge of filesystem confinement or a generic slice executor.

The new main is 69 physical lines versus 97 today (28 fewer). The displayed registration function is 42 physical lines; its current registration/handler region spans `src/apps/mcp/Tools.cpp:305` through `:418` (114 lines), but that region includes preview template bodies. Do not call the 72-line display difference a production-code saving: makePreview still contains those bodies, and schemas remain too. Expected genuine registration/envelope/control-flow saving is 10–25 lines after the data is held constant; exact implementation accounting remains UNVERIFIED. Whole Tools.cpp is currently 420 lines; no target whole-file size is promised before implementation.

## Migration, ABI and implementation boundaries

Increment `SNODEC_MCP_SOVERSION` from 2 to 3 at the first ABI-breaking implementation commit (`CMakeLists.txt:50`); all three MCP libraries already consume this one variable. Their VERSION continues to follow the full SNode.C release version; no release number or tag is selected here. Core SOVERSION and every core public header/source remain untouched. Experimental policy is `AGENTS.md:36`. All MCP consumers rebuild against the new headers/SONAMEs; no old-header/new-library ABI compatibility is promised and no compatibility overloads or dual dispatchers remain.

Migration mapping: Server(name) becomes Server(name, version, instructions); raw description Json becomes a kind-specific definition; Handler becomes ToolCall/ResourceRead/PromptGet plus shared Reply. Include Reply/Request/Results explicitly rather than relying on accidental transitive JSON internals. Replace result-envelope Json with factories/records; raw integer error with ErrorCode; progress metadata extraction with Reply::progress; polling with context.onCancel; Config string lookup with typed setters/getters or app option handles; bool authorization with Authorization; retainProtocol/ctor/init/start with stdio::Transport::run. Transport-focused tests exercise private dispatch through private build headers or, preferably, their public protocol boundary. No core tests are rewritten to accommodate the new API.

Estimated commit plan, not approved ceilings. Numbers include installed declarations and implementations, count production separately from tests/docs, and exclude no generated production code. Each commit must build and keep relevant existing behavior tests passing; do not leave an alternate implementation operating alongside the replacement.

| Narrow commit | Production changed / estimated net | Test additions / changes | Required evidence |
| --- | --- | --- | --- |
| 1. Hide existing Server/Reply implementation and bump MCP SOVERSION 2→3 | 180–280 / +40–80 | 60–100 | Installed consumer no longer sees dispatch/keys/Output; three SONAMEs .so.3, core unchanged; public include/IWYU checks. |
| 2. Typed results and ErrorCode | 220–340 / +100–180 | 140–220 | Exact read bytes through wire + official SDK both transports; text/JSON mirror (including non-object modern versus 2025 content-only encoding), all content variants, resource/prompt shapes, output-schema failure and toolError; remove raw envelope API together with call sites. |
| 3. Shared request context, kind-specific handlers, deadline timer and onCancel | 240–360 / +60–120 | 160–240 | Success/timeout/client cancellation/disconnect/shutdown races, late observer, callback exception, no ownership cycles, no duplicate output; migrate Handler inputs and remove expiry polling together, preserve transport pumping. |
| 4. Typed registration and identity/instructions | 180–260 / +40–90 | 100–160 | Named failures for duplicate, missing name, handler, unsupported schema and bad prompt arguments; no invalid registry latch; both discovery eras. Migrate outer registration descriptions in this same commit. |
| 5. Config and typed application options | 100–160 / +20–50 | 60–100 | Existing CLI spellings/defaults/ranges/show-config round trip, setter precedence, origin behavior and reconfiguration; demonstrate no per-request origins copy. |
| 6. HTTP authorization result/principal | 80–140 / +30–60 | 90–140 | 401/challenge, 403, anonymous/identity, malformed challenge/exception, Origin independence; handlers receive principal; no credential logging. |
| 7. Ordered stdio entry and final demo simplification | 150–240 / −30–+30 | 100–180 | Help/error stdout purity, forced foreground, invalid order, duplicate run, EOF/quiet/exception cleanup, listener failure exit code, disabled instance; exact CLI and demos both transports. |

Total expected net production growth: approximately +260–610 lines, mainly public value types/helpers and cancellation coverage; total production changed: approximately 1,150–1,780. Test work: 710–1,140 lines. Documentation/migration: approximately 100–180 lines. These are estimates rather than a disguised neutral-LOC claim. Re-estimate after reviewing the declarations; obtain explicit bounded implementation approval before adding production code. The pimpl and private-test migration belong in the same first commit; do not preserve the old public plumbing just to avoid coordinating that change.

Final implementation gates: GCC and strict Clang+ASan CTest with short TMPDIR, no relevant skips; installed consumer against staged MCP .so.3; wire/SDK exact-byte read; source-sized search on both transports within the existing explicit budgets plus concurrent response; real Claude Code stdio and HTTP transcripts showing the declaration and HTTP init search. Existing regression anchors are `tests/component/mcp/CMakeLists.txt:40`, `:52`, `:70`. No claim that these checks have run for this proposal. The proposed declarations/examples are UNVERIFIED as compiled or executable code; only source inspection, official protocol-page checks and document checks have run. Code fences were formatted with the repository clang-format configuration. Review production/test line counts and the core diff (must be empty) before every commit. No push, release, merge, or installation into the live system is authorized by this plan.

## Out-of-scope observations and decisions

The previous repository-wide path cleanup is not in this local-master baseline: unrelated example paths remain at `src/apps/http/httpserver.cpp:75` and elsewhere. The current request explicitly forbids touching them; no cleanup is included. The existing docs' protocol subscription/cancellation qualification (`docs/mcp/README.md`, “Protocol eras and wire behavior”) remains a qualification, not something the new C++ façade can resolve. No specification claim or new capability should be smuggled into an API refactor. Shared formatting/analysis files are not changed during this proposal.

Real decisions for Volker:

1. Approve the process-owning stdio entry as the sole supported public stdio mode, or require embedding into an already initialized loop? The latter needs a separately specified safe pre-init token/descriptor ownership API and more lifecycle surface; the proposal recommends the smaller process-owning entry.
2. Accept one checked Reply<Result> with compile-time result-kind safety, or require distinct ToolReply/ResourceReply/PromptReply types for compile-time result-kind enforcement? The latter improves static misuse detection at the cost of three public asynchronous handles; this proposal recommends one Reply and explicit checks.
3. Accept typed outer descriptions with raw JSON Schema/annotations and no raw-description overload? Recommendation: yes. A fluent schema DSL would duplicate the existing validator and is not recommended.
4. For non-object JSON output, accept the explicit content-only 2025 representation described above, or reject such calls for that era? Recommendation: preserve the serialized value as text; the demo uses objects and is unaffected.

SOVERSION 3, unchanged core, immediate named registration failure, mirrored JSON tool results, onCancel, CLI spellings and named HTTP authorization outcomes are requirements/recommendations already made concrete here, not extra permission questions. Implementation line growth and any extension beyond this proposal still require explicit approval. Stop after reviewing this document.
