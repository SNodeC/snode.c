// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_REPLY_H
#define MCP_SERVER_REPLY_H
#include "mcp/server/Results.h"

#include <memory>
#include <string>
namespace mcp::server {
    namespace detail {
        class Access;    ///< Module-private construction access.
        struct Response; ///< Shared opaque request and completion state.
    } // namespace detail
    /// Share the handle for asynchronous work; construction is module-private.
    template <class Result>
    class Reply {
    public:
        Reply(const Reply&) = delete;            ///< Non-transferable handle; share ownership through std::shared_ptr instead.
        Reply& operator=(const Reply&) = delete; ///< Non-transferable handle; share ownership through std::shared_ptr instead.
        Reply(Reply&&) = delete;                 ///< Non-transferable handle; share ownership through std::shared_ptr instead.
        Reply& operator=(Reply&&) = delete;      ///< Non-transferable handle; share ownership through std::shared_ptr instead.
        /// Complete once with the matching result kind on the event-loop thread.
        Completion result(Result result);
        /// Complete once with an application error on the event-loop thread.
        Completion error(ErrorCode code, std::string message, Json data = nullptr);
        /// Send progress on the event loop with the original typed token; no token is a no-op.
        void progress(Progress progress);

    private:
        explicit Reply(std::shared_ptr<detail::Response> impl);
        std::shared_ptr<detail::Response> impl;
        friend class detail::Access;
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
