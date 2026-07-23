/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/TurnCodec.h"

#include "ai/openai/codex/detail/ConversationCodec.h"
#include "ai/openai/codex/detail/ItemDecoder.h"

#include <cstdint>
#include <exception>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace ai::openai::codex::detail {

    namespace {

        const Json* findMember(const Json& object, const std::string_view name) noexcept {
            if (!object.is_object()) {
                return nullptr;
            }

            const auto member = object.find(name);

            return member == object.end() ? nullptr : &*member;
        }

        bool fail(std::string& error, std::string message) {
            error = std::move(message);

            return false;
        }

        bool decodeInt64(const Json& value, std::int64_t& result) noexcept {
            if (value.is_number_unsigned()) {
                const auto raw = value.get_ref<const Json::number_unsigned_t&>();

                if (raw > static_cast<Json::number_unsigned_t>(std::numeric_limits<std::int64_t>::max())) {
                    return false;
                }

                result = static_cast<std::int64_t>(raw);
                return true;
            }

            if (!value.is_number_integer()) {
                return false;
            }

            result = value.get_ref<const Json::number_integer_t&>();
            return true;
        }

        bool decodeOptionalInt64(const Json& object, const std::string_view name, std::optional<std::int64_t>& result, std::string& error) {
            const Json* value = findMember(object, name);

            if (value == nullptr || value->is_null()) {
                result.reset();
                return true;
            }

            std::int64_t decoded = 0;
            if (!decodeInt64(*value, decoded)) {
                return fail(error, "turn field '" + std::string(name) + "' must be an int64 integer or null");
            }

            result = decoded;
            return true;
        }

        bool decodeOptionalError(const Json& object, std::optional<Json>& result, std::string& error) {
            const Json* value = findMember(object, "error");

            if (value == nullptr || value->is_null()) {
                result.reset();
                return true;
            }
            if (!value->is_object()) {
                return fail(error, "turn field 'error' must be an object or null");
            }

            const Json* message = findMember(*value, "message");
            if (message == nullptr || !message->is_string()) {
                return fail(error, "turn field 'error.message' must be a string");
            }

            const Json* additionalDetails = findMember(*value, "additionalDetails");
            if (additionalDetails != nullptr && !additionalDetails->is_null() && !additionalDetails->is_string()) {
                return fail(error, "turn field 'error.additionalDetails' must be a string or null");
            }

            result = *value;
            return true;
        }

        bool decodeItemsView(const Json& object, typed::TurnItemsView& result, std::string& error) {
            const Json* value = findMember(object, "itemsView");
            if (value == nullptr) {
                result = typed::TurnItemsView::full();
                return true;
            }
            if (!value->is_string()) {
                return fail(error, "turn field 'itemsView' must be a string");
            }

            result.value = value->get_ref<const std::string&>();
            return true;
        }

    } // namespace

    std::optional<Json> encodeTurnInput(const typed::TurnInput& input, std::string& error) {
        return encodeUserInput(input, error);
    }

    std::optional<Json> encodeTurnStartParams(const typed::ThreadId& threadId,
                                              const std::vector<typed::TurnInput>& input,
                                              const typed::TurnStartOptions& options,
                                              std::string& error) {
        try {
            error.clear();

            Json encodedInput = Json::array();
            std::size_t index = 0;
            for (const typed::TurnInput& inputValue : input) {
                std::string inputError;
                std::optional<Json> encoded = encodeTurnInput(inputValue, inputError);

                if (!encoded.has_value()) {
                    fail(error, "turn input[" + std::to_string(index) + "]: " + inputError);
                    return std::nullopt;
                }

                encodedInput.emplace_back(std::move(*encoded));
                ++index;
            }

            Json params = Json::object({{"threadId", threadId.value}, {"input", std::move(encodedInput)}});

            if (options.cwd.has_value()) {
                params.emplace("cwd", *options.cwd);
            }
            if (options.model.has_value()) {
                params.emplace("model", options.model->value);
            }
            if (options.reasoningEffort.has_value()) {
                if (options.reasoningEffort->value.empty()) {
                    fail(error, "turn reasoning effort must not be empty");
                    return std::nullopt;
                }
                params.emplace("effort", options.reasoningEffort->value);
            }
            if (options.approvalPolicy.has_value()) {
                params.emplace("approvalPolicy", options.approvalPolicy->value);
            }
            if (options.sandboxPolicy.has_value()) {
                std::optional<Json> sandboxPolicy = encodeSandboxPolicy(*options.sandboxPolicy, error);
                if (!sandboxPolicy.has_value()) {
                    return std::nullopt;
                }
                params.emplace("sandboxPolicy", std::move(*sandboxPolicy));
            }

            return std::optional<Json>{std::move(params)};
        } catch (const std::exception& exception) {
            error = std::string("failed to encode turn/start parameters: ") + exception.what();
        } catch (...) {
            error = "failed to encode turn/start parameters: unknown local exception";
        }

        return std::nullopt;
    }

    std::optional<Json> encodeTurnInterruptParams(const typed::ThreadId& threadId, const typed::TurnId& turnId, std::string& error) {
        try {
            error.clear();
            return std::optional<Json>{Json::object({{"threadId", threadId.value}, {"turnId", turnId.value}})};
        } catch (const std::exception& exception) {
            error = std::string("failed to encode turn/interrupt parameters: ") + exception.what();
        } catch (...) {
            error = "failed to encode turn/interrupt parameters: unknown local exception";
        }

        return std::nullopt;
    }

    std::optional<typed::Turn> decodeTurn(const Json& value, const typed::ThreadId& threadId, std::string& error) {
        try {
            error.clear();

            if (!value.is_object()) {
                fail(error, "turn must be an object");
                return std::nullopt;
            }

            const Json* id = findMember(value, "id");
            if (id == nullptr) {
                fail(error, "turn is missing required field 'id'");
                return std::nullopt;
            }
            if (!id->is_string()) {
                fail(error, "turn field 'id' must be a string");
                return std::nullopt;
            }

            const Json* status = findMember(value, "status");
            if (status == nullptr) {
                fail(error, "turn is missing required field 'status'");
                return std::nullopt;
            }
            if (!status->is_string()) {
                fail(error, "turn field 'status' must be a string");
                return std::nullopt;
            }

            const Json* items = findMember(value, "items");
            if (items == nullptr) {
                fail(error, "turn is missing required field 'items'");
                return std::nullopt;
            }
            if (!items->is_array()) {
                fail(error, "turn field 'items' must be an array");
                return std::nullopt;
            }

            typed::Turn result;
            result.id = typed::TurnId{id->get_ref<const std::string&>()};
            result.threadId = threadId;
            result.status = typed::TurnStatus{status->get_ref<const std::string&>()};

            if (!decodeItemsView(value, result.itemsView, error) || !decodeOptionalError(value, result.error, error) ||
                !decodeOptionalInt64(value, "startedAt", result.startedAt, error) ||
                !decodeOptionalInt64(value, "completedAt", result.completedAt, error) ||
                !decodeOptionalInt64(value, "durationMs", result.durationMs, error)) {
                return std::nullopt;
            }

            result.items.reserve(items->size());
            std::size_t index = 0;
            for (const Json& itemValue : *items) {
                std::string itemError;
                std::optional<typed::Item> item =
                    decodeItem(itemValue, std::optional<typed::ThreadId>{threadId}, std::optional<typed::TurnId>{result.id}, itemError);

                if (!item.has_value()) {
                    fail(error, "turn field 'items'[" + std::to_string(index) + "]: " + itemError);
                    return std::nullopt;
                }

                result.items.emplace_back(std::move(*item));
                ++index;
            }

            result.raw = value;
            return result;
        } catch (const std::exception& exception) {
            error = std::string("failed to decode turn: ") + exception.what();
        } catch (...) {
            error = "failed to decode turn: unknown local exception";
        }

        return std::nullopt;
    }

    std::optional<typed::Turn> decodeTurnStartResult(const Json& value, const typed::ThreadId& threadId, std::string& error) {
        try {
            error.clear();

            if (!value.is_object()) {
                fail(error, "turn/start result must be an object");
                return std::nullopt;
            }

            const Json* turn = findMember(value, "turn");
            if (turn == nullptr) {
                fail(error, "turn/start result is missing required field 'turn'");
                return std::nullopt;
            }
            if (!turn->is_object()) {
                fail(error, "turn/start result field 'turn' must be an object");
                return std::nullopt;
            }

            return decodeTurn(*turn, threadId, error);
        } catch (const std::exception& exception) {
            error = std::string("failed to decode turn/start result: ") + exception.what();
        } catch (...) {
            error = "failed to decode turn/start result: unknown local exception";
        }

        return std::nullopt;
    }

    std::optional<typed::TurnInterruptResult> decodeTurnInterruptResult(const Json& value, std::string& error) {
        try {
            error.clear();

            if (!value.is_object()) {
                fail(error, "turn/interrupt result must be an object");
                return std::nullopt;
            }

            return typed::TurnInterruptResult{};
        } catch (const std::exception& exception) {
            error = std::string("failed to decode turn/interrupt result: ") + exception.what();
        } catch (...) {
            error = "failed to decode turn/interrupt result: unknown local exception";
        }

        return std::nullopt;
    }

} // namespace ai::openai::codex::detail
