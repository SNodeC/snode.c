// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_STDIO_TRANSPORT_H
#define MCP_SERVER_STDIO_TRANSPORT_H
#include <memory>
namespace mcp::server {
    class Server;
    class Config;
    namespace stdio {
        class Transport {
        public:
            static int retainProtocol();
            Transport(Server& server, Config& config, int protocolFd);
            ~Transport();

        private:
            struct State;
            std::shared_ptr<State> state;
        };
    } // namespace stdio
} // namespace mcp::server
#endif
