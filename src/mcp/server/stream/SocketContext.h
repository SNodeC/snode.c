// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_STREAM_SOCKETCONTEXT_H
#define MCP_SERVER_STREAM_SOCKETCONTEXT_H
#include "core/socket/stream/SocketContext.h"

#include <cstddef>
#include <memory>
namespace core::socket::stream {
    class SocketConnection;
}
namespace mcp::server {
    class Server;
    class Config;
} // namespace mcp::server
namespace mcp::server::stream {
    /// Newline-framed MCP over the original stream engine; server/config must outlive the connection.
    class SocketContext final : public core::socket::stream::SocketContext {
    public:
        /// Construct on the event-loop thread; the original connection owns and attaches this context.
        SocketContext(core::socket::stream::SocketConnection* connection, Server& server, Config& config);
        /// Destroy through the original detach path after pending requests are cancelled.
        ~SocketContext() override;

    private:
        void onConnected() override;
        void onDisconnected() override;
        std::size_t onReceivedFromPeer() override;
        void onReadError(int error) override;
        void onWriteError(int error) override;
        bool onSignal(int signal) override;
        struct State;
        std::shared_ptr<State> state;
    };
} // namespace mcp::server::stream
#endif // MCP_SERVER_STREAM_SOCKETCONTEXT_H
