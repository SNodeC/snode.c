// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_HTTP_ROUTER_H
#define MCP_SERVER_HTTP_ROUTER_H
#include "express/Router.h"
#include "mcp/server/Types.h"
#include <optional>
#include <string>
#include <variant>
namespace mcp::server {
    class Server; ///< Registry borrowed through transport drain.
    class Config; ///< Configuration borrowed through transport drain.
    namespace http {
        /// Accepted request; an absent principal deliberately permits anonymous access.
        struct Authorized {
            std::optional<Principal> principal; ///< Owned authenticated identity, copied into the request context.
        };
        /// Authentication required; emit 401 with the supplied nonempty, header-safe challenge.
        struct Unauthorized {
            std::string wwwAuthenticate; ///< Owned WWW-Authenticate value; CR/LF and control bytes are rejected.
        };
        struct Forbidden {}; ///< Deliberate 403 denial without an authentication challenge.
        using Authorization = std::variant<Authorized, Unauthorized, Forbidden>; ///< Owned admission decision for one HTTP request.
        /// Express router; copy/mount it while the borrowed registry and config remain alive.
        class Router : public express::Router {
        public:
            /// Runs on the event loop after Origin validation; exceptions become 500 without credential logging.
            using Authorize = std::function<Authorization(const std::shared_ptr<express::Request>&)>;
            /// Attach on the event loop after init; seals registration; empty authorize permits anonymous access.
            /// Retain server and config until this router and its operations have drained.
            Router(Server& server, Config& config, Authorize authorize = {});
        };
    } // namespace http
} // namespace mcp::server
#endif
