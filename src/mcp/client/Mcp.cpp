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

#include "mcp/client/Mcp.h"

#include "core/timer/Timer.h"
#include "jsonrpc/JsonRpcContext.h"
#include "jsonrpc/messages/Error.h"
#include "jsonrpc/messages/Notification.h"
#include "jsonrpc/messages/Request.h"
#include "jsonrpc/messages/Result.h"
#include "log/SemanticLogger.h"
#include "mcp/Results.h"
#include "mcp/detail/ClientInputs.h"
#include "mcp/detail/Serialization.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>
// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>
// IWYU pragma: no_include <nlohmann/detail/iterators/iteration_proxy.hpp>
// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>
#endif
namespace mcp::client {
    using jsonrpc::messages::Id;

    struct detail::InputState {
        std::function<void(Json)> callback;
        std::function<void()> cancelled;
        Json request;
        Id origin;
        bool incoming = false;
        core::timer::Timer timer;

        ~InputState() {
            timer.cancel();
        }

        Completion finish(Json response) {
            Completion accepted = Completion::Inactive;
            if (callback && (response.contains("error") || mcp::detail::validInputResult(request, response.at("result")))) {
                auto completion = std::exchange(callback, {});
                auto observer = std::exchange(cancelled, {});
                timer.cancel();
                if (response.contains("error") && observer) {
                    try {
                        observer();
                    } catch (...) {
                    }
                }
                accepted = Completion::Accepted;
                completion(std::move(response));
            }
            return accepted;
        }
    };

    ElicitationReply::ElicitationReply(std::shared_ptr<detail::InputState> state)
        : state(std::move(state)) {
    }

    ElicitationReply::~ElicitationReply() {
        result(InputResponse{ElicitationAction::Decline});
    }

    Completion ElicitationReply::result(InputResponse response) {
        Json value{{"action",
                    response.action == ElicitationAction::Accept    ? "accept"
                    : response.action == ElicitationAction::Decline ? "decline"
                                                                    : "cancel"}};
        if (!response.content.is_null()) {
            value["content"] = std::move(response.content);
        }
        return state->finish({{"result", std::move(value)}});
    }

    ClientReply::ClientReply(std::shared_ptr<detail::InputState> state)
        : state(std::move(state)) {
    }

    ClientReply::~ClientReply() {
        error(ErrorCode::InternalError, "Client input was not answered");
    }

    Completion ClientReply::result(SamplingResult result) {
        return state->request["method"] == "sampling/createMessage" ? state->finish({{"result", result.toJson()}}) : Completion::Inactive;
    }

    Completion ClientReply::result(RootsResult result) {
        return state->request["method"] == "roots/list" ? state->finish({{"result", result.toJson()}}) : Completion::Inactive;
    }

    Completion ClientReply::error(ErrorCode code, std::string message, Json data) {
        Json value{{"code", static_cast<int>(code)}, {"message", std::move(message)}};
        if (!data.is_null()) {
            value["data"] = std::move(data);
        }
        return state->finish({{"error", std::move(value)}});
    }

    void ClientReply::onCancel(std::function<void()> callback) {
        if (state->callback) {
            state->cancelled = std::move(callback);
        }
    }

    struct Mcp::Operation {
        std::string method;
        Json params;
        RequestOptions options;
        Id root;
        std::vector<Json> entries;
        std::set<std::string> cursors;
        std::size_t rounds = 0;
        std::string task;
        std::optional<double> progress;
    };

    struct Mcp::State {
        explicit State(Options options)
            : options(std::move(options)) {
        }

        struct Watch {
            Operation origin;
            TaskInfo info;
            core::timer::Timer timer;
            std::set<std::string> inputs;
        };

        Options options;
        ProtocolEra era = ProtocolEra::PerRequest20260728;
        bool ready = false;
        Json capabilities{{"elicitation", {{"form", Json::object()}, {"url", Json::object()}}},
                          {"extensions", {{"io.modelcontextprotocol/tasks", Json::object()}}}};
        Json serverCapabilities = Json::object();
        std::map<Id, Operation> operations;
        std::map<Id, Json> subscriptions;
        std::set<std::string> resources;
        std::map<std::string, Watch> tasks;
        std::vector<std::weak_ptr<detail::InputState>> inputs;
        SamplingHandler sampling;
        RootsHandler roots;
    };

    Mcp::Mcp(Options options, const std::string& connectionName)
        : Super(connectionName)
        , state(std::make_unique<State>(std::move(options))) {
        setBatchesEnabled(false);
    }

    Mcp::~Mcp() {
        cancelInputs();
        for (auto& [id, task] : state->tasks) {
            static_cast<void>(id);
            task.timer.cancel();
        }
    }

    void Mcp::onConnected() {
        Super::onConnected();
        if (state->options.negotiation == Negotiation::Legacy) {
            initialize();
        } else {
            state->era = ProtocolEra::PerRequest20260728;
            request("server/discover");
        }
    }

    void Mcp::onDisconnected() {
        cancelInputs();
        for (auto& [id, task] : state->tasks) {
            static_cast<void>(id);
            task.timer.cancel();
        }
        state->tasks.clear();
        state->operations.clear();
        state->subscriptions.clear();
        state->resources.clear();
        state->ready = false;
        Super::onDisconnected();
    }

    bool Mcp::onSignal([[maybe_unused]] int signal) {
        disconnect();
        return true;
    }

    ProtocolEra Mcp::getProtocolEra() const {
        return state->era;
    }

    const Json& Mcp::getServerCapabilities() const {
        return state->serverCapabilities;
    }

    void Mcp::setNegotiation(Negotiation negotiation) {
        if (!state->ready && state->operations.empty()) {
            state->options.negotiation = negotiation;
        }
    }

    void Mcp::setClientCapabilities(Json capabilities) {
        if (state->ready || !capabilities.is_object()) {
            throw std::invalid_argument("Capabilities must be configured before connection");
        }
        state->capabilities = std::move(capabilities);
    }

    void Mcp::setSamplingHandler(SamplingHandler handler, bool tools, bool context) {
        if (state->ready || !state->operations.empty()) {
            throw std::invalid_argument("Configure sampling before connection");
        }
        state->sampling = std::move(handler);
        state->capabilities.erase("sampling");
        if (state->sampling) {
            state->capabilities["sampling"] = Json::object();
            if (tools) {
                state->capabilities["sampling"]["tools"] = Json::object();
            }
            if (context) {
                state->capabilities["sampling"]["context"] = Json::object();
            }
        }
    }

    void Mcp::setRootsHandler(RootsHandler handler, bool listChanged) {
        if (state->ready || !state->operations.empty()) {
            throw std::invalid_argument("Configure roots before connection");
        }
        state->roots = std::move(handler);
        state->capabilities.erase("roots");
        if (state->roots) {
            state->capabilities["roots"] = listChanged ? Json{{"listChanged", true}} : Json::object();
        }
    }

    Completion Mcp::notifyRootsChanged() {
        Completion result = Completion::Inactive;
        const auto roots = state->capabilities.value("roots", Json());
        if (state->ready && state->era == ProtocolEra::Initialization20251125 && roots.is_object() &&
            roots.value("listChanged", Json()) == true) {
            sendNotification("notifications/roots/list_changed");
            result = Completion::Accepted;
        }
        return result;
    }

    void Mcp::disconnect() {
        cancelInputs();
        if (jsonRpcContext) {
            jsonRpcContext->end();
        }
    }

    Json Mcp::clientCapabilities() const {
        Json capabilities = state->capabilities;
        if (state->era == ProtocolEra::PerRequest20260728 && capabilities.value("roots", Json()).is_object()) {
            capabilities["roots"].erase("listChanged");
        }
        return capabilities;
    }

    void Mcp::initialize() {
        state->era = ProtocolEra::Initialization20251125;
        request("initialize",
                {{"protocolVersion", protocolVersion2025},
                 {"clientInfo", mcp::detail::implementation(state->options.implementation)},
                 {"capabilities", clientCapabilities()}});
    }

    Id Mcp::request(const std::string& method, Json params, RequestOptions options) {
        return send(Operation{method, std::move(params), options, nullptr, {}, {}, 0, {}, {}});
    }

    Id Mcp::send(Operation operation) {
        const Id id = getRequestIdentifier();
        if (operation.root.is_null()) {
            operation.root = id;
        }
        if (!operation.params.is_object() || state->operations.size() >= 128 ||
            (!state->ready && operation.method != "initialize" && operation.method != "server/discover")) {
            invalid(operation.root, "Invalid MCP command or client not ready");
        } else {
            Json params = operation.params;
            if (!params.contains("_meta")) {
                params["_meta"] = Json::object();
            }
            if (!params["_meta"].is_object()) {
                throw std::invalid_argument("Request metadata must be an object");
            }
            if (state->era == ProtocolEra::PerRequest20260728) {
                params["_meta"][versionKey] = protocolVersion2026;
                params["_meta"][capabilitiesKey] = clientCapabilities();
                params["_meta"]["io.modelcontextprotocol/clientInfo"] = mcp::detail::implementation(state->options.implementation);
            }
            if (operation.options.progressToken) {
                params["_meta"]["progressToken"] = std::visit(
                    [](const auto& value) -> Json {
                        return value;
                    },
                    *operation.options.progressToken);
            }
            if (state->era == ProtocolEra::Initialization20251125 && operation.method == "tools/call" && operation.options.task) {
                params["task"] = {{"ttl", operation.options.ttlMs}};
            }
            if (params["_meta"].empty()) {
                params.erase("_meta");
            }
            operation.params = params;
            const auto timeout = operation.options.timeout;
            const auto method = operation.method;
            state->operations.emplace(id, std::move(operation));
            if (!Super::sendRequest(jsonrpc::messages::Request{id, method, std::optional<Json>(std::move(params))}, timeout)) {
                state->operations.erase(id);
                invalid(id, "JSON-RPC rejected MCP command");
            }
        }
        return id;
    }

    void Mcp::onRequest(const jsonrpc::messages::Request& value) {
        _onRequest(value);
    }

    void Mcp::onNotification(const jsonrpc::messages::Notification& value) {
        _onNotification(value);
    }

    void Mcp::onResult(const jsonrpc::messages::Result& value) {
        _onResult(value);
    }

    void Mcp::onError(const jsonrpc::messages::Error& value) {
        _onError(value);
    }

    void Mcp::invalid(const Id& id, const std::string& message) {
        log().warn() << message;
        onRemoteError(jsonrpc::messages::Error{id, -32603, message});
    }

    bool Mcp::cancelInputs(const Id* origin, bool incoming) {
        bool cancelled = false;
        const auto pending = state->inputs;
        for (const auto& weak : pending) {
            if (const auto input = weak.lock();
                input && input->callback && (!origin || (input->origin == *origin && input->incoming == incoming))) {
                input->timer.cancel();
                input->callback = {};
                cancelled = true;
                if (auto observer = std::exchange(input->cancelled, {})) {
                    try {
                        observer();
                    } catch (...) {
                        log().warn() << "Client input cancellation observer failed";
                    }
                }
            }
        }
        if (cancelled && origin && incoming) {
            discardRequest(*origin);
        }
        std::erase_if(state->inputs, [](const auto& weak) {
            const auto input = weak.lock();
            return !input || !input->callback;
        });
        return cancelled;
    }

    void Mcp::inputs(const Json& requests, std::function<void(Json)> completion, const Id& origin, bool incoming) {
        std::erase_if(state->inputs, [](const auto& weak) {
            const auto input = weak.lock();
            return !input || !input->callback;
        });
        if (!requests.is_object() || requests.size() > 16 || state->inputs.size() + requests.size() > 128) {
            throw std::invalid_argument("Invalid or excessive client inputs");
        }
        std::vector<std::pair<std::string, ClientInput>> parsed;
        for (const auto& [key, request] : requests.items()) {
            if (key.empty() || key.size() > 256 || !mcp::detail::missingInputCapabilities(request, state->capabilities).empty()) {
                throw std::invalid_argument("Unsupported client input request");
            }
            const Json params = request.value("params", Json::object());
            const std::string method = request.at("method");
            ClientInput input = RootsRequest{};
            if (method == "sampling/createMessage" && state->sampling) {
                input = SamplingRequest(params);
            } else if (method == "roots/list" && state->roots && params.is_object()) {
                input = RootsRequest{};
            } else if (method == "elicitation/create") {
                const std::string mode = params.value("mode", "form");
                if (mode == "form") {
                    input = FormInput{params.at("message").get<std::string>(), params.at("requestedSchema")};
                } else if (mode == "url") {
                    input = UrlInput{params.at("message").get<std::string>(), params.at("url").get<std::string>()};
                } else {
                    throw std::invalid_argument("Unknown elicitation mode");
                }
            } else {
                throw std::invalid_argument("Unsupported client input method");
            }
            static_cast<void>(mcp::detail::encodeInput(input));
            parsed.emplace_back(key, std::move(input));
        }
        struct Pending {
            std::size_t remaining;
            Json responses;
            std::function<void(Json)> complete;
        };
        const auto pending = std::make_shared<Pending>(Pending{parsed.size(), Json::object(), std::move(completion)});
        if (parsed.empty()) {
            pending->complete(Json::object());
        }
        for (const auto& [key, input] : parsed) {
            if (pending->remaining != 0) {
                const auto receiver = std::make_shared<detail::InputState>();
                receiver->request = mcp::detail::encodeInput(input);
                receiver->origin = origin;
                receiver->incoming = incoming;
                receiver->callback = [this, pending, key, origin, incoming](Json response) {
                    if (pending->remaining != 0) {
                        if (response.contains("error")) {
                            pending->remaining = 0;
                            cancelInputs(&origin, incoming);
                            const auto& error = response["error"];
                            if (incoming) {
                                sendError(origin,
                                          error["code"].get<std::int64_t>(),
                                          error["message"].get<std::string>(),
                                          error.contains("data") ? std::optional<Json>(error["data"]) : std::nullopt);
                            } else {
                                std::vector<std::string> jobs;
                                for (const auto& [id, watch] : state->tasks) {
                                    if (watch.origin.root == origin) {
                                        jobs.push_back(id);
                                    }
                                }
                                for (const auto& id : jobs) {
                                    state->tasks.at(id).timer.cancel();
                                    state->tasks.erase(id);
                                    cancelTask(id);
                                }
                                onRemoteError(
                                    jsonrpc::messages::Error{origin,
                                                             error["code"].get<std::int64_t>(),
                                                             error["message"].get<std::string>(),
                                                             error.contains("data") ? std::optional<Json>(error["data"]) : std::nullopt});
                            }
                        } else {
                            pending->responses[key] = std::move(response["result"]);
                            if (--pending->remaining == 0) {
                                auto complete = std::move(pending->complete);
                                complete(std::move(pending->responses));
                            }
                        }
                    }
                };
                receiver->timer = core::timer::Timer::singleshotTimer(
                    [weak = std::weak_ptr<detail::InputState>(receiver)] {
                        if (const auto current = weak.lock()) {
                            current->finish(current->request["method"] == "elicitation/create"
                                                ? Json{{"result", {{"action", "decline"}}}}
                                                : Json{{"error", {{"code", -32603}, {"message", "Client input timed out"}}}});
                        }
                    },
                    state->options.elicitationTimeout);
                state->inputs.push_back(receiver);
                if (std::holds_alternative<FormInput>(input) || std::holds_alternative<UrlInput>(input)) {
                    const auto reply = std::shared_ptr<ElicitationReply>(new ElicitationReply(receiver));
                    const std::variant<FormInput, UrlInput> request = std::holds_alternative<FormInput>(input)
                                                                          ? std::variant<FormInput, UrlInput>(std::get<FormInput>(input))
                                                                          : std::variant<FormInput, UrlInput>(std::get<UrlInput>(input));
                    try {
                        onElicitation(key, request, reply);
                    } catch (const std::exception&) {
                        reply->result({ElicitationAction::Decline});
                    }
                } else {
                    const auto reply = std::shared_ptr<ClientReply>(new ClientReply(receiver));
                    try {
                        if (std::holds_alternative<SamplingRequest>(input)) {
                            state->sampling(key, std::get<SamplingRequest>(input), reply);
                        } else {
                            state->roots(key, reply);
                        }
                    } catch (...) {
                        reply->error(ErrorCode::InternalError, "Client input handler failed");
                    }
                }
            }
        }
    }

    void Mcp::_onRequest(const jsonrpc::messages::Request& request) {
        if (request.id.is_null() || (request.params && !request.params->is_object())) {
            sendError(request.id, -32600, "Invalid Request");
        } else if (state->era == ProtocolEra::Initialization20251125 && request.method == "ping") {
            sendResult(request.id, Json::object());
        } else if (!state->ready || state->era != ProtocolEra::Initialization20251125 ||
                   (request.method != "elicitation/create" && request.method != "sampling/createMessage" &&
                    request.method != "roots/list") ||
                   (request.method == "sampling/createMessage" && !state->sampling) || (request.method == "roots/list" && !state->roots)) {
            Super::onRequest(request);
        } else {
            try {
                const auto metadata = request.params.value_or(Json::object()).value("_meta", Json::object());
                inputs(
                    {{request.id.dump(), {{"method", request.method}, {"params", request.params.value_or(Json::object())}}}},
                    [this, id = request.id, metadata](Json responses) {
                        Json result = responses.at(id.dump());
                        if (metadata.contains("io.modelcontextprotocol/related-task")) {
                            result["_meta"]["io.modelcontextprotocol/related-task"] = metadata["io.modelcontextprotocol/related-task"];
                        }
                        sendResult(id, result);
                    },
                    request.id,
                    true);
            } catch (const std::exception&) {
                sendError(request.id, -32602, "Invalid or unsupported client input request");
            }
        }
    }

    void Mcp::_onResult(const jsonrpc::messages::Result& response) {
        const auto found = state->operations.find(response.id);
        if (found != state->operations.end()) {
            auto operation = std::move(found->second);
            const Id root = operation.root;
            state->operations.erase(found);
            try {
                const auto& value = response.result;
                if (!value.is_object()) {
                    throw std::invalid_argument("MCP result must be an object");
                }
                if (operation.method == "server/discover") {
                    const auto versions = value.at("supportedVersions").get<std::vector<std::string>>();
                    if (std::find(versions.begin(), versions.end(), protocolVersion2026) == versions.end()) {
                        if (state->options.negotiation == Negotiation::Auto &&
                            std::find(versions.begin(), versions.end(), protocolVersion2025) != versions.end()) {
                            initialize();
                        } else {
                            invalid(operation.root, "Unsupported server protocol revision");
                        }
                    } else {
                        state->serverCapabilities = value.at("capabilities");
                        if (!state->serverCapabilities.is_object() || value.value("resultType", "") != "complete") {
                            throw std::invalid_argument("Invalid discovery result");
                        }
                        state->ready = true;
                        onReady(state->era, state->serverCapabilities);
                    }
                } else if (operation.method == "initialize") {
                    if (value.at("protocolVersion") != protocolVersion2025 || !value.at("capabilities").is_object()) {
                        throw std::invalid_argument("Invalid initialize result");
                    }
                    state->serverCapabilities = value["capabilities"];
                    sendNotification("notifications/initialized");
                    state->ready = true;
                    onReady(state->era, state->serverCapabilities);
                } else if (state->era == ProtocolEra::PerRequest20260728 && value.value("resultType", "") == "input_required") {
                    if ((operation.method != "tools/call" && operation.method != "resources/read" && operation.method != "prompts/get") ||
                        ++operation.rounds > 16 || (!value.contains("inputRequests") && !value.contains("requestState"))) {
                        throw std::invalid_argument("Invalid MRTR result or retry limit exceeded");
                    }
                    operation.params.erase("inputResponses");
                    operation.params.erase("requestState");
                    if (value.contains("requestState")) {
                        operation.params["requestState"] = value["requestState"].get<std::string>();
                    }
                    if (value.contains("inputRequests")) {
                        const Id origin = operation.root;
                        inputs(
                            value["inputRequests"],
                            [this, operation = std::move(operation)](Json responses) mutable {
                                operation.params["inputResponses"] = std::move(responses);
                                send(std::move(operation));
                            },
                            origin);
                    } else {
                        send(std::move(operation));
                    }
                } else if (operation.method == "tools/call" &&
                           ((state->era == ProtocolEra::PerRequest20260728 && value.value("resultType", "") == "task") ||
                            (state->era == ProtocolEra::Initialization20251125 && value.contains("task")))) {
                    if (state->tasks.size() >= 128) {
                        throw std::invalid_argument("Task watch limit exceeded");
                    }
                    const TaskInfo task = parseTask(state->era == ProtocolEra::PerRequest20260728 ? value : value.at("task"));
                    const Id root = operation.root;
                    if (state->tasks.contains(task.taskId)) {
                        throw std::invalid_argument("Duplicate task handle");
                    }
                    state->tasks.emplace(task.taskId, State::Watch{std::move(operation), task, {}, {}});
                    onTaskCreated(root, task);
                    if (state->options.pollTasks) {
                        scheduleTask(task.taskId);
                    }
                } else if (operation.method == "tasks/get") {
                    taskStatus(parseTask(value));
                } else if (operation.method == "tasks/update" && !operation.task.empty()) {
                    scheduleTask(operation.task);
                } else {
                    if (state->era == ProtocolEra::PerRequest20260728 && value.value("resultType", "") != "complete") {
                        throw std::invalid_argument("Invalid MCP result discriminator");
                    }
                    const std::string field = operation.method == "tools/list"                 ? "tools"
                                              : operation.method == "resources/list"           ? "resources"
                                              : operation.method == "resources/templates/list" ? "resourceTemplates"
                                              : operation.method == "prompts/list"             ? "prompts"
                                                                                               : "";
                    if (!field.empty()) {
                        const auto& page = value.at(field);
                        if (!page.is_array() || operation.entries.size() + page.size() > 16384) {
                            throw std::invalid_argument("Invalid or excessive list page");
                        }
                        operation.entries.insert(operation.entries.end(), page.begin(), page.end());
                        const Json cursor = value.value("nextCursor", Json());
                        if (state->options.followPagination && !cursor.is_null()) {
                            if (!cursor.is_string() || operation.cursors.size() >= 128 ||
                                !operation.cursors.insert(cursor.get<std::string>()).second) {
                                throw std::invalid_argument("Invalid or repeated list cursor");
                            }
                            operation.params["cursor"] = cursor;
                            send(std::move(operation));
                        } else {
                            Json aggregate = value;
                            aggregate[field] = std::move(operation.entries);
                            complete(std::move(operation), aggregate);
                        }
                    } else {
                        complete(std::move(operation), value);
                    }
                }
            } catch (const std::exception&) {
                invalid(root, "Invalid MCP result or application hook failure");
            }
        }
    }

    void Mcp::complete(Operation operation, const Json& result) {
        const Id id = operation.root;
        const std::string method = operation.method;
        if (!operation.task.empty() && method == "tasks/result") {
            const auto watch = state->tasks.find(operation.task);
            if (watch != state->tasks.end()) {
                operation = std::move(watch->second.origin);
                watch->second.timer.cancel();
                state->tasks.erase(watch);
                complete(std::move(operation), result);
            }
        } else if (method == "tools/list") {
            onTools(id, ToolsListResult{result.at("tools").get<std::vector<Json>>()});
        } else if (method == "resources/list") {
            onResources(id, ResourcesListResult{result.at("resources").get<std::vector<Json>>()});
        } else if (method == "resources/templates/list") {
            onResourceTemplates(id, ResourceTemplatesListResult{result.at("resourceTemplates").get<std::vector<Json>>()});
        } else if (method == "prompts/list") {
            onPrompts(id, PromptsListResult{result.at("prompts").get<std::vector<Json>>()});
        } else if (method == "tools/call") {
            onToolResult(id, ToolResult::fromJson(result));
        } else if (method == "resources/read") {
            onResourceResult(id, mcp::detail::decodeResourceResult(result));
        } else if (method == "prompts/get") {
            onPromptResult(id, mcp::detail::decodePromptResult(result));
        } else if (method == "completion/complete") {
            onCompletion(id, mcp::detail::decodeCompletionResult(result));
        } else if (method == "tasks/list") {
            std::vector<TaskInfo> tasks;
            for (const auto& item : result.at("tasks")) {
                tasks.push_back(parseTask(item));
            }
            const auto cursor = result.value("nextCursor", Json());
            onTasks(id, tasks, cursor.is_string() ? std::optional<std::string>(cursor.get<std::string>()) : std::nullopt);
        } else {
            if (method == "resources/subscribe") {
                state->resources.insert(operation.params.at("uri").get<std::string>());
            }
            if (method == "resources/unsubscribe") {
                state->resources.erase(operation.params.at("uri").get<std::string>());
            }
            if (method == "subscriptions/listen") {
                state->subscriptions.erase(id);
            }
            onOtherResult(id, method, result);
        }
    }

    void Mcp::_onError(const jsonrpc::messages::Error& error) {
        const auto found = state->operations.find(error.id);
        if (found == state->operations.end()) {
            onRemoteError(error);
        } else {
            auto operation = std::move(found->second);
            state->operations.erase(found);
            if (operation.method == "server/discover" && state->options.negotiation == Negotiation::Auto &&
                (error.code == -32601 || error.code == -32020 || error.code == -32022)) {
                initialize();
            } else if (error.code == -32602 && operation.params.contains("cursor") && !operation.entries.empty() && operation.rounds == 0) {
                operation.params.erase("cursor");
                operation.entries.clear();
                operation.cursors.clear();
                ++operation.rounds;
                send(std::move(operation));
            } else {
                if (!operation.task.empty()) {
                    const auto task = state->tasks.find(operation.task);
                    if (task != state->tasks.end()) {
                        task->second.timer.cancel();
                        state->tasks.erase(task);
                    }
                }
                onRemoteError(jsonrpc::messages::Error{operation.root, error.code, error.message, error.data});
            }
        }
    }

    void Mcp::onRequestTimeout(const Id& id) {
        const auto found = state->operations.find(id);
        if (found != state->operations.end()) {
            auto operation = std::move(found->second);
            state->operations.erase(found);
            if (operation.method == "server/discover" && state->options.negotiation == Negotiation::Auto) {
                initialize();
            } else {
                if (!operation.task.empty()) {
                    const auto watch = state->tasks.find(operation.task);
                    if (watch != state->tasks.end()) {
                        watch->second.timer.cancel();
                        state->tasks.erase(watch);
                    }
                }
                onTimeout(operation.root, operation.method);
            }
        }
    }

    TaskInfo Mcp::parseTask(const Json& value) const {
        const std::string status = value.at("status");
        if (status != "working" && status != "input_required" && status != "completed" && status != "failed" && status != "cancelled") {
            throw std::invalid_argument("Unknown task state");
        }
        const auto ttl = value.at(state->era == ProtocolEra::PerRequest20260728 ? "ttlMs" : "ttl");
        if (!ttl.is_null() && !ttl.is_number_unsigned()) {
            throw std::invalid_argument("Invalid task TTL");
        }
        return TaskInfo{value.at("taskId").get<std::string>(),
                        status == "working"          ? TaskState::Working
                        : status == "input_required" ? TaskState::InputRequired
                        : status == "completed"      ? TaskState::Completed
                        : status == "failed"         ? TaskState::Failed
                                                     : TaskState::Cancelled,
                        value.at("createdAt").get<std::string>(),
                        value.at("lastUpdatedAt").get<std::string>(),
                        ttl.is_null() ? std::nullopt : std::optional<std::uint64_t>(ttl.get<std::uint64_t>()),
                        value.value(state->era == ProtocolEra::PerRequest20260728 ? "pollIntervalMs" : "pollInterval", std::uint64_t(1000)),
                        value.value("result", Json()),
                        value.value("error", Json()),
                        value.value("inputRequests", Json::object())};
    }

    void Mcp::scheduleTask(const std::string& id) {
        const auto found = state->tasks.find(id);
        if (found != state->tasks.end() && state->options.pollTasks) {
            auto previous = std::move(found->second.timer);
            previous.cancel();
            found->second.timer = core::timer::Timer::singleshotTimer(
                [this, id] {
                    const auto current = state->tasks.find(id);
                    if (current != state->tasks.end()) {
                        const auto running = std::move(current->second.timer);
                        send({"tasks/get", {{"taskId", id}}, RequestOptions(), current->second.origin.root, {}, {}, 0, id, {}});
                    }
                },
                utils::Timeval(static_cast<double>(std::clamp(found->second.info.pollIntervalMs, std::uint64_t(1), std::uint64_t(60000))) /
                               1000.0));
        }
    }

    void Mcp::taskStatus(const TaskInfo& task) {
        onTaskStatus(task);
        const auto found = state->tasks.find(task.taskId);
        if (found != state->tasks.end()) {
            found->second.info = task;
            if (task.status == TaskState::Working) {
                scheduleTask(task.taskId);
            } else if (task.status == TaskState::InputRequired && state->era == ProtocolEra::PerRequest20260728) {
                Json pending = Json::object();
                for (const auto& [key, value] : task.inputRequests.items()) {
                    if (found->second.inputs.insert(key).second) {
                        pending[key] = value;
                    }
                }
                if (found->second.inputs.size() > 1024) {
                    throw std::invalid_argument("Task input limit exceeded");
                }
                if (!pending.empty()) {
                    const Id origin = found->second.origin.root;
                    inputs(
                        pending,
                        [this, id = task.taskId](Json responses) {
                            const auto current = state->tasks.find(id);
                            if (current != state->tasks.end() && current->second.info.status == TaskState::InputRequired) {
                                send({"tasks/update",
                                      {{"taskId", id}, {"inputResponses", std::move(responses)}},
                                      RequestOptions(),
                                      current->second.origin.root,
                                      {},
                                      {},
                                      0,
                                      id,
                                      {}});
                            }
                        },
                        origin);
                } else {
                    scheduleTask(task.taskId);
                }
            } else if (task.status == TaskState::InputRequired) {
                scheduleTask(task.taskId);
            } else if (state->era == ProtocolEra::Initialization20251125 && task.status != TaskState::Cancelled) {
                send({"tasks/result", {{"taskId", task.taskId}}, RequestOptions(), found->second.origin.root, {}, {}, 0, task.taskId, {}});
            } else {
                auto origin = std::move(found->second.origin);
                found->second.timer.cancel();
                state->tasks.erase(found);
                if (task.status == TaskState::Cancelled) {
                    onCancelled(origin.root, "Task cancelled");
                } else if (task.status == TaskState::Failed) {
                    onRemoteError(
                        jsonrpc::messages::Error{origin.root,
                                                 task.error.at("code").get<std::int64_t>(),
                                                 task.error.at("message").get<std::string>(),
                                                 task.error.contains("data") ? std::optional<Json>(task.error["data"]) : std::nullopt});
                } else {
                    complete(std::move(origin), task.result);
                }
            }
        }
    }

    void Mcp::_onNotification(const jsonrpc::messages::Notification& notification) {
        try {
            const Json params = notification.params.value_or(Json::object());
            const auto meta = params.value("_meta", Json::object());
            std::optional<Id> subscription;
            bool allowed = state->ready && state->era == ProtocolEra::Initialization20251125;
            if (meta.contains(subscriptionKey)) {
                subscription = meta[subscriptionKey];
            }
            if (notification.method == "notifications/subscriptions/acknowledged" && subscription) {
                const auto operation = state->operations.find(*subscription);
                const auto filter = params.at("notifications");
                if (operation != state->operations.end() && operation->second.method == "subscriptions/listen" && filter.is_object()) {
                    const auto requested = operation->second.params.at("notifications");
                    bool valid = true;
                    for (const auto& [key, value] : filter.items()) {
                        valid = valid && requested.contains(key);
                        if (value.is_array() && requested.value(key, Json()).is_array()) {
                            for (const auto& entry : value) {
                                valid = valid && std::find(requested[key].begin(), requested[key].end(), entry) != requested[key].end();
                            }
                        } else {
                            valid = valid && requested.value(key, Json()) == value;
                        }
                    }
                    if (valid) {
                        state->subscriptions[*subscription] = filter;
                        onSubscriptionAcknowledged(*subscription, filter);
                    }
                }
            } else {
                Json filter = Json::object();
                if (subscription && state->subscriptions.contains(*subscription)) {
                    filter = state->subscriptions.at(*subscription);
                }
                const std::string method = notification.method;
                if (method == "notifications/tools/list_changed" && (allowed || filter.value("toolsListChanged", false))) {
                    onToolsListChanged(subscription);
                } else if (method == "notifications/resources/list_changed" && (allowed || filter.value("resourcesListChanged", false))) {
                    onResourcesListChanged(subscription);
                } else if (method == "notifications/prompts/list_changed" && (allowed || filter.value("promptsListChanged", false))) {
                    onPromptsListChanged(subscription);
                } else if (method == "notifications/resources/updated") {
                    const auto uri = params.at("uri").get<std::string>();
                    const auto uris = filter.value("resourceSubscriptions", Json::array());
                    if ((allowed && state->resources.contains(uri)) || std::find(uris.begin(), uris.end(), Json(uri)) != uris.end()) {
                        onResourceUpdated(uri, subscription);
                    }
                } else if (method == "notifications/message" && allowed) {
                    const auto level = parseLoggingLevel(params.at("level").get<std::string>());
                    if (level) {
                        onLogMessage(LogMessage{*level, params.at("data"), params.value("logger", std::string())});
                    }
                } else if (method == "notifications/tasks/status" && allowed) {
                    taskStatus(parseTask(params));
                } else if (method == "notifications/tasks" && subscription) {
                    const auto ids = filter.value("taskIds", Json::array());
                    if (std::find(ids.begin(), ids.end(), params.at("taskId")) != ids.end()) {
                        taskStatus(parseTask(params));
                    }
                } else if (method == "notifications/progress") {
                    const auto token = params.at("progressToken");
                    const double progress = params.at("progress").get<double>();
                    const std::optional<double> total =
                        params.contains("total") ? std::optional<double>(params["total"].get<double>()) : std::nullopt;
                    bool correlated = false;
                    const auto accept = [&](Operation& operation) {
                        const auto metadata = operation.params.value("_meta", Json::object());
                        if (metadata.value("progressToken", Json()) == token && !token.is_null() &&
                            (!operation.progress || progress > *operation.progress)) {
                            operation.progress = progress;
                            correlated = true;
                        }
                    };
                    if (std::isfinite(progress) && progress >= 0 && (!total || (std::isfinite(*total) && *total >= progress))) {
                        for (auto& [id, operation] : state->operations) {
                            static_cast<void>(id);
                            accept(operation);
                        }
                        if (state->era == ProtocolEra::Initialization20251125) {
                            for (auto& [id, watch] : state->tasks) {
                                static_cast<void>(id);
                                accept(watch.origin);
                            }
                        }
                        if (correlated && (token.is_string() || token.is_number_integer())) {
                            const ProgressToken parsed =
                                token.is_string() ? ProgressToken(token.get<std::string>()) : ProgressToken(token.get<std::int64_t>());
                            onProgress(parsed, Progress{progress, total, params.value("message", std::string())});
                        }
                    }
                } else if (method == "notifications/cancelled") {
                    const auto id = params.at("requestId");
                    const auto operation = state->operations.find(id);
                    if (operation != state->operations.end()) {
                        const Id root = operation->second.root;
                        cancelRequest(id);
                        state->operations.erase(operation);
                        state->subscriptions.erase(id);
                        cancelInputs(&root);
                        onCancelled(root, params.value("reason", std::string()));
                    } else if (cancelInputs(&id, true)) {
                        onCancelled(id, params.value("reason", std::string()));
                    }
                }
            }
        } catch (const std::exception&) {
            log().warn() << "Invalid MCP notification or application hook failure";
        }
    }

    void Mcp::cancel(const Id& id, const std::string& reason) {
        cancelInputs(&id);
        std::vector<std::string> jobs;
        for (const auto& [taskId, watch] : state->tasks) {
            if (watch.origin.root == id) {
                jobs.push_back(taskId);
            }
        }
        for (const auto& taskId : jobs) {
            cancelTask(taskId);
        }
        std::vector<Id> pending;
        for (const auto& [wire, operation] : state->operations) {
            if ((wire == id || operation.root == id) && operation.task.empty()) {
                pending.push_back(wire);
            }
        }
        for (const auto& wire : pending) {
            Json params{{"requestId", wire}};
            if (!reason.empty()) {
                params["reason"] = reason;
            }
            if (state->era == ProtocolEra::PerRequest20260728) {
                params["_meta"] = {{versionKey, protocolVersion2026},
                                   {capabilitiesKey, clientCapabilities()},
                                   {"io.modelcontextprotocol/clientInfo", mcp::detail::implementation(state->options.implementation)}};
            }
            cancelRequest(wire);
            state->operations.erase(wire);
            state->subscriptions.erase(wire);
            sendNotification("notifications/cancelled", params);
        }
    }

    Id Mcp::listTools() {
        return request("tools/list");
    }

    Id Mcp::listResources() {
        return request("resources/list");
    }

    Id Mcp::listResourceTemplates() {
        return request("resources/templates/list");
    }

    Id Mcp::listPrompts() {
        return request("prompts/list");
    }

    Id Mcp::callTool(const std::string& name, Json arguments, RequestOptions options) {
        return request("tools/call", {{"name", name}, {"arguments", std::move(arguments)}}, options);
    }

    Id Mcp::readResource(const std::string& uri, RequestOptions options) {
        return request("resources/read", {{"uri", uri}}, options);
    }

    Id Mcp::getPrompt(const std::string& name, Json arguments, RequestOptions options) {
        return request("prompts/get", {{"name", name}, {"arguments", std::move(arguments)}}, options);
    }

    Id Mcp::complete(Json reference, const std::string& name, const std::string& value, Json arguments) {
        return request("completion/complete",
                       {{"ref", std::move(reference)},
                        {"argument", {{"name", name}, {"value", value}}},
                        {"context", {{"arguments", std::move(arguments)}}}});
    }

    Id Mcp::subscribe(const std::string& uri) {
        Id id;
        if (state->era == ProtocolEra::Initialization20251125) {
            id = request("resources/subscribe", {{"uri", uri}});
        } else {
            id = listen({{"resourceSubscriptions", {uri}}});
        }
        return id;
    }

    Id Mcp::unsubscribe(const std::string& uri) {
        Id id;
        if (state->era == ProtocolEra::Initialization20251125) {
            id = request("resources/unsubscribe", {{"uri", uri}});
        } else {
            const auto found = std::find_if(state->operations.rbegin(), state->operations.rend(), [&uri](const auto& entry) {
                return entry.second.method == "subscriptions/listen" &&
                       entry.second.params.value("notifications", Json::object()) == Json{{"resourceSubscriptions", {uri}}};
            });
            if (found != state->operations.rend()) {
                id = found->first;
                cancel(id);
            }
        }
        return id;
    }

    Id Mcp::listen(Json notifications) {
        return request("subscriptions/listen", {{"notifications", std::move(notifications)}}, RequestOptions{utils::Timeval(0)});
    }

    Id Mcp::setLoggingLevel(LogLevel level) {
        Id id;
        if (state->era == ProtocolEra::Initialization20251125) {
            id = request("logging/setLevel", {{"level", loggingLevelName(level)}});
        } else {
            id = getRequestIdentifier();
            onRemoteError(jsonrpc::messages::Error{id, -32601, "Logging is unavailable in this revision"});
        }
        return id;
    }

    Id Mcp::getTask(const std::string& id) {
        return request("tasks/get", {{"taskId", id}});
    }

    Id Mcp::getTaskResult(const std::string& task) {
        Id id;
        if (state->era == ProtocolEra::Initialization20251125) {
            id = request("tasks/result", {{"taskId", task}}, RequestOptions{utils::Timeval(0)});
        } else {
            id = getRequestIdentifier();
            onRemoteError(jsonrpc::messages::Error{id, -32601, "Use tasks/get for this revision"});
        }
        return id;
    }

    Id Mcp::listTasks(const std::optional<std::string>& cursor) {
        Id id;
        if (state->era == ProtocolEra::Initialization20251125) {
            id = request("tasks/list", cursor ? Json{{"cursor", *cursor}} : Json::object());
        } else {
            id = getRequestIdentifier();
            onRemoteError(jsonrpc::messages::Error{id, -32601, "Task listing is unavailable in this revision"});
        }
        return id;
    }

    Id Mcp::cancelTask(const std::string& id) {
        return request("tasks/cancel", {{"taskId", id}});
    }

    Id Mcp::updateTask(const std::string& id, Json responses) {
        return request("tasks/update", {{"taskId", id}, {"inputResponses", std::move(responses)}});
    }

    void Mcp::onReady([[maybe_unused]] ProtocolEra era, [[maybe_unused]] const Json& capabilities) {
    }

    void Mcp::onTools([[maybe_unused]] const Id& id, [[maybe_unused]] const ToolsListResult& result) {
    }

    void Mcp::onResources([[maybe_unused]] const Id& id, [[maybe_unused]] const ResourcesListResult& result) {
    }

    void Mcp::onResourceTemplates([[maybe_unused]] const Id& id, [[maybe_unused]] const ResourceTemplatesListResult& result) {
    }

    void Mcp::onPrompts([[maybe_unused]] const Id& id, [[maybe_unused]] const PromptsListResult& result) {
    }

    void Mcp::onToolResult([[maybe_unused]] const Id& id, [[maybe_unused]] const ToolResult& result) {
    }

    void Mcp::onResourceResult([[maybe_unused]] const Id& id, [[maybe_unused]] const ResourceResult& result) {
    }

    void Mcp::onPromptResult([[maybe_unused]] const Id& id, [[maybe_unused]] const PromptResult& result) {
    }

    void Mcp::onCompletion([[maybe_unused]] const Id& id, [[maybe_unused]] const CompletionResult& result) {
    }

    void Mcp::onToolsListChanged([[maybe_unused]] const std::optional<Id>& subscription) {
    }

    void Mcp::onResourcesListChanged([[maybe_unused]] const std::optional<Id>& subscription) {
    }

    void Mcp::onPromptsListChanged([[maybe_unused]] const std::optional<Id>& subscription) {
    }

    void Mcp::onResourceUpdated([[maybe_unused]] const std::string& uri, [[maybe_unused]] const std::optional<Id>& subscription) {
    }

    void Mcp::onSubscriptionAcknowledged([[maybe_unused]] const Id& id, [[maybe_unused]] const Json& notifications) {
    }

    void Mcp::onLogMessage([[maybe_unused]] const LogMessage& message) {
    }

    void Mcp::onTaskCreated([[maybe_unused]] const Id& id, [[maybe_unused]] const TaskInfo& task) {
    }

    void Mcp::onTaskStatus([[maybe_unused]] const TaskInfo& task) {
    }

    void Mcp::onTasks([[maybe_unused]] const Id& id,
                      [[maybe_unused]] const std::vector<TaskInfo>& tasks,
                      [[maybe_unused]] const std::optional<std::string>& cursor) {
    }

    void Mcp::onProgress([[maybe_unused]] const ProgressToken& token, [[maybe_unused]] const Progress& progress) {
    }

    void Mcp::onCancelled([[maybe_unused]] const Id& id, [[maybe_unused]] const std::string& reason) {
    }

    void Mcp::onRemoteError([[maybe_unused]] const jsonrpc::messages::Error& error) {
    }

    void Mcp::onTimeout([[maybe_unused]] const Id& id, [[maybe_unused]] const std::string& method) {
    }

    void
    Mcp::onOtherResult([[maybe_unused]] const Id& id, [[maybe_unused]] const std::string& method, [[maybe_unused]] const Json& result) {
    }

    void Mcp::onElicitation([[maybe_unused]] const std::string& key,
                            [[maybe_unused]] const std::variant<FormInput, UrlInput>& request,
                            const std::shared_ptr<ElicitationReply>& reply) {
        reply->result(InputResponse{ElicitationAction::Decline});
    }
} // namespace mcp::client
