// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_SERVER_H
#define MCP_SERVER_SERVER_H
#include "mcp/server/Reply.h"
#include "mcp/server/Request.h"
#include "mcp/server/Types.h"
#include <functional>
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <vector>
#include <string>
namespace mcp::server {
    namespace detail {
        class Access; ///< Module-private registry access.
    }
    /// Registration category used in immediate validation diagnostics.
    enum class RegistrationKind { Tool, Resource, Prompt };
    /// Whether a prompt argument must be present before its handler runs.
    enum class Requirement { Optional, Required };
    /// Owned tool metadata; raw JSON Schema remains the sole schema representation.
    struct ToolDefinition {
        std::string name;                   ///< Nonempty owned protocol identifier.
        std::string description{};          ///< Owned human-readable description.
        Json inputSchema{};                 ///< Owned supported object-input schema.
        std::optional<Json> outputSchema{}; ///< Optional owned successful-output schema.
        std::optional<std::string> title{}; ///< Optional owned display title.
        Json annotations{};                 ///< Optional protocol tool annotations; null omits them.
    };
    /// Owned resource metadata; the URI is the registration key.
    struct ResourceDefinition {
        std::string uri;                    ///< Nonempty owned resource URI.
        std::string name;                   ///< Nonempty owned protocol name.
        std::string description{};          ///< Owned human-readable description.
        std::string mimeType{};             ///< Owned resource MIME type.
        std::optional<std::string> title{}; ///< Optional owned display title.
    };
    /// Owned prompt argument metadata; names must be nonempty and unique.
    struct PromptArgument {
        std::string name;                                ///< Owned argument name.
        std::string description{};                       ///< Owned human-readable description.
        Requirement requirement = Requirement::Optional; ///< Required arguments are checked before dispatch.
    };
    /// Owned prompt metadata; its handler receives only string-valued arguments.
    struct PromptDefinition {
        std::string name;                        ///< Nonempty owned protocol identifier.
        std::string description{};               ///< Owned human-readable description.
        std::vector<PromptArgument> arguments{}; ///< Owned ordered argument declarations.
        std::optional<std::string> title{};      ///< Optional owned display title.
    };
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
        /// Register transactionally before attachment on the event loop; throws RegistrationError; returns this.
        Server* resource(ResourceDefinition definition, ResourceHandler handler);
        /// Register transactionally before attachment on the event loop; throws RegistrationError; returns this.
        Server* prompt(PromptDefinition definition, PromptHandler handler);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl;
        friend class detail::Access;
    };
} // namespace mcp::server
#endif
