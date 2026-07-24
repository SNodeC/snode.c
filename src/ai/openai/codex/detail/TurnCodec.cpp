/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/TurnCodec.h"

#include "ai/openai/codex/detail/CodexErrorInfoCodec.h"
#include "ai/openai/codex/detail/ConversationCodec.h"
#include "ai/openai/codex/detail/ItemDecoder.h"
#include "ai/openai/codex/detail/ThreadCodec.h"
#include "ai/openai/codex/typed/CodexErrorInfo.h"
#include "ai/openai/codex/typed/Items.h"
#include "ai/openai/codex/typed/Types.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <map>
#include <nlohmann/detail/json_ref.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace ai::openai::codex::detail {

    namespace {

        const Json* member(const Json& object, std::string_view name) noexcept {
            if (!object.is_object()) {
                return nullptr;
            }
            const auto iterator = object.find(name);
            return iterator == object.end() ? nullptr : &*iterator;
        }

        bool fail(std::string& error, std::string message) {
            error = std::move(message);
            return false;
        }

        typed::DecodeDiagnostic unknownEnumDiagnostic(std::string surface, std::string path) {
            return {typed::DecodeIssueKind::UnknownEnumValue,
                    typed::DecodeIssueSeverity::ForwardCompatibility,
                    std::move(surface),
                    std::move(path),
                    "protocol surface contains a future string-enum value"};
        }

        bool decodeInt64(const Json& value, std::int64_t& result) noexcept {
            if (value.is_number_unsigned()) {
                const auto number = value.get_ref<const Json::number_unsigned_t&>();
                if (number > static_cast<Json::number_unsigned_t>(std::numeric_limits<std::int64_t>::max())) {
                    return false;
                }
                result = static_cast<std::int64_t>(number);
                return true;
            }
            if (!value.is_number_integer()) {
                return false;
            }
            result = value.get_ref<const Json::number_integer_t&>();
            return true;
        }

        template <typename T, typename Decode>
        bool decodeOptionalNullable(const Json& object,
                                    std::string_view name,
                                    typed::OptionalNullable<T>& result,
                                    Decode&& decode,
                                    std::string& error) {
            result = typed::OptionalNullable<T>::omitted();
            const Json* value = member(object, name);
            if (value == nullptr) {
                return true;
            }
            result.present = true;
            if (value->is_null()) {
                return true;
            }
            T decoded;
            if (!decode(*value, decoded)) {
                return fail(error, "Turn field '" + std::string(name) + "' has the wrong type");
            }
            result.value = std::move(decoded);
            return true;
        }

        bool decodeTurnErrorValue(const Json& value,
                                  typed::TurnError& result,
                                  std::optional<typed::DecodeDiagnostic>& diagnostic) {
            auto decoded = decodeTurnError(value, diagnostic);
            if (!decoded) {
                return false;
            }
            result = std::move(*decoded);
            return true;
        }

        template <typename T, typename Encode>
        bool encodeOptionalNullable(Json& object,
                                    std::string_view name,
                                    const typed::OptionalNullable<T>& value,
                                    Encode&& encode,
                                    std::string& error) {
            if (value.isOmitted()) {
                return true;
            }
            if (value.isNull()) {
                object[std::string(name)] = nullptr;
                return true;
            }
            std::optional<Json> encoded = encode(*value.value, error);
            if (!encoded) {
                return false;
            }
            object[std::string(name)] = std::move(*encoded);
            return true;
        }

        template <typename T>
        std::optional<Json> scalar(const T& value, std::string&) {
            return std::optional<Json>{std::in_place, value};
        }

        template <typename T>
        std::optional<Json> openValue(const T& value, std::string&) {
            return std::optional<Json>{std::in_place, value.value};
        }

        std::optional<Json> encodeInputArray(const std::vector<typed::UserInput>& input, std::string& error) {
            Json result = Json::array();
            for (std::size_t index = 0; index < input.size(); ++index) {
                std::string nestedError;
                auto encoded = encodeUserInput(input[index], nestedError);
                if (!encoded) {
                    error = "turn input[" + std::to_string(index) + "]: " + nestedError;
                    return std::nullopt;
                }
                result.emplace_back(std::move(*encoded));
            }
            return std::optional<Json>{std::in_place, std::move(result)};
        }

    } // namespace

    bool hasMalformedKnownPayload(const typed::Turn& turn) noexcept {
        const auto malformed = [](const typed::DecodeDiagnostic& diagnostic) {
            return diagnostic.kind == typed::DecodeIssueKind::MalformedKnownPayload &&
                   diagnostic.severity == typed::DecodeIssueSeverity::ProtocolWarning;
        };
        if (std::any_of(turn.diagnostics.begin(), turn.diagnostics.end(), malformed)) {
            return true;
        }
        if (turn.error.hasValue() && turn.error->codexErrorDiagnostic &&
            malformed(*turn.error->codexErrorDiagnostic)) {
            return true;
        }
        return std::any_of(turn.items.begin(), turn.items.end(), [&](const typed::ThreadItem& item) {
            return std::visit(
                [&](const auto& alternative) {
                    using Alternative = std::decay_t<decltype(alternative)>;
                    if constexpr (std::is_same_v<Alternative, typed::UnknownItem>) {
                        return alternative.diagnostic && malformed(*alternative.diagnostic);
                    } else if constexpr (requires { alternative.diagnostics; }) {
                        return std::any_of(
                            alternative.diagnostics.begin(), alternative.diagnostics.end(), malformed);
                    } else {
                        return false;
                    }
                },
                item);
        });
    }

    std::optional<Json> encodeTurnInterruptParams(const typed::TurnInterruptParams& params, std::string& error) {
        try {
            error.clear();
            return std::optional<Json>{
                std::in_place, Json{{"threadId", params.threadId.value}, {"turnId", params.turnId.value}}};
        } catch (const std::exception& exception) {
            error = std::string("failed to encode turn/interrupt parameters: ") + exception.what();
        } catch (...) {
            error = "failed to encode turn/interrupt parameters";
        }
        return std::nullopt;
    }

    std::optional<Json> encodeTurnStartParams(const typed::TurnStartParams& params, std::string& error) {
        try {
            error.clear();
            auto input = encodeInputArray(params.input, error);
            if (!input) {
                return std::nullopt;
            }
            Json result{{"threadId", params.threadId.value}, {"input", std::move(*input)}};
            auto encodeEffort = [](const typed::ReasoningEffort& value, std::string& nestedError) -> std::optional<Json> {
                if (value.value.empty()) {
                    nestedError = "turn/start reasoning effort must not be empty";
                    return std::nullopt;
                }
                return std::optional<Json>{std::in_place, value.value};
            };
            auto encodeOpaque = [](const Json& value, std::string&) -> std::optional<Json> {
                return std::optional<Json>{std::in_place, value};
            };
            if (!encodeOptionalNullable(result, "personality", params.personality, openValue<typed::Personality>, error) ||
                !encodeOptionalNullable(result, "approvalPolicy", params.approvalPolicy, encodeAskForApproval, error) ||
                !encodeOptionalNullable(
                    result, "approvalsReviewer", params.approvalsReviewer, openValue<typed::ApprovalsReviewer>, error) ||
                !encodeOptionalNullable(
                    result, "clientUserMessageId", params.clientUserMessageId, openValue<typed::ClientUserMessageId>, error) ||
                !encodeOptionalNullable(result, "serviceTier", params.serviceTier, scalar<std::string>, error) ||
                !encodeOptionalNullable(result, "cwd", params.cwd, scalar<std::string>, error) ||
                !encodeOptionalNullable(result, "effort", params.effort, encodeEffort, error) ||
                !encodeOptionalNullable(result, "model", params.model, openValue<typed::ModelId>, error) ||
                !encodeOptionalNullable(result, "summary", params.summary, openValue<typed::ReasoningSummary>, error) ||
                !encodeOptionalNullable(result, "outputSchema", params.outputSchema, encodeOpaque, error) ||
                !encodeOptionalNullable(result, "sandboxPolicy", params.sandboxPolicy, encodeSandboxPolicy, error)) {
                return std::nullopt;
            }
            return std::optional<Json>{std::in_place, std::move(result)};
        } catch (const std::exception& exception) {
            error = std::string("failed to encode turn/start parameters: ") + exception.what();
        } catch (...) {
            error = "failed to encode turn/start parameters";
        }
        return std::nullopt;
    }

    std::optional<Json> encodeTurnSteerParams(const typed::TurnSteerParams& params, std::string& error) {
        try {
            error.clear();
            auto input = encodeInputArray(params.input, error);
            if (!input) {
                return std::nullopt;
            }
            Json result{{"threadId", params.threadId.value},
                        {"expectedTurnId", params.expectedTurnId.value},
                        {"input", std::move(*input)}};
            if (!encodeOptionalNullable(
                    result, "clientUserMessageId", params.clientUserMessageId, openValue<typed::ClientUserMessageId>, error)) {
                return std::nullopt;
            }
            return std::optional<Json>{std::in_place, std::move(result)};
        } catch (const std::exception& exception) {
            error = std::string("failed to encode turn/steer parameters: ") + exception.what();
        } catch (...) {
            error = "failed to encode turn/steer parameters";
        }
        return std::nullopt;
    }

    std::optional<typed::Turn> decodeTurn(const Json& value, const typed::ThreadId& threadId, std::string& error) {
        try {
            error.clear();
            if (!value.is_object()) {
                fail(error, "Turn must be an object");
                return std::nullopt;
            }
            const Json* id = member(value, "id");
            const Json* status = member(value, "status");
            const Json* items = member(value, "items");
            if (id == nullptr || !id->is_string()) {
                fail(error, "Turn field 'id' must be a string");
                return std::nullopt;
            }
            if (status == nullptr || !status->is_string()) {
                fail(error, "Turn field 'status' must be a string");
                return std::nullopt;
            }
            if (items == nullptr || !items->is_array()) {
                fail(error, "Turn field 'items' must be an array");
                return std::nullopt;
            }
            typed::Turn result;
            result.id.value = id->get<std::string>();
            result.threadId = threadId;
            result.status.value = status->get<std::string>();
            if (!result.status.isKnown()) {
                result.diagnostics.emplace_back(unknownEnumDiagnostic("TurnStatus", "$.status"));
            }
            const Json* itemsView = member(value, "itemsView");
            if (itemsView == nullptr) {
                result.itemsView = typed::TurnItemsView::full();
            } else if (!itemsView->is_string()) {
                fail(error, "Turn field 'itemsView' must be a string");
                return std::nullopt;
            } else {
                result.itemsView = typed::TurnItemsView{itemsView->get<std::string>()};
                if (!result.itemsView.isKnown()) {
                    result.diagnostics.emplace_back(unknownEnumDiagnostic("TurnItemsView", "$.itemsView"));
                }
            }
            std::optional<typed::DecodeDiagnostic> errorDiagnostic;
            if (!decodeOptionalNullable(
                    value,
                    "error",
                    result.error,
                    [&](const Json& input, typed::TurnError& output) {
                        return decodeTurnErrorValue(input, output, errorDiagnostic);
                    },
                    error) ||
                !decodeOptionalNullable(value, "startedAt", result.startedAt, decodeInt64, error) ||
                !decodeOptionalNullable(value, "completedAt", result.completedAt, decodeInt64, error) ||
                !decodeOptionalNullable(value, "durationMs", result.durationMs, decodeInt64, error)) {
                return std::nullopt;
            }
            if (errorDiagnostic) {
                result.diagnostics.emplace_back(*errorDiagnostic);
            }
            result.items.reserve(items->size());
            for (std::size_t index = 0; index < items->size(); ++index) {
                std::string nestedError;
                auto item = decodeItem((*items)[index], threadId, result.id, nestedError);
                if (!item) {
                    fail(error, "Turn field 'items[" + std::to_string(index) + "]': " + nestedError);
                    return std::nullopt;
                }
                result.items.emplace_back(std::move(*item));
            }
            result.raw = value;
            return result;
        } catch (const std::exception& exception) {
            error = std::string("failed to decode Turn: ") + exception.what();
        } catch (...) {
            error = "failed to decode Turn";
        }
        return std::nullopt;
    }

    std::optional<typed::TurnStartResponse>
    decodeTurnStartResponse(const Json& value, const typed::ThreadId& threadId, std::string& error) {
        error.clear();
        if (!value.is_object()) {
            fail(error, "TurnStartResponse must be an object");
            return std::nullopt;
        }
        const Json* turn = member(value, "turn");
        if (turn == nullptr) {
            fail(error, "TurnStartResponse is missing required field 'turn'");
            return std::nullopt;
        }
        auto decoded = decodeTurn(*turn, threadId, error);
        if (!decoded) {
            error = "TurnStartResponse field 'turn': " + error;
            return std::nullopt;
        }
        if (hasMalformedKnownPayload(*decoded)) {
            fail(error, "TurnStartResponse field 'turn' contains a malformed known payload");
            return std::nullopt;
        }
        typed::TurnStartResponse result;
        result.turn = std::move(*decoded);
        result.raw = value;
        return result;
    }

    std::optional<typed::TurnSteerResponse> decodeTurnSteerResponse(const Json& value, std::string& error) {
        error.clear();
        if (!value.is_object()) {
            fail(error, "TurnSteerResponse must be an object");
            return std::nullopt;
        }
        const Json* turnId = member(value, "turnId");
        if (turnId == nullptr || !turnId->is_string()) {
            fail(error, "TurnSteerResponse field 'turnId' must be a string");
            return std::nullopt;
        }
        typed::TurnSteerResponse result;
        result.turnId.value = turnId->get<std::string>();
        result.raw = value;
        return result;
    }

    std::optional<typed::Unit> decodeTurnInterruptResult(const Json& value, std::string& error) {
        return decodeUnitResult(value, error);
    }

    std::optional<Json> encodeTurnInput(const typed::TurnInput& input, std::string& error) {
        return encodeUserInput(input, error);
    }

    std::optional<Json> encodeTurnStartParams(const typed::ThreadId& threadId,
                                              const std::vector<typed::TurnInput>& input,
                                              const typed::TurnStartOptions& options,
                                              std::string& error) {
        return encodeTurnStartParams(typed::toTurnStartParams(threadId, input, options), error);
    }

    std::optional<Json>
    encodeTurnInterruptParams(const typed::ThreadId& threadId, const typed::TurnId& turnId, std::string& error) {
        return encodeTurnInterruptParams(typed::toTurnInterruptParams(threadId, turnId), error);
    }

    std::optional<typed::Turn> decodeTurnStartResult(const Json& value,
                                                     const typed::ThreadId& threadId,
                                                     std::string& error) {
        auto response = decodeTurnStartResponse(value, threadId, error);
        if (!response) {
            return std::nullopt;
        }
        return std::move(response->turn);
    }

} // namespace ai::openai::codex::detail
