// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_TOOL_H
#define MCP_SERVER_TOOL_H
#include "mcp/server/Reply.h"
#include "mcp/server/Types.h"

#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

namespace mcp::server {
    struct ToolCall; ///< Borrowed request declared in Request.h.

    /// Owned tool metadata; raw JSON Schema remains the sole schema representation.
    struct ToolDefinition {
        std::string name;                   ///< Nonempty owned protocol identifier.
        std::string description{};          ///< Owned human-readable description.
        Json inputSchema{};                 ///< Owned supported object-input schema.
        std::optional<Json> outputSchema{}; ///< Optional owned successful-output schema.
        std::optional<std::string> title{}; ///< Optional owned display title.
        Json annotations{};                 ///< Optional protocol tool annotations; null omits them.
    };
    /// Application subclass; the server retains shared ownership until its destruction.
    class Tool {
    public:
        /// Own the definition; construct before registration on the event-loop thread.
        explicit Tool(ToolDefinition definition);
        /// Destroy on the event loop after requests using this object have drained.
        virtual ~Tool();
        /// Borrow the immutable definition for this object's lifetime; event-loop access only.
        const ToolDefinition& getDefinition() const;
        /// Run on the event loop with validated input; retain the shared reply for async work.
        /// Complete at most once; request references are borrowed only through this call.
        virtual void call(const ToolCall& request, const std::shared_ptr<ToolReply>& reply) = 0;

    private:
        const ToolDefinition definition;
    };
} // namespace mcp::server
#endif
