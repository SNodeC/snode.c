// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_SERVER_H
#define MCP_SERVER_SERVER_H
#include "mcp/server/Prompt.h" // IWYU pragma: export
#include "mcp/server/Reply.h"
#include "mcp/server/Request.h"
#include "mcp/server/Resource.h" // IWYU pragma: export
#include "mcp/server/Tool.h"     // IWYU pragma: export

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
namespace mcp::server {
    namespace detail {
        class Access; ///< Module-private registry access.
    }
    /// Registration category used in immediate validation diagnostics.
    enum class RegistrationKind { Tool, Resource, Prompt };
    /// Immediate registration failure; what() owns kind, identifier, field and reason.
    class RegistrationError : public std::invalid_argument {
    public:
        /// Construct an owned diagnostic; no borrowed strings survive the call.
        RegistrationError(RegistrationKind kind, std::string identifier, std::string field, std::string reason);
        ~RegistrationError() override; ///< Release the owned diagnostic; key function is supplied by the library.
    };
    /// Noncopyable, nonmovable registry owner; retain through attached transports' shutdown.
    class Server {
    public:
        /// Event-loop callback; retain the shared reply for asynchronous work.
        using ToolHandler = std::function<void(const ToolCall&, const std::shared_ptr<ToolReply>&)>;
        /// Resource callback on the event loop; shared completion owns request state.
        using ResourceHandler = std::function<void(const ResourceRead&, const std::shared_ptr<ResourceReply>&)>;
        /// Prompt callback on the event loop; shared completion owns request state.
        using PromptHandler = std::function<void(const PromptGet&, const std::shared_ptr<PromptReply>&)>;
        /// Construct before attachment; owns its name and registration data.
        explicit Server(std::string name, std::string version, std::string instructions = {});
        /// Destroy only after attached transports have drained.
        ~Server();
        Server(const Server&) = delete;            ///< Stable owner identity; share through a reference instead.
        /// Register transactionally before attachment on the event loop; throws RegistrationError; returns this.
        Server* tool(ToolDefinition definition, ToolHandler handler);
        /// Retain shared ownership and validate transactionally before attachment; returns this.
        Server* tool(std::shared_ptr<Tool> handler);
        /// Register transactionally before attachment on the event loop; throws RegistrationError; returns this.
        Server* resource(ResourceDefinition definition, ResourceHandler handler);
        /// Retain shared ownership and validate transactionally before attachment; returns this.
        Server* resource(std::shared_ptr<Resource> handler);
        /// Register transactionally before attachment on the event loop; throws RegistrationError; returns this.
        Server* prompt(PromptDefinition definition, PromptHandler handler);
        /// Retain shared ownership and validate transactionally before attachment; returns this.
        Server* prompt(std::shared_ptr<Prompt> handler);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl;
        friend class detail::Access;
    };
} // namespace mcp::server
#endif
