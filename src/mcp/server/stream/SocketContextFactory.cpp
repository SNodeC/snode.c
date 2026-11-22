// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "mcp/server/stream/SocketContextFactory.h"

#include "mcp/server/detail/Server.h"
#include "mcp/server/stream/SocketContext.h"
namespace mcp::server::stream {
    SocketContextFactory::SocketContextFactory(Server& server, Config& config)
        : server(server)
        , config(config) {
    }
    SocketContextFactory::~SocketContextFactory() = default;
    core::socket::stream::SocketContext* SocketContextFactory::create(core::socket::stream::SocketConnection* connection) {
        detail::Access::registry(server).seal();
        return new SocketContext(connection, server, config);
    }
} // namespace mcp::server::stream
