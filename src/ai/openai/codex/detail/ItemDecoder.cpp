/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ItemDecoder.h"

#include "ai/openai/codex/detail/ConversationCodec.h"
#include "ai/openai/codex/detail/DecodeDiagnostic.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "ai/openai/codex/typed/Conversation.h"
#include "ai/openai/codex/typed/Types.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <map>
#include <nlohmann/detail/iterators/iter_impl.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace ai::openai::codex::detail {

    namespace {

        constexpr std::string_view ThreadItemSurface = "ThreadItem";
        constexpr std::string_view ResponseItemSurface = "ResponseItem";
        constexpr std::string_view TypeField = "type";

        const Json* member(const Json& object, std::string_view name) noexcept {
            if (!object.is_object()) {
                return nullptr;
            }
            const auto iterator = object.find(name);
            return iterator == object.end() ? nullptr : &*iterator;
        }

        std::string fieldPath(std::string_view name) {
            return "$." + std::string(name);
        }

        std::string indexedPath(std::string_view base, std::size_t index) {
            return std::string(base) + "[" + std::to_string(index) + "]";
        }

        std::string childPath(std::string_view base, std::string_view child) {
            return std::string(base) + "." + std::string(child);
        }

        std::string prefixedDiagnosticPath(std::string_view prefix, std::string_view nestedPath) {
            if (nestedPath.empty() || nestedPath == "$") {
                return std::string(prefix);
            }
            if (nestedPath.front() == '$') {
                return std::string(prefix) + std::string(nestedPath.substr(1));
            }
            return std::string(prefix);
        }

        struct DecodeState {
            std::string_view surface;
            std::string error;
            std::optional<typed::DecodeDiagnostic> diagnostic;

            explicit DecodeState(std::string_view inputSurface)
                : surface(inputSurface) {
            }

            bool fail(std::string path) {
                error = std::string(surface) + " payload does not match its typed contract at " + path;
                diagnostic = malformedKnownDiagnostic(std::string(surface), std::move(path));
                return false;
            }

            bool failNested(typed::DecodeDiagnostic nested, std::string_view prefix) {
                nested.fieldPath = prefixedDiagnosticPath(prefix, nested.fieldPath);
                nested.surface = std::string(surface);
                error = std::string(surface) + " payload contains a malformed nested value at " + nested.fieldPath;
                diagnostic = std::move(nested);
                return false;
            }
        };

        void appendNestedDiagnostic(std::vector<typed::DecodeDiagnostic>& diagnostics,
                                    const std::optional<typed::DecodeDiagnostic>& nested,
                                    std::string_view prefix) {
            if (!nested) {
                return;
            }
            typed::DecodeDiagnostic relocated = *nested;
            relocated.fieldPath = prefixedDiagnosticPath(prefix, relocated.fieldPath);
            diagnostics.emplace_back(std::move(relocated));
        }

        bool decodeStringValue(const Json& value, std::string& result, std::string_view path, DecodeState& state) {
            if (!value.is_string()) {
                return state.fail(std::string(path));
            }
            result = value.get<std::string>();
            return true;
        }

        bool decodeNonEmptyStringValue(const Json& value, std::string& result, std::string_view path, DecodeState& state) {
            if (!decodeStringValue(value, result, path, state)) {
                return false;
            }
            if (result.empty()) {
                return state.fail(std::string(path));
            }
            return true;
        }

        bool decodeBoolValue(const Json& value, bool& result, std::string_view path, DecodeState& state) {
            if (!value.is_boolean()) {
                return state.fail(std::string(path));
            }
            result = value.get<bool>();
            return true;
        }

        bool decodeInt32Value(const Json& value, std::int32_t& result, std::string_view path, DecodeState& state) {
            if (value.is_number_unsigned()) {
                const std::uint64_t integer = value.get<std::uint64_t>();
                if (integer > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())) {
                    return state.fail(std::string(path));
                }
                result = static_cast<std::int32_t>(integer);
                return true;
            }
            if (!value.is_number_integer()) {
                return state.fail(std::string(path));
            }
            const std::int64_t integer = value.get<std::int64_t>();
            if (integer < std::numeric_limits<std::int32_t>::min() || integer > std::numeric_limits<std::int32_t>::max()) {
                return state.fail(std::string(path));
            }
            result = static_cast<std::int32_t>(integer);
            return true;
        }

        bool decodeInt64Value(const Json& value, std::int64_t& result, std::string_view path, DecodeState& state) {
            if (value.is_number_unsigned()) {
                const std::uint64_t integer = value.get<std::uint64_t>();
                if (integer > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                    return state.fail(std::string(path));
                }
                result = static_cast<std::int64_t>(integer);
                return true;
            }
            if (!value.is_number_integer()) {
                return state.fail(std::string(path));
            }
            result = value.get<std::int64_t>();
            return true;
        }

        bool decodeUint64Value(const Json& value, std::uint64_t& result, std::string_view path, DecodeState& state) {
            if (value.is_number_unsigned()) {
                result = value.get<std::uint64_t>();
                return true;
            }
            if (!value.is_number_integer()) {
                return state.fail(std::string(path));
            }
            const std::int64_t integer = value.get<std::int64_t>();
            if (integer < 0) {
                return state.fail(std::string(path));
            }
            result = static_cast<std::uint64_t>(integer);
            return true;
        }

        bool decodeUint32Value(const Json& value, std::uint32_t& result, std::string_view path, DecodeState& state) {
            std::uint64_t decoded = 0;
            if (!decodeUint64Value(value, decoded, path, state) || decoded > std::numeric_limits<std::uint32_t>::max()) {
                if (!state.diagnostic) {
                    state.fail(std::string(path));
                }
                return false;
            }
            result = static_cast<std::uint32_t>(decoded);
            return true;
        }

        bool requiredString(const Json& object, std::string_view name, std::string& result, DecodeState& state) {
            const Json* value = member(object, name);
            const std::string path = fieldPath(name);
            return value != nullptr ? decodeStringValue(*value, result, path, state) : state.fail(path);
        }

        bool requiredNonEmptyString(const Json& object, std::string_view name, std::string& result, DecodeState& state) {
            const Json* value = member(object, name);
            const std::string path = fieldPath(name);
            return value != nullptr ? decodeNonEmptyStringValue(*value, result, path, state) : state.fail(path);
        }

        template <typename T, typename Decode>
        bool requiredDecoded(const Json& object, std::string_view name, T& result, DecodeState& state, Decode&& decode) {
            const Json* value = member(object, name);
            const std::string path = fieldPath(name);
            return value != nullptr ? decode(*value, result, path, state) : state.fail(path);
        }

        template <typename T, typename Decode>
        bool optionalNullable(
            const Json& object, std::string_view name, typed::OptionalNullable<T>& result, DecodeState& state, Decode&& decode) {
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
            if (!decode(*value, decoded, fieldPath(name), state)) {
                return false;
            }
            result.value = std::move(decoded);
            return true;
        }

        template <typename T, typename Decode>
        bool optionalValue(const Json& object, std::string_view name, std::optional<T>& result, DecodeState& state, Decode&& decode) {
            result.reset();
            const Json* value = member(object, name);
            if (value == nullptr) {
                return true;
            }
            T decoded;
            if (!decode(*value, decoded, fieldPath(name), state)) {
                return false;
            }
            result = std::move(decoded);
            return true;
        }

        template <typename T, typename Decode>
        bool decodeArrayValue(const Json& value, std::vector<T>& result, std::string_view path, DecodeState& state, Decode&& decode) {
            if (!value.is_array()) {
                return state.fail(std::string(path));
            }
            std::vector<T> decoded;
            decoded.reserve(value.size());
            for (std::size_t index = 0; index < value.size(); ++index) {
                T element;
                const std::string elementPath = indexedPath(path, index);
                if (!decode(value[index], element, elementPath, state)) {
                    return false;
                }
                decoded.emplace_back(std::move(element));
            }
            result = std::move(decoded);
            return true;
        }

        template <typename T, typename Decode>
        bool requiredArray(const Json& object, std::string_view name, std::vector<T>& result, DecodeState& state, Decode&& decode) {
            return requiredDecoded(object,
                                   name,
                                   result,
                                   state,
                                   [&](const Json& value, std::vector<T>& decoded, std::string_view path, DecodeState& nestedState) {
                                       return decodeArrayValue(value, decoded, path, nestedState, decode);
                                   });
        }

        template <typename T, typename Decode>
        bool optionalArray(
            const Json& object, std::string_view name, std::optional<std::vector<T>>& result, DecodeState& state, Decode&& decode) {
            return optionalValue(object,
                                 name,
                                 result,
                                 state,
                                 [&](const Json& value, std::vector<T>& decoded, std::string_view path, DecodeState& nestedState) {
                                     return decodeArrayValue(value, decoded, path, nestedState, decode);
                                 });
        }

        template <typename T, typename Decode>
        bool optionalNullableArray(const Json& object,
                                   std::string_view name,
                                   typed::OptionalNullable<std::vector<T>>& result,
                                   DecodeState& state,
                                   Decode&& decode) {
            return optionalNullable(object,
                                    name,
                                    result,
                                    state,
                                    [&](const Json& value, std::vector<T>& decoded, std::string_view path, DecodeState& nestedState) {
                                        return decodeArrayValue(value, decoded, path, nestedState, decode);
                                    });
        }

        template <typename Enum>
        bool decodeOpenEnumValue(
            const Json& value, Enum& result, std::string_view path, DecodeState& state, std::vector<typed::DecodeDiagnostic>& diagnostics) {
            if (!decodeStringValue(value, result.value, path, state)) {
                return false;
            }
            if (!result.isKnown()) {
                diagnostics.emplace_back(unknownEnumDiagnostic(std::string(state.surface), std::string(path)));
            }
            return true;
        }

        template <typename Union, typename Decode>
        bool decodeNestedUnion(const Json& value,
                               Union& result,
                               std::string_view path,
                               DecodeState& state,
                               std::vector<typed::DecodeDiagnostic>& diagnostics,
                               Decode&& decode) {
            auto decoded = decode(value);
            if (!decoded.value) {
                typed::DecodeDiagnostic diagnostic =
                    decoded.diagnostic.value_or(malformedKnownDiagnostic(std::string(state.surface), std::string(path)));
                return state.failNested(std::move(diagnostic), path);
            }
            result = std::move(*decoded.value);
            appendNestedDiagnostic(diagnostics, decoded.diagnostic, path);
            return true;
        }

        bool decodeStrongThreadId(const Json& value, typed::ThreadId& result, std::string_view path, DecodeState& state) {
            return decodeStringValue(value, result.value, path, state);
        }

        bool decodeStrongClientUserMessageId(
            const Json& value, typed::ClientUserMessageId& result, std::string_view path, DecodeState& state) {
            return decodeStringValue(value, result.value, path, state);
        }

        bool decodeStrongModelId(const Json& value, typed::ModelId& result, std::string_view path, DecodeState& state) {
            return decodeStringValue(value, result.value, path, state);
        }

        bool decodeStrongResponseItemId(const Json& value, typed::ResponseItemId& result, std::string_view path, DecodeState& state) {
            return decodeStringValue(value, result.value, path, state);
        }

        bool decodeStrongResponseCallId(const Json& value, typed::ResponseCallId& result, std::string_view path, DecodeState& state) {
            return decodeStringValue(value, result.value, path, state);
        }

        bool decodeAbsolutePath(const Json& value, typed::AbsolutePathBuf& result, std::string_view path, DecodeState& state) {
            return decodeStringValue(value, result.value, path, state);
        }

        bool decodeLegacyPath(const Json& value, typed::LegacyAppPathString& result, std::string_view path, DecodeState& state) {
            return decodeStringValue(value, result.value, path, state);
        }

        bool decodeMemoryCitationEntry(const Json& value, typed::MemoryCitationEntry& result, std::string_view path, DecodeState& state) {
            if (!value.is_object()) {
                return state.fail(std::string(path));
            }
            const Json* lineEnd = member(value, "lineEnd");
            const Json* lineStart = member(value, "lineStart");
            const Json* note = member(value, "note");
            const Json* citationPath = member(value, "path");
            if (lineEnd == nullptr) {
                return state.fail(childPath(path, "lineEnd"));
            }
            if (!decodeUint32Value(*lineEnd, result.lineEnd, childPath(path, "lineEnd"), state)) {
                return false;
            }
            if (lineStart == nullptr) {
                return state.fail(childPath(path, "lineStart"));
            }
            if (!decodeUint32Value(*lineStart, result.lineStart, childPath(path, "lineStart"), state)) {
                return false;
            }
            if (note == nullptr) {
                return state.fail(childPath(path, "note"));
            }
            if (!decodeStringValue(*note, result.note, childPath(path, "note"), state)) {
                return false;
            }
            if (citationPath == nullptr) {
                return state.fail(childPath(path, "path"));
            }
            return decodeStringValue(*citationPath, result.path, childPath(path, "path"), state);
        }

        bool decodeMemoryCitation(const Json& value, typed::MemoryCitation& result, std::string_view path, DecodeState& state) {
            if (!value.is_object()) {
                return state.fail(std::string(path));
            }
            const Json* entries = member(value, "entries");
            const std::string entriesPath = childPath(path, "entries");
            if (entries == nullptr ||
                !decodeArrayValue(
                    *entries,
                    result.entries,
                    entriesPath,
                    state,
                    [](const Json& entry, typed::MemoryCitationEntry& decoded, std::string_view entryPath, DecodeState& nestedState) {
                        return decodeMemoryCitationEntry(entry, decoded, entryPath, nestedState);
                    })) {
                if (entries == nullptr) {
                    return state.fail(entriesPath);
                }
                return false;
            }
            const Json* threadIds = member(value, "threadIds");
            const std::string threadIdsPath = childPath(path, "threadIds");
            if (threadIds == nullptr) {
                return state.fail(threadIdsPath);
            }
            return decodeArrayValue(*threadIds,
                                    result.threadIds,
                                    threadIdsPath,
                                    state,
                                    [](const Json& id, typed::ThreadId& decoded, std::string_view idPath, DecodeState& nestedState) {
                                        return decodeStrongThreadId(id, decoded, idPath, nestedState);
                                    });
        }

        bool decodeCollabAgentState(const Json& value,
                                    typed::CollabAgentState& result,
                                    std::string_view path,
                                    DecodeState& state,
                                    std::vector<typed::DecodeDiagnostic>& diagnostics) {
            if (!value.is_object()) {
                return state.fail(std::string(path));
            }
            const Json* status = member(value, "status");
            const std::string statusPath = childPath(path, "status");
            if (status == nullptr || !decodeOpenEnumValue(*status, result.status, statusPath, state, diagnostics)) {
                if (status == nullptr) {
                    return state.fail(statusPath);
                }
                return false;
            }
            result.message = typed::OptionalNullable<std::string>::omitted();
            const Json* message = member(value, "message");
            if (message == nullptr) {
                return true;
            }
            result.message.present = true;
            if (message->is_null()) {
                return true;
            }
            std::string decodedMessage;
            if (!decodeStringValue(*message, decodedMessage, childPath(path, "message"), state)) {
                return false;
            }
            result.message.value = std::move(decodedMessage);
            return true;
        }

        bool decodeFileUpdateChange(const Json& value,
                                    typed::FileUpdateChange& result,
                                    std::string_view path,
                                    DecodeState& state,
                                    std::vector<typed::DecodeDiagnostic>& diagnostics) {
            if (!value.is_object()) {
                return state.fail(std::string(path));
            }
            const Json* diff = member(value, "diff");
            const Json* kind = member(value, "kind");
            const Json* changePath = member(value, "path");
            if (diff == nullptr) {
                return state.fail(childPath(path, "diff"));
            }
            if (!decodeStringValue(*diff, result.diff, childPath(path, "diff"), state)) {
                return false;
            }
            if (kind == nullptr) {
                return state.fail(childPath(path, "kind"));
            }
            if (!decodeNestedUnion(*kind, result.kind, childPath(path, "kind"), state, diagnostics, decodePatchChangeKind)) {
                return false;
            }
            if (changePath == nullptr) {
                return state.fail(childPath(path, "path"));
            }
            return decodeStringValue(*changePath, result.path, childPath(path, "path"), state);
        }

        bool decodeHookPromptFragment(const Json& value, typed::HookPromptFragment& result, std::string_view path, DecodeState& state) {
            if (!value.is_object()) {
                return state.fail(std::string(path));
            }
            const Json* hookRunId = member(value, "hookRunId");
            const Json* text = member(value, "text");
            if (hookRunId == nullptr) {
                return state.fail(childPath(path, "hookRunId"));
            }
            if (!decodeStringValue(*hookRunId, result.hookRunId, childPath(path, "hookRunId"), state)) {
                return false;
            }
            if (text == nullptr) {
                return state.fail(childPath(path, "text"));
            }
            return decodeStringValue(*text, result.text, childPath(path, "text"), state);
        }

        bool decodeInternalMetadata(const Json& value,
                                    typed::InternalChatMessageMetadataPassthrough& result,
                                    std::string_view path,
                                    DecodeState& state) {
            if (!value.is_object()) {
                return state.fail(std::string(path));
            }
            result.turnId = typed::OptionalNullable<typed::TurnId>::omitted();
            const Json* turnId = member(value, "turn_id");
            if (turnId == nullptr) {
                return true;
            }
            result.turnId.present = true;
            if (turnId->is_null()) {
                return true;
            }
            typed::TurnId decoded;
            if (!decodeStringValue(*turnId, decoded.value, childPath(path, "turn_id"), state)) {
                return false;
            }
            result.turnId.value = std::move(decoded);
            return true;
        }

        bool
        decodeMcpToolCallAppContext(const Json& value, typed::McpToolCallAppContext& result, std::string_view path, DecodeState& state) {
            if (!value.is_object()) {
                return state.fail(std::string(path));
            }
            const Json* connectorId = member(value, "connectorId");
            if (connectorId == nullptr || !decodeStringValue(*connectorId, result.connectorId, childPath(path, "connectorId"), state)) {
                if (connectorId == nullptr) {
                    return state.fail(childPath(path, "connectorId"));
                }
                return false;
            }

            auto decodeNullableString = [&](std::string_view field, typed::OptionalNullable<std::string>& target) {
                target = typed::OptionalNullable<std::string>::omitted();
                const Json* fieldValue = member(value, field);
                if (fieldValue == nullptr) {
                    return true;
                }
                target.present = true;
                if (fieldValue->is_null()) {
                    return true;
                }
                std::string decoded;
                if (!decodeStringValue(*fieldValue, decoded, childPath(path, field), state)) {
                    return false;
                }
                target.value = std::move(decoded);
                return true;
            };

            return decodeNullableString("actionName", result.actionName) && decodeNullableString("appName", result.appName) &&
                   decodeNullableString("linkId", result.linkId) && decodeNullableString("resourceUri", result.resourceUri) &&
                   decodeNullableString("templateId", result.templateId);
        }

        bool decodeMcpToolCallError(const Json& value, typed::McpToolCallError& result, std::string_view path, DecodeState& state) {
            if (!value.is_object()) {
                return state.fail(std::string(path));
            }
            const Json* message = member(value, "message");
            return message != nullptr ? decodeStringValue(*message, result.message, childPath(path, "message"), state)
                                      : state.fail(childPath(path, "message"));
        }

        bool decodeMcpToolCallResult(const Json& value, typed::McpToolCallResult& result, std::string_view path, DecodeState& state) {
            if (!value.is_object()) {
                return state.fail(std::string(path));
            }
            const Json* content = member(value, "content");
            const std::string contentPath = childPath(path, "content");
            if (content == nullptr || !content->is_array()) {
                return state.fail(contentPath);
            }
            result.content.clear();
            result.content.reserve(content->size());
            for (const Json& entry : *content) {
                result.content.emplace_back(entry);
            }

            auto decodeOpaque = [&](std::string_view field, typed::OptionalNullable<Json>& target) {
                target = typed::OptionalNullable<Json>::omitted();
                const Json* fieldValue = member(value, field);
                if (fieldValue == nullptr) {
                    return;
                }
                target.present = true;
                if (!fieldValue->is_null()) {
                    target.value = *fieldValue;
                }
            };
            decodeOpaque("_meta", result.meta);
            decodeOpaque("structuredContent", result.structuredContent);
            return true;
        }

        bool makeMetadata(const Json& value,
                          const std::optional<typed::ThreadId>& threadId,
                          const std::optional<typed::TurnId>& turnId,
                          typed::ItemMetadata& metadata,
                          DecodeState& state) {
            std::string id;
            if (!requiredNonEmptyString(value, "id", id, state)) {
                return false;
            }
            metadata = typed::ItemMetadata{typed::ItemId{std::move(id)}, threadId, turnId, value};
            return true;
        }

        bool decodeResponseCommon(const Json& value,
                                  typed::OptionalNullable<typed::ResponseItemId>& id,
                                  typed::OptionalNullable<typed::InternalChatMessageMetadataPassthrough>& metadata,
                                  DecodeState& state) {
            return optionalNullable(value, "id", id, state, decodeStrongResponseItemId) &&
                   optionalNullable(value,
                                    "internal_chat_message_metadata_passthrough",
                                    metadata,
                                    state,
                                    [](const Json& field,
                                       typed::InternalChatMessageMetadataPassthrough& decoded,
                                       std::string_view path,
                                       DecodeState& nestedState) {
                                        return decodeInternalMetadata(field, decoded, path, nestedState);
                                    });
        }

        typed::UnknownItem makeUnknownItem(const Json& value,
                                           std::optional<std::string> type,
                                           const std::optional<typed::ThreadId>& threadId,
                                           const std::optional<typed::TurnId>& turnId,
                                           std::optional<std::string> decodingError,
                                           std::optional<typed::DecodeDiagnostic> diagnostic) {
            typed::UnknownItem item{std::move(type), value, std::move(decodingError), {}, std::move(diagnostic)};
            item.metadata.threadId = threadId;
            item.metadata.turnId = turnId;

            const Json* id = member(value, "id");
            if (id == nullptr) {
                if (!item.decodingError) {
                    item.decodingError = "ThreadItem payload does not match its typed contract at $.id";
                    item.diagnostic = malformedKnownDiagnostic(std::string(ThreadItemSurface), "$.id");
                }
            } else if (!id->is_string() || id->get_ref<const std::string&>().empty()) {
                if (!item.decodingError) {
                    item.decodingError = "ThreadItem payload does not match its typed contract at $.id";
                    item.diagnostic = malformedKnownDiagnostic(std::string(ThreadItemSurface), "$.id");
                }
            } else {
                item.metadata.id = typed::ItemId{id->get<std::string>()};
            }

            if (!item.diagnostic) {
                item.diagnostic = unknownDiscriminatorDiagnostic(std::string(ThreadItemSurface), "$.type");
            }
            return item;
        }

        typed::UnknownResponseItem makeUnknownResponseItem(const Json& value,
                                                           std::optional<std::string> type,
                                                           std::optional<std::string> decodingError,
                                                           std::optional<typed::DecodeDiagnostic> diagnostic) {
            typed::UnknownResponseItem item{std::move(type), value, std::move(decodingError), {}, std::move(diagnostic)};
            if (const Json* id = member(value, "id"); id != nullptr && !id->is_null()) {
                if (!id->is_string()) {
                    if (!item.decodingError) {
                        item.decodingError = "ResponseItem payload does not match its typed contract at $.id";
                        item.diagnostic = malformedKnownDiagnostic(std::string(ResponseItemSurface), "$.id");
                    }
                } else {
                    item.metadata.id = typed::ResponseItemId{id->get<std::string>()};
                }
            }
            if (!item.diagnostic) {
                item.diagnostic = unknownDiscriminatorDiagnostic(std::string(ResponseItemSurface), "$.type");
            }
            return item;
        }

        const ThreadItemCodecDescriptor* descriptorForThreadItem(const ProtocolSurfaceEntry& entry) noexcept {
            if (entry.runtimeDisposition != RuntimeDisposition::Typed ||
                entry.typedImplementation != TypedImplementationStatus::Implemented) {
                return nullptr;
            }
            const ItemDiscriminatorTarget* target = std::get_if<ItemDiscriminatorTarget>(&entry.runtimeTarget);
            if (target == nullptr) {
                return nullptr;
            }
            const std::span<const ThreadItemCodecDescriptor> descriptors = threadItemCodecDescriptors();
            const auto descriptor = std::find_if(descriptors.begin(), descriptors.end(), [&](const ThreadItemCodecDescriptor& candidate) {
                return candidate.key == entry.key && candidate.target == *target;
            });
            if (descriptor == descriptors.end() || descriptor->shape != ConversationUnionCodecShape::InternallyTaggedObject ||
                descriptor->direction != ConversationUnionCodecDirection::DecodeOnly) {
                return nullptr;
            }
            return &*descriptor;
        }

        const ResponseItemCodecDescriptor* descriptorForResponseItem(const ProtocolSurfaceEntry& entry) noexcept {
            if (entry.runtimeDisposition != RuntimeDisposition::Typed ||
                entry.typedImplementation != TypedImplementationStatus::Implemented) {
                return nullptr;
            }
            const ResponseItemTarget* target = std::get_if<ResponseItemTarget>(&entry.runtimeTarget);
            if (target == nullptr) {
                return nullptr;
            }
            const std::span<const ResponseItemCodecDescriptor> descriptors = responseItemCodecDescriptors();
            const auto descriptor = std::find_if(descriptors.begin(), descriptors.end(), [&](const ResponseItemCodecDescriptor& candidate) {
                return candidate.key == entry.key && candidate.target == *target;
            });
            if (descriptor == descriptors.end() || descriptor->shape != ConversationUnionCodecShape::InternallyTaggedObject ||
                descriptor->direction != ConversationUnionCodecDirection::DecodeOnly) {
                return nullptr;
            }
            return &*descriptor;
        }

        std::optional<typed::ThreadItem> decodeAgentMessage(const Json& value,
                                                            const std::optional<typed::ThreadId>& threadId,
                                                            const std::optional<typed::TurnId>& turnId,
                                                            DecodeState& state) {
            typed::AgentMessageThreadItem item;
            if (!makeMetadata(value, threadId, turnId, item.metadata, state) || !requiredString(value, "text", item.text, state) ||
                !optionalNullable(value,
                                  "memoryCitation",
                                  item.memoryCitation,
                                  state,
                                  [](const Json& field, typed::MemoryCitation& decoded, std::string_view path, DecodeState& nestedState) {
                                      return decodeMemoryCitation(field, decoded, path, nestedState);
                                  }) ||
                !optionalNullable(value,
                                  "phase",
                                  item.phase,
                                  state,
                                  [&](const Json& field, typed::MessagePhase& decoded, std::string_view path, DecodeState& nestedState) {
                                      return decodeOpenEnumValue(field, decoded, path, nestedState, item.diagnostics);
                                  })) {
                return std::nullopt;
            }
            return typed::ThreadItem{std::move(item)};
        }

        std::optional<typed::ThreadItem> decodeCollabAgentToolCall(const Json& value,
                                                                   const std::optional<typed::ThreadId>& threadId,
                                                                   const std::optional<typed::TurnId>& turnId,
                                                                   DecodeState& state) {
            typed::CollabAgentToolCallThreadItem item;
            if (!makeMetadata(value, threadId, turnId, item.metadata, state)) {
                return std::nullopt;
            }

            const Json* agentsStates = member(value, "agentsStates");
            if (agentsStates == nullptr || !agentsStates->is_object()) {
                state.fail("$.agentsStates");
                return std::nullopt;
            }
            for (auto iterator = agentsStates->begin(); iterator != agentsStates->end(); ++iterator) {
                typed::CollabAgentState decoded;
                // Map keys are protocol data, not schema path components. Keep
                // them out of diagnostics so an agent identifier cannot leak.
                constexpr std::string_view path = "$.agentsStates[*]";
                if (!decodeCollabAgentState(iterator.value(), decoded, path, state, item.diagnostics)) {
                    return std::nullopt;
                }
                item.agentsStates.emplace(iterator.key(), std::move(decoded));
            }

            if (!requiredArray(value,
                               "receiverThreadIds",
                               item.receiverThreadIds,
                               state,
                               [](const Json& field, typed::ThreadId& decoded, std::string_view path, DecodeState& nestedState) {
                                   return decodeStrongThreadId(field, decoded, path, nestedState);
                               }) ||
                !requiredDecoded(value, "senderThreadId", item.senderThreadId, state, decodeStrongThreadId) ||
                !requiredDecoded(
                    value,
                    "status",
                    item.status,
                    state,
                    [&](const Json& field, typed::CollabAgentToolCallStatus& decoded, std::string_view path, DecodeState& nestedState) {
                        return decodeOpenEnumValue(field, decoded, path, nestedState, item.diagnostics);
                    }) ||
                !requiredDecoded(value,
                                 "tool",
                                 item.tool,
                                 state,
                                 [&](const Json& field, typed::CollabAgentTool& decoded, std::string_view path, DecodeState& nestedState) {
                                     return decodeOpenEnumValue(field, decoded, path, nestedState, item.diagnostics);
                                 }) ||
                !optionalNullable(value, "model", item.model, state, decodeStrongModelId) ||
                !optionalNullable(value, "prompt", item.prompt, state, decodeStringValue) ||
                !optionalNullable(value,
                                  "reasoningEffort",
                                  item.reasoningEffort,
                                  state,
                                  [](const Json& field, typed::ReasoningEffort& decoded, std::string_view path, DecodeState& nestedState) {
                                      return decodeNonEmptyStringValue(field, decoded.value, path, nestedState);
                                  })) {
                return std::nullopt;
            }
            return typed::ThreadItem{std::move(item)};
        }

        std::optional<typed::ThreadItem> decodeCommandExecution(const Json& value,
                                                                const std::optional<typed::ThreadId>& threadId,
                                                                const std::optional<typed::TurnId>& turnId,
                                                                DecodeState& state) {
            typed::CommandExecutionThreadItem item;
            if (!makeMetadata(value, threadId, turnId, item.metadata, state) || !requiredString(value, "command", item.command, state) ||
                !requiredArray(value,
                               "commandActions",
                               item.commandActions,
                               state,
                               [&](const Json& field, typed::CommandAction& decoded, std::string_view path, DecodeState& nestedState) {
                                   return decodeNestedUnion(field, decoded, path, nestedState, item.diagnostics, decodeCommandAction);
                               }) ||
                !requiredDecoded(value, "cwd", item.cwd, state, decodeLegacyPath) ||
                !requiredDecoded(
                    value,
                    "status",
                    item.status,
                    state,
                    [&](const Json& field, typed::CommandExecutionStatus& decoded, std::string_view path, DecodeState& nestedState) {
                        return decodeOpenEnumValue(field, decoded, path, nestedState, item.diagnostics);
                    }) ||
                !optionalNullable(value, "aggregatedOutput", item.aggregatedOutput, state, decodeStringValue) ||
                !optionalNullable(value, "durationMs", item.durationMs, state, decodeInt64Value) ||
                !optionalNullable(value, "exitCode", item.exitCode, state, decodeInt32Value) ||
                !optionalNullable(value, "processId", item.processId, state, decodeStringValue) ||
                !optionalValue(
                    value,
                    "source",
                    item.source,
                    state,
                    [&](const Json& field, typed::CommandExecutionSource& decoded, std::string_view path, DecodeState& nestedState) {
                        return decodeOpenEnumValue(field, decoded, path, nestedState, item.diagnostics);
                    })) {
                return std::nullopt;
            }
            return typed::ThreadItem{std::move(item)};
        }

        std::optional<typed::ThreadItem> decodeContextCompaction(const Json& value,
                                                                 const std::optional<typed::ThreadId>& threadId,
                                                                 const std::optional<typed::TurnId>& turnId,
                                                                 DecodeState& state) {
            typed::ContextCompactionThreadItem item;
            if (!makeMetadata(value, threadId, turnId, item.metadata, state)) {
                return std::nullopt;
            }
            return typed::ThreadItem{std::move(item)};
        }

        std::optional<typed::ThreadItem> decodeDynamicToolCall(const Json& value,
                                                               const std::optional<typed::ThreadId>& threadId,
                                                               const std::optional<typed::TurnId>& turnId,
                                                               DecodeState& state) {
            typed::DynamicToolCallThreadItem item;
            if (!makeMetadata(value, threadId, turnId, item.metadata, state)) {
                return std::nullopt;
            }
            const Json* arguments = member(value, "arguments");
            if (arguments == nullptr) {
                state.fail("$.arguments");
                return std::nullopt;
            }
            if (!arguments->is_null()) {
                item.arguments = *arguments;
            }

            if (!requiredDecoded(
                    value,
                    "status",
                    item.status,
                    state,
                    [&](const Json& field, typed::DynamicToolCallStatus& decoded, std::string_view path, DecodeState& nestedState) {
                        return decodeOpenEnumValue(field, decoded, path, nestedState, item.diagnostics);
                    }) ||
                !requiredString(value, "tool", item.tool, state) ||
                !optionalNullableArray(value,
                                       "contentItems",
                                       item.contentItems,
                                       state,
                                       [&](const Json& field,
                                           typed::DynamicToolCallOutputContentItem& decoded,
                                           std::string_view path,
                                           DecodeState& nestedState) {
                                           return decodeNestedUnion(
                                               field, decoded, path, nestedState, item.diagnostics, decodeDynamicToolCallOutputContentItem);
                                       }) ||
                !optionalNullable(value, "durationMs", item.durationMs, state, decodeInt64Value) ||
                !optionalNullable(value, "namespace", item.nameSpace, state, decodeStringValue) ||
                !optionalNullable(value, "success", item.success, state, decodeBoolValue)) {
                return std::nullopt;
            }
            return typed::ThreadItem{std::move(item)};
        }

        template <typename ReviewItem>
        std::optional<typed::ThreadItem> decodeReviewMode(const Json& value,
                                                          const std::optional<typed::ThreadId>& threadId,
                                                          const std::optional<typed::TurnId>& turnId,
                                                          DecodeState& state) {
            ReviewItem item;
            if (!makeMetadata(value, threadId, turnId, item.metadata, state) || !requiredString(value, "review", item.review, state)) {
                return std::nullopt;
            }
            return typed::ThreadItem{std::move(item)};
        }

        std::optional<typed::ThreadItem> decodeFileChange(const Json& value,
                                                          const std::optional<typed::ThreadId>& threadId,
                                                          const std::optional<typed::TurnId>& turnId,
                                                          DecodeState& state) {
            typed::FileChangeThreadItem item;
            if (!makeMetadata(value, threadId, turnId, item.metadata, state) ||
                !requiredArray(value,
                               "changes",
                               item.changes,
                               state,
                               [&](const Json& field, typed::FileUpdateChange& decoded, std::string_view path, DecodeState& nestedState) {
                                   return decodeFileUpdateChange(field, decoded, path, nestedState, item.diagnostics);
                               }) ||
                !requiredDecoded(value,
                                 "status",
                                 item.status,
                                 state,
                                 [&](const Json& field, typed::PatchApplyStatus& decoded, std::string_view path, DecodeState& nestedState) {
                                     return decodeOpenEnumValue(field, decoded, path, nestedState, item.diagnostics);
                                 })) {
                return std::nullopt;
            }
            return typed::ThreadItem{std::move(item)};
        }

        std::optional<typed::ThreadItem> decodeHookPrompt(const Json& value,
                                                          const std::optional<typed::ThreadId>& threadId,
                                                          const std::optional<typed::TurnId>& turnId,
                                                          DecodeState& state) {
            typed::HookPromptThreadItem item;
            if (!makeMetadata(value, threadId, turnId, item.metadata, state) ||
                !requiredArray(value,
                               "fragments",
                               item.fragments,
                               state,
                               [](const Json& field, typed::HookPromptFragment& decoded, std::string_view path, DecodeState& nestedState) {
                                   return decodeHookPromptFragment(field, decoded, path, nestedState);
                               })) {
                return std::nullopt;
            }
            return typed::ThreadItem{std::move(item)};
        }

        std::optional<typed::ThreadItem> decodeImageGeneration(const Json& value,
                                                               const std::optional<typed::ThreadId>& threadId,
                                                               const std::optional<typed::TurnId>& turnId,
                                                               DecodeState& state) {
            typed::ImageGenerationThreadItem item;
            if (!makeMetadata(value, threadId, turnId, item.metadata, state) || !requiredString(value, "result", item.result, state) ||
                !requiredString(value, "status", item.status, state) ||
                !optionalNullable(value, "revisedPrompt", item.revisedPrompt, state, decodeStringValue) ||
                !optionalNullable(value, "savedPath", item.savedPath, state, decodeAbsolutePath)) {
                return std::nullopt;
            }
            return typed::ThreadItem{std::move(item)};
        }

        std::optional<typed::ThreadItem> decodeImageView(const Json& value,
                                                         const std::optional<typed::ThreadId>& threadId,
                                                         const std::optional<typed::TurnId>& turnId,
                                                         DecodeState& state) {
            typed::ImageViewThreadItem item;
            if (!makeMetadata(value, threadId, turnId, item.metadata, state) ||
                !requiredDecoded(value, "path", item.path, state, decodeLegacyPath)) {
                return std::nullopt;
            }
            return typed::ThreadItem{std::move(item)};
        }

        std::optional<typed::ThreadItem> decodeMcpToolCall(const Json& value,
                                                           const std::optional<typed::ThreadId>& threadId,
                                                           const std::optional<typed::TurnId>& turnId,
                                                           DecodeState& state) {
            typed::McpToolCallThreadItem item;
            if (!makeMetadata(value, threadId, turnId, item.metadata, state)) {
                return std::nullopt;
            }
            const Json* arguments = member(value, "arguments");
            if (arguments == nullptr) {
                state.fail("$.arguments");
                return std::nullopt;
            }
            if (!arguments->is_null()) {
                item.arguments = *arguments;
            }

            if (!requiredString(value, "server", item.server, state) ||
                !requiredDecoded(
                    value,
                    "status",
                    item.status,
                    state,
                    [&](const Json& field, typed::McpToolCallStatus& decoded, std::string_view path, DecodeState& nestedState) {
                        return decodeOpenEnumValue(field, decoded, path, nestedState, item.diagnostics);
                    }) ||
                !requiredString(value, "tool", item.tool, state) ||
                !optionalNullable(
                    value,
                    "appContext",
                    item.appContext,
                    state,
                    [](const Json& field, typed::McpToolCallAppContext& decoded, std::string_view path, DecodeState& nestedState) {
                        return decodeMcpToolCallAppContext(field, decoded, path, nestedState);
                    }) ||
                !optionalNullable(value, "durationMs", item.durationMs, state, decodeInt64Value) ||
                !optionalNullable(value,
                                  "error",
                                  item.error,
                                  state,
                                  [](const Json& field, typed::McpToolCallError& decoded, std::string_view path, DecodeState& nestedState) {
                                      return decodeMcpToolCallError(field, decoded, path, nestedState);
                                  }) ||
                !optionalNullable(value, "mcpAppResourceUri", item.mcpAppResourceUri, state, decodeStringValue) ||
                !optionalNullable(value, "pluginId", item.pluginId, state, decodeStringValue) ||
                !optionalNullable(
                    value,
                    "result",
                    item.result,
                    state,
                    [](const Json& field, typed::McpToolCallResult& decoded, std::string_view path, DecodeState& nestedState) {
                        return decodeMcpToolCallResult(field, decoded, path, nestedState);
                    })) {
                return std::nullopt;
            }
            return typed::ThreadItem{std::move(item)};
        }

        std::optional<typed::ThreadItem> decodePlan(const Json& value,
                                                    const std::optional<typed::ThreadId>& threadId,
                                                    const std::optional<typed::TurnId>& turnId,
                                                    DecodeState& state) {
            typed::PlanThreadItem item;
            if (!makeMetadata(value, threadId, turnId, item.metadata, state) || !requiredString(value, "text", item.text, state)) {
                return std::nullopt;
            }
            return typed::ThreadItem{std::move(item)};
        }

        std::optional<typed::ThreadItem> decodeReasoning(const Json& value,
                                                         const std::optional<typed::ThreadId>& threadId,
                                                         const std::optional<typed::TurnId>& turnId,
                                                         DecodeState& state) {
            typed::ReasoningThreadItem item;
            const auto decodeStringEntry = [](const Json& field, std::string& decoded, std::string_view path, DecodeState& nestedState) {
                return decodeStringValue(field, decoded, path, nestedState);
            };
            if (!makeMetadata(value, threadId, turnId, item.metadata, state) ||
                !optionalArray(value, "content", item.content, state, decodeStringEntry) ||
                !optionalArray(value, "summary", item.summary, state, decodeStringEntry)) {
                return std::nullopt;
            }
            return typed::ThreadItem{std::move(item)};
        }

        std::optional<typed::ThreadItem> decodeSleep(const Json& value,
                                                     const std::optional<typed::ThreadId>& threadId,
                                                     const std::optional<typed::TurnId>& turnId,
                                                     DecodeState& state) {
            typed::SleepThreadItem item;
            if (!makeMetadata(value, threadId, turnId, item.metadata, state) ||
                !requiredDecoded(value, "durationMs", item.durationMs, state, decodeUint64Value)) {
                return std::nullopt;
            }
            return typed::ThreadItem{std::move(item)};
        }

        std::optional<typed::ThreadItem> decodeSubAgentActivity(const Json& value,
                                                                const std::optional<typed::ThreadId>& threadId,
                                                                const std::optional<typed::TurnId>& turnId,
                                                                DecodeState& state) {
            typed::SubAgentActivityThreadItem item;
            if (!makeMetadata(value, threadId, turnId, item.metadata, state) ||
                !requiredString(value, "agentPath", item.agentPath, state) ||
                !requiredDecoded(value, "agentThreadId", item.agentThreadId, state, decodeStrongThreadId) ||
                !requiredDecoded(
                    value,
                    "kind",
                    item.kind,
                    state,
                    [&](const Json& field, typed::SubAgentActivityKind& decoded, std::string_view path, DecodeState& nestedState) {
                        return decodeOpenEnumValue(field, decoded, path, nestedState, item.diagnostics);
                    })) {
                return std::nullopt;
            }
            return typed::ThreadItem{std::move(item)};
        }

        std::optional<typed::ThreadItem> decodeUserMessage(const Json& value,
                                                           const std::optional<typed::ThreadId>& threadId,
                                                           const std::optional<typed::TurnId>& turnId,
                                                           DecodeState& state) {
            typed::UserMessageThreadItem item;
            if (!makeMetadata(value, threadId, turnId, item.metadata, state) ||
                !requiredArray(value,
                               "content",
                               item.content,
                               state,
                               [&](const Json& field, typed::UserInput& decoded, std::string_view path, DecodeState& nestedState) {
                                   return decodeNestedUnion(field, decoded, path, nestedState, item.diagnostics, decodeUserInput);
                               }) ||
                !optionalNullable(value, "clientId", item.clientId, state, decodeStrongClientUserMessageId)) {
                return std::nullopt;
            }
            return typed::ThreadItem{std::move(item)};
        }

        std::optional<typed::ThreadItem> decodeWebSearch(const Json& value,
                                                         const std::optional<typed::ThreadId>& threadId,
                                                         const std::optional<typed::TurnId>& turnId,
                                                         DecodeState& state) {
            typed::WebSearchThreadItem item;
            if (!makeMetadata(value, threadId, turnId, item.metadata, state) || !requiredString(value, "query", item.query, state) ||
                !optionalNullable(value,
                                  "action",
                                  item.action,
                                  state,
                                  [&](const Json& field, typed::WebSearchAction& decoded, std::string_view path, DecodeState& nestedState) {
                                      return decodeNestedUnion(field, decoded, path, nestedState, item.diagnostics, decodeWebSearchAction);
                                  })) {
                return std::nullopt;
            }
            return typed::ThreadItem{std::move(item)};
        }

        std::optional<typed::ResponseItem> decodeMessageResponseItem(const Json& value, DecodeState& state) {
            typed::MessageResponseItem item;
            if (!requiredArray(value,
                               "content",
                               item.content,
                               state,
                               [&](const Json& field, typed::ContentItem& decoded, std::string_view path, DecodeState& nestedState) {
                                   return decodeNestedUnion(field, decoded, path, nestedState, item.diagnostics, decodeContentItem);
                               }) ||
                !requiredString(value, "role", item.role, state) ||
                !decodeResponseCommon(value, item.id, item.internalChatMessageMetadataPassthrough, state) ||
                !optionalNullable(value,
                                  "phase",
                                  item.phase,
                                  state,
                                  [&](const Json& field, typed::MessagePhase& decoded, std::string_view path, DecodeState& nestedState) {
                                      return decodeOpenEnumValue(field, decoded, path, nestedState, item.diagnostics);
                                  })) {
                return std::nullopt;
            }
            item.raw = value;
            return typed::ResponseItem{std::move(item)};
        }

        std::optional<typed::ResponseItem> decodeAgentMessageResponseItem(const Json& value, DecodeState& state) {
            typed::AgentMessageResponseItem item;
            if (!requiredString(value, "author", item.author, state) ||
                !requiredArray(
                    value,
                    "content",
                    item.content,
                    state,
                    [&](const Json& field, typed::AgentMessageInputContent& decoded, std::string_view path, DecodeState& nestedState) {
                        return decodeNestedUnion(field, decoded, path, nestedState, item.diagnostics, decodeAgentMessageInputContent);
                    }) ||
                !requiredString(value, "recipient", item.recipient, state) ||
                !decodeResponseCommon(value, item.id, item.internalChatMessageMetadataPassthrough, state)) {
                return std::nullopt;
            }
            item.raw = value;
            return typed::ResponseItem{std::move(item)};
        }

        std::optional<typed::ResponseItem> decodeReasoningResponseItem(const Json& value, DecodeState& state) {
            typed::ReasoningResponseItem item;
            if (!requiredArray(
                    value,
                    "summary",
                    item.summary,
                    state,
                    [&](const Json& field, typed::ReasoningItemReasoningSummary& decoded, std::string_view path, DecodeState& nestedState) {
                        return decodeNestedUnion(field, decoded, path, nestedState, item.diagnostics, decodeReasoningItemReasoningSummary);
                    }) ||
                !optionalNullableArray(
                    value,
                    "content",
                    item.content,
                    state,
                    [&](const Json& field, typed::ReasoningItemContent& decoded, std::string_view path, DecodeState& nestedState) {
                        return decodeNestedUnion(field, decoded, path, nestedState, item.diagnostics, decodeReasoningItemContent);
                    }) ||
                !optionalNullable(value, "encrypted_content", item.encryptedContent, state, decodeStringValue) ||
                !decodeResponseCommon(value, item.id, item.internalChatMessageMetadataPassthrough, state)) {
                return std::nullopt;
            }
            item.raw = value;
            return typed::ResponseItem{std::move(item)};
        }

        std::optional<typed::ResponseItem> decodeLocalShellCallResponseItem(const Json& value, DecodeState& state) {
            typed::LocalShellCallResponseItem item;
            if (!requiredDecoded(value,
                                 "action",
                                 item.action,
                                 state,
                                 [&](const Json& field, typed::LocalShellAction& decoded, std::string_view path, DecodeState& nestedState) {
                                     return decodeNestedUnion(field, decoded, path, nestedState, item.diagnostics, decodeLocalShellAction);
                                 }) ||
                !requiredDecoded(value,
                                 "status",
                                 item.status,
                                 state,
                                 [&](const Json& field, typed::LocalShellStatus& decoded, std::string_view path, DecodeState& nestedState) {
                                     return decodeOpenEnumValue(field, decoded, path, nestedState, item.diagnostics);
                                 }) ||
                !optionalNullable(value, "call_id", item.callId, state, decodeStrongResponseCallId) ||
                !decodeResponseCommon(value, item.id, item.internalChatMessageMetadataPassthrough, state)) {
                return std::nullopt;
            }
            item.raw = value;
            return typed::ResponseItem{std::move(item)};
        }

        std::optional<typed::ResponseItem> decodeFunctionCallResponseItem(const Json& value, DecodeState& state) {
            typed::FunctionCallResponseItem item;
            if (!requiredString(value, "arguments", item.arguments, state) ||
                !requiredDecoded(value, "call_id", item.callId, state, decodeStrongResponseCallId) ||
                !requiredString(value, "name", item.name, state) ||
                !decodeResponseCommon(value, item.id, item.internalChatMessageMetadataPassthrough, state) ||
                !optionalNullable(value, "namespace", item.nameSpace, state, decodeStringValue)) {
                return std::nullopt;
            }
            item.raw = value;
            return typed::ResponseItem{std::move(item)};
        }

        std::optional<typed::ResponseItem> decodeToolSearchCallResponseItem(const Json& value, DecodeState& state) {
            typed::ToolSearchCallResponseItem item;
            const Json* arguments = member(value, "arguments");
            if (arguments == nullptr) {
                state.fail("$.arguments");
                return std::nullopt;
            }
            if (!arguments->is_null()) {
                item.arguments = *arguments;
            }
            if (!requiredString(value, "execution", item.execution, state) ||
                !optionalNullable(value, "call_id", item.callId, state, decodeStrongResponseCallId) ||
                !decodeResponseCommon(value, item.id, item.internalChatMessageMetadataPassthrough, state) ||
                !optionalNullable(value, "status", item.status, state, decodeStringValue)) {
                return std::nullopt;
            }
            item.raw = value;
            return typed::ResponseItem{std::move(item)};
        }

        bool decodeFunctionCallOutputBody(const Json& value,
                                          typed::FunctionCallOutputBody& result,
                                          std::string_view path,
                                          DecodeState& state,
                                          std::vector<typed::DecodeDiagnostic>& diagnostics) {
            if (value.is_string()) {
                result = value.get<std::string>();
                return true;
            }
            if (!value.is_array()) {
                return state.fail(std::string(path));
            }
            std::vector<typed::FunctionCallOutputContentItem> content;
            if (!decodeArrayValue(value,
                                  content,
                                  path,
                                  state,
                                  [&](const Json& field,
                                      typed::FunctionCallOutputContentItem& decoded,
                                      std::string_view nestedPath,
                                      DecodeState& nestedState) {
                                      return decodeNestedUnion(
                                          field, decoded, nestedPath, nestedState, diagnostics, decodeFunctionCallOutputContentItem);
                                  })) {
                return false;
            }
            result = std::move(content);
            return true;
        }

        std::optional<typed::ResponseItem> decodeFunctionCallOutputResponseItem(const Json& value, DecodeState& state) {
            typed::FunctionCallOutputResponseItem item;
            if (!requiredDecoded(value, "call_id", item.callId, state, decodeStrongResponseCallId) ||
                !requiredDecoded(
                    value,
                    "output",
                    item.output,
                    state,
                    [&](const Json& field, typed::FunctionCallOutputBody& decoded, std::string_view path, DecodeState& nestedState) {
                        return decodeFunctionCallOutputBody(field, decoded, path, nestedState, item.diagnostics);
                    }) ||
                !decodeResponseCommon(value, item.id, item.internalChatMessageMetadataPassthrough, state)) {
                return std::nullopt;
            }
            item.raw = value;
            return typed::ResponseItem{std::move(item)};
        }

        std::optional<typed::ResponseItem> decodeCustomToolCallResponseItem(const Json& value, DecodeState& state) {
            typed::CustomToolCallResponseItem item;
            if (!requiredDecoded(value, "call_id", item.callId, state, decodeStrongResponseCallId) ||
                !requiredString(value, "input", item.input, state) || !requiredString(value, "name", item.name, state) ||
                !decodeResponseCommon(value, item.id, item.internalChatMessageMetadataPassthrough, state) ||
                !optionalNullable(value, "namespace", item.nameSpace, state, decodeStringValue) ||
                !optionalNullable(value, "status", item.status, state, decodeStringValue)) {
                return std::nullopt;
            }
            item.raw = value;
            return typed::ResponseItem{std::move(item)};
        }

        std::optional<typed::ResponseItem> decodeCustomToolCallOutputResponseItem(const Json& value, DecodeState& state) {
            typed::CustomToolCallOutputResponseItem item;
            if (!requiredDecoded(value, "call_id", item.callId, state, decodeStrongResponseCallId) ||
                !requiredDecoded(
                    value,
                    "output",
                    item.output,
                    state,
                    [&](const Json& field, typed::FunctionCallOutputBody& decoded, std::string_view path, DecodeState& nestedState) {
                        return decodeFunctionCallOutputBody(field, decoded, path, nestedState, item.diagnostics);
                    }) ||
                !decodeResponseCommon(value, item.id, item.internalChatMessageMetadataPassthrough, state) ||
                !optionalNullable(value, "name", item.name, state, decodeStringValue)) {
                return std::nullopt;
            }
            item.raw = value;
            return typed::ResponseItem{std::move(item)};
        }

        std::optional<typed::ResponseItem> decodeToolSearchOutputResponseItem(const Json& value, DecodeState& state) {
            typed::ToolSearchOutputResponseItem item;
            if (!requiredString(value, "execution", item.execution, state) || !requiredString(value, "status", item.status, state)) {
                return std::nullopt;
            }
            const Json* tools = member(value, "tools");
            if (tools == nullptr || !tools->is_array()) {
                state.fail("$.tools");
                return std::nullopt;
            }
            item.tools.clear();
            item.tools.reserve(tools->size());
            for (const Json& tool : *tools) {
                item.tools.emplace_back(tool);
            }
            if (!optionalNullable(value, "call_id", item.callId, state, decodeStrongResponseCallId) ||
                !decodeResponseCommon(value, item.id, item.internalChatMessageMetadataPassthrough, state)) {
                return std::nullopt;
            }
            item.raw = value;
            return typed::ResponseItem{std::move(item)};
        }

        std::optional<typed::ResponseItem> decodeWebSearchCallResponseItem(const Json& value, DecodeState& state) {
            typed::WebSearchCallResponseItem item;
            if (!optionalNullable(
                    value,
                    "action",
                    item.action,
                    state,
                    [&](const Json& field, typed::ResponsesApiWebSearchAction& decoded, std::string_view path, DecodeState& nestedState) {
                        return decodeNestedUnion(field, decoded, path, nestedState, item.diagnostics, decodeResponsesApiWebSearchAction);
                    }) ||
                !decodeResponseCommon(value, item.id, item.internalChatMessageMetadataPassthrough, state) ||
                !optionalNullable(value, "status", item.status, state, decodeStringValue)) {
                return std::nullopt;
            }
            item.raw = value;
            return typed::ResponseItem{std::move(item)};
        }

        std::optional<typed::ResponseItem> decodeImageGenerationCallResponseItem(const Json& value, DecodeState& state) {
            typed::ImageGenerationCallResponseItem item;
            if (!requiredString(value, "result", item.result, state) || !requiredString(value, "status", item.status, state) ||
                !decodeResponseCommon(value, item.id, item.internalChatMessageMetadataPassthrough, state) ||
                !optionalNullable(value, "revised_prompt", item.revisedPrompt, state, decodeStringValue)) {
                return std::nullopt;
            }
            item.raw = value;
            return typed::ResponseItem{std::move(item)};
        }

        std::optional<typed::ResponseItem> decodeCompactionResponseItem(const Json& value, DecodeState& state) {
            typed::CompactionResponseItem item;
            if (!requiredString(value, "encrypted_content", item.encryptedContent, state) ||
                !decodeResponseCommon(value, item.id, item.internalChatMessageMetadataPassthrough, state)) {
                return std::nullopt;
            }
            item.raw = value;
            return typed::ResponseItem{std::move(item)};
        }

        std::optional<typed::ResponseItem> decodeCompactionTriggerResponseItem(const Json& value) {
            return typed::ResponseItem{typed::CompactionTriggerResponseItem{value, {}}};
        }

        std::optional<typed::ResponseItem> decodeContextCompactionResponseItem(const Json& value, DecodeState& state) {
            typed::ContextCompactionResponseItem item;
            if (!optionalNullable(value, "encrypted_content", item.encryptedContent, state, decodeStringValue) ||
                !decodeResponseCommon(value, item.id, item.internalChatMessageMetadataPassthrough, state)) {
                return std::nullopt;
            }
            item.raw = value;
            return typed::ResponseItem{std::move(item)};
        }

        std::optional<typed::ResponseItem> decodeOtherResponseItem(const Json& value) {
            return typed::ResponseItem{typed::OtherResponseItem{value, {}}};
        }

    } // namespace

    std::optional<typed::Item>
    decodeItem(const Json& value, std::optional<typed::ThreadId> threadId, std::optional<typed::TurnId> turnId, std::string& error) {
        try {
            error.clear();
            if (!value.is_object()) {
                error = "ThreadItem payload must be an object";
                return std::nullopt;
            }

            const Json* discriminator = member(value, TypeField);
            if (discriminator == nullptr || !discriminator->is_string() || discriminator->get_ref<const std::string&>().empty()) {
                DecodeState state{ThreadItemSurface};
                state.fail("$.type");
                return typed::Item{
                    makeUnknownItem(value, std::nullopt, threadId, turnId, std::move(state.error), std::move(state.diagnostic))};
            }

            const std::string type = discriminator->get<std::string>();
            const ProtocolSurfaceEntry* entry = findSurface(SurfaceCategory::ItemDiscriminator, ThreadItemSurface, TypeField, type);
            if (entry == nullptr) {
                return typed::Item{makeUnknownItem(value, type, threadId, turnId, std::nullopt, std::nullopt)};
            }

            const ThreadItemCodecDescriptor* descriptor = descriptorForThreadItem(*entry);
            if (descriptor == nullptr) {
                DecodeState state{ThreadItemSurface};
                state.fail("$.type");
                return typed::Item{makeUnknownItem(value, type, threadId, turnId, std::move(state.error), std::move(state.diagnostic))};
            }

            DecodeState state{ThreadItemSurface};
            std::optional<typed::ThreadItem> decoded;
            switch (descriptor->target) {
                case ItemDiscriminatorTarget::AgentMessage:
                    decoded = decodeAgentMessage(value, threadId, turnId, state);
                    break;
                case ItemDiscriminatorTarget::CollabAgentToolCall:
                    decoded = decodeCollabAgentToolCall(value, threadId, turnId, state);
                    break;
                case ItemDiscriminatorTarget::CommandExecution:
                    decoded = decodeCommandExecution(value, threadId, turnId, state);
                    break;
                case ItemDiscriminatorTarget::ContextCompaction:
                    decoded = decodeContextCompaction(value, threadId, turnId, state);
                    break;
                case ItemDiscriminatorTarget::DynamicToolCall:
                    decoded = decodeDynamicToolCall(value, threadId, turnId, state);
                    break;
                case ItemDiscriminatorTarget::EnteredReviewMode:
                    decoded = decodeReviewMode<typed::EnteredReviewModeThreadItem>(value, threadId, turnId, state);
                    break;
                case ItemDiscriminatorTarget::ExitedReviewMode:
                    decoded = decodeReviewMode<typed::ExitedReviewModeThreadItem>(value, threadId, turnId, state);
                    break;
                case ItemDiscriminatorTarget::FileChange:
                    decoded = decodeFileChange(value, threadId, turnId, state);
                    break;
                case ItemDiscriminatorTarget::HookPrompt:
                    decoded = decodeHookPrompt(value, threadId, turnId, state);
                    break;
                case ItemDiscriminatorTarget::ImageGeneration:
                    decoded = decodeImageGeneration(value, threadId, turnId, state);
                    break;
                case ItemDiscriminatorTarget::ImageView:
                    decoded = decodeImageView(value, threadId, turnId, state);
                    break;
                case ItemDiscriminatorTarget::McpToolCall:
                    decoded = decodeMcpToolCall(value, threadId, turnId, state);
                    break;
                case ItemDiscriminatorTarget::Plan:
                    decoded = decodePlan(value, threadId, turnId, state);
                    break;
                case ItemDiscriminatorTarget::Reasoning:
                    decoded = decodeReasoning(value, threadId, turnId, state);
                    break;
                case ItemDiscriminatorTarget::Sleep:
                    decoded = decodeSleep(value, threadId, turnId, state);
                    break;
                case ItemDiscriminatorTarget::SubAgentActivity:
                    decoded = decodeSubAgentActivity(value, threadId, turnId, state);
                    break;
                case ItemDiscriminatorTarget::UserMessage:
                    decoded = decodeUserMessage(value, threadId, turnId, state);
                    break;
                case ItemDiscriminatorTarget::WebSearch:
                    decoded = decodeWebSearch(value, threadId, turnId, state);
                    break;
                case ItemDiscriminatorTarget::Count:
                    state.fail("$.type");
                    break;
            }

            if (decoded) {
                return std::optional<typed::Item>{std::move(*decoded)};
            }
            if (state.error.empty()) {
                state.fail("$");
            }
            error.clear();
            return typed::Item{makeUnknownItem(value, type, threadId, turnId, std::move(state.error), std::move(state.diagnostic))};
        } catch (const std::exception&) {
            error = "ThreadItem decoding failed without exposing payload data";
        } catch (...) {
            error = "ThreadItem decoding failed without exposing payload data";
        }
        return std::nullopt;
    }

    std::optional<typed::ResponseItem> decodeResponseItem(const Json& value, std::string& error) {
        try {
            error.clear();
            if (!value.is_object()) {
                error = "ResponseItem payload must be an object";
                return std::nullopt;
            }

            const Json* discriminator = member(value, TypeField);
            if (discriminator == nullptr || !discriminator->is_string() || discriminator->get_ref<const std::string&>().empty()) {
                DecodeState state{ResponseItemSurface};
                state.fail("$.type");
                return typed::ResponseItem{
                    makeUnknownResponseItem(value, std::nullopt, std::move(state.error), std::move(state.diagnostic))};
            }

            const std::string type = discriminator->get<std::string>();
            const ProtocolSurfaceEntry* entry = findSurface(SurfaceCategory::ItemDiscriminator, ResponseItemSurface, TypeField, type);
            if (entry == nullptr) {
                return typed::ResponseItem{makeUnknownResponseItem(value, type, std::nullopt, std::nullopt)};
            }

            const ResponseItemCodecDescriptor* descriptor = descriptorForResponseItem(*entry);
            if (descriptor == nullptr) {
                DecodeState state{ResponseItemSurface};
                state.fail("$.type");
                return typed::ResponseItem{makeUnknownResponseItem(value, type, std::move(state.error), std::move(state.diagnostic))};
            }

            DecodeState state{ResponseItemSurface};
            std::optional<typed::ResponseItem> decoded;
            switch (descriptor->target) {
                case ResponseItemTarget::AgentMessage:
                    decoded = decodeAgentMessageResponseItem(value, state);
                    break;
                case ResponseItemTarget::Compaction:
                    decoded = decodeCompactionResponseItem(value, state);
                    break;
                case ResponseItemTarget::CompactionTrigger:
                    decoded = decodeCompactionTriggerResponseItem(value);
                    break;
                case ResponseItemTarget::ContextCompaction:
                    decoded = decodeContextCompactionResponseItem(value, state);
                    break;
                case ResponseItemTarget::CustomToolCall:
                    decoded = decodeCustomToolCallResponseItem(value, state);
                    break;
                case ResponseItemTarget::CustomToolCallOutput:
                    decoded = decodeCustomToolCallOutputResponseItem(value, state);
                    break;
                case ResponseItemTarget::FunctionCall:
                    decoded = decodeFunctionCallResponseItem(value, state);
                    break;
                case ResponseItemTarget::FunctionCallOutput:
                    decoded = decodeFunctionCallOutputResponseItem(value, state);
                    break;
                case ResponseItemTarget::ImageGenerationCall:
                    decoded = decodeImageGenerationCallResponseItem(value, state);
                    break;
                case ResponseItemTarget::LocalShellCall:
                    decoded = decodeLocalShellCallResponseItem(value, state);
                    break;
                case ResponseItemTarget::Message:
                    decoded = decodeMessageResponseItem(value, state);
                    break;
                case ResponseItemTarget::Other:
                    decoded = decodeOtherResponseItem(value);
                    break;
                case ResponseItemTarget::Reasoning:
                    decoded = decodeReasoningResponseItem(value, state);
                    break;
                case ResponseItemTarget::ToolSearchCall:
                    decoded = decodeToolSearchCallResponseItem(value, state);
                    break;
                case ResponseItemTarget::ToolSearchOutput:
                    decoded = decodeToolSearchOutputResponseItem(value, state);
                    break;
                case ResponseItemTarget::WebSearchCall:
                    decoded = decodeWebSearchCallResponseItem(value, state);
                    break;
                case ResponseItemTarget::Count:
                    state.fail("$.type");
                    break;
            }

            if (decoded) {
                return std::optional<typed::ResponseItem>{std::move(*decoded)};
            }
            if (state.error.empty()) {
                state.fail("$");
            }
            error.clear();
            return typed::ResponseItem{makeUnknownResponseItem(value, type, std::move(state.error), std::move(state.diagnostic))};
        } catch (const std::exception&) {
            error = "ResponseItem decoding failed without exposing payload data";
        } catch (...) {
            error = "ResponseItem decoding failed without exposing payload data";
        }
        return std::nullopt;
    }

} // namespace ai::openai::codex::detail
