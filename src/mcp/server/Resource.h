// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_RESOURCE_H
#define MCP_SERVER_RESOURCE_H
#include "mcp/server/Reply.h"
#include "mcp/server/Request.h"
namespace mcp::server {
    /// Owned resource metadata; the URI is the registration key.
    struct ResourceDefinition {
        std::string uri;                    ///< Nonempty owned resource URI.
        std::string name;                   ///< Nonempty owned protocol name.
        std::string description{};          ///< Owned human-readable description.
        std::string mimeType{};             ///< Owned resource MIME type.
        std::optional<std::string> title{}; ///< Optional owned display title.
    };
    /// Application subclass; the server retains shared ownership until its destruction.
    class Resource {
    public:
        /// Own the definition; construct before registration on the event-loop thread.
        explicit Resource(ResourceDefinition definition);
        /// Destroy on the event loop after requests using this object have drained.
        virtual ~Resource();
        /// Borrow the immutable definition for this object's lifetime; event-loop access only.
        const ResourceDefinition& getDefinition() const;
        /// Run on the event loop with validated input; retain the shared reply for async work.
        /// Complete at most once; request references are borrowed only through this call.
        virtual void read(const ResourceRead& request, const std::shared_ptr<ResourceReply>& reply) = 0;

    private:
        const ResourceDefinition definition;
    };
} // namespace mcp::server
#endif
