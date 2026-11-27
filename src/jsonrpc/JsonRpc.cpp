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

#include "jsonrpc/JsonRpc.h"

#include "core/socket/stream/SocketConnection.h"
#include "jsonrpc/JsonRpcContext.h"
#include "jsonrpc/messages/Error.h"
#include "jsonrpc/messages/ErrorCode.h"
#include "jsonrpc/messages/Notification.h"
#include "jsonrpc/messages/Request.h"
#include "jsonrpc/messages/Result.h"
#include "log/LogScopeOwner.h"
#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cmath>
#include <functional>
#include <limits>
#include <stdexcept>
#include <utility>
#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace jsonrpc {

    using nlohmann::json;

    JsonRpc::JsonRpc(const std::string& connectionName)
        : connectionName(connectionName) {
    }

    JsonRpc::~JsonRpc() {
        for (auto& [id, timer] : outgoing) {
            static_cast<void>(id);
            timer.cancel();
        }
    }

    const logger::BoundaryLogger& JsonRpc::log() const {
        const unsigned long generation = logger::LogManager::generation();
        if (!log_ || logGeneration_ != generation) {
            logGeneration_ = generation;
            const auto* connection = jsonRpcContext != nullptr ? jsonRpcContext->getSocketConnection() : nullptr;
            log_.emplace(logger::LogScopeOwner(logger::LogOrigin::Framework,
                                               logger::LogBoundary::Connection,
                                               "jsonrpc",
                                               connection != nullptr ? std::optional<std::string>(connection->getInstanceName())
                                                                     : std::optional<std::string>(connectionName),
                                               std::nullopt,
                                               connection != nullptr
                                                   ? std::optional<std::string>(std::to_string(connection->getConnectionId()))
                                                   : std::nullopt)
                             .logger(logger::Logger::semanticSink()));
        }
        return *log_;
    }

    void JsonRpc::setJsonRpcContext(JsonRpcContext* context) {
        jsonRpcContext = context;
        log_.reset();
    }

    const JsonRpcContext* JsonRpc::getJsonRpcContext() const {
        return jsonRpcContext;
    }

    const std::string& JsonRpc::getConnectionName() const {
        return connectionName;
    }

    void JsonRpc::onConnected() {
        if (!connected) {
            connected = true;
            log().info() << "jsonrpc protocol started";
        }
    }

    void JsonRpc::onDisconnected() {
        for (auto& [id, timer] : outgoing) {
            static_cast<void>(id);
            timer.cancel();
        }
        outgoing.clear();
        incoming.clear();
        if (connected) {
            connected = false;
            log().info() << "jsonrpc protocol stopped";
        }
    }

    bool JsonRpc::onSignal([[maybe_unused]] int sig) {
        return false;
    }

    void JsonRpc::setBatchesEnabled(bool enabled) {
        batchesEnabled = enabled;
    }

    void JsonRpc::setMaximumMessageSize(std::size_t size) {
        maximumMessageSize = size;
    }

    void JsonRpc::setMaximumNestingDepth(std::size_t depth) {
        maximumNestingDepth = depth;
    }

    std::size_t JsonRpc::getMaximumMessageSize() const {
        return maximumMessageSize;
    }

    bool JsonRpc::isRequest(const json& value) {
        return value.is_object() && value.value("jsonrpc", json()) == "2.0" && value.value("method", json()).is_string() &&
               (!value.contains("id") || messages::isValidId(value["id"])) &&
               (!value.contains("params") || value["params"].is_object() || value["params"].is_array());
    }

    bool JsonRpc::isResponse(const json& value) {
        const json error = value.is_object() ? value.value("error", json()) : json();
        return value.is_object() && value.value("jsonrpc", json()) == "2.0" && !value.contains("method") && value.contains("id") &&
               messages::isValidId(value["id"]) && value.contains("result") != value.contains("error") &&
               (!value.contains("error") ||
                (error.is_object() && error.value("code", json()).is_number_integer() && error.value("message", json()).is_string()));
    }

    void JsonRpc::onMessage(const std::string& message) {
        if (message.size() > maximumMessageSize) {
            send(messages::Error{nullptr, messages::InvalidRequest, "Message size limit exceeded"}.toJson());
        } else {
            json value;
            bool parsed = false;
            try {
                value = json::parse(message, [this](int depth, json::parse_event_t event, json&) {
                    if ((event == json::parse_event_t::object_start || event == json::parse_event_t::array_start) &&
                        static_cast<std::size_t>(depth) >= maximumNestingDepth) {
                        throw std::length_error("JSON nesting depth limit exceeded");
                    }
                    return true;
                });
                parsed = true;
            } catch (const json::exception&) {
                send(messages::Error{nullptr, messages::ParseError, "Parse error"}.toJson());
            } catch (const std::length_error&) {
                send(messages::Error{nullptr, messages::InvalidRequest, "JSON nesting depth limit exceeded"}.toJson());
            }
            if (parsed) {
                if (value.is_array()) {
                    if (!batchesEnabled || value.empty()) {
                        send(messages::Error{nullptr, messages::InvalidRequest, "Invalid Request"}.toJson());
                    } else {
                        const auto batch = std::make_shared<Batch>();
                        for (const auto& item : value) {
                            dispatch(item, batch);
                        }
                        batch->dispatching = false;
                        finishBatch(batch);
                    }
                } else {
                    dispatch(value, {});
                }
            }
        }
    }

    void JsonRpc::dispatch(const json& value, const std::shared_ptr<Batch>& batch) {
        if (isRequest(value)) {
            const auto params = value.contains("params") ? std::optional<json>(value["params"]) : std::nullopt;
            const std::string method = value["method"].get<std::string>();
            if (value.contains("id")) {
                _onRequest(messages::Request{value["id"], method, params}, batch);
            } else {
                _onNotification(messages::Notification{method, params});
            }
        } else if (isResponse(value)) {
            if (value.contains("result")) {
                _onResult(messages::Result{value["id"], value["result"]});
            } else {
                const auto& error = value["error"];
                _onError(messages::Error{value["id"],
                                         error["code"].get<std::int64_t>(),
                                         error["message"].get<std::string>(),
                                         error.contains("data") ? std::optional<json>(error["data"]) : std::nullopt});
            }
        } else if (value.is_object() && !value.contains("method") && (value.contains("result") || value.contains("error"))) {
            log().warn() << "invalid response dropped";
        } else {
            const bool notification = value.is_object() && value.value("jsonrpc", json()) == "2.0" &&
                                      value.value("method", json()).is_string() && !value.contains("id");
            if (!notification) {
                const auto id = value.is_object() && messages::isValidId(value.value("id", json())) ? value.value("id", json()) : json();
                emit(messages::Error{id, messages::InvalidRequest, "Invalid Request"}.toJson(), batch);
            }
        }
    }

    void JsonRpc::_onRequest(const messages::Request& request, const std::shared_ptr<Batch>& batch) {
        if (incoming.emplace(request.id, batch).second) {
            if (batch) {
                ++batch->unanswered;
            }
            onRequest(request);
        } else {
            log().warn() << "duplicate incoming request id: " << request.id;
            emit(messages::Error{request.id, messages::InvalidRequest, "Duplicate id"}.toJson(), batch);
        }
    }

    void JsonRpc::_onNotification(const messages::Notification& notification) {
        onNotification(notification);
    }

    void JsonRpc::_onResult(const messages::Result& result) {
        if (outgoing.contains(result.id)) {
            if (validateResult(result)) {
                cancelRequest(result.id);
                onResult(result);
            }
        } else {
            log().warn() << "unknown response id: " << result.id;
        }
    }

    void JsonRpc::_onError(const messages::Error& error) {
        if (error.id.is_null() || cancelRequest(error.id)) {
            onError(error);
        } else {
            log().warn() << "unknown response id: " << error.id;
        }
    }

    void JsonRpc::onRequest(const messages::Request& request) {
        sendError(request.id, messages::MethodNotFound, "Method not found");
    }

    void JsonRpc::onNotification([[maybe_unused]] const messages::Notification& notification) {
    }

    bool JsonRpc::validateResult([[maybe_unused]] const messages::Result& result) const {
        return true;
    }

    void JsonRpc::onResult([[maybe_unused]] const messages::Result& result) {
    }

    void JsonRpc::onError([[maybe_unused]] const messages::Error& error) {
    }

    void JsonRpc::onRequestTimeout([[maybe_unused]] const messages::Id& id) {
    }

    messages::Id JsonRpc::getRequestIdentifier() {
        do {
            requestIdentifier = requestIdentifier == std::numeric_limits<std::int64_t>::max() ? 1 : requestIdentifier + 1;
        } while (outgoing.contains(messages::Id(requestIdentifier)));
        return requestIdentifier;
    }

    messages::Id JsonRpc::sendRequest(const std::string& method, const json& params, const utils::Timeval& timeout) {
        const auto id = getRequestIdentifier();
        sendRequest(messages::Request{id, method, params.is_null() ? std::nullopt : std::optional<json>(params)}, timeout);
        return id;
    }

    bool JsonRpc::sendRequest(const messages::Request& request, const utils::Timeval& timeout) {
        bool accepted = false;
        if (!request.id.is_null() && isRequest(request.toJson()) && !outgoing.contains(request.id)) {
            auto& timer = outgoing[request.id];
            if (timeout > 0) {
                timer = core::timer::Timer::singleshotTimer(
                    [this, id = request.id] {
                        if (cancelRequest(id)) {
                            onRequestTimeout(id);
                        }
                    },
                    timeout);
            }
            accepted = true;
            send(request.toJson());
        } else {
            log().warn() << "invalid or duplicate outgoing request id: " << request.id;
        }
        return accepted;
    }

    bool JsonRpc::cancelRequest(const messages::Id& id) {
        const auto found = outgoing.find(id);
        const bool present = found != outgoing.end();
        if (present) {
            found->second.cancel();
            outgoing.erase(found);
        }
        return present;
    }

    void JsonRpc::sendNotification(const std::string& method, const json& params) {
        const messages::Notification notification{method, params.is_null() ? std::nullopt : std::optional<json>(params)};
        if (isRequest(notification.toJson())) {
            send(notification.toJson());
        } else {
            log().warn() << "invalid outgoing notification";
        }
    }

    void JsonRpc::sendResult(const messages::Id& id, const json& result) {
        answer(id, messages::Result{id, result}.toJson());
    }

    void JsonRpc::sendError(const messages::Id& id, std::int64_t code, const std::string& message, std::optional<json> data) {
        answer(id, messages::Error{id, code, message, std::move(data)}.toJson());
    }

    void JsonRpc::answer(const messages::Id& id, const json& response) {
        const auto found = incoming.find(id);
        if (found != incoming.end()) {
            const auto batch = found->second;
            incoming.erase(found);
            if (batch) {
                --batch->unanswered;
            }
            emit(response, batch);
        } else {
            log().warn() << "duplicate or unknown answer id: " << id;
        }
    }

    void JsonRpc::discardRequest(const messages::Id& id) {
        const auto found = incoming.find(id);
        if (found != incoming.end()) {
            const auto batch = found->second;
            incoming.erase(found);
            if (batch) {
                --batch->unanswered;
                finishBatch(batch);
            }
        }
    }

    void JsonRpc::emit(const json& response, const std::shared_ptr<Batch>& batch) {
        if (batch) {
            batch->responses.push_back(response);
            finishBatch(batch);
        } else {
            send(response);
        }
    }

    void JsonRpc::finishBatch(const std::shared_ptr<Batch>& batch) {
        if (!batch->dispatching && batch->unanswered == 0 && !batch->responses.empty()) {
            const json responses = std::exchange(batch->responses, json::array());
            send(responses);
        }
    }

    void JsonRpc::send(const json& value) {
        if (jsonRpcContext != nullptr) {
            jsonRpcContext->sendMessage(value.dump());
        }
    }

} // namespace jsonrpc
