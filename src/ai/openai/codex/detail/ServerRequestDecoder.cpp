/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ServerRequestDecoder.h"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <optional>
#include <string>
#include <utility>

namespace ai::openai::codex::detail {

    namespace {
        constexpr const char* COMMAND_APPROVAL_METHOD = "item/commandExecution/requestApproval";
        constexpr const char* FILE_CHANGE_APPROVAL_METHOD = "item/fileChange/requestApproval";
        constexpr const char* USER_INPUT_METHOD = "item/tool/requestUserInput";
        constexpr const char* AUTHENTICATION_METHOD = "account/chatgptAuthTokens/refresh";

        typed::UnknownServerRequest unknownRequest(const ServerRequest& request, std::optional<std::string> decodingError = std::nullopt) {
            return {request.id, request.token, request.method, request.params, request.raw, std::move(decodingError)};
        }

        bool requireObject(const Json& value, const char* context, std::string& error) {
            if (value.is_object()) {
                return true;
            }

            error = std::string(context) + " params is not an object";
            return false;
        }

        bool readRequiredString(const Json& object, const char* field, const char* context, std::string& value, std::string& error) {
            const auto member = object.find(field);
            if (member == object.end()) {
                error = std::string(context) + " params is missing required string field '" + field + "'";
                return false;
            }
            if (!member->is_string()) {
                error = std::string(context) + " params field '" + field + "' is not a string";
                return false;
            }

            value = member->get<std::string>();
            return true;
        }

        bool readOptionalString(
            const Json& object, const char* field, const char* context, std::optional<std::string>& value, std::string& error) {
            const auto member = object.find(field);
            if (member == object.end() || member->is_null()) {
                value.reset();
                return true;
            }
            if (!member->is_string()) {
                error = std::string(context) + " params field '" + field + "' is neither a string nor null";
                return false;
            }

            value = member->get<std::string>();
            return true;
        }

        bool readOptionalBoolean(
            const Json& object, const char* field, const char* context, bool defaultValue, bool& value, std::string& error) {
            const auto member = object.find(field);
            if (member == object.end()) {
                value = defaultValue;
                return true;
            }
            if (!member->is_boolean()) {
                error = std::string(context) + " field '" + field + "' is not a boolean";
                return false;
            }

            value = member->get<bool>();
            return true;
        }

        bool readRequiredInt64(const Json& object, const char* field, const char* context, std::int64_t& decodedValue, std::string& error) {
            const auto member = object.find(field);
            if (member == object.end()) {
                error = std::string(context) + " params is missing required integer field '" + field + "'";
                return false;
            }

            if (member->is_number_unsigned()) {
                const std::uint64_t value = member->get<std::uint64_t>();
                if (value <= static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                    decodedValue = static_cast<std::int64_t>(value);
                    return true;
                }
            } else if (member->is_number_integer()) {
                decodedValue = member->get<std::int64_t>();
                return true;
            }

            error = std::string(context) + " params field '" + field + "' is outside the signed 64-bit integer range";
            return false;
        }

        bool readOptionalUint64(
            const Json& object, const char* field, const char* context, std::optional<std::uint64_t>& value, std::string& error) {
            const auto member = object.find(field);
            if (member == object.end() || member->is_null()) {
                value.reset();
                return true;
            }

            if (member->is_number_unsigned()) {
                value = member->get<std::uint64_t>();
                return true;
            }
            if (member->is_number_integer()) {
                const std::int64_t signedValue = member->get<std::int64_t>();
                if (signedValue >= 0) {
                    value = static_cast<std::uint64_t>(signedValue);
                    return true;
                }
            }

            error = std::string(context) + " params field '" + field + "' is not an unsigned 64-bit integer or null";
            return false;
        }

        std::optional<typed::CommandApprovalRequest> decodeCommandApproval(const ServerRequest& request, std::string& error) {
            const Json& params = request.params;
            if (!requireObject(params, COMMAND_APPROVAL_METHOD, error)) {
                return std::nullopt;
            }

            std::string threadId;
            std::string turnId;
            std::string itemId;
            std::int64_t startedAtMs = 0;
            std::optional<std::string> command;
            std::optional<std::string> cwd;
            std::optional<std::string> reason;
            if (!readRequiredString(params, "threadId", COMMAND_APPROVAL_METHOD, threadId, error) ||
                !readRequiredString(params, "turnId", COMMAND_APPROVAL_METHOD, turnId, error) ||
                !readRequiredString(params, "itemId", COMMAND_APPROVAL_METHOD, itemId, error) ||
                !readRequiredInt64(params, "startedAtMs", COMMAND_APPROVAL_METHOD, startedAtMs, error) ||
                !readOptionalString(params, "command", COMMAND_APPROVAL_METHOD, command, error) ||
                !readOptionalString(params, "cwd", COMMAND_APPROVAL_METHOD, cwd, error) ||
                !readOptionalString(params, "reason", COMMAND_APPROVAL_METHOD, reason, error)) {
                return std::nullopt;
            }

            return typed::CommandApprovalRequest{request.id,
                                                 request.token,
                                                 typed::ThreadId{std::move(threadId)},
                                                 typed::TurnId{std::move(turnId)},
                                                 typed::ItemId{std::move(itemId)},
                                                 startedAtMs,
                                                 std::move(command),
                                                 std::move(cwd),
                                                 std::move(reason),
                                                 params,
                                                 request.raw};
        }

        std::optional<typed::FileChangeApprovalRequest> decodeFileChangeApproval(const ServerRequest& request, std::string& error) {
            const Json& params = request.params;
            if (!requireObject(params, FILE_CHANGE_APPROVAL_METHOD, error)) {
                return std::nullopt;
            }

            std::string threadId;
            std::string turnId;
            std::string itemId;
            std::int64_t startedAtMs = 0;
            std::optional<std::string> reason;
            std::optional<std::string> grantRoot;
            if (!readRequiredString(params, "threadId", FILE_CHANGE_APPROVAL_METHOD, threadId, error) ||
                !readRequiredString(params, "turnId", FILE_CHANGE_APPROVAL_METHOD, turnId, error) ||
                !readRequiredString(params, "itemId", FILE_CHANGE_APPROVAL_METHOD, itemId, error) ||
                !readRequiredInt64(params, "startedAtMs", FILE_CHANGE_APPROVAL_METHOD, startedAtMs, error) ||
                !readOptionalString(params, "reason", FILE_CHANGE_APPROVAL_METHOD, reason, error) ||
                !readOptionalString(params, "grantRoot", FILE_CHANGE_APPROVAL_METHOD, grantRoot, error)) {
                return std::nullopt;
            }

            return typed::FileChangeApprovalRequest{request.id,
                                                    request.token,
                                                    typed::ThreadId{std::move(threadId)},
                                                    typed::TurnId{std::move(turnId)},
                                                    typed::ItemId{std::move(itemId)},
                                                    startedAtMs,
                                                    std::move(reason),
                                                    std::move(grantRoot),
                                                    request.raw};
        }

        bool decodeUserInputOption(
            const Json& raw, typed::UserInputOption& option, std::string& error, std::size_t questionIndex, std::size_t optionIndex) {
            const std::string context =
                std::string(USER_INPUT_METHOD) + " question " + std::to_string(questionIndex) + " option " + std::to_string(optionIndex);
            if (!raw.is_object()) {
                error = context + " is not an object";
                return false;
            }

            if (!readRequiredString(raw, "label", context.c_str(), option.label, error) ||
                !readRequiredString(raw, "description", context.c_str(), option.description, error)) {
                return false;
            }

            option.raw = raw;
            return true;
        }

        bool decodeUserInputQuestion(const Json& raw, typed::UserInputQuestion& question, std::string& error, std::size_t questionIndex) {
            const std::string context = std::string(USER_INPUT_METHOD) + " question " + std::to_string(questionIndex);
            if (!raw.is_object()) {
                error = context + " is not an object";
                return false;
            }

            if (!readRequiredString(raw, "id", context.c_str(), question.id, error) ||
                !readRequiredString(raw, "header", context.c_str(), question.header, error) ||
                !readRequiredString(raw, "question", context.c_str(), question.prompt, error) ||
                !readOptionalBoolean(raw, "isOther", context.c_str(), false, question.allowsFreeText, error) ||
                !readOptionalBoolean(raw, "isSecret", context.c_str(), false, question.secret, error)) {
                return false;
            }

            const auto options = raw.find("options");
            if (options != raw.end() && !options->is_null()) {
                if (!options->is_array()) {
                    error = context + " field 'options' is neither an array nor null";
                    return false;
                }

                question.options.reserve(options->size());
                std::size_t optionIndex = 0;
                for (const Json& rawOption : *options) {
                    typed::UserInputOption option;
                    if (!decodeUserInputOption(rawOption, option, error, questionIndex, optionIndex)) {
                        return false;
                    }
                    question.options.push_back(std::move(option));
                    ++optionIndex;
                }
            }

            question.raw = raw;
            return true;
        }

        std::optional<typed::UserInputRequest> decodeUserInput(const ServerRequest& request, std::string& error) {
            const Json& params = request.params;
            if (!requireObject(params, USER_INPUT_METHOD, error)) {
                return std::nullopt;
            }

            std::string threadId;
            std::string turnId;
            std::string itemId;
            if (!readRequiredString(params, "threadId", USER_INPUT_METHOD, threadId, error) ||
                !readRequiredString(params, "turnId", USER_INPUT_METHOD, turnId, error) ||
                !readRequiredString(params, "itemId", USER_INPUT_METHOD, itemId, error)) {
                return std::nullopt;
            }

            const auto questions = params.find("questions");
            if (questions == params.end() || !questions->is_array()) {
                error = std::string(USER_INPUT_METHOD) + " params is missing required array field 'questions'";
                return std::nullopt;
            }

            std::vector<typed::UserInputQuestion> decodedQuestions;
            decodedQuestions.reserve(questions->size());
            std::size_t questionIndex = 0;
            for (const Json& rawQuestion : *questions) {
                typed::UserInputQuestion question;
                if (!decodeUserInputQuestion(rawQuestion, question, error, questionIndex)) {
                    return std::nullopt;
                }
                decodedQuestions.push_back(std::move(question));
                ++questionIndex;
            }

            std::optional<std::uint64_t> autoResolutionMs;
            if (!readOptionalUint64(params, "autoResolutionMs", USER_INPUT_METHOD, autoResolutionMs, error)) {
                return std::nullopt;
            }

            return typed::UserInputRequest{request.id,
                                           request.token,
                                           typed::ThreadId{std::move(threadId)},
                                           typed::TurnId{std::move(turnId)},
                                           typed::ItemId{std::move(itemId)},
                                           std::move(decodedQuestions),
                                           autoResolutionMs,
                                           request.raw};
        }

        std::optional<typed::AuthenticationRequest> decodeAuthentication(const ServerRequest& request, std::string& error) {
            const Json& params = request.params;
            if (!requireObject(params, AUTHENTICATION_METHOD, error)) {
                return std::nullopt;
            }

            std::string reason;
            std::optional<std::string> previousAccountId;
            if (!readRequiredString(params, "reason", AUTHENTICATION_METHOD, reason, error) ||
                !readOptionalString(params, "previousAccountId", AUTHENTICATION_METHOD, previousAccountId, error)) {
                return std::nullopt;
            }

            return typed::AuthenticationRequest{request.id, request.token, std::move(reason), std::move(previousAccountId), request.raw};
        }
    } // namespace

    typed::TypedServerRequest decodeServerRequest(const ServerRequest& request) noexcept {
        try {
            std::string error;
            if (request.method == COMMAND_APPROVAL_METHOD) {
                std::optional<typed::CommandApprovalRequest> decoded = decodeCommandApproval(request, error);
                return decoded ? typed::TypedServerRequest{std::move(*decoded)}
                               : typed::TypedServerRequest{unknownRequest(request, std::move(error))};
            }
            if (request.method == FILE_CHANGE_APPROVAL_METHOD) {
                std::optional<typed::FileChangeApprovalRequest> decoded = decodeFileChangeApproval(request, error);
                return decoded ? typed::TypedServerRequest{std::move(*decoded)}
                               : typed::TypedServerRequest{unknownRequest(request, std::move(error))};
            }
            if (request.method == USER_INPUT_METHOD) {
                std::optional<typed::UserInputRequest> decoded = decodeUserInput(request, error);
                return decoded ? typed::TypedServerRequest{std::move(*decoded)}
                               : typed::TypedServerRequest{unknownRequest(request, std::move(error))};
            }
            if (request.method == AUTHENTICATION_METHOD) {
                std::optional<typed::AuthenticationRequest> decoded = decodeAuthentication(request, error);
                return decoded ? typed::TypedServerRequest{std::move(*decoded)}
                               : typed::TypedServerRequest{unknownRequest(request, std::move(error))};
            }

            return unknownRequest(request);
        } catch (const Json::exception& exception) {
            return unknownRequest(request, std::string("unable to decode server request: ") + exception.what());
        } catch (const std::exception& exception) {
            return unknownRequest(request, std::string("unable to decode server request: ") + exception.what());
        } catch (...) {
            return unknownRequest(request, "unable to decode server request: unknown exception");
        }
    }

} // namespace ai::openai::codex::detail
