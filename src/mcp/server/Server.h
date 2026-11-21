// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef MCP_SERVER_SERVER_H
#define MCP_SERVER_SERVER_H
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
// IWYU pragma: no_include <nlohmann/json_fwd.hpp>
namespace mcp::server {
    /// Application JSON value, owned by value; use on the event-loop thread.
    using Json = nlohmann::json;
    namespace detail {
        class Access;
    }
    /// Shared asynchronous completion handle; created only by the server.
    class Reply {
    public:
        /// Release this handle; outstanding server work retains its own state.
        ~Reply();
        /// Complete at most once on the event-loop thread with an owned result.
        void result(Json value);
        /// Complete at most once on the event-loop thread with an error.
        void error(int code, const std::string& message, Json data = nullptr);
        /// Observe completion on the event-loop thread without mutation.
        bool active() const;
        /// Observe the deadline on the event-loop thread without mutation.
        bool expired() const;
        /// Apply the deadline on the event-loop thread; temporary migration API.
        void expire();

    private:
        struct Impl;
        explicit Reply(std::shared_ptr<Impl> impl);
        std::shared_ptr<Impl> impl;
        friend class detail::Access;
    };
    /// Registry owner; retain through every attached transport's shutdown.
    class Server {
    public:
        /// Event-loop callback; retain the shared reply for asynchronous work.
        using Handler = std::function<void(const Json&, const std::shared_ptr<Reply>&)>;
        /// Construct before attachment; owns its name and registration data.
        explicit Server(std::string name);
        /// Destroy only after attached transports have drained.
        ~Server();
        /// Register before attachment on the event-loop thread.
        void tool(Json description, Handler handler);
        /// Register before attachment on the event-loop thread.
        void resource(Json description, Handler handler);
        /// Register before attachment on the event-loop thread.
        void prompt(Json description, Handler handler);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl;
        friend class detail::Access;
    };
} // namespace mcp::server
#endif
