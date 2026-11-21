// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#pragma once
#include "mcp/server/Server.h"

#include <chrono>
#include <map>
#include <variant>
namespace mcp::server::detail {
    inline constexpr const char* modernVersion = "2026-07-28";
    inline constexpr const char* legacyVersion = "2025-11-25";
    inline constexpr const char* versionKey = "io.modelcontextprotocol/protocolVersion";
    inline constexpr const char* capabilitiesKey = "io.modelcontextprotocol/clientCapabilities";
    inline constexpr const char* subscriptionKey = "io.modelcontextprotocol/subscriptionId";

} // namespace mcp::server::detail
namespace mcp::server::detail {
    struct Response {
    public:
        using Output = std::function<void(const Json&, bool)>;
        Response(Json id, bool modern, bool cacheable, Output output, std::chrono::milliseconds timeout);
        void result(Json value);
        void error(int code, const std::string& message, Json data = nullptr);
        void notify(const std::string& method, Json params);
        void cancel(bool serverInitiated = false);
        bool active() const;
        bool expired() const;
        void expire();
        void listen();
        bool isSubscription() const;

    public:
        Json outputSchema;
        Json id;
        bool modern;
        bool cacheable;
        bool toolRequest = false;
        bool done = false;
        bool subscription = false;
        Output output;
        std::chrono::steady_clock::time_point deadline;
    };

    class Access {
    public:
        using Response = detail::Response;
        using Registry = Server::Impl;
        static Registry& registry(const Server& server);
        template <class Result>
        static Response& response(const Reply<Result>& reply) {
            return *reply.impl;
        }
        template <class Result>
        static std::shared_ptr<Reply<Result>> wrap(std::shared_ptr<Response> response) {
            return std::shared_ptr<Reply<Result>>(new Reply<Result>(std::move(response)));
        }
    };
} // namespace mcp::server::detail
namespace mcp::server {
    struct Server::Impl {
    public:
        using Handler = std::variant<Server::ToolHandler, Server::ResourceHandler, Server::PromptHandler>;
        explicit Impl(std::string name);
        void tool(Json description, Server::ToolHandler handler);
        void resource(Json description, Server::ResourceHandler handler);
        void prompt(Json description, Server::PromptHandler handler);
        bool valid() const;
        std::shared_ptr<detail::Access::Response> dispatch(const Json& request,
                                                           bool legacy,
                                                           detail::Access::Response::Output output,
                                                           std::chrono::milliseconds timeout = std::chrono::seconds(30)) const;
        static bool checkSchema(const Json& schema, std::string& error);
        static bool validate(const Json& schema, const Json& value, std::string& error);
        static bool validRequest(const Json& request);
        static int httpStatus(const Json& message);

    private:
        enum class Kind { Tool, Resource, Prompt };
        struct Entry {
            Kind kind;
            Json description;
            Handler handler;
        };
        void add(Kind kind, Json description, Handler handler);
        Json capabilities() const;
        std::string name;
        bool registrationsValid = true;
        std::map<std::string, Entry> entries;
    };
} // namespace mcp::server
