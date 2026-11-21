// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_SERVER_H
#define MCP_SERVER_SERVER_H
#include "mcp/server/Reply.h"
#include "mcp/server/Request.h"
#include "mcp/server/Types.h"
#include <functional>
#include <memory>
#include <string>
namespace mcp::server {
    namespace detail {
        class Access;
    }
    /// Registry owner; retain through every attached transport's shutdown.
    class Server {
    public:
        /// Event-loop callback; retain the shared reply for asynchronous work.
        using ToolHandler = std::function<void(const ToolCall&, const std::shared_ptr<ToolReply>&)>;
        /// Resource callback on the event loop; shared completion owns request state.
        using ResourceHandler = std::function<void(const ResourceRead&, const std::shared_ptr<ResourceReply>&)>;
        /// Prompt callback on the event loop; shared completion owns request state.
        using PromptHandler = std::function<void(const PromptGet&, const std::shared_ptr<PromptReply>&)>;
        /// Construct before attachment; owns its name and registration data.
        explicit Server(std::string name);
        /// Destroy only after attached transports have drained.
        ~Server();
        /// Register before attachment on the event-loop thread.
        void tool(Json description, ToolHandler handler);
        /// Register before attachment on the event-loop thread.
        void resource(Json description, ResourceHandler handler);
        /// Register before attachment on the event-loop thread.
        void prompt(Json description, PromptHandler handler);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl;
        friend class detail::Access;
    };
} // namespace mcp::server
#endif
