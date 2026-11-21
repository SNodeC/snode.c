// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_PROMPT_H
#define MCP_SERVER_PROMPT_H
#include "mcp/server/Reply.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mcp::server {
    struct PromptGet; ///< Borrowed request declared in Request.h.

    /// Whether a prompt argument must be present before its handler runs.
    enum class Requirement { Optional, Required };
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
    /// Application subclass; the server retains shared ownership until its destruction.
    class Prompt {
    public:
        /// Own the definition; construct before registration on the event-loop thread.
        explicit Prompt(PromptDefinition definition);
        /// Destroy on the event loop after requests using this object have drained.
        virtual ~Prompt();
        /// Borrow the immutable definition for this object's lifetime; event-loop access only.
        const PromptDefinition& getDefinition() const;
        /// Run on the event loop with validated input; retain the shared reply for async work.
        /// Complete at most once; request references are borrowed only through this call.
        virtual void get(const PromptGet& request, const std::shared_ptr<PromptReply>& reply) = 0;

    private:
        const PromptDefinition definition;
    };
} // namespace mcp::server
#endif
