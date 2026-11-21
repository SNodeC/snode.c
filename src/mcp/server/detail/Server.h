// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#pragma once
#include "mcp/server/Server.h"

#include "core/timer/Timer.h"
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
    struct Response : std::enable_shared_from_this<Response> {
    public:
        using Output = std::function<void(const Json&, bool)>;
        Response(Json id, bool modern, bool cacheable, Output output, const utils::Timeval& timeout);
        ~Response();
        void result(Json value);
        void error(int code, const std::string& message, Json data = nullptr);
        void notify(const std::string& method, Json params);
        void cancel(CancelReason reason = CancelReason::Client);
        void observe(RequestContext::OnCancel callback);
        bool active() const;
        bool expired() const;
        void expire();
        void listen();
        bool isSubscription() const;

    public:
        std::optional<Principal> principal;
        std::optional<ProgressToken> progressToken;
        std::optional<CancelReason> cancellation;
        std::vector<RequestContext::OnCancel> observers;
        core::timer::Timer timer;
        std::optional<double> lastProgress;
        Json outputSchema;
        Json id;
        bool modern;
        bool cacheable;
        bool toolRequest = false;
        bool done = false;
        bool subscription = false;
        Output output;
        utils::Timeval deadline;
    };

    class Access {
    public:
        using Response = detail::Response;
        using Registry = Server::Impl;
        static Registry& registry(const Server& server);
        static RequestContext context(std::shared_ptr<Response> response) {
            return RequestContext(std::move(response));
        }
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
        explicit Impl(std::string name, std::string version, std::string instructions);
        void seal();
        void add(RegistrationKind kind, Json description, Handler handler);
        std::shared_ptr<detail::Access::Response> dispatch(const Json& request,
                                                           bool legacy,
                                                           detail::Access::Response::Output output,
                                                           const utils::Timeval& timeout = utils::Timeval(30),
                                                           std::optional<Principal> principal = {}) const;
        static bool checkSchema(const Json& schema, std::string& error);
        static bool validate(const Json& schema, const Json& value, std::string& error);
        static bool validRequest(const Json& request);
        static int httpStatus(const Json& message);

    private:
        using Kind = RegistrationKind;
        struct Entry {
            Kind kind;
            Json description;
            Handler handler;
        };
        Json capabilities() const;
        std::string name;
        std::string version, instructions;
        bool sealed = false;
        std::map<std::string, Entry> entries;
    };
} // namespace mcp::server
