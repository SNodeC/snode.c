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
        class Access;    ///< Module-private construction access.
        struct Response; ///< Shared opaque request state.
    } // namespace detail
    /// Copyable request handle; all access and final release occur on the event loop.
    class RequestContext {
    public:
        /// Once-only observer on cancellation; never called after successful completion.
        using OnCancel = std::function<void(CancelReason)>;
        /// Borrow the authenticated identity while this context remains alive.
        const std::optional<Principal>& getPrincipal() const;
        /// Borrow the original typed progress token while this context remains alive.
        const std::optional<ProgressToken>& getProgressToken() const;
        /// Register on the event loop; late observers run immediately after cancellation.
        /// Observer exceptions are contained and logged; captures live until termination.
        void onCancel(OnCancel callback) const;

    private:
        explicit RequestContext(std::shared_ptr<detail::Response> impl);
        std::shared_ptr<detail::Response> impl;
        friend class detail::Access;
    };
    /// Owned tool input; arguments have passed the registered schema.
    struct ToolCall {
        RequestContext context; ///< Shared event-loop request context.
        Json arguments;         ///< Owned validated argument object.
    };
    /// Owned resource input, independent of the protocol envelope.
    struct ResourceRead {
        RequestContext context; ///< Shared event-loop request context.
        std::string uri;        ///< Owned requested resource URI.
    };
    /// Owned prompt input; all argument values are protocol strings.
    struct PromptGet {
        RequestContext context;                       ///< Shared event-loop request context.
        std::map<std::string, std::string> arguments; ///< Owned named string arguments.
    };
} // namespace mcp::server
#endif
