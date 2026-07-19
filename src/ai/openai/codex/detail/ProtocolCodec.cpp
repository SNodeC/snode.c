/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ProtocolCodec.h"

#include "ai/openai/codex/AppServerClient.h"

#include <nlohmann/json.hpp>

// IWYU pragma: no_include <map>
// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>
// IWYU pragma: no_include <nlohmann/json_fwd.hpp>

#include <exception>
#include <utility>

namespace ai::openai::codex::detail {

    namespace {
        using Json = nlohmann::json;

        bool readRequiredString(const Json& object, const char* field, std::string& value, std::string& errorMessage) {
            const auto iterator = object.find(field);
            if (iterator == object.end() || !iterator->is_string()) {
                errorMessage = std::string("initialize result is missing string field '") + field + "'";
                return false;
            }

            value = iterator->get<std::string>();
            return true;
        }
    } // namespace

    std::string ProtocolCodec::initializeRequest(std::int64_t id, const ClientInfo& clientInfo) {
        const Json message = {
            {"method", "initialize"},
            {"id", id},
            {"params", {{"clientInfo", {{"name", clientInfo.name}, {"title", clientInfo.title}, {"version", clientInfo.version}}}}}};

        return message.dump();
    }

    std::string ProtocolCodec::initializedNotification() {
        return Json({{"method", "initialized"}, {"params", Json::object()}}).dump();
    }

    std::optional<ProtocolMessage> ProtocolCodec::decode(std::string_view wireMessage, std::string& errorMessage) {
        try {
            const Json message = Json::parse(wireMessage);
            if (!message.is_object()) {
                errorMessage = "protocol message is not a JSON object";
                return std::nullopt;
            }

            ProtocolMessage decoded;
            const auto method = message.find("method");
            const auto id = message.find("id");

            if (method != message.end()) {
                if (!method->is_string()) {
                    errorMessage = "protocol method is not a string";
                    return std::nullopt;
                }

                decoded.method = method->get<std::string>();
                if (id == message.end()) {
                    decoded.kind = ProtocolMessage::Kind::Notification;
                } else if (id->is_number_integer()) {
                    decoded.kind = ProtocolMessage::Kind::Request;
                    decoded.id = id->get<std::int64_t>();
                } else if (id->is_string()) {
                    decoded.kind = ProtocolMessage::Kind::Request;
                } else {
                    errorMessage = "protocol request id is neither a string nor an integer";
                    return std::nullopt;
                }

                return decoded;
            }

            if (id == message.end()) {
                errorMessage = "protocol response is missing an id";
                return std::nullopt;
            }

            decoded.kind = ProtocolMessage::Kind::Response;
            if (id->is_number_integer()) {
                decoded.id = id->get<std::int64_t>();
            } else if (!id->is_string()) {
                errorMessage = "protocol response id is neither a string nor an integer";
                return std::nullopt;
            }

            const auto result = message.find("result");
            const auto responseError = message.find("error");
            if ((result == message.end()) == (responseError == message.end())) {
                errorMessage = "protocol response must contain exactly one of result or error";
                return std::nullopt;
            }

            if (responseError != message.end()) {
                if (!responseError->is_object()) {
                    errorMessage = "protocol error response is not an object";
                    return std::nullopt;
                }

                const auto code = responseError->find("code");
                const auto messageText = responseError->find("message");
                if (code == responseError->end() || !code->is_number_integer() || messageText == responseError->end() ||
                    !messageText->is_string()) {
                    errorMessage = "protocol error response has invalid code or message";
                    return std::nullopt;
                }

                decoded.error = ProtocolError{code->get<int>(), messageText->get<std::string>()};
                return decoded;
            }

            decoded.hasResult = true;
            if (!result->is_object()) {
                decoded.resultError = "initialize result is not an object";
                return decoded;
            }

            InitializeResult initializeResult;
            std::string resultError;
            if (!readRequiredString(*result, "codexHome", initializeResult.codexHome, resultError) ||
                !readRequiredString(*result, "platformFamily", initializeResult.platformFamily, resultError) ||
                !readRequiredString(*result, "platformOs", initializeResult.platformOs, resultError) ||
                !readRequiredString(*result, "userAgent", initializeResult.userAgent, resultError)) {
                decoded.resultError = std::move(resultError);
                return decoded;
            }

            decoded.initializeResult = std::move(initializeResult);
            return decoded;
        } catch (const Json::exception& exception) {
            errorMessage = std::string("invalid JSON protocol message: ") + exception.what();
            return std::nullopt;
        } catch (const std::exception& exception) {
            errorMessage = std::string("unable to decode protocol message: ") + exception.what();
            return std::nullopt;
        }
    }

} // namespace ai::openai::codex::detail
