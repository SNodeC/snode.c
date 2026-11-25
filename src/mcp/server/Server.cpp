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

#include "mcp/server/Server.h"

#include "Log.h"
#include "core/timer/Timer.h"
#include "jsonrpc/messages/Id.h"
#include "jsonrpc/messages/Notification.h"
#include "jsonrpc/messages/Request.h"
#include "jsonrpc/messages/Result.h"
#include "mcp/Protocol.h"
#include "mcp/Schema.h"
#include "mcp/Types.h"
#include "mcp/detail/Serialization.h"
#include "mcp/server/Mcp.h"
#include "mcp/server/Reply.h"
#include "mcp/server/Request.h"
#include "mcp/server/Results.h"
#include "mcp/server/Types.h"
#include "mcp/server/detail/Server.h"
#include "mcp/server/detail/UriTemplate.h"
#include "utils/base64.h"

#include <compare>
#include <cstddef>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>
// IWYU pragma: no_include <nlohmann/detail/iterators/iteration_proxy.hpp>
// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace mcp::server {

    using namespace detail;
    using mcp::detail::display;
    using mcp::detail::encode;
    using mcp::detail::metadata;

    namespace {
        void registrationMetadata(Json& value, const Json& meta, RegistrationKind kind, const std::string& identifier) {
            try {
                metadata(value, meta);
            } catch (const std::invalid_argument& error) {
                throw RegistrationError(kind, identifier, "_meta", error.what());
            }
        }

        Json registrationAnnotations(const Annotations& annotations, RegistrationKind kind, const std::string& identifier) {
            Json value;
            try {
                value = encode(annotations);
            } catch (const std::invalid_argument& error) {
                throw RegistrationError(kind, identifier, "annotations", error.what());
            }

            return value;
        }

    } // namespace

    namespace {

        class InputValidationError final : public std::invalid_argument {
        public:
            using std::invalid_argument::invalid_argument;
            ~InputValidationError() final;
        };

        InputValidationError::~InputValidationError() = default;

    } // namespace

    detail::Response::Response(Json id, ProtocolEra era, CachePolicy cachePolicy, Output output, const utils::Timeval& timeout)
        : id(jsonrpc::messages::isValidId(id) ? std::move(id) : Json())
        , era(era)
        , cachePolicy(cachePolicy)
        , output(std::move(output))
        , deadline(utils::Timeval::currentTime() + timeout) {
    }

    detail::Response::~Response() {
        timer.cancel();
    }

    void detail::Response::expire() {
        if (!done && utils::Timeval::currentTime() >= deadline && !subscription) {
            cancel(CancelReason::Deadline);
        }
    }

    Completion detail::Response::result(Json value) {
        Completion completion = Completion::Inactive;
        const auto keep = weak_from_this().lock();
        if (!cancellation) {
            expire();
        }

        if (!done) {
            std::string validationError;
            if (value.value("resultType", Json()) != "input_required" && !outputSchema.is_null() &&
                value.value("isError", Json()) != true &&
                (!value.contains("structuredContent") ||
                 !Access::Registry::validate(outputSchema, value["structuredContent"], validationError))) {
                value = {{"isError", true}, {"content", {{{"type", "text"}, {"text", "Tool output failed its registered schema"}}}}};
            }

            if (era == ProtocolEra::Initialization20251125 && value.contains("structuredContent") &&
                !value["structuredContent"].is_object()) {
                value.erase("structuredContent");
            }

            if (era == ProtocolEra::PerRequest20260728) {
                if (!value.contains("resultType")) {
                    value["resultType"] = "complete";
                }
                if (cachePolicy == CachePolicy::Private && value["resultType"] == "complete") {
                    if (!value.contains("ttlMs")) {
                        value["ttlMs"] = 0;
                        value["cacheScope"] = "private";
                    }
                }
            }

            if (era == ProtocolEra::PerRequest20260728 && !serverInfo.is_null()) {
                value["_meta"]["io.modelcontextprotocol/serverInfo"] = serverInfo;
            }

            if (const auto current = channel.lock()) {
                current->forget(*this);
            }

            done = true;
            completion = Completion::Accepted;
            timer.cancel();
            observers.clear();
            output(jsonrpc::messages::Result{id, std::move(value)}.toJson(), true);
        }

        return completion;
    }

    Completion detail::Response::inputRequired(InputRequired input) {
        Completion completion = Completion::Inactive;
        const auto keep = weak_from_this().lock();
        expire();
        if (!done) {
            if (input.requests.size() > 16 || (input.requests.empty() && !input.requestState) ||
                (input.requestState && input.requestState->size() > 16384)) {
                completion = error(-32603, "Invalid continuation size or missing input/state");
            } else {
                Json value{{"resultType", "input_required"}};
                Json missing = Json::object();
                bool valid = true;
                std::string validationError;
                const auto capability = clientCapabilities.value("elicitation", Json());
                for (const auto& [key, request] : input.requests) {
                    std::visit(
                        [&](const auto& item) {
                            using Input = std::decay_t<decltype(item)>;
                            const bool form = std::is_same_v<Input, FormInput>;
                            const auto mode = form ? "form" : "url";
                            if (!capability.is_object() ||
                                (!(capability.contains(mode) && capability[mode].is_object()) && !(form && capability.empty()))) {
                                missing["elicitation"][mode] = Json::object();
                            }

                            Json params{{"mode", mode}, {"message", item.message}};
                            valid = valid && !key.empty() && key.size() <= 256;
                            if constexpr (std::is_same_v<Input, FormInput>) {
                                valid = valid && item.requestedSchema.is_object() &&
                                        item.requestedSchema.value("type", Json()) == "object" &&
                                        item.requestedSchema.value("properties", Json()).is_object() &&
                                        Access::Registry::checkSchema(item.requestedSchema, validationError);
                                if (valid) {
                                    for (const auto& property : item.requestedSchema["properties"]) {
                                        const auto type = property.is_object() ? property.value("type", Json()) : Json();
                                        valid = valid && (type == "string" || type == "number" || type == "integer" || type == "boolean" ||
                                                          type == "array");
                                    }
                                }

                                params["requestedSchema"] = item.requestedSchema;
                            } else {
                                valid = valid && (item.url.starts_with("https://") || item.url.starts_with("http://")) &&
                                        item.url.size() <= 16384;
                                params["url"] = item.url;
                            }

                            value["inputRequests"][key] = {{"method", "elicitation/create"}, {"params", std::move(params)}};
                        },
                        request);
                }

                if (!missing.empty()) {
                    if (era == ProtocolEra::Initialization20251125 && toolRequest) {
                        completion =
                            result({{"isError", true},
                                    {"content", {{{"type", "text"}, {"text", "Client does not support the requested elicitation mode"}}}}});
                    } else {
                        completion = error(era == ProtocolEra::PerRequest20260728 ? -32021 : -32602,
                                           "Missing required client capability",
                                           {{"requiredCapabilities", std::move(missing)}});
                    }
                } else if (!valid) {
                    completion = error(-32603, "Malformed elicitation request");
                } else {
                    if (input.requestState && era == ProtocolEra::PerRequest20260728) {
                        value["requestState"] = std::move(*input.requestState);
                    }

                    if (era == ProtocolEra::PerRequest20260728) {
                        completion = result(std::move(value));
                    } else if (const auto current = channel.lock(); current && current->active && resume && !input.requests.empty()) {
                        const bool waiting = std::any_of(current->inputs.begin(), current->inputs.end(), [this](const auto& item) {
                            const auto owner = item.second.response.lock();
                            return owner.get() == this;
                        });
                        if (waiting || current->inputs.size() + input.requests.size() > 1024 ||
                            current->nextInput > UINT64_MAX - input.requests.size()) {
                            completion = error(-32603, "Elicitation already pending or capacity exceeded");
                        } else {
                            inputResponses.clear();
                            requestState = std::move(input.requestState);
                            std::vector<Json> messages;
                            for (auto& [key, request] : value["inputRequests"].items()) {
                                const Json requestId = "snodec.elicitation." + std::to_string(++current->nextInput);
                                if (request["params"]["mode"] == "url") {
                                    request["params"]["elicitationId"] = requestId;
                                }

                                current->inputs.emplace(requestId.dump(),
                                                        Channel::Input{keep, key, request["params"].value("requestedSchema", Json())});
                                messages.push_back(jsonrpc::messages::Request{
                                    requestId, "elicitation/create", std::optional<Json>(std::move(request["params"]))}
                                                       .toJson());
                            }

                            completion = Completion::Accepted;
                            for (const auto& message : messages) {
                                if (!done) {
                                    output(message, false);
                                }
                            }
                        }
                    } else {
                        completion = error(-32602, "Legacy elicitation requires a live initialized channel and input requests");
                    }
                }
            }
        }

        return completion;
    }

    Completion detail::Response::error(int code, const std::string& message, Json data) {
        Completion completion = Completion::Inactive;
        const auto keep = weak_from_this().lock();
        if (!cancellation) {
            expire();
        }

        if (!done) {
            Json response = Mcp::errorResponse(id, era, code, message, std::move(data));

            if (const auto current = channel.lock()) {
                current->forget(*this);
            }

            done = true;
            completion = Completion::Accepted;
            timer.cancel();
            observers.clear();
            output(response, true);
        }

        return completion;
    }

    void detail::Response::notify(const std::string& method, Json params) {
        if (!done) {
            if (subscription) {
                params["_meta"][subscriptionKey] = id;
            }

            output(jsonrpc::messages::Notification{method, std::optional<Json>(std::move(params))}.toJson(), false);
        }
    }

    void detail::Response::listen(Json filter) {
        subscription = true;
        notifications = std::move(filter);
        notify("notifications/subscriptions/acknowledged", {{"notifications", notifications}});
    }

    void detail::Response::observe(RequestContext::OnCancel callback) {
        if (callback) {
            if (cancellation) {
                try {
                    callback(*cancellation);
                } catch (...) {
                    snode::log::framework("mcp.server", snode::log::Boundary::Application).error() << "cancel observer failed";
                }
            } else if (!done) {
                observers.push_back(std::move(callback));
            }
        }
    }

    void detail::Response::cancel(CancelReason reason) {
        const auto keep = weak_from_this().lock();
        if (!done) {
            cancellation = reason;
            auto callbacks = std::move(observers);
            if (reason == CancelReason::Deadline) {
                if (toolRequest) {
                    result({{"isError", true}, {"content", {{{"type", "text"}, {"text", "Request deadline exceeded"}}}}});
                } else {
                    error(-32603, "Request deadline exceeded");
                }
            } else if (reason == CancelReason::Shutdown && subscription && closure == SubscriptionClosure::Complete) {
                result({{"_meta", {{subscriptionKey, id}}}});
            } else {
                if (reason == CancelReason::Shutdown && subscription) {
                    notify("notifications/cancelled", {{"requestId", id}, {"reason", "subscription closed"}});
                }

                if (const auto current = channel.lock()) {
                    current->forget(*this);
                }

                done = true;
                timer.cancel();
                output(nullptr, true);
            }

            for (auto& callback : callbacks) {
                observe(std::move(callback));
            }
        }
    }

    bool Server::Impl::checkSchema(const Json& schema, std::string& error) {
        return Schema::check(schema, error);
    }

    bool Server::Impl::validate(const Json& schema, const Json& value, std::string& error) {
        return Schema::validate(schema, value, error);
    }

    void Server::Impl::add(RegistrationKind kind, Json description, Handler handler) {
        const std::string keyName = kind == Kind::Resource ? "uri" : kind == Kind::ResourceTemplate ? "uriTemplate" : "name";
        const std::string identifier = description.value(keyName, std::string());
        const std::string key = std::to_string(static_cast<int>(kind)) + ":" + identifier;
        const auto reject = [&](const std::string& field, const std::string& reason) {
            return RegistrationError(kind, identifier, field, reason);
        };
        if (identifier.empty() || description.value("name", std::string()).empty()) {
            throw reject(identifier.empty() ? keyName : "name", "must not be empty");
        } else if (!std::visit(
                       [](const auto& callback) {
                           return static_cast<bool>(callback);
                       },
                       handler)) {
            throw reject("handler", "must be callable");
        } else if (entries.size() >= 128 || entries.contains(key)) {
            throw reject(keyName, entries.contains(key) ? "duplicate registration" : "128-registration limit exceeded");
        }

        if (kind == Kind::Tool) {
            std::string error;
            if (description.contains("annotations") && !description["annotations"].is_object()) {
                throw reject("annotations", "must be an object");
            }

            const Json& input = description["inputSchema"];
            if (!input.is_object() || input.value("type", Json()) != "object" || !checkSchema(input, error)) {
                throw reject("inputSchema", error.empty() ? "supported object schema required" : error);
            }

            if (description.contains("outputSchema") && !checkSchema(description["outputSchema"], error)) {
                throw reject("outputSchema", error.empty() ? "supported schema required" : error);
            }
        } else if (kind == Kind::Prompt) {
            std::set<std::string> names;
            for (const auto& argument : description["arguments"]) {
                const std::string name = argument["name"];
                if (name.empty() || !names.insert(name).second) {
                    throw reject("arguments.name", "must be nonempty and unique");
                }
            }
        }

        std::optional<UriTemplate> matcher;
        if (kind == Kind::ResourceTemplate) {
            try {
                matcher.emplace(identifier);
                std::set<std::string> seen;
                const auto& names = matcher->getVariables();
                for (const auto& variable : std::get<std::shared_ptr<ResourceTemplate>>(handler)->getDefinition().variables) {
                    if (!seen.insert(variable.name).second || std::find(names.begin(), names.end(), variable.name) == names.end()) {
                        throw std::invalid_argument("completer variable must be unique and present in template");
                    }
                }
            } catch (const std::invalid_argument& error) {
                throw reject("uriTemplate", error.what());
            }
        }

        entries.emplace(key, Entry{kind, std::move(description), std::move(handler), std::move(matcher)});
        if (kind == Kind::ResourceTemplate) {
            templates.push_back(key);
        }
        changed(kind);
    }

    Server::Impl::~Impl() {
        changeTimer.cancel();
    }

    void Server::Impl::attach() {
        attached = true;
    }

    bool Server::Impl::remove(RegistrationKind kind, const std::string& identifier) {
        const std::string key = std::to_string(static_cast<int>(kind)) + ":" + identifier;
        const bool removed = entries.erase(key) != 0;
        if (removed) {
            std::erase(templates, key);
            changed(kind);
        }
        return removed;
    }

    bool Server::remove(RegistrationKind kind, const std::string& identifier) {
        return impl->remove(kind, identifier);
    }

    void Server::Impl::changed(RegistrationKind kind) {
        ++revision;
        if (attached) {
            const bool schedule = changes.empty();
            changes.insert(kind == Kind::ResourceTemplate ? Kind::Resource : kind);
            if (schedule) {
                changeTimer = core::timer::Timer::singleshotTimer(
                    [this] {
                        flushChanges();
                    },
                    utils::Timeval(0));
            }
        }
    }

    void Server::Impl::flushChanges() {
        // Detach the running receiver before callbacks can schedule the next turn.
        const auto running = std::move(changeTimer);
        const auto pending = std::exchange(changes, {});
        std::vector<std::shared_ptr<Response>> streams;
        std::vector<std::shared_ptr<Channel>> clients;
        std::erase_if(subscriptions, [](const auto& weak) {
            return weak.expired();
        });
        std::erase_if(channels, [](const auto& weak) {
            return weak.expired();
        });
        for (const auto& weak : subscriptions) {
            if (const auto current = weak.lock(); current && !current->done) {
                streams.push_back(current);
            }
        }
        for (const auto& weak : channels) {
            if (const auto current = weak.lock(); current && current->active) {
                clients.push_back(current);
            }
        }
        for (const auto kind : pending) {
            const std::string category = kind == Kind::Tool ? "tools" : kind == Kind::Prompt ? "prompts" : "resources";
            const std::string filter = category + "ListChanged";
            const std::string method = "notifications/" + category + "/list_changed";
            for (const auto& stream : streams) {
                if (stream->notifications.value(filter, false)) {
                    stream->notify(method, Json::object());
                }
            }
            for (const auto& client : clients) {
                if (client->active && client->output) {
                    client->output(jsonrpc::messages::Notification{method}.toJson());
                }
            }
        }
    }

    Json Server::Impl::capabilities() const {
        Json result{{"tools", {{"listChanged", true}}}, {"resources", {{"listChanged", true}}}, {"prompts", {{"listChanged", true}}}};
        for (const auto& [key, entry] : entries) {
            static_cast<void>(key);
            if (entry.kind == Kind::Prompt) {
                for (const auto& argument : std::get<std::shared_ptr<Prompt>>(entry.handler)->getDefinition().arguments) {
                    if (argument.complete) {
                        result["completions"] = Json::object();
                    }
                }
            } else if (entry.kind == Kind::ResourceTemplate) {
                for (const auto& variable : std::get<std::shared_ptr<ResourceTemplate>>(entry.handler)->getDefinition().variables) {
                    if (variable.complete) {
                        result["completions"] = Json::object();
                    }
                }
            }
        }

        return result;
    }

    const Server::Impl::Entry* Server::Impl::resolveResource(const std::string& uri, std::map<std::string, std::string>& variables) const {
        const Entry* result = nullptr;
        const auto exact = entries.find(std::to_string(static_cast<int>(Kind::Resource)) + ":" + uri);
        if (exact != entries.end()) {
            result = &exact->second;
        } else {
            for (auto candidate = templates.begin(); candidate != templates.end() && result == nullptr; ++candidate) {
                const auto& entry = entries.at(*candidate);
                if (auto matched = entry.matcher->match(uri)) {
                    result = &entry;
                    variables = std::move(*matched);
                }
            }
        }

        return result;
    }

    std::shared_ptr<detail::Access::Response> Server::Impl::dispatchRequest(const jsonrpc::messages::Request& request,
                                                                            ProtocolEra era,
                                                                            detail::Access::Response::Output output,
                                                                            const utils::Timeval& timeout,
                                                                            std::optional<Principal> principal,
                                                                            std::shared_ptr<detail::Channel> channel,
                                                                            Mcp* protocol,
                                                                            std::size_t pageSize) const {
        const Json& id = request.id;
        const std::string& method = request.method;
        auto reply = std::make_shared<detail::Access::Response>(
            id,
            era,
            method == "server/discover" || method.ends_with("/list") || method == "resources/read" ? CachePolicy::Private
                                                                                                   : CachePolicy::None,
            std::move(output),
            timeout);
        reply->protocol = protocol;
        reply->toolRequest = method == "tools/call";
        reply->serverInfo = serverInfo;
        reply->closure = channel && era == ProtocolEra::PerRequest20260728 ? SubscriptionClosure::Cancel : SubscriptionClosure::Complete;
        reply->principal = std::move(principal);
        reply->channel = channel;
        if (era == ProtocolEra::Initialization20251125 || method != "subscriptions/listen") {
            const std::weak_ptr<Response> weak = reply;
            reply->timer = core::timer::Timer::singleshotTimer(
                [weak] {
                    if (const auto current = weak.lock()) {
                        current->cancel(CancelReason::Deadline);
                    }
                },
                timeout);
        }

        try {
            const Json params = request.params.value_or(Json::object());
            const Json meta = params.value("_meta", Json::object());
            const Json version = meta.is_object() ? meta.value(versionKey, Json()) : Json();
            reply->clientCapabilities = era == ProtocolEra::Initialization20251125 && channel ? channel->capabilities
                                        : meta.is_object()                                    ? meta.value(capabilitiesKey, Json::object())
                                                                                              : Json::object();
            const Json retry = params.value("inputResponses", Json::object());
            bool validRetry =
                retry.is_object() && retry.size() <= 16 &&
                (era == ProtocolEra::PerRequest20260728 || (!params.contains("inputResponses") && !params.contains("requestState"))) &&
                (!params.contains("requestState") ||
                 (params["requestState"].is_string() && params["requestState"].get_ref<const std::string&>().size() <= 16384));
            for (auto input = retry.begin(); input != retry.end() && validRetry; ++input) {
                const auto action = input.value().is_object() ? input.value().value("action", Json()) : Json();
                const auto content = input.value().is_object() ? input.value().value("content", Json()) : Json();
                validRetry =
                    (action == "accept" || action == "decline" || action == "cancel") && (content.is_null() || content.is_object());
                if (validRetry) {
                    reply->inputResponses.emplace(input.key(),
                                                  InputResponse{action == "accept"    ? ElicitationAction::Accept
                                                                : action == "decline" ? ElicitationAction::Decline
                                                                                      : ElicitationAction::Cancel,
                                                                content});
                }
            }

            if (validRetry && params.contains("requestState")) {
                reply->requestState = params["requestState"].get<std::string>();
            }

            const Json client = params.value("clientInfo", Json());
            const Json token = meta.is_object() ? meta.value("progressToken", Json()) : Json();
            const bool validToken = !meta.is_object() || !meta.contains("progressToken") || token.is_string() ||
                                    (token.is_number_integer() && (!token.is_number_unsigned() || token.get<std::uint64_t>() <= INT64_MAX));
            if (token.is_string()) {
                reply->progressToken = token.get<std::string>();
            } else if (validToken && !token.is_null()) {
                reply->progressToken = token.get<std::int64_t>();
            }

            if (era == ProtocolEra::PerRequest20260728 &&
                (!meta.is_object() || !version.is_string() || !meta.contains(capabilitiesKey) || !meta[capabilitiesKey].is_object())) {
                reply->error(-32602, "Missing or malformed per-request metadata");
            } else if (!validRetry) {
                reply->error(-32602, "Malformed retry input or state");
            } else if (!validToken) {
                reply->error(-32602, "Progress token must be a string or signed integer");
            } else if (era == ProtocolEra::PerRequest20260728 && version != protocolVersion2026) {
                reply->error(-32022,
                             "Unsupported protocol version",
                             {{"supported", {protocolVersion2026, protocolVersion2025}}, {"requested", version}});

            } else if (era == ProtocolEra::Initialization20251125 && method == "initialize" &&
                       (!params.value("protocolVersion", Json()).is_string() || !params.value("capabilities", Json()).is_object() ||
                        !client.is_object() || !client.value("name", Json()).is_string() || !client.value("version", Json()).is_string())) {
                reply->error(-32602, "Malformed initialize parameters");
            } else if ((era == ProtocolEra::PerRequest20260728 && method == "server/discover") ||
                       (era == ProtocolEra::Initialization20251125 && method == "initialize")) {
                if (era == ProtocolEra::Initialization20251125 && channel) {
                    channel->capabilities = params["capabilities"];
                    std::erase_if(channels, [](const auto& weak) {
                        return weak.expired();
                    });
                    if (std::none_of(channels.begin(), channels.end(), [&channel](const auto& weak) {
                            return weak.lock() == channel;
                        })) {
                        if (channels.size() >= 1024) {
                            throw std::runtime_error("Notification channel capacity exceeded");
                        }
                        channels.push_back(channel);
                    }
                }

                Json advertised = capabilities();
                if (advertised.contains("resources") &&
                    (era == ProtocolEra::PerRequest20260728 || (channel && channel->resourceSubscriptions))) {
                    advertised["resources"]["subscribe"] = true;
                }

                Json value{{"capabilities", std::move(advertised)}};
                if (era == ProtocolEra::Initialization20251125) {
                    value["protocolVersion"] = protocolVersion2025;
                    value["serverInfo"] = serverInfo;
                } else {
                    value["supportedVersions"] = {protocolVersion2026, protocolVersion2025};
                    value["_meta"]["io.modelcontextprotocol/serverInfo"] = serverInfo;
                }

                if (!instructions.empty()) {
                    value["instructions"] = instructions;
                }

                reply->result(std::move(value));
            } else if (era == ProtocolEra::Initialization20251125 && method == "ping") {
                reply->result(Json::object());
            } else if (era == ProtocolEra::PerRequest20260728 && method == "subscriptions/listen") {
                if (params.contains("notifications") && !params["notifications"].is_object()) {
                    reply->error(-32602, "Invalid subscription filter");
                } else {
                    const Json filter = params.value("notifications", Json::object());
                    const Json uris = filter.value("resourceSubscriptions", Json::array());
                    bool valid = uris.is_array() && uris.size() <= 128;
                    for (const auto& uri : uris) {
                        valid = valid && uri.is_string() && uri.get_ref<const std::string&>().size() <= 16384;
                    }

                    for (const std::string name : {"toolsListChanged", "promptsListChanged", "resourcesListChanged"}) {
                        valid = valid && (!filter.contains(name) || filter[name].is_boolean());
                    }

                    std::erase_if(subscriptions, [](const auto& weak) {
                        const auto current = weak.lock();
                        return !current || current->done;
                    });
                    if (!valid) {
                        reply->error(-32602, "Malformed resource subscription filter or 128-URI limit exceeded");
                    } else if (subscriptions.size() >= 1024) {
                        reply->error(-32603, "Subscription capacity exceeded");
                    } else {
                        subscriptions.push_back(reply);
                        Json honored = Json::object();
                        for (const std::string name : {"toolsListChanged", "promptsListChanged", "resourcesListChanged"}) {
                            if (filter.value(name, false)) {
                                honored[name] = true;
                            }
                        }
                        if (!uris.empty()) {
                            honored["resourceSubscriptions"] = uris;
                        }

                        reply->listen(std::move(honored));
                    }
                }
            } else if (era == ProtocolEra::Initialization20251125 &&
                       (method == "resources/subscribe" || method == "resources/unsubscribe")) {
                const auto uri = params.value("uri", Json());
                std::map<std::string, std::string> variables;
                if (!channel || !channel->active || !channel->resourceSubscriptions) {
                    reply->error(-32601, "Transport has no legacy notification channel");
                } else if (!uri.is_string() || uri.get_ref<const std::string&>().size() > 16384) {
                    reply->error(-32602, "Invalid resource subscription URI");
                } else if (method == "resources/unsubscribe") {
                    channel->resources.erase(uri.get<std::string>());
                    reply->result(Json::object());
                } else if (!resolveResource(uri.get<std::string>(), variables)) {
                    reply->error(-32002, "Unknown resource subscription URI");
                } else if (channel->resources.size() >= 128 && !channel->resources.contains(uri.get<std::string>())) {
                    reply->error(-32602, "128-resource subscription limit exceeded");
                } else {
                    std::erase_if(channels, [](const auto& weak) {
                        return weak.expired();
                    });
                    const bool registered = std::any_of(channels.begin(), channels.end(), [&channel](const auto& weak) {
                        return weak.lock() == channel;
                    });
                    if (!registered && channels.size() >= 1024) {
                        reply->error(-32603, "Subscription capacity exceeded");
                    } else {
                        if (!registered) {
                            channels.push_back(channel);
                        }

                        channel->resources.insert(uri.get<std::string>());
                        reply->result(Json::object());
                    }
                }
            } else if (method == "completion/complete") {
                const auto now = utils::Timeval::currentTime();
                if (completionReset < now) {
                    completionReset = now + utils::Timeval(1);
                    completionCount = 0;
                }

                if (!capabilities().contains("completions")) {
                    reply->error(-32601, "Completion capability not supported");
                } else if (++completionCount > 128) {
                    reply->error(-32000, "Completion rate limit exceeded");
                } else {
                    const Json reference = params.value("ref", Json());
                    const Json argument = params.value("argument", Json());
                    const Json context = params.value("context", Json::object());
                    const Json arguments = context.is_object() ? context.value("arguments", Json::object()) : Json();
                    if (!reference.is_object() || !argument.is_object() || !argument.value("name", Json()).is_string() ||
                        !argument.value("value", Json()).is_string() || !arguments.is_object() ||
                        !std::all_of(arguments.begin(), arguments.end(), [](const Json& item) {
                            return item.is_string();
                        })) {
                        reply->error(-32602, "Malformed completion parameters");
                    } else {
                        CompletionHandler complete;
                        bool found = false;
                        const auto referenceType = reference.value("type", Json());
                        const auto identifier = reference.value(referenceType == "ref/prompt" ? "name" : "uri", Json());
                        if (identifier.is_string() && (referenceType == "ref/prompt" || referenceType == "ref/resource")) {
                            const auto kind = referenceType == "ref/prompt" ? Kind::Prompt : Kind::ResourceTemplate;
                            auto entry = entries.find(std::to_string(static_cast<int>(kind)) + ":" + identifier.get<std::string>());
                            if (entry == entries.end() && kind == Kind::ResourceTemplate) {
                                std::map<std::string, std::string> variables;
                                const auto* matched = resolveResource(identifier.get<std::string>(), variables);
                                if (matched && matched->kind == Kind::ResourceTemplate) {
                                    entry = entries.find(std::to_string(static_cast<int>(kind)) + ":" +
                                                         matched->description["uriTemplate"].get<std::string>());
                                }
                            }
                            if (entry != entries.end()) {
                                found = true;
                                if (kind == Kind::Prompt) {
                                    for (const auto& candidate :
                                         std::get<std::shared_ptr<Prompt>>(entry->second.handler)->getDefinition().arguments) {
                                        if (candidate.name == argument["name"].get<std::string>()) {
                                            complete = candidate.complete;
                                        }
                                    }
                                } else {
                                    for (const auto& candidate :
                                         std::get<std::shared_ptr<ResourceTemplate>>(entry->second.handler)->getDefinition().variables) {
                                        if (candidate.name == argument["name"].get<std::string>()) {
                                            complete = candidate.complete;
                                        }
                                    }
                                }
                            }
                        }

                        if (!found) {
                            reply->error(-32602, "Unknown completion reference");
                        } else if (complete) {
                            complete(CompletionRequest{Access::context(reply),
                                                       argument["name"],
                                                       argument["value"],
                                                       arguments.get<std::map<std::string, std::string>>()},
                                     Access::wrap<CompletionResult>(reply));
                        } else {
                            Access::wrap<CompletionResult>(reply)->result(CompletionResult{});
                        }
                    }
                }
            } else if (method == "resources/templates/list") {
                Json list = Json::array();
                for (const auto& [key, entry] : entries) {
                    static_cast<void>(key);
                    if (entry.kind == Kind::ResourceTemplate) {
                        list.push_back(entry.description);
                    }
                }
                reply->result(paginate(method, params, list, "resourceTemplates", pageSize));
            } else if (method == "tools/list" || method == "resources/list" || method == "prompts/list") {
                const Kind kind = method == "tools/list" ? Kind::Tool : method == "resources/list" ? Kind::Resource : Kind::Prompt;
                Json list = Json::array();
                for (const auto& [key, entry] : entries) {
                    static_cast<void>(key);
                    if (entry.kind == kind) {
                        Json description = entry.description;
                        if (era == ProtocolEra::Initialization20251125 && kind == Kind::Tool && description.contains("outputSchema") &&
                            (!description["outputSchema"].is_object() || description["outputSchema"].value("type", Json()) != "object")) {
                            description.erase("outputSchema");
                        }

                        list.push_back(std::move(description));
                    }
                }

                reply->result(paginate(method, params, list, method.substr(0, method.find('/')), pageSize));
            } else if (method == "tools/call" || method == "resources/read" || method == "prompts/get") {
                const Kind kind = method == "tools/call" ? Kind::Tool : method == "resources/read" ? Kind::Resource : Kind::Prompt;
                const std::string argument = kind == Kind::Resource ? "uri" : "name";
                if (!params.contains(argument) || !params[argument].is_string()) {
                    reply->error(-32602, "Missing name or uri");
                } else {
                    const auto exact = entries.find(std::to_string(static_cast<int>(kind)) + ":" + params[argument].get<std::string>());
                    std::map<std::string, std::string> variables;
                    const Entry* entry = kind == Kind::Resource   ? resolveResource(params["uri"].get<std::string>(), variables)
                                         : exact == entries.end() ? nullptr
                                                                  : &exact->second;
                    const Json arguments = params.value("arguments", Json::object());
                    std::string error;
                    if (entry == nullptr || !arguments.is_object() ||
                        (kind == Kind::Prompt && !std::all_of(arguments.begin(), arguments.end(), [](const Json& value) {
                             return value.is_string();
                         }))) {
                        reply->error(era == ProtocolEra::Initialization20251125 && kind == Kind::Resource && entry == nullptr ? -32002
                                                                                                                              : -32602,
                                     "Unknown registration or malformed arguments");
                    } else if (kind == Kind::Tool && !validate(entry->description["inputSchema"], arguments, error)) {
                        reply->result({{"isError", true}, {"content", {{{"type", "text"}, {"text", error}}}}});
                    } else {
                        if (kind == Kind::Prompt) {
                            for (const auto& argument : entry->description["arguments"]) {
                                if (argument.value("required", false) && !arguments.contains(argument["name"].get<std::string>())) {
                                    reply->error(-32602, "Missing required prompt argument: " + argument["name"].get<std::string>());
                                }
                            }
                        }

                        if (entry->description.contains("outputSchema")) {
                            reply->outputSchema = entry->description["outputSchema"];
                        }

                        const std::weak_ptr<Response> weak = reply;
                        reply->resume = [weak,
                                         kind,
                                         handler = entry->handler,
                                         arguments,
                                         uri = kind == Kind::Resource ? params["uri"].get<std::string>() : std::string(),
                                         variables] {
                            if (const auto current = weak.lock(); current && !current->done) {
                                try {
                                    if (kind == Kind::Tool) {
                                        std::get<std::shared_ptr<Tool>>(handler)->call(ToolCall{Access::context(current), arguments},
                                                                                       Access::wrap<ToolResult>(current));
                                    } else if (kind == Kind::Resource) {
                                        const ResourceRead input{Access::context(current), uri, variables};
                                        if (const auto* resource = std::get_if<std::shared_ptr<Resource>>(&handler)) {
                                            (*resource)->read(input, Access::wrap<ResourceResult>(current));
                                        } else {
                                            std::get<std::shared_ptr<ResourceTemplate>>(handler)->read(
                                                input, Access::wrap<ResourceResult>(current));
                                        }
                                    } else {
                                        std::get<std::shared_ptr<Prompt>>(handler)->get(
                                            PromptGet{Access::context(current), arguments.get<std::map<std::string, std::string>>()},
                                            Access::wrap<PromptResult>(current));
                                    }
                                } catch (const InputValidationError& error) {
                                    current->error(-32602, error.what());
                                } catch (...) {
                                    snode::log::framework("mcp.server", snode::log::Boundary::Application).error() << "handler failed";
                                    current->error(-32603, "Internal error");
                                }
                            }
                        };
                        const auto invoke = reply->resume;
                        invoke();
                    }
                }
            } else {
                reply->error(-32601, "Method not found");
            }
        } catch (const InputValidationError& error) {
            reply->error(-32602, error.what());
        } catch (...) {
            snode::log::framework("mcp.server", snode::log::Boundary::Application).error() << "dispatch failed";
            reply->error(-32603, "Internal error");
        }
        reply->expire();

        return reply;
    }

    Json Server::Impl::paginate(
        const std::string& method, const Json& params, const Json& list, const std::string& field, std::size_t pageSize) const {
        std::size_t offset = 0;
        if (pageSize == 0 || pageSize > 128) {
            throw InputValidationError("Invalid page size");
        }
        if (params.contains("cursor")) {
            const auto& cursor = params["cursor"];
            if (!cursor.is_string() || cursor.get_ref<const std::string&>().size() > 1024) {
                throw InputValidationError("Invalid cursor");
            }
            const Json decoded = Json::parse(base64::base64_decode(cursor.get<std::string>()), nullptr, false);
            if (!decoded.is_array() || decoded.size() != 5 || decoded[0] != 1 || decoded[1] != revision || decoded[2] != method ||
                decoded[3] != pageSize || !decoded[4].is_number_unsigned() || decoded[4] == 0 || decoded[4] >= list.size() ||
                decoded[4].get<std::uint64_t>() % pageSize != 0) {
                throw InputValidationError("Invalid or expired cursor; restart listing");
            }
            offset = decoded[4].get<std::size_t>();
        }
        Json page = Json::array();
        const auto end = std::min(list.size(), offset + pageSize);
        for (auto index = offset; index < end; ++index) {
            page.push_back(list[index]);
        }
        Json result{{field, std::move(page)}};
        if (end < list.size()) {
            const std::string cursor = Json::array({1, revision, method, pageSize, end}).dump();
            result["nextCursor"] = base64::base64_encode(reinterpret_cast<const unsigned char*>(cursor.data()), cursor.size());
        }
        return result;
    }

    Server* Server::resourceUpdated(const std::string& uri) {
        impl->resourceUpdated(uri);

        return this;
    }

    void Server::Impl::resourceUpdated(const std::string& uri) {
        std::vector<std::shared_ptr<detail::Response>> requests;
        std::vector<std::shared_ptr<detail::Channel>> clients;
        std::erase_if(subscriptions, [](const auto& weak) {
            const auto current = weak.lock();
            return !current || current->done;
        });
        std::erase_if(channels, [](const auto& weak) {
            return weak.expired();
        });
        for (const auto& weak : subscriptions) {
            if (const auto current = weak.lock(); current && !current->done) {
                const auto filter = current->notifications.value("resourceSubscriptions", Json::array());
                if (std::find(filter.begin(), filter.end(), Json(uri)) != filter.end()) {
                    requests.push_back(current);
                }
            }
        }

        for (const auto& weak : channels) {
            if (const auto channel = weak.lock(); channel && channel->active && channel->resources.contains(uri)) {
                clients.push_back(channel);
            }
        }

        for (const auto& request : requests) {
            request->notify("notifications/resources/updated", {{"uri", uri}});
        }

        for (const auto& client : clients) {
            if (client->active && client->output) {
                client->output(
                    jsonrpc::messages::Notification{"notifications/resources/updated", std::optional<Json>(Json{{"uri", uri}})}.toJson());
            }
        }
    }

    int Server::Impl::httpStatus(const Json& message) {
        int status = 200;
        if (message.contains("error")) {
            const int code = message["error"]["code"].get<int>();
            status = code == -32000 ? 429 : code == -32601 ? 404 : code == -32603 ? 500 : 400;
        }

        return status;
    }

    template <class Result>
    Reply<Result>::Reply(std::shared_ptr<detail::Response> impl)
        : impl(std::move(impl)) {
    }

    template <class Result>
    Completion Reply<Result>::result(Result result) {
        Json value;
        if constexpr (std::is_same_v<Result, ToolResult>) {
            value = result.toJson();
        } else if constexpr (std::is_same_v<Result, ResourceResult>) {
            value["contents"] = Json::array();
            for (const auto& resource : result.contents) {
                value["contents"].push_back(encode(resource));
            }
        } else if constexpr (std::is_same_v<Result, CompletionResult>) {
            const auto count = result.values.size();
            if (count > 100) {
                result.values.resize(100);
                result.hasMore = true;
            }

            value = {{"completion", {{"values", std::move(result.values)}, {"hasMore", result.hasMore}}}};
            if (result.total) {
                value["completion"]["total"] = std::max(*result.total, count);
            }
        } else {
            value = {{"messages", Json::array()}};
            if (result.description) {
                value["description"] = *result.description;
            }

            for (const auto& message : result.messages) {
                value["messages"].push_back(
                    {{"role", message.role == Role::User ? "user" : "assistant"}, {"content", encode(message.content)}});
            }
        }

        if constexpr (!std::is_same_v<Result, ToolResult>) {
            metadata(value, result.meta);
        }

        if constexpr (std::is_same_v<Result, ResourceResult>) {
            if (result.cache && impl->era == ProtocolEra::PerRequest20260728) {
                value["ttlMs"] = result.cache->ttlMs;
                value["cacheScope"] = result.cache->scope == CacheScope::Public ? "public" : "private";
            }
        }

        return impl->result(std::move(value));
    }

    template <class Result>
    Completion Reply<Result>::inputRequired(InputRequired input)
        requires(!std::is_same_v<Result, CompletionResult>)
    {
        return impl->inputRequired(std::move(input));
    }

    template <class Result>
    Completion Reply<Result>::error(ErrorCode code, std::string message, Json data) {
        return impl->error(static_cast<int>(code), message, std::move(data));
    }

    template <class Result>
    void Reply<Result>::progress(Progress progress) {
        const auto request = impl;
        request->expire();
        if (request->progressToken && !request->done && std::isfinite(progress.value) && progress.value >= 0 &&
            (!request->lastProgress || progress.value > *request->lastProgress) &&
            (!progress.total || (std::isfinite(*progress.total) && *progress.total >= progress.value))) {
            request->lastProgress = progress.value;
            Json params{{"progress", progress.value}};
            params["progressToken"] = std::visit(
                [](const auto& token) {
                    return Json(token);
                },
                *request->progressToken);
            if (progress.total) {
                params["total"] = *progress.total;
            }

            if (!progress.message.empty()) {
                params["message"] = std::move(progress.message);
            }

            request->notify("notifications/progress", std::move(params));
        }
    }

    RequestContext::RequestContext(std::shared_ptr<detail::Response> impl)
        : impl(std::move(impl)) {
    }

    const std::optional<Principal>& RequestContext::getPrincipal() const {
        return impl->principal;
    }

    const std::optional<ProgressToken>& RequestContext::getProgressToken() const {
        return impl->progressToken;
    }

    const std::map<std::string, InputResponse>& RequestContext::getInputResponses() const {
        return impl->inputResponses;
    }

    const InputResponse* RequestContext::getInputResponse(const std::string& key, const Json& requestedSchema) const {
        const InputResponse* result = nullptr;
        const auto found = impl->inputResponses.find(key);
        if (found != impl->inputResponses.end()) {
            std::string error;
            if (found->second.action == ElicitationAction::Accept &&
                (!Access::Registry::checkSchema(requestedSchema, error) ||
                 !Access::Registry::validate(requestedSchema, found->second.content, error))) {
                throw InputValidationError("Invalid elicitation response " + key + ": " + error);
            }

            result = &found->second;
        }

        return result;
    }

    const std::optional<std::string>& RequestContext::getRequestState() const {
        return impl->requestState;
    }

    void RequestContext::onCancel(OnCancel callback) const {
        impl->observe(std::move(callback));
    }
    template class Reply<CompletionResult>;
    template class Reply<ToolResult>;
    template class Reply<ResourceResult>;
    template class Reply<PromptResult>;

    namespace {

        struct FunctionTool final : Tool {
            Server::ToolHandler handler;

            FunctionTool(ToolDefinition definition, Server::ToolHandler handler)
                : Tool(std::move(definition))
                , handler(std::move(handler)) {
                if (!this->handler) {
                    throw RegistrationError(RegistrationKind::Tool, getDefinition().name, "handler", "must be callable");
                }
            }

            void call(const ToolCall& request, const std::shared_ptr<ToolReply>& reply) override {
                handler(request, reply);
            }
        };

        struct FunctionResource final : Resource {
            Server::ResourceHandler handler;

            FunctionResource(ResourceDefinition definition, Server::ResourceHandler handler)
                : Resource(std::move(definition))
                , handler(std::move(handler)) {
                if (!this->handler) {
                    throw RegistrationError(RegistrationKind::Resource, getDefinition().uri, "handler", "must be callable");
                }
            }

            void read(const ResourceRead& request, const std::shared_ptr<ResourceReply>& reply) override {
                handler(request, reply);
            }
        };

        struct FunctionPrompt final : Prompt {
            Server::PromptHandler handler;

            FunctionPrompt(PromptDefinition definition, Server::PromptHandler handler)
                : Prompt(std::move(definition))
                , handler(std::move(handler)) {
                if (!this->handler) {
                    throw RegistrationError(RegistrationKind::Prompt, getDefinition().name, "handler", "must be callable");
                }
            }

            void get(const PromptGet& request, const std::shared_ptr<PromptReply>& reply) override {
                handler(request, reply);
            }
        };

    } // namespace

    ResourceTemplate::ResourceTemplate(ResourceTemplateDefinition definition)
        : definition(std::move(definition)) {
    }

    ResourceTemplate::~ResourceTemplate() = default;

    const ResourceTemplateDefinition& ResourceTemplate::getDefinition() const {
        return definition;
    }

    Server* Server::resourceTemplate(ResourceTemplateDefinition definition, ResourceHandler handler) {
        struct FunctionTemplate final : ResourceTemplate {
            ResourceHandler handler;

            FunctionTemplate(ResourceTemplateDefinition definition, ResourceHandler handler)
                : ResourceTemplate(std::move(definition))
                , handler(std::move(handler)) {
                if (!this->handler) {
                    throw RegistrationError(RegistrationKind::ResourceTemplate, getDefinition().uriTemplate, "handler", "must be callable");
                }
            }

            void read(const ResourceRead& request, const std::shared_ptr<ResourceReply>& reply) override {
                handler(request, reply);
            }
        };

        return resourceTemplate(std::make_shared<FunctionTemplate>(std::move(definition), std::move(handler)));
    }

    Server* Server::resourceTemplate(std::shared_ptr<ResourceTemplate> handler) {
        const auto definition = handler ? handler->getDefinition() : ResourceTemplateDefinition{};
        Json description{{"uriTemplate", definition.uriTemplate},
                         {"name", definition.name},
                         {"description", definition.description},
                         {"mimeType", definition.mimeType}};
        display(description, definition.title, definition.icons);
        registrationMetadata(description, definition.meta, RegistrationKind::ResourceTemplate, definition.uriTemplate);

        const auto annotations =
            registrationAnnotations(definition.annotations, RegistrationKind::ResourceTemplate, definition.uriTemplate);
        if (!annotations.empty()) {
            description["annotations"] = annotations;
        }

        impl->add(RegistrationKind::ResourceTemplate, std::move(description), std::move(handler));

        return this;
    }

    Tool::Tool(ToolDefinition definition)
        : definition(std::move(definition)) {
    }

    Tool::~Tool() = default;

    const ToolDefinition& Tool::getDefinition() const {
        return definition;
    }

    Resource::Resource(ResourceDefinition definition)
        : definition(std::move(definition)) {
    }

    Resource::~Resource() = default;

    const ResourceDefinition& Resource::getDefinition() const {
        return definition;
    }

    Prompt::Prompt(PromptDefinition definition)
        : definition(std::move(definition)) {
    }

    Prompt::~Prompt() = default;

    const PromptDefinition& Prompt::getDefinition() const {
        return definition;
    }

    RegistrationError::RegistrationError(RegistrationKind kind, std::string identifier, std::string field, std::string reason)
        : std::invalid_argument(std::string(kind == RegistrationKind::Tool               ? "tool "
                                            : kind == RegistrationKind::ResourceTemplate ? "resource template "
                                            : kind == RegistrationKind::Resource         ? "resource "
                                                                                         : "prompt ") +
                                (identifier.empty() ? "<unnamed>" : identifier) + ": " + field + ": registration rejected: " + reason) {
    }

    RegistrationError::~RegistrationError() = default;

    Server::Server(std::string name, std::string version, std::string instructions)
        : Server(Implementation{std::move(name), std::move(version)}, std::move(instructions)) {
    }

    Server::Server(Implementation implementation, std::string instructions)
        : impl(std::make_unique<Impl>()) {
        impl->serverInfo = {{"name", std::move(implementation.name)}, {"version", std::move(implementation.version)}};
        display(impl->serverInfo, implementation.title, implementation.icons);
        if (implementation.description) {
            impl->serverInfo["description"] = *implementation.description;
        }

        if (implementation.websiteUrl) {
            impl->serverInfo["websiteUrl"] = *implementation.websiteUrl;
        }

        impl->instructions = std::move(instructions);
    }

    Server::~Server() = default;

    Server* Server::tool(ToolDefinition definition, ToolHandler handler) {
        return tool(std::make_shared<FunctionTool>(std::move(definition), std::move(handler)));
    }

    Server* Server::tool(std::shared_ptr<Tool> handler) {
        const auto definition = handler ? handler->getDefinition() : ToolDefinition{};
        Json description{{"name", definition.name}, {"description", definition.description}, {"inputSchema", definition.inputSchema}};
        if (definition.outputSchema) {
            description["outputSchema"] = *definition.outputSchema;
        }

        display(description, definition.title, definition.icons);
        registrationMetadata(description, definition.meta, RegistrationKind::Tool, definition.name);

        if (!definition.annotations.is_null()) {
            description["annotations"] = std::move(definition.annotations);
        }

        impl->add(RegistrationKind::Tool, std::move(description), std::move(handler));

        return this;
    }

    Server* Server::resource(ResourceDefinition definition, ResourceHandler handler) {
        return resource(std::make_shared<FunctionResource>(std::move(definition), std::move(handler)));
    }

    Server* Server::resource(std::shared_ptr<Resource> handler) {
        const auto definition = handler ? handler->getDefinition() : ResourceDefinition{};
        Json description{
            {"uri", definition.uri}, {"name", definition.name}, {"description", definition.description}, {"mimeType", definition.mimeType}};
        display(description, definition.title, definition.icons);
        registrationMetadata(description, definition.meta, RegistrationKind::Resource, definition.uri);

        const auto annotations = registrationAnnotations(definition.annotations, RegistrationKind::Resource, definition.uri);
        if (!annotations.empty()) {
            description["annotations"] = annotations;
        }

        if (definition.size) {
            description["size"] = *definition.size;
        }

        impl->add(RegistrationKind::Resource, std::move(description), std::move(handler));

        return this;
    }

    Server* Server::prompt(PromptDefinition definition, PromptHandler handler) {
        return prompt(std::make_shared<FunctionPrompt>(std::move(definition), std::move(handler)));
    }

    Server* Server::prompt(std::shared_ptr<Prompt> handler) {
        const auto definition = handler ? handler->getDefinition() : PromptDefinition{};
        Json description{{"name", definition.name}, {"description", definition.description}, {"arguments", Json::array()}};
        for (const auto& argument : definition.arguments) {
            description["arguments"].push_back({{"name", argument.name},
                                                {"description", argument.description},
                                                {"required", argument.requirement == Requirement::Required}});
            if (argument.title) {
                description["arguments"].back()["title"] = *argument.title;
            }
        }

        display(description, definition.title, definition.icons);
        registrationMetadata(description, definition.meta, RegistrationKind::Prompt, definition.name);

        impl->add(RegistrationKind::Prompt, std::move(description), std::move(handler));

        return this;
    }

    detail::Access::Registry& detail::Access::registry(const Server& server) {
        return *server.impl;
    }

} // namespace mcp::server
