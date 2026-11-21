// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#pragma once
#include "mcp/server/Server.h"

#include "core/timer/Timer.h"
#include <map>
#include <variant>
namespace mcp::server::detail {
    enum class ProtocolEra { Initialization20251125, PerRequest20260728 };
    enum class CachePolicy { None, Private };
    inline constexpr const char* protocolVersion2026 = "2026-07-28";
    inline constexpr const char* protocolVersion2025 = "2025-11-25";
    inline constexpr const char* versionKey = "io.modelcontextprotocol/protocolVersion";
    inline constexpr const char* capabilitiesKey = "io.modelcontextprotocol/clientCapabilities";
    inline constexpr const char* subscriptionKey = "io.modelcontextprotocol/subscriptionId";

    struct Response : std::enable_shared_from_this<Response> {
        using Output = std::function<void(const Json&, bool)>;
        Response(Json id, ProtocolEra era, CachePolicy cachePolicy, Output output, const utils::Timeval& timeout);
        ~Response();
        Completion result(Json value);
        Completion error(int code, const std::string& message, Json data = nullptr);
        void notify(const std::string& method, Json params);
        void cancel(CancelReason reason = CancelReason::Client);
        void observe(RequestContext::OnCancel callback);
        void expire();
        void listen();

        std::optional<Principal> principal;
        std::optional<ProgressToken> progressToken;
        std::optional<CancelReason> cancellation;
        std::vector<RequestContext::OnCancel> observers;
        core::timer::Timer timer;
        std::optional<double> lastProgress;
        Json outputSchema;
        Json id;
        ProtocolEra era;
        CachePolicy cachePolicy;
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
        using Handler = std::variant<std::shared_ptr<Tool>, std::shared_ptr<Resource>, std::shared_ptr<Prompt>>;
        void seal();
        void add(RegistrationKind kind, Json description, Handler handler);
        std::shared_ptr<detail::Access::Response> dispatch(const Json& request,
                                                           detail::ProtocolEra era,
                                                           detail::Access::Response::Output output,
                                                           const utils::Timeval& timeout = utils::Timeval(30),
                                                           std::optional<Principal> principal = {}) const;
        static bool checkSchema(const Json& schema, std::string& error);
        static bool validate(const Json& schema, const Json& value, std::string& error);
        static bool validRequest(const Json& request);
        static int httpStatus(const Json& message);

    private:
        friend class Server;
        using Kind = RegistrationKind;
        struct Entry {
            Kind kind;
            Json description;
            Handler handler;
        };
        Json capabilities() const;
        Json serverInfo;
        std::string instructions;
        bool sealed = false;
        std::map<std::string, Entry> entries;
    };
} // namespace mcp::server
