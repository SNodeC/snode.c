// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_HTTP_ROUTER_H
#define MCP_SERVER_HTTP_ROUTER_H
#include "express/Router.h"
namespace mcp::server {
    class Server;
    class Config;
    namespace http {
        class Router : public express::Router {
        public:
            using Authorize = std::function<bool(const std::shared_ptr<express::Request>&)>;
            Router(Server& server, Config& config, Authorize authorize = {});
        };
    } // namespace http
} // namespace mcp::server
#endif
