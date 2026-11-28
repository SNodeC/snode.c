/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include "core/timer/Timer.h"
#include "mcp/Protocol.h"
#include "mcp/detail/Tasks.h"
#include "mcp/server/Server.h"
#include "mcp/server/detail/UriTemplate.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <set>
#include <variant>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace jsonrpc {
    class JsonRpcContext;

    namespace messages {
        struct Request;
    }
} // namespace jsonrpc

namespace mcp::server {
    class Mcp;
}

namespace mcp::server::detail {

    using mcp::capabilitiesKey;
    using mcp::ProtocolEra;
    using mcp::protocolVersion2025;
    using mcp::protocolVersion2026;
    using mcp::subscriptionKey;
    using mcp::versionKey;
    enum class CachePolicy { None, Private };
    enum class SubscriptionClosure { Complete, Cancel };

    struct Response;
    using mcp::detail::Task;
    using mcp::detail::Tasks;

    struct LogStream {
        LogLevel threshold = LogLevel::Info;
        utils::Timeval reset;
        std::size_t count = 0;
        Json message(LogLevel level, Json data, const std::string& logger);
    };

    struct Channel {
        struct Input {
            std::weak_ptr<Response> response;
            std::string key;
        };

        bool log(LogLevel level, Json data, const std::string& logger);
        LogStream logging;
        bool respond(const Json& message);
        bool complete(const Json& message);
        void forget(const Response& response);

        void rootsChanged() const;
        void progress(const Json& params) const;
        std::function<void()> onRootsChanged;
        Json capabilities = Json::object();
        std::map<std::string, Input> inputs;
        std::uint64_t nextInput = 0;
        std::set<std::string> resources;
        std::function<void(const Json&)> output;
        std::string credential;
        bool active = true;
        bool resourceSubscriptions = true;
    };

    struct Response final
        : mcp::detail::TaskExecution
        , std::enable_shared_from_this<Response> {
        using Output = std::function<void(const Json&, bool)>;

        Response(Json id, ProtocolEra era, CachePolicy cachePolicy, Output output, const utils::Timeval& timeout);

        ~Response() final;

        Completion result(Json value) final;
        Completion inputRequired(ClientInputRequired input);
        Completion requestsRequired(Json requests, std::optional<std::string> state);
        bool acceptInput(const std::string& key, const Json& value) final;
        Completion error(int code, const std::string& message, Json data = nullptr) final;
        void notify(const std::string& method, Json params);
        void cancel(CancelReason reason = CancelReason::Client) final;
        std::shared_ptr<const void> session() const final;
        void sendTaskMessage(const Json& message) final;
        void observe(RequestContext::OnCancel callback);
        void expire();
        void listen(Json notifications = Json::object());

        std::optional<ProgressToken> progressToken;
        std::optional<CancelReason> cancellation;
        std::vector<RequestContext::OnCancel> observers;
        std::optional<double> lastProgress;
        Json outputSchema;
        std::optional<LogStream> logging;
        std::map<std::string, InputResponse> inputResponses;
        std::map<std::string, Json> clientInputResponses;
        Json clientRequests = Json::object();

        struct ClientProgress {
            Json token;
            std::optional<Progress> latest;
            std::function<void(const Progress&)> observer;
        };

        std::map<std::string, ClientProgress> clientProgress;
        std::weak_ptr<Channel> channel;
        std::function<Completion(TaskOptions)> startTask;
        Mcp* protocol = nullptr;
        std::shared_ptr<jsonrpc::JsonRpcContext> protocolOwner;
        CachePolicy cachePolicy;
        bool toolRequest = false;
        bool subscription = false;
        SubscriptionClosure closure = SubscriptionClosure::Complete;
        Json notifications;
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
        using Handler =
            std::variant<std::shared_ptr<Tool>, std::shared_ptr<Resource>, std::shared_ptr<Prompt>, std::shared_ptr<ResourceTemplate>>;

        Impl();
        ~Impl();
        void attach();
        void endChannel(const std::shared_ptr<detail::Channel>& channel) const;
        bool remove(RegistrationKind kind, const std::string& identifier);
        void add(RegistrationKind kind, Json description, Handler handler);
        std::shared_ptr<detail::Access::Response> dispatch(const Json& request,
                                                           detail::ProtocolEra era,
                                                           detail::Access::Response::Output output,
                                                           const utils::Timeval& timeout = utils::Timeval(30),
                                                           std::optional<Principal> principal = {},
                                                           std::shared_ptr<detail::Channel> channel = {},
                                                           std::size_t pageSize = 128) const;
        std::shared_ptr<detail::Access::Response> dispatchRequest(const jsonrpc::messages::Request& request,
                                                                  detail::ProtocolEra era,
                                                                  detail::Access::Response::Output output,
                                                                  const utils::Timeval& timeout = utils::Timeval(30),
                                                                  std::optional<Principal> principal = {},
                                                                  std::shared_ptr<detail::Channel> channel = {},
                                                                  Mcp* protocol = nullptr,
                                                                  std::size_t pageSize = 128) const;
        Json toolInputSchema(const std::string& name) const;
        void resourceUpdated(const std::string& uri);
        void log(LogLevel level, const Json& data, const std::string& logger);
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
            std::optional<detail::UriTemplate> matcher{};
        };

        Json capabilities() const;
        Json
        paginate(const std::string& method, const Json& params, const Json& list, const std::string& field, std::size_t pageSize) const;
        void changed(RegistrationKind kind);
        void flushChanges();
        void taskChanged(const std::shared_ptr<detail::Task>& task);
        const Entry* resolveResource(const std::string& uri, std::map<std::string, std::string>& variables) const;

        std::shared_ptr<detail::Tasks> tasks;
        Json serverInfo;
        std::string instructions;
        std::uint64_t revision = 0;
        bool attached = false;
        std::set<RegistrationKind> changes;
        core::timer::Timer changeTimer;
        mutable utils::Timeval completionReset;
        mutable std::size_t completionCount = 0;
        std::map<std::string, Entry> entries;
        std::vector<std::string> templates;
        mutable std::vector<std::weak_ptr<detail::Response>> subscriptions;
        mutable std::vector<std::weak_ptr<detail::Channel>> channels;
    };

} // namespace mcp::server
