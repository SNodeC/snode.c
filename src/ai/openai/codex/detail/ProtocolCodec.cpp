/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ProtocolCodec.h"

#include "ai/openai/codex/AppServerClient.h"

// IWYU pragma: no_include <map>
// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>
// IWYU pragma: no_include <nlohmann/json_fwd.hpp>

#include <cmath>
#include <exception>
#include <limits>
#include <stdexcept>
#include <utility>

namespace ai::openai::codex::detail {

    namespace {
        bool readRequiredString(const Json& object, const char* field, std::string& value, std::string& errorMessage) {
            const auto iterator = object.find(field);
            if (iterator == object.end() || !iterator->is_string()) {
                errorMessage = std::string("initialize result is missing string field '") + field + "'";
                return false;
            }

            value = iterator->get<std::string>();
            return true;
        }

        std::optional<ProtocolId> decodeProtocolId(const Json& id, std::string_view context, std::string& errorMessage) {
            if (id.is_string()) {
                return ProtocolId{id.get<std::string>()};
            }

            if (id.is_number_unsigned()) {
                const std::uint64_t value = id.get<std::uint64_t>();
                if (value <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                    return ProtocolId{static_cast<std::int64_t>(value)};
                }
            } else if (id.is_number_integer()) {
                return ProtocolId{id.get<std::int64_t>()};
            }

            errorMessage = std::string(context) + " id is neither a string nor a signed 64-bit integer";
            return std::nullopt;
        }

        Json encodeProtocolId(const ProtocolId& id) {
            return std::visit(
                [](const auto& value) -> Json {
                    return value;
                },
                id);
        }

        bool isRepresentableJson(const Json& value) {
            if (value.is_discarded() || value.is_binary()) {
                return false;
            }
            if (value.is_number_float() && !std::isfinite(value.get<double>())) {
                return false;
            }
            if (value.is_array() || value.is_object()) {
                for (const Json& child : value) {
                    if (!isRepresentableJson(child)) {
                        return false;
                    }
                }
            }
            return true;
        }

        std::optional<std::string> encode(Json message, std::string_view context, std::string& errorMessage) {
            errorMessage.clear();

            try {
                if (!isRepresentableJson(message)) {
                    errorMessage = std::string("unable to encode ") + std::string(context) + ": value is not representable as JSON";
                    return std::nullopt;
                }
                return message.dump();
            } catch (const Json::exception& exception) {
                errorMessage = std::string("unable to encode ") + std::string(context) + ": " + exception.what();
            } catch (const std::exception& exception) {
                errorMessage = std::string("unable to encode ") + std::string(context) + ": " + exception.what();
            }

            return std::nullopt;
        }

        bool decodeError(const Json& responseError, ProtocolError& error, std::string& errorMessage) {
            if (!responseError.is_object()) {
                errorMessage = "protocol error response is not an object";
                return false;
            }

            const auto code = responseError.find("code");
            const auto message = responseError.find("message");
            if (code == responseError.end() || !code->is_number_integer() || message == responseError.end() || !message->is_string()) {
                errorMessage = "protocol error response has invalid code or message";
                return false;
            }

            if (code->is_number_unsigned()) {
                const std::uint64_t value = code->get<std::uint64_t>();
                if (value > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                    errorMessage = "protocol error response code is outside the signed 64-bit integer range";
                    return false;
                }
                error.code = static_cast<std::int64_t>(value);
            } else {
                error.code = code->get<std::int64_t>();
            }

            error.message = message->get<std::string>();
            const auto data = responseError.find("data");
            if (data != responseError.end()) {
                error.data = *data;
            }

            return true;
        }
    } // namespace

    std::optional<InitializeResult> decodeInitializeResult(const Json& result, std::string& errorMessage) {
        errorMessage.clear();

        try {
            if (!result.is_object()) {
                errorMessage = "initialize result is not an object";
                return std::nullopt;
            }

            InitializeResult initializeResult;
            if (!readRequiredString(result, "codexHome", initializeResult.codexHome, errorMessage) ||
                !readRequiredString(result, "platformFamily", initializeResult.platformFamily, errorMessage) ||
                !readRequiredString(result, "platformOs", initializeResult.platformOs, errorMessage) ||
                !readRequiredString(result, "userAgent", initializeResult.userAgent, errorMessage)) {
                return std::nullopt;
            }

            initializeResult.raw = result;
            return initializeResult;
        } catch (const Json::exception& exception) {
            errorMessage = std::string("unable to decode initialize result: ") + exception.what();
        } catch (const std::exception& exception) {
            errorMessage = std::string("unable to decode initialize result: ") + exception.what();
        }

        return std::nullopt;
    }

    std::optional<std::string>
    ProtocolCodec::encodeRequest(std::int64_t id, std::string_view method, const Json& params, std::string& errorMessage) {
        return encode({{"method", method}, {"id", id}, {"params", params}}, "protocol request", errorMessage);
    }

    std::optional<std::string> ProtocolCodec::encodeNotification(std::string_view method, const Json& params, std::string& errorMessage) {
        return encode({{"method", method}, {"params", params}}, "protocol notification", errorMessage);
    }

    std::optional<std::string> ProtocolCodec::encodeSuccessResponse(const ProtocolId& id, const Json& result, std::string& errorMessage) {
        return encode({{"id", encodeProtocolId(id)}, {"result", result}}, "protocol response", errorMessage);
    }

    std::optional<std::string>
    ProtocolCodec::encodeErrorResponse(const ProtocolId& id, const ProtocolError& error, std::string& errorMessage) {
        Json encodedError = {{"code", error.code}, {"message", error.message}};
        if (error.data) {
            encodedError["data"] = *error.data;
        }

        return encode({{"id", encodeProtocolId(id)}, {"error", std::move(encodedError)}}, "protocol error response", errorMessage);
    }

    std::string ProtocolCodec::initializeRequest(std::int64_t id, const ClientInfo& clientInfo) {
        const Json params = {{"clientInfo", {{"name", clientInfo.name}, {"title", clientInfo.title}, {"version", clientInfo.version}}}};
        std::string errorMessage;
        std::optional<std::string> message = encodeRequest(id, "initialize", params, errorMessage);
        if (!message) {
            throw std::runtime_error(errorMessage);
        }

        return std::move(*message);
    }

    std::string ProtocolCodec::initializedNotification() {
        std::string errorMessage;
        std::optional<std::string> message = encodeNotification("initialized", Json::object(), errorMessage);
        if (!message) {
            throw std::runtime_error(errorMessage);
        }

        return std::move(*message);
    }

    std::optional<ProtocolMessage> ProtocolCodec::decode(std::string_view wireMessage, std::string& errorMessage) {
        errorMessage.clear();

        try {
            const Json message = Json::parse(wireMessage);
            if (!message.is_object()) {
                errorMessage = "protocol message is not a JSON object";
                return std::nullopt;
            }

            const auto method = message.find("method");
            const auto id = message.find("id");
            const auto result = message.find("result");
            const auto responseError = message.find("error");
            const bool hasMethod = method != message.end();
            const bool hasId = id != message.end();
            const bool hasResult = result != message.end();
            const bool hasError = responseError != message.end();

            if (hasMethod && !method->is_string()) {
                errorMessage = "protocol method is not a string";
                return std::nullopt;
            }

            std::optional<ProtocolId> decodedId;
            if (hasId) {
                decodedId = decodeProtocolId(*id, hasMethod ? "protocol request" : "protocol response", errorMessage);
                if (!decodedId) {
                    return std::nullopt;
                }
            }

            if (hasResult && hasError) {
                errorMessage = "protocol response must contain exactly one of result or error";
                return std::nullopt;
            }

            if (hasResult || hasError) {
                if (hasMethod) {
                    errorMessage = "protocol envelope ambiguously contains both a method and a response";
                    return std::nullopt;
                }
                if (!hasId) {
                    errorMessage = "protocol response is missing an id";
                    return std::nullopt;
                }

                ProtocolMessage decoded;
                decoded.kind = ProtocolMessage::Kind::Response;
                decoded.id = std::move(decodedId);
                decoded.raw = message;
                if (hasResult) {
                    decoded.result = *result;
                } else {
                    ProtocolError error;
                    if (!decodeError(*responseError, error, errorMessage)) {
                        return std::nullopt;
                    }
                    decoded.error = std::move(error);
                }

                return decoded;
            }

            if (hasMethod) {
                ProtocolMessage decoded;
                decoded.kind = hasId ? ProtocolMessage::Kind::Request : ProtocolMessage::Kind::Notification;
                decoded.id = std::move(decodedId);
                decoded.method = method->get<std::string>();
                const auto params = message.find("params");
                if (params != message.end()) {
                    decoded.params = *params;
                }
                decoded.raw = message;
                return decoded;
            }

            if (hasId) {
                errorMessage = "protocol response must contain exactly one of result or error";
            } else {
                errorMessage = "protocol envelope is neither a request, notification, nor response";
            }
            return std::nullopt;
        } catch (const Json::exception& exception) {
            errorMessage = std::string("invalid JSON protocol message: ") + exception.what();
            return std::nullopt;
        } catch (const std::exception& exception) {
            errorMessage = std::string("unable to decode protocol message: ") + exception.what();
            return std::nullopt;
        }
    }

} // namespace ai::openai::codex::detail
