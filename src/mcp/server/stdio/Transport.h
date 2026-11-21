// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_STDIO_TRANSPORT_H
#define MCP_SERVER_STDIO_TRANSPORT_H
#include <functional>
namespace mcp::server {
    class Server; ///< Borrowed registry; retain until run returns.
    class Config; ///< Borrowed CLI configuration; create before run and retain until it returns.
    namespace stdio {
        /// Process-owning stdio entry; no independently constructible transport instance.
        class Transport {
        public:
            /// Called at most once after successful init on the event loop to register handlers.
            using Ready = std::function<void()>;
            /// Call once in core state LOADED; otherwise returns failure before touching descriptors.
            /// Owns stdout retention, foreground init, Ready, attachment, event loop and cleanup.
            /// Borrows argv/server/config through return; catches exceptions and returns failure.
            static int run(int argc, char* argv[], Server& server, Config& config, Ready ready);

        private:
            Transport() = delete;
            struct State;
            static int retainProtocol();
        };
    } // namespace stdio
} // namespace mcp::server
#endif
