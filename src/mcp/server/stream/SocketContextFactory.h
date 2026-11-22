// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_STREAM_SOCKETCONTEXTFACTORY_H
#define MCP_SERVER_STREAM_SOCKETCONTEXTFACTORY_H
#include "core/socket/stream/SocketContextFactory.h"
namespace mcp::server {
    class Server;
    class Config;
} // namespace mcp::server
namespace mcp::server::stream {
    /// Original stream factory for newline MCP; independent of descriptor/network transport selection.
    class SocketContextFactory final : public core::socket::stream::SocketContextFactory {
    public:
        /// Borrow the registry and CLI limits until every attached connection drains; event-loop thread only.
        SocketContextFactory(Server& server, Config& config);
        /// Destroy after connections created by this factory have drained.
        ~SocketContextFactory() override;
        /// Seal registration and return a connection-owned context; registration errors propagate to attachment status.
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* connection) override;

    private:
        Server& server;
        Config& config;
    };
} // namespace mcp::server::stream
#endif // MCP_SERVER_STREAM_SOCKETCONTEXTFACTORY_H
