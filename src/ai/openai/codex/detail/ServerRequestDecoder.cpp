/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ServerRequestDecoder.h"

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/detail/AccountCodec.h"
#include "ai/openai/codex/detail/ApprovalCodec.h"
#include "ai/openai/codex/detail/DecodeDiagnostic.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "ai/openai/codex/typed/Accounts.h"
#include "ai/openai/codex/typed/Items.h"
#include "ai/openai/codex/typed/Types.h"

#include <cstddef>
#include <cstdint>
#include <exception>
#include <map>
#include <nlohmann/detail/iterators/iter_impl.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace ai::openai::codex::detail {

    namespace {
        typed::UnknownServerRequest unknownRequest(const ServerRequest& request, std::optional<std::string> decodingError = std::nullopt) {
            const bool malformed = decodingError.has_value();
            std::string fieldPath = "$.params";
            if (malformed && request.method == "account/chatgptAuthTokens/refresh") {
                const std::size_t begin = decodingError->find("'$");
                if (begin != std::string::npos) {
                    const std::size_t end = decodingError->find('\'', begin + 1);
                    if (end != std::string::npos) {
                        fieldPath = decodingError->substr(begin + 1, end - begin - 1);
                    }
                }
            }
            return {request.id,
                    request.token,
                    request.method,
                    request.params,
                    request.raw,
                    std::move(decodingError),
                    malformed ? std::optional<typed::DecodeDiagnostic>{malformedKnownDiagnostic(request.method, std::move(fieldPath))}
                              : std::optional<typed::DecodeDiagnostic>{unknownMethodDiagnostic(request.method)}};
        }

        bool requireObject(const Json& value, std::string_view context, std::string& error) {
            if (value.is_object()) {
                return true;
            }

            error = std::string(context) + " params is not an object";
            return false;
        }

        bool readRequiredString(const Json& object, const char* field, std::string_view context, std::string& value, std::string& error) {
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

        bool readOptionalBoolean(
            const Json& object, const char* field, std::string_view context, bool defaultValue, bool& value, std::string& error) {
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

        bool readOptionalUint64(
            const Json& object, const char* field, std::string_view context, std::optional<std::uint64_t>& value, std::string& error) {
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
            std::optional<typed::CommandExecutionRequestApprovalParams> canonical =
                decodeCommandExecutionRequestApprovalParams(request.params, error);
            if (!canonical) {
                return std::nullopt;
            }

            std::optional<std::string> command;
            std::optional<std::string> cwd;
            std::optional<std::string> reason;
            if (canonical->command.value) {
                command = *canonical->command.value;
            }
            if (canonical->cwd.value) {
                cwd = canonical->cwd.value->value;
            }
            if (canonical->reason.value) {
                reason = *canonical->reason.value;
            }

            std::vector<typed::DecodeDiagnostic> diagnostics = canonical->diagnostics;
            return typed::CommandApprovalRequest{request.id,
                                                 request.token,
                                                 canonical->threadId,
                                                 canonical->turnId,
                                                 canonical->itemId,
                                                 canonical->startedAtMs,
                                                 std::move(command),
                                                 std::move(cwd),
                                                 std::move(reason),
                                                 request.params,
                                                 request.raw,
                                                 std::move(*canonical),
                                                 std::move(diagnostics)};
        }

        std::optional<typed::FileChangeApprovalRequest> decodeFileChangeApproval(const ServerRequest& request, std::string& error) {
            std::optional<typed::FileChangeRequestApprovalParams> canonical = decodeFileChangeRequestApprovalParams(request.params, error);
            if (!canonical) {
                return std::nullopt;
            }

            std::optional<std::string> reason;
            std::optional<std::string> grantRoot;
            if (canonical->reason.value) {
                reason = *canonical->reason.value;
            }
            if (canonical->grantRoot.value) {
                grantRoot = *canonical->grantRoot.value;
            }

            std::vector<typed::DecodeDiagnostic> diagnostics = canonical->diagnostics;
            return typed::FileChangeApprovalRequest{request.id,
                                                    request.token,
                                                    canonical->threadId,
                                                    canonical->turnId,
                                                    canonical->itemId,
                                                    canonical->startedAtMs,
                                                    std::move(reason),
                                                    std::move(grantRoot),
                                                    request.raw,
                                                    std::move(*canonical),
                                                    std::move(diagnostics)};
        }

        std::optional<typed::ApplyPatchApprovalRequest> decodeApplyPatchApproval(const ServerRequest& request, std::string& error) {
            std::optional<typed::ApplyPatchApprovalParams> params = decodeApplyPatchApprovalParams(request.params, error);
            if (!params) {
                return std::nullopt;
            }
            std::vector<typed::DecodeDiagnostic> diagnostics = params->diagnostics;
            return typed::ApplyPatchApprovalRequest{request.id, request.token, std::move(*params), request.raw, std::move(diagnostics)};
        }

        std::optional<typed::ExecCommandApprovalRequest> decodeExecCommandApproval(const ServerRequest& request, std::string& error) {
            std::optional<typed::ExecCommandApprovalParams> params = decodeExecCommandApprovalParams(request.params, error);
            if (!params) {
                return std::nullopt;
            }
            std::vector<typed::DecodeDiagnostic> diagnostics = params->diagnostics;
            return typed::ExecCommandApprovalRequest{request.id, request.token, std::move(*params), request.raw, std::move(diagnostics)};
        }

        std::optional<typed::PermissionsApprovalRequest> decodePermissionsApproval(const ServerRequest& request, std::string& error) {
            std::optional<typed::PermissionsRequestApprovalParams> params = decodePermissionsRequestApprovalParams(request.params, error);
            if (!params) {
                return std::nullopt;
            }
            std::vector<typed::DecodeDiagnostic> diagnostics = params->diagnostics;
            return typed::PermissionsApprovalRequest{request.id, request.token, std::move(*params), request.raw, std::move(diagnostics)};
        }

        bool decodeUserInputOption(const Json& raw,
                                   typed::UserInputOption& option,
                                   std::string_view method,
                                   std::string& error,
                                   std::size_t questionIndex,
                                   std::size_t optionIndex) {
            const std::string context =
                std::string(method) + " question " + std::to_string(questionIndex) + " option " + std::to_string(optionIndex);
            if (!raw.is_object()) {
                error = context + " is not an object";
                return false;
            }

            if (!readRequiredString(raw, "label", context, option.label, error) ||
                !readRequiredString(raw, "description", context, option.description, error)) {
                return false;
            }

            option.raw = raw;
            return true;
        }

        bool decodeUserInputQuestion(
            const Json& raw, typed::UserInputQuestion& question, std::string_view method, std::string& error, std::size_t questionIndex) {
            const std::string context = std::string(method) + " question " + std::to_string(questionIndex);
            if (!raw.is_object()) {
                error = context + " is not an object";
                return false;
            }

            if (!readRequiredString(raw, "id", context, question.id, error) ||
                !readRequiredString(raw, "header", context, question.header, error) ||
                !readRequiredString(raw, "question", context, question.prompt, error) ||
                !readOptionalBoolean(raw, "isOther", context, false, question.allowsFreeText, error) ||
                !readOptionalBoolean(raw, "isSecret", context, false, question.secret, error)) {
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
                    if (!decodeUserInputOption(rawOption, option, method, error, questionIndex, optionIndex)) {
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
            const std::string_view method = entryFor(ServerRequestTarget::ToolRequestUserInput).key.name;
            if (!requireObject(params, method, error)) {
                return std::nullopt;
            }

            std::string threadId;
            std::string turnId;
            std::string itemId;
            if (!readRequiredString(params, "threadId", method, threadId, error) ||
                !readRequiredString(params, "turnId", method, turnId, error) ||
                !readRequiredString(params, "itemId", method, itemId, error)) {
                return std::nullopt;
            }

            const auto questions = params.find("questions");
            if (questions == params.end() || !questions->is_array()) {
                error = std::string(method) + " params is missing required array field 'questions'";
                return std::nullopt;
            }

            std::vector<typed::UserInputQuestion> decodedQuestions;
            decodedQuestions.reserve(questions->size());
            std::size_t questionIndex = 0;
            for (const Json& rawQuestion : *questions) {
                typed::UserInputQuestion question;
                if (!decodeUserInputQuestion(rawQuestion, question, method, error, questionIndex)) {
                    return std::nullopt;
                }
                decodedQuestions.push_back(std::move(question));
                ++questionIndex;
            }

            std::optional<std::uint64_t> autoResolutionMs;
            if (!readOptionalUint64(params, "autoResolutionMs", method, autoResolutionMs, error)) {
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
            std::optional<typed::ChatgptAuthTokensRefreshParams> canonical = decodeChatgptAuthTokensRefreshParams(request.params, error);
            if (!canonical) {
                return std::nullopt;
            }

            std::optional<std::string> previousAccountId;
            if (canonical->previousAccountId.value) {
                previousAccountId = canonical->previousAccountId.value->value;
            }

            std::vector<typed::DecodeDiagnostic> diagnostics = canonical->diagnostics;
            return typed::AuthenticationRequest{request.id,
                                                request.token,
                                                canonical->reason.value,
                                                std::move(previousAccountId),
                                                request.raw,
                                                std::move(*canonical),
                                                std::move(diagnostics)};
        }
    } // namespace

    typed::TypedServerRequest decodeServerRequest(const ServerRequest& request) noexcept {
        try {
            std::string error;
            const ProtocolSurfaceEntry* entry = findSurface(SurfaceCategory::ServerRequest, "ServerRequest", "method", request.method);
            const ServerRequestTarget* target = entry == nullptr || entry->runtimeDisposition != RuntimeDisposition::Typed
                                                    ? nullptr
                                                    : std::get_if<ServerRequestTarget>(&entry->runtimeTarget);
            if (target == nullptr) {
                return unknownRequest(request);
            }

            switch (*target) {
                case ServerRequestTarget::CommandExecutionRequestApproval: {
                    std::optional<typed::CommandApprovalRequest> decoded = decodeCommandApproval(request, error);
                    return decoded ? typed::TypedServerRequest{std::move(*decoded)}
                                   : typed::TypedServerRequest{unknownRequest(request, std::move(error))};
                }
                case ServerRequestTarget::FileChangeRequestApproval: {
                    std::optional<typed::FileChangeApprovalRequest> decoded = decodeFileChangeApproval(request, error);
                    return decoded ? typed::TypedServerRequest{std::move(*decoded)}
                                   : typed::TypedServerRequest{unknownRequest(request, std::move(error))};
                }
                case ServerRequestTarget::ToolRequestUserInput: {
                    std::optional<typed::UserInputRequest> decoded = decodeUserInput(request, error);
                    return decoded ? typed::TypedServerRequest{std::move(*decoded)}
                                   : typed::TypedServerRequest{unknownRequest(request, std::move(error))};
                }
                case ServerRequestTarget::ChatgptAuthTokensRefresh: {
                    std::optional<typed::AuthenticationRequest> decoded = decodeAuthentication(request, error);
                    return decoded ? typed::TypedServerRequest{std::move(*decoded)}
                                   : typed::TypedServerRequest{unknownRequest(request, std::move(error))};
                }
                case ServerRequestTarget::ApplyPatchApproval: {
                    std::optional<typed::ApplyPatchApprovalRequest> decoded = decodeApplyPatchApproval(request, error);
                    return decoded ? typed::TypedServerRequest{std::move(*decoded)}
                                   : typed::TypedServerRequest{unknownRequest(request, std::move(error))};
                }
                case ServerRequestTarget::ExecCommandApproval: {
                    std::optional<typed::ExecCommandApprovalRequest> decoded = decodeExecCommandApproval(request, error);
                    return decoded ? typed::TypedServerRequest{std::move(*decoded)}
                                   : typed::TypedServerRequest{unknownRequest(request, std::move(error))};
                }
                case ServerRequestTarget::PermissionsRequestApproval: {
                    std::optional<typed::PermissionsApprovalRequest> decoded = decodePermissionsApproval(request, error);
                    return decoded ? typed::TypedServerRequest{std::move(*decoded)}
                                   : typed::TypedServerRequest{unknownRequest(request, std::move(error))};
                }
                case ServerRequestTarget::Count:
                    break;
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
