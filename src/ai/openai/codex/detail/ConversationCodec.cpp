/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ConversationCodec.h"

#include "ai/openai/codex/detail/DecodeDiagnostic.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "ai/openai/codex/typed/Types.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace ai::openai::codex::detail {

    namespace {

        constexpr std::string_view VariantField = "$variant";
        constexpr std::string_view TypeField = "type";

        struct ResolvedDiscriminator {
            std::string discriminator;
            const ProtocolSurfaceEntry* entry = nullptr;
            const ConversationUnionCodecDescriptor* descriptor = nullptr;
            std::optional<typed::DecodeDiagnostic> diagnostic;

            [[nodiscard]] bool unknown() const noexcept {
                return entry == nullptr && !diagnostic.has_value();
            }

            [[nodiscard]] bool malformed() const noexcept {
                return diagnostic.has_value();
            }
        };

        const Json* member(const Json& object, std::string_view name) noexcept {
            if (!object.is_object()) {
                return nullptr;
            }
            const auto iterator = object.find(name);
            return iterator == object.end() ? nullptr : &*iterator;
        }

        const ConversationUnionCodecDescriptor* descriptorForEntry(const ProtocolSurfaceEntry& entry) noexcept {
            if (entry.runtimeDisposition != RuntimeDisposition::Typed ||
                entry.typedImplementation != TypedImplementationStatus::Implemented) {
                return nullptr;
            }
            const ConversationUnionTarget* target = std::get_if<ConversationUnionTarget>(&entry.runtimeTarget);
            if (target == nullptr) {
                return nullptr;
            }
            const auto descriptors = conversationUnionCodecDescriptors();
            const auto descriptor =
                std::find_if(descriptors.begin(), descriptors.end(), [&](const ConversationUnionCodecDescriptor& candidate) {
                    return candidate.key == entry.key && candidate.target == *target;
                });
            return descriptor == descriptors.end() ? nullptr : &*descriptor;
        }

        ResolvedDiscriminator resolveInternal(const Json& value, std::string_view surface) {
            ResolvedDiscriminator result;
            if (!value.is_object()) {
                result.diagnostic = malformedKnownDiagnostic(std::string(surface), "$");
                return result;
            }

            const Json* type = member(value, TypeField);
            if (type == nullptr || !type->is_string()) {
                result.diagnostic = malformedKnownDiagnostic(std::string(surface), "$.type");
                return result;
            }
            result.discriminator = type->get<std::string>();
            result.entry = findSurface(SurfaceCategory::TaggedUnionDiscriminator, surface, TypeField, result.discriminator);
            if (result.entry == nullptr) {
                return result;
            }
            result.descriptor = descriptorForEntry(*result.entry);
            if (result.descriptor == nullptr || result.descriptor->shape != ConversationUnionCodecShape::InternallyTaggedObject) {
                result.diagnostic = malformedKnownDiagnostic(std::string(surface), "$.type");
            }
            return result;
        }

        template <typename T>
        ConversationDecodeResult<T> malformed(std::string_view surface, std::string fieldPath) {
            return {std::nullopt, malformedKnownDiagnostic(std::string(surface), std::move(fieldPath))};
        }

        template <typename Union, typename Unknown>
        ConversationDecodeResult<Union>
        unknown(const Json& value, std::optional<std::string> discriminator, std::string_view surface, std::string fieldPath) {
            typed::DecodeDiagnostic diagnostic = unknownDiscriminatorDiagnostic(std::string(surface), std::move(fieldPath));
            return {Union{Unknown{std::move(discriminator), value, diagnostic}}, std::move(diagnostic)};
        }

        template <typename Union, typename Alternative>
        ConversationDecodeResult<Union> decoded(Alternative value, std::optional<typed::DecodeDiagnostic> diagnostic = std::nullopt) {
            return {Union{std::move(value)}, std::move(diagnostic)};
        }

        bool requiredString(const Json& object,
                            std::string_view name,
                            std::string& result,
                            std::string_view surface,
                            std::optional<typed::DecodeDiagnostic>& diagnostic,
                            std::string_view prefix = "$.") {
            const Json* value = member(object, name);
            if (value == nullptr || !value->is_string()) {
                diagnostic = malformedKnownDiagnostic(std::string(surface), std::string(prefix) + std::string(name));
                return false;
            }
            result = value->get<std::string>();
            return true;
        }

        bool optionalBool(const Json& object,
                          std::string_view name,
                          std::optional<bool>& result,
                          std::string_view surface,
                          std::optional<typed::DecodeDiagnostic>& diagnostic) {
            const Json* value = member(object, name);
            if (value == nullptr) {
                result.reset();
                return true;
            }
            if (!value->is_boolean()) {
                diagnostic = malformedKnownDiagnostic(std::string(surface), "$." + std::string(name));
                return false;
            }
            result = value->get<bool>();
            return true;
        }

        template <typename T, typename Decode>
        bool optionalNullable(const Json& object,
                              std::string_view name,
                              typed::OptionalNullable<T>& result,
                              std::string_view surface,
                              std::optional<typed::DecodeDiagnostic>& diagnostic,
                              Decode&& decode) {
            result = typed::OptionalNullable<T>::omitted();
            const Json* value = member(object, name);
            if (value == nullptr) {
                return true;
            }
            result.present = true;
            if (value->is_null()) {
                return true;
            }
            T decodedValue;
            if (!decode(*value, decodedValue)) {
                diagnostic = malformedKnownDiagnostic(std::string(surface), "$." + std::string(name));
                return false;
            }
            result.value = std::move(decodedValue);
            return true;
        }

        bool decodeStringValue(const Json& value, std::string& result) {
            if (!value.is_string()) {
                return false;
            }
            result = value.get<std::string>();
            return true;
        }

        bool decodeStringArray(const Json& value, std::vector<std::string>& result) {
            if (!value.is_array()) {
                return false;
            }
            std::vector<std::string> decodedValues;
            decodedValues.reserve(value.size());
            for (const Json& element : value) {
                if (!element.is_string()) {
                    return false;
                }
                decodedValues.emplace_back(element.get<std::string>());
            }
            result = std::move(decodedValues);
            return true;
        }

        bool requiredStringArray(const Json& object,
                                 std::string_view name,
                                 std::vector<std::string>& result,
                                 std::string_view surface,
                                 std::optional<typed::DecodeDiagnostic>& diagnostic) {
            const Json* value = member(object, name);
            if (value == nullptr || !value->is_array()) {
                diagnostic = malformedKnownDiagnostic(std::string(surface), "$." + std::string(name));
                return false;
            }
            std::vector<std::string> decodedValues;
            decodedValues.reserve(value->size());
            for (std::size_t index = 0; index < value->size(); ++index) {
                if (!(*value)[index].is_string()) {
                    diagnostic =
                        malformedKnownDiagnostic(std::string(surface), "$." + std::string(name) + "[" + std::to_string(index) + "]");
                    return false;
                }
                decodedValues.emplace_back((*value)[index].get<std::string>());
            }
            result = std::move(decodedValues);
            return true;
        }

        bool decodeStringMap(const Json& value, std::map<std::string, std::string>& result) {
            if (!value.is_object()) {
                return false;
            }
            std::map<std::string, std::string> decodedValues;
            for (auto iterator = value.begin(); iterator != value.end(); ++iterator) {
                if (!iterator.value().is_string()) {
                    return false;
                }
                decodedValues.emplace(iterator.key(), iterator.value().get<std::string>());
            }
            result = std::move(decodedValues);
            return true;
        }

        bool decodeImageDetail(const Json& value, typed::ImageDetail& result) {
            if (!value.is_string()) {
                return false;
            }
            result.value = value.get<std::string>();
            return true;
        }

        bool decodeUint64(const Json& value, std::uint64_t& result) {
            if (value.is_number_unsigned()) {
                result = value.get<std::uint64_t>();
                return true;
            }
            if (!value.is_number_integer()) {
                return false;
            }
            const std::int64_t signedValue = value.get<std::int64_t>();
            if (signedValue < 0) {
                return false;
            }
            result = static_cast<std::uint64_t>(signedValue);
            return true;
        }

        bool decodeTextElement(const Json& value,
                               typed::TextElement& result,
                               std::size_t index,
                               std::optional<typed::DecodeDiagnostic>& diagnostic) {
            constexpr std::string_view Surface = "UserInput";
            const std::string prefix = "$.text_elements[" + std::to_string(index) + "]";
            if (!value.is_object()) {
                diagnostic = malformedKnownDiagnostic(std::string(Surface), prefix);
                return false;
            }
            const Json* byteRange = member(value, "byteRange");
            if (byteRange == nullptr || !byteRange->is_object()) {
                diagnostic = malformedKnownDiagnostic(std::string(Surface), prefix + ".byteRange");
                return false;
            }
            const Json* start = member(*byteRange, "start");
            if (start == nullptr || !decodeUint64(*start, result.byteRange.start)) {
                diagnostic = malformedKnownDiagnostic(std::string(Surface), prefix + ".byteRange.start");
                return false;
            }
            const Json* end = member(*byteRange, "end");
            if (end == nullptr || !decodeUint64(*end, result.byteRange.end)) {
                diagnostic = malformedKnownDiagnostic(std::string(Surface), prefix + ".byteRange.end");
                return false;
            }
            result.placeholder = typed::OptionalNullable<std::string>::omitted();
            const Json* placeholder = member(value, "placeholder");
            if (placeholder != nullptr) {
                result.placeholder.present = true;
                if (placeholder->is_string()) {
                    result.placeholder.value = placeholder->get<std::string>();
                } else if (!placeholder->is_null()) {
                    diagnostic = malformedKnownDiagnostic(std::string(Surface), prefix + ".placeholder");
                    return false;
                }
            }
            return true;
        }

        bool decodeOptionalTextElements(const Json& object,
                                        std::optional<std::vector<typed::TextElement>>& result,
                                        std::optional<typed::DecodeDiagnostic>& diagnostic) {
            const Json* value = member(object, "text_elements");
            if (value == nullptr) {
                result.reset();
                return true;
            }
            if (!value->is_array()) {
                diagnostic = malformedKnownDiagnostic("UserInput", "$.text_elements");
                return false;
            }
            std::vector<typed::TextElement> decodedValues;
            decodedValues.reserve(value->size());
            for (std::size_t index = 0; index < value->size(); ++index) {
                typed::TextElement element;
                if (!decodeTextElement((*value)[index], element, index, diagnostic)) {
                    return false;
                }
                decodedValues.emplace_back(std::move(element));
            }
            result = std::move(decodedValues);
            return true;
        }

        std::optional<typed::DecodeDiagnostic> imageDetailDiagnostic(const typed::OptionalNullable<typed::ImageDetail>& detail,
                                                                     std::string_view surface) {
            if (detail.value && !detail.value->isKnown()) {
                return unknownEnumDiagnostic(std::string(surface), "$.detail");
            }
            return std::nullopt;
        }

        const ConversationUnionCodecDescriptor* descriptorForEncoding(ConversationUnionTarget target, std::string& error) {
            const ProtocolSurfaceEntry* entry = nullptr;
            try {
                entry = &entryFor(target);
            } catch (const std::exception&) {
                error = "conversation union registry target is unavailable";
                return nullptr;
            } catch (...) {
                error = "conversation union registry target is unavailable";
                return nullptr;
            }
            const ConversationUnionCodecDescriptor* descriptor = descriptorForEntry(*entry);
            if (descriptor == nullptr || descriptor->direction != ConversationUnionCodecDirection::Bidirectional) {
                error = "conversation union registry target is not bidirectionally encodable";
                return nullptr;
            }
            return descriptor;
        }

        std::optional<Json>
        beginInternalEncoding(ConversationUnionTarget target, const ConversationUnionCodecDescriptor*& descriptor, std::string& error) {
            descriptor = descriptorForEncoding(target, error);
            if (descriptor == nullptr || descriptor->shape != ConversationUnionCodecShape::InternallyTaggedObject) {
                if (error.empty()) {
                    error = "conversation union registry target has the wrong encoding shape";
                }
                return std::nullopt;
            }
            return std::optional<Json>{Json::object({{std::string(descriptor->key.field), std::string(descriptor->key.name)}})};
        }

        template <typename T>
        bool validTriState(const typed::OptionalNullable<T>& value) noexcept {
            return value.present || !value.value.has_value();
        }

        template <typename T, typename Encode>
        bool encodeOptionalNullable(
            Json& object, std::string_view field, const typed::OptionalNullable<T>& value, std::string& error, Encode&& encode) {
            if (!validTriState(value)) {
                error = "optional-nullable field '" + std::string(field) + "' has an inconsistent omitted state";
                return false;
            }
            if (!value.present) {
                return true;
            }
            if (!value.value) {
                object.emplace(std::string(field), nullptr);
                return true;
            }
            std::optional<Json> encodedValue = encode(*value.value);
            if (!encodedValue) {
                error = "optional-nullable field '" + std::string(field) + "' has an invalid typed value";
                return false;
            }
            object.emplace(std::string(field), std::move(*encodedValue));
            return true;
        }

        std::optional<Json> encodeTextElement(const typed::TextElement& value, std::string& error) {
            Json encoded = Json::object({{"byteRange", Json::object({{"start", value.byteRange.start}, {"end", value.byteRange.end}})}});
            if (!encodeOptionalNullable(
                    encoded, "placeholder", value.placeholder, error, [](const std::string& text) -> std::optional<Json> {
                        return std::optional<Json>{Json(text)};
                    })) {
                return std::nullopt;
            }
            return std::optional<Json>{std::move(encoded)};
        }

    } // namespace

    ConversationDecodeResult<typed::AskForApproval> decodeAskForApproval(const Json& value) noexcept {
        constexpr std::string_view Surface = "AskForApproval";
        try {
            if (value.is_string()) {
                const std::string discriminator = value.get<std::string>();
                const ProtocolSurfaceEntry* entry =
                    findSurface(SurfaceCategory::TaggedUnionDiscriminator, Surface, VariantField, discriminator);
                if (entry == nullptr) {
                    return unknown<typed::AskForApproval, typed::UnknownAskForApproval>(value, discriminator, Surface, "$");
                }
                const ConversationUnionCodecDescriptor* descriptor = descriptorForEntry(*entry);
                if (descriptor == nullptr || descriptor->shape != ConversationUnionCodecShape::ScalarString) {
                    return malformed<typed::AskForApproval>(Surface, "$");
                }
                const ConversationUnionTarget* target = std::get_if<ConversationUnionTarget>(&entry->runtimeTarget);
                if (target == nullptr || (*target != ConversationUnionTarget::AskForApprovalNever &&
                                          *target != ConversationUnionTarget::AskForApprovalOnRequest &&
                                          *target != ConversationUnionTarget::AskForApprovalUntrusted)) {
                    return malformed<typed::AskForApproval>(Surface, "$");
                }
                return decoded<typed::AskForApproval>(typed::ApprovalPolicy{discriminator});
            }
            if (!value.is_object() || value.empty()) {
                return malformed<typed::AskForApproval>(Surface, "$");
            }

            std::optional<std::string> knownDiscriminator;
            for (auto iterator = value.begin(); iterator != value.end(); ++iterator) {
                if (findSurface(SurfaceCategory::TaggedUnionDiscriminator, Surface, VariantField, iterator.key()) != nullptr) {
                    if (knownDiscriminator) {
                        return malformed<typed::AskForApproval>(Surface, "$");
                    }
                    knownDiscriminator = iterator.key();
                }
            }
            if (value.size() != 1) {
                return malformed<typed::AskForApproval>(Surface, "$");
            }

            const auto outer = value.begin();
            const std::string discriminator = outer.key();
            const ProtocolSurfaceEntry* entry =
                findSurface(SurfaceCategory::TaggedUnionDiscriminator, Surface, VariantField, discriminator);
            if (entry == nullptr) {
                return unknown<typed::AskForApproval, typed::UnknownAskForApproval>(value, discriminator, Surface, "$");
            }
            const ConversationUnionCodecDescriptor* descriptor = descriptorForEntry(*entry);
            if (descriptor == nullptr || descriptor->shape != ConversationUnionCodecShape::ExternallyTaggedObject) {
                return malformed<typed::AskForApproval>(Surface, "$." + discriminator);
            }
            const ConversationUnionTarget* target = std::get_if<ConversationUnionTarget>(&entry->runtimeTarget);
            if (target == nullptr || *target != ConversationUnionTarget::AskForApprovalGranular || !outer.value().is_object()) {
                return malformed<typed::AskForApproval>(Surface, "$." + discriminator);
            }

            typed::GranularAskForApproval result;
            result.raw = value;
            std::optional<typed::DecodeDiagnostic> diagnostic;
            const Json& granular = outer.value();
            const Json* mcpElicitations = member(granular, "mcp_elicitations");
            if (mcpElicitations == nullptr || !mcpElicitations->is_boolean()) {
                return malformed<typed::AskForApproval>(Surface, "$.granular.mcp_elicitations");
            }
            result.granular.mcpElicitations = mcpElicitations->get<bool>();
            const Json* rules = member(granular, "rules");
            if (rules == nullptr || !rules->is_boolean()) {
                return malformed<typed::AskForApproval>(Surface, "$.granular.rules");
            }
            result.granular.rules = rules->get<bool>();
            const Json* sandboxApproval = member(granular, "sandbox_approval");
            if (sandboxApproval == nullptr || !sandboxApproval->is_boolean()) {
                return malformed<typed::AskForApproval>(Surface, "$.granular.sandbox_approval");
            }
            result.granular.sandboxApproval = sandboxApproval->get<bool>();
            if (!optionalBool(granular, "request_permissions", result.granular.requestPermissions, Surface, diagnostic) ||
                !optionalBool(granular, "skill_approval", result.granular.skillApproval, Surface, diagnostic)) {
                if (diagnostic) {
                    diagnostic->fieldPath = "$.granular" + diagnostic->fieldPath.substr(1);
                }
                return {std::nullopt, std::move(diagnostic)};
            }
            return decoded<typed::AskForApproval>(std::move(result));
        } catch (const std::exception&) {
            return malformed<typed::AskForApproval>(Surface, "$");
        } catch (...) {
            return malformed<typed::AskForApproval>(Surface, "$");
        }
    }

    ConversationDecodeResult<typed::AgentMessageInputContent> decodeAgentMessageInputContent(const Json& value) noexcept {
        constexpr std::string_view Surface = "AgentMessageInputContent";
        try {
            ResolvedDiscriminator resolved = resolveInternal(value, Surface);
            if (resolved.malformed()) {
                return {std::nullopt, std::move(resolved.diagnostic)};
            }
            if (resolved.unknown()) {
                return unknown<typed::AgentMessageInputContent, typed::UnknownAgentMessageInputContent>(
                    value, resolved.discriminator, Surface, "$.type");
            }

            std::optional<typed::DecodeDiagnostic> diagnostic;
            const ConversationUnionTarget target = resolved.descriptor->target;
            if (target == ConversationUnionTarget::AgentMessageInputContentInputText) {
                typed::InputTextAgentMessageInputContent result;
                if (!requiredString(value, "text", result.text, Surface, diagnostic)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::AgentMessageInputContent>(std::move(result));
            }
            if (target == ConversationUnionTarget::AgentMessageInputContentEncryptedContent) {
                typed::EncryptedContentAgentMessageInputContent result;
                if (!requiredString(value, "encrypted_content", result.encryptedContent, Surface, diagnostic)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::AgentMessageInputContent>(std::move(result));
            }
            return malformed<typed::AgentMessageInputContent>(Surface, "$.type");
        } catch (const std::exception&) {
            return malformed<typed::AgentMessageInputContent>(Surface, "$");
        } catch (...) {
            return malformed<typed::AgentMessageInputContent>(Surface, "$");
        }
    }

    ConversationDecodeResult<typed::CommandAction> decodeCommandAction(const Json& value) noexcept {
        constexpr std::string_view Surface = "CommandAction";
        try {
            ResolvedDiscriminator resolved = resolveInternal(value, Surface);
            if (resolved.malformed()) {
                return {std::nullopt, std::move(resolved.diagnostic)};
            }
            if (resolved.unknown()) {
                return unknown<typed::CommandAction, typed::UnrecognizedCommandAction>(value, resolved.discriminator, Surface, "$.type");
            }
            std::optional<typed::DecodeDiagnostic> diagnostic;
            std::string command;
            if (!requiredString(value, "command", command, Surface, diagnostic)) {
                return {std::nullopt, std::move(diagnostic)};
            }
            const ConversationUnionTarget target = resolved.descriptor->target;
            if (target == ConversationUnionTarget::CommandActionRead) {
                typed::ReadCommandAction result;
                result.command = std::move(command);
                std::string path;
                if (!requiredString(value, "name", result.name, Surface, diagnostic) ||
                    !requiredString(value, "path", path, Surface, diagnostic)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.path = typed::AbsolutePathBuf{std::move(path)};
                result.raw = value;
                return decoded<typed::CommandAction>(std::move(result));
            }
            if (target == ConversationUnionTarget::CommandActionListFiles) {
                typed::ListFilesCommandAction result;
                result.command = std::move(command);
                if (!optionalNullable(value, "path", result.path, Surface, diagnostic, decodeStringValue)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::CommandAction>(std::move(result));
            }
            if (target == ConversationUnionTarget::CommandActionSearch) {
                typed::SearchCommandAction result;
                result.command = std::move(command);
                if (!optionalNullable(value, "path", result.path, Surface, diagnostic, decodeStringValue) ||
                    !optionalNullable(value, "query", result.query, Surface, diagnostic, decodeStringValue)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::CommandAction>(std::move(result));
            }
            if (target == ConversationUnionTarget::CommandActionUnknown) {
                typed::UnknownCommandAction result;
                result.command = std::move(command);
                result.raw = value;
                return decoded<typed::CommandAction>(std::move(result));
            }
            return malformed<typed::CommandAction>(Surface, "$.type");
        } catch (const std::exception&) {
            return malformed<typed::CommandAction>(Surface, "$");
        } catch (...) {
            return malformed<typed::CommandAction>(Surface, "$");
        }
    }

    ConversationDecodeResult<typed::ContentItem> decodeContentItem(const Json& value) noexcept {
        constexpr std::string_view Surface = "ContentItem";
        try {
            ResolvedDiscriminator resolved = resolveInternal(value, Surface);
            if (resolved.malformed()) {
                return {std::nullopt, std::move(resolved.diagnostic)};
            }
            if (resolved.unknown()) {
                return unknown<typed::ContentItem, typed::UnknownContentItem>(value, resolved.discriminator, Surface, "$.type");
            }

            std::optional<typed::DecodeDiagnostic> diagnostic;
            const ConversationUnionTarget target = resolved.descriptor->target;
            if (target == ConversationUnionTarget::ContentItemInputText) {
                typed::InputTextContentItem result;
                if (!requiredString(value, "text", result.text, Surface, diagnostic)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::ContentItem>(std::move(result));
            }
            if (target == ConversationUnionTarget::ContentItemInputImage) {
                typed::InputImageContentItem result;
                if (!requiredString(value, "image_url", result.imageUrl, Surface, diagnostic) ||
                    !optionalNullable(value, "detail", result.detail, Surface, diagnostic, decodeImageDetail)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                diagnostic = imageDetailDiagnostic(result.detail, Surface);
                if (diagnostic) {
                    result.diagnostics.push_back(*diagnostic);
                }
                result.raw = value;
                return decoded<typed::ContentItem>(std::move(result), std::move(diagnostic));
            }
            if (target == ConversationUnionTarget::ContentItemOutputText) {
                typed::OutputTextContentItem result;
                if (!requiredString(value, "text", result.text, Surface, diagnostic)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::ContentItem>(std::move(result));
            }
            return malformed<typed::ContentItem>(Surface, "$.type");
        } catch (const std::exception&) {
            return malformed<typed::ContentItem>(Surface, "$");
        } catch (...) {
            return malformed<typed::ContentItem>(Surface, "$");
        }
    }

    ConversationDecodeResult<typed::DynamicToolCallOutputContentItem> decodeDynamicToolCallOutputContentItem(const Json& value) noexcept {
        constexpr std::string_view Surface = "DynamicToolCallOutputContentItem";
        try {
            ResolvedDiscriminator resolved = resolveInternal(value, Surface);
            if (resolved.malformed()) {
                return {std::nullopt, std::move(resolved.diagnostic)};
            }
            if (resolved.unknown()) {
                return unknown<typed::DynamicToolCallOutputContentItem, typed::UnknownDynamicToolCallOutputContentItem>(
                    value, resolved.discriminator, Surface, "$.type");
            }
            std::optional<typed::DecodeDiagnostic> diagnostic;
            const ConversationUnionTarget target = resolved.descriptor->target;
            if (target == ConversationUnionTarget::DynamicToolCallOutputContentItemInputText) {
                typed::InputTextDynamicToolCallOutputContentItem result;
                if (!requiredString(value, "text", result.text, Surface, diagnostic)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::DynamicToolCallOutputContentItem>(std::move(result));
            }
            if (target == ConversationUnionTarget::DynamicToolCallOutputContentItemInputImage) {
                typed::InputImageDynamicToolCallOutputContentItem result;
                if (!requiredString(value, "imageUrl", result.imageUrl, Surface, diagnostic)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::DynamicToolCallOutputContentItem>(std::move(result));
            }
            return malformed<typed::DynamicToolCallOutputContentItem>(Surface, "$.type");
        } catch (const std::exception&) {
            return malformed<typed::DynamicToolCallOutputContentItem>(Surface, "$");
        } catch (...) {
            return malformed<typed::DynamicToolCallOutputContentItem>(Surface, "$");
        }
    }

    ConversationDecodeResult<typed::FunctionCallOutputContentItem> decodeFunctionCallOutputContentItem(const Json& value) noexcept {
        constexpr std::string_view Surface = "FunctionCallOutputContentItem";
        try {
            ResolvedDiscriminator resolved = resolveInternal(value, Surface);
            if (resolved.malformed()) {
                return {std::nullopt, std::move(resolved.diagnostic)};
            }
            if (resolved.unknown()) {
                return unknown<typed::FunctionCallOutputContentItem, typed::UnknownFunctionCallOutputContentItem>(
                    value, resolved.discriminator, Surface, "$.type");
            }

            std::optional<typed::DecodeDiagnostic> diagnostic;
            const ConversationUnionTarget target = resolved.descriptor->target;
            if (target == ConversationUnionTarget::FunctionCallOutputContentItemInputText) {
                typed::InputTextFunctionCallOutputContentItem result;
                if (!requiredString(value, "text", result.text, Surface, diagnostic)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::FunctionCallOutputContentItem>(std::move(result));
            }
            if (target == ConversationUnionTarget::FunctionCallOutputContentItemInputImage) {
                typed::InputImageFunctionCallOutputContentItem result;
                if (!requiredString(value, "image_url", result.imageUrl, Surface, diagnostic) ||
                    !optionalNullable(value, "detail", result.detail, Surface, diagnostic, decodeImageDetail)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                diagnostic = imageDetailDiagnostic(result.detail, Surface);
                if (diagnostic) {
                    result.diagnostics.push_back(*diagnostic);
                }
                result.raw = value;
                return decoded<typed::FunctionCallOutputContentItem>(std::move(result), std::move(diagnostic));
            }
            if (target == ConversationUnionTarget::FunctionCallOutputContentItemEncryptedContent) {
                typed::EncryptedContentFunctionCallOutputContentItem result;
                if (!requiredString(value, "encrypted_content", result.encryptedContent, Surface, diagnostic)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::FunctionCallOutputContentItem>(std::move(result));
            }
            return malformed<typed::FunctionCallOutputContentItem>(Surface, "$.type");
        } catch (const std::exception&) {
            return malformed<typed::FunctionCallOutputContentItem>(Surface, "$");
        } catch (...) {
            return malformed<typed::FunctionCallOutputContentItem>(Surface, "$");
        }
    }

    ConversationDecodeResult<typed::LocalShellAction> decodeLocalShellAction(const Json& value) noexcept {
        constexpr std::string_view Surface = "LocalShellAction";
        try {
            ResolvedDiscriminator resolved = resolveInternal(value, Surface);
            if (resolved.malformed()) {
                return {std::nullopt, std::move(resolved.diagnostic)};
            }
            if (resolved.unknown()) {
                return unknown<typed::LocalShellAction, typed::UnknownLocalShellAction>(value, resolved.discriminator, Surface, "$.type");
            }
            if (resolved.descriptor->target != ConversationUnionTarget::LocalShellActionExec) {
                return malformed<typed::LocalShellAction>(Surface, "$.type");
            }

            typed::ExecLocalShellAction result;
            std::optional<typed::DecodeDiagnostic> diagnostic;
            if (!requiredStringArray(value, "command", result.command, Surface, diagnostic) ||
                !optionalNullable(value, "env", result.env, Surface, diagnostic, decodeStringMap) ||
                !optionalNullable(value, "timeout_ms", result.timeoutMs, Surface, diagnostic, decodeUint64) ||
                !optionalNullable(value, "user", result.user, Surface, diagnostic, decodeStringValue) ||
                !optionalNullable(value, "working_directory", result.workingDirectory, Surface, diagnostic, decodeStringValue)) {
                return {std::nullopt, std::move(diagnostic)};
            }
            result.raw = value;
            return decoded<typed::LocalShellAction>(std::move(result));
        } catch (const std::exception&) {
            return malformed<typed::LocalShellAction>(Surface, "$");
        } catch (...) {
            return malformed<typed::LocalShellAction>(Surface, "$");
        }
    }

    ConversationDecodeResult<typed::PatchChangeKind> decodePatchChangeKind(const Json& value) noexcept {
        constexpr std::string_view Surface = "PatchChangeKind";
        try {
            ResolvedDiscriminator resolved = resolveInternal(value, Surface);
            if (resolved.malformed()) {
                return {std::nullopt, std::move(resolved.diagnostic)};
            }
            if (resolved.unknown()) {
                return unknown<typed::PatchChangeKind, typed::UnknownPatchChangeKind>(value, resolved.discriminator, Surface, "$.type");
            }
            const ConversationUnionTarget target = resolved.descriptor->target;
            if (target == ConversationUnionTarget::PatchChangeKindAdd) {
                return decoded<typed::PatchChangeKind>(typed::AddPatchChangeKind{value, {}});
            }
            if (target == ConversationUnionTarget::PatchChangeKindDelete) {
                return decoded<typed::PatchChangeKind>(typed::DeletePatchChangeKind{value, {}});
            }
            if (target == ConversationUnionTarget::PatchChangeKindUpdate) {
                typed::UpdatePatchChangeKind result;
                std::optional<typed::DecodeDiagnostic> diagnostic;
                if (!optionalNullable(value, "move_path", result.movePath, Surface, diagnostic, decodeStringValue)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::PatchChangeKind>(std::move(result));
            }
            return malformed<typed::PatchChangeKind>(Surface, "$.type");
        } catch (const std::exception&) {
            return malformed<typed::PatchChangeKind>(Surface, "$");
        } catch (...) {
            return malformed<typed::PatchChangeKind>(Surface, "$");
        }
    }

    ConversationDecodeResult<typed::ReasoningItemContent> decodeReasoningItemContent(const Json& value) noexcept {
        constexpr std::string_view Surface = "ReasoningItemContent";
        try {
            ResolvedDiscriminator resolved = resolveInternal(value, Surface);
            if (resolved.malformed()) {
                return {std::nullopt, std::move(resolved.diagnostic)};
            }
            if (resolved.unknown()) {
                return unknown<typed::ReasoningItemContent, typed::UnknownReasoningItemContent>(
                    value, resolved.discriminator, Surface, "$.type");
            }

            std::optional<typed::DecodeDiagnostic> diagnostic;
            const ConversationUnionTarget target = resolved.descriptor->target;
            if (target == ConversationUnionTarget::ReasoningItemContentReasoningText) {
                typed::ReasoningTextReasoningItemContent result;
                if (!requiredString(value, "text", result.text, Surface, diagnostic)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::ReasoningItemContent>(std::move(result));
            }
            if (target == ConversationUnionTarget::ReasoningItemContentText) {
                typed::TextReasoningItemContent result;
                if (!requiredString(value, "text", result.text, Surface, diagnostic)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::ReasoningItemContent>(std::move(result));
            }
            return malformed<typed::ReasoningItemContent>(Surface, "$.type");
        } catch (const std::exception&) {
            return malformed<typed::ReasoningItemContent>(Surface, "$");
        } catch (...) {
            return malformed<typed::ReasoningItemContent>(Surface, "$");
        }
    }

    ConversationDecodeResult<typed::ReasoningItemReasoningSummary> decodeReasoningItemReasoningSummary(const Json& value) noexcept {
        constexpr std::string_view Surface = "ReasoningItemReasoningSummary";
        try {
            ResolvedDiscriminator resolved = resolveInternal(value, Surface);
            if (resolved.malformed()) {
                return {std::nullopt, std::move(resolved.diagnostic)};
            }
            if (resolved.unknown()) {
                return unknown<typed::ReasoningItemReasoningSummary, typed::UnknownReasoningItemReasoningSummary>(
                    value, resolved.discriminator, Surface, "$.type");
            }
            if (resolved.descriptor->target != ConversationUnionTarget::ReasoningItemReasoningSummarySummaryText) {
                return malformed<typed::ReasoningItemReasoningSummary>(Surface, "$.type");
            }

            typed::SummaryTextReasoningItemReasoningSummary result;
            std::optional<typed::DecodeDiagnostic> diagnostic;
            if (!requiredString(value, "text", result.text, Surface, diagnostic)) {
                return {std::nullopt, std::move(diagnostic)};
            }
            result.raw = value;
            return decoded<typed::ReasoningItemReasoningSummary>(std::move(result));
        } catch (const std::exception&) {
            return malformed<typed::ReasoningItemReasoningSummary>(Surface, "$");
        } catch (...) {
            return malformed<typed::ReasoningItemReasoningSummary>(Surface, "$");
        }
    }

    ConversationDecodeResult<typed::ResponsesApiWebSearchAction> decodeResponsesApiWebSearchAction(const Json& value) noexcept {
        constexpr std::string_view Surface = "ResponsesApiWebSearchAction";
        try {
            ResolvedDiscriminator resolved = resolveInternal(value, Surface);
            if (resolved.malformed()) {
                return {std::nullopt, std::move(resolved.diagnostic)};
            }
            if (resolved.unknown()) {
                return unknown<typed::ResponsesApiWebSearchAction, typed::UnknownResponsesApiWebSearchAction>(
                    value, resolved.discriminator, Surface, "$.type");
            }

            std::optional<typed::DecodeDiagnostic> diagnostic;
            const ConversationUnionTarget target = resolved.descriptor->target;
            if (target == ConversationUnionTarget::ResponsesApiWebSearchActionSearch) {
                typed::SearchResponsesApiWebSearchAction result;
                if (!optionalNullable(value, "queries", result.queries, Surface, diagnostic, decodeStringArray) ||
                    !optionalNullable(value, "query", result.query, Surface, diagnostic, decodeStringValue)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::ResponsesApiWebSearchAction>(std::move(result));
            }
            if (target == ConversationUnionTarget::ResponsesApiWebSearchActionOpenPage) {
                typed::OpenPageResponsesApiWebSearchAction result;
                if (!optionalNullable(value, "url", result.url, Surface, diagnostic, decodeStringValue)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::ResponsesApiWebSearchAction>(std::move(result));
            }
            if (target == ConversationUnionTarget::ResponsesApiWebSearchActionFindInPage) {
                typed::FindInPageResponsesApiWebSearchAction result;
                if (!optionalNullable(value, "pattern", result.pattern, Surface, diagnostic, decodeStringValue) ||
                    !optionalNullable(value, "url", result.url, Surface, diagnostic, decodeStringValue)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::ResponsesApiWebSearchAction>(std::move(result));
            }
            if (target == ConversationUnionTarget::ResponsesApiWebSearchActionOther) {
                return decoded<typed::ResponsesApiWebSearchAction>(typed::OtherResponsesApiWebSearchAction{value, {}});
            }
            return malformed<typed::ResponsesApiWebSearchAction>(Surface, "$.type");
        } catch (const std::exception&) {
            return malformed<typed::ResponsesApiWebSearchAction>(Surface, "$");
        } catch (...) {
            return malformed<typed::ResponsesApiWebSearchAction>(Surface, "$");
        }
    }

    ConversationDecodeResult<typed::SandboxPolicy> decodeSandboxPolicy(const Json& value) noexcept {
        constexpr std::string_view Surface = "SandboxPolicy";
        try {
            ResolvedDiscriminator resolved = resolveInternal(value, Surface);
            if (resolved.malformed()) {
                return {std::nullopt, std::move(resolved.diagnostic)};
            }
            if (resolved.unknown()) {
                return unknown<typed::SandboxPolicy, typed::UnknownSandboxPolicy>(value, resolved.discriminator, Surface, "$.type");
            }
            std::optional<typed::DecodeDiagnostic> diagnostic;
            const ConversationUnionTarget target = resolved.descriptor->target;
            if (target == ConversationUnionTarget::SandboxPolicyDangerFullAccess) {
                return decoded<typed::SandboxPolicy>(typed::DangerFullAccessSandboxPolicy{value, {}});
            }
            if (target == ConversationUnionTarget::SandboxPolicyReadOnly) {
                typed::ReadOnlySandboxPolicy result;
                if (!optionalBool(value, "networkAccess", result.networkAccess, Surface, diagnostic)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::SandboxPolicy>(std::move(result));
            }
            if (target == ConversationUnionTarget::SandboxPolicyExternalSandbox) {
                typed::ExternalSandboxSandboxPolicy result;
                const Json* networkAccess = member(value, "networkAccess");
                if (networkAccess != nullptr) {
                    if (!networkAccess->is_string()) {
                        return malformed<typed::SandboxPolicy>(Surface, "$.networkAccess");
                    }
                    result.networkAccess = typed::NetworkAccess{networkAccess->get<std::string>()};
                    if (!result.networkAccess->isKnown()) {
                        diagnostic = unknownEnumDiagnostic(std::string(Surface), "$.networkAccess");
                        result.diagnostics.push_back(*diagnostic);
                    }
                }
                result.raw = value;
                return decoded<typed::SandboxPolicy>(std::move(result), std::move(diagnostic));
            }
            if (target == ConversationUnionTarget::SandboxPolicyWorkspaceWrite) {
                typed::WorkspaceWriteSandboxPolicy result;
                const Json* writableRoots = member(value, "writableRoots");
                if (writableRoots != nullptr) {
                    if (!writableRoots->is_array()) {
                        return malformed<typed::SandboxPolicy>(Surface, "$.writableRoots");
                    }
                    std::vector<typed::AbsolutePathBuf> roots;
                    roots.reserve(writableRoots->size());
                    for (std::size_t index = 0; index < writableRoots->size(); ++index) {
                        const Json& root = (*writableRoots)[index];
                        if (!root.is_string()) {
                            return malformed<typed::SandboxPolicy>(Surface, "$.writableRoots[" + std::to_string(index) + "]");
                        }
                        roots.push_back(typed::AbsolutePathBuf{root.get<std::string>()});
                    }
                    result.writableRoots = std::move(roots);
                }
                if (!optionalBool(value, "networkAccess", result.networkAccess, Surface, diagnostic) ||
                    !optionalBool(value, "excludeTmpdirEnvVar", result.excludeTmpdirEnvVar, Surface, diagnostic) ||
                    !optionalBool(value, "excludeSlashTmp", result.excludeSlashTmp, Surface, diagnostic)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::SandboxPolicy>(std::move(result));
            }
            return malformed<typed::SandboxPolicy>(Surface, "$.type");
        } catch (const std::exception&) {
            return malformed<typed::SandboxPolicy>(Surface, "$");
        } catch (...) {
            return malformed<typed::SandboxPolicy>(Surface, "$");
        }
    }

    ConversationDecodeResult<typed::UserInput> decodeUserInput(const Json& value) noexcept {
        constexpr std::string_view Surface = "UserInput";
        try {
            ResolvedDiscriminator resolved = resolveInternal(value, Surface);
            if (resolved.malformed()) {
                return {std::nullopt, std::move(resolved.diagnostic)};
            }
            if (resolved.unknown()) {
                return unknown<typed::UserInput, typed::UnknownUserInput>(value, resolved.discriminator, Surface, "$.type");
            }
            std::optional<typed::DecodeDiagnostic> diagnostic;
            const ConversationUnionTarget target = resolved.descriptor->target;
            if (target == ConversationUnionTarget::UserInputText) {
                typed::TextUserInput result;
                if (!requiredString(value, "text", result.text, Surface, diagnostic) ||
                    !decodeOptionalTextElements(value, result.textElements, diagnostic)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::UserInput>(std::move(result));
            }
            if (target == ConversationUnionTarget::UserInputImage) {
                typed::ImageUserInput result;
                if (!requiredString(value, "url", result.url, Surface, diagnostic) ||
                    !optionalNullable(
                        value, "detail", result.detail, Surface, diagnostic, [](const Json& detail, typed::ImageDetail& decodedDetail) {
                            if (!detail.is_string()) {
                                return false;
                            }
                            decodedDetail.value = detail.get<std::string>();
                            return true;
                        })) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                diagnostic = imageDetailDiagnostic(result.detail, Surface);
                if (diagnostic) {
                    result.diagnostics.push_back(*diagnostic);
                }
                result.raw = value;
                return decoded<typed::UserInput>(std::move(result), std::move(diagnostic));
            }
            if (target == ConversationUnionTarget::UserInputLocalImage) {
                typed::LocalImageUserInput result;
                if (!requiredString(value, "path", result.path, Surface, diagnostic) ||
                    !optionalNullable(
                        value, "detail", result.detail, Surface, diagnostic, [](const Json& detail, typed::ImageDetail& decodedDetail) {
                            if (!detail.is_string()) {
                                return false;
                            }
                            decodedDetail.value = detail.get<std::string>();
                            return true;
                        })) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                diagnostic = imageDetailDiagnostic(result.detail, Surface);
                if (diagnostic) {
                    result.diagnostics.push_back(*diagnostic);
                }
                result.raw = value;
                return decoded<typed::UserInput>(std::move(result), std::move(diagnostic));
            }
            if (target == ConversationUnionTarget::UserInputSkill) {
                typed::SkillUserInput result;
                if (!requiredString(value, "name", result.name, Surface, diagnostic) ||
                    !requiredString(value, "path", result.path, Surface, diagnostic)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::UserInput>(std::move(result));
            }
            if (target == ConversationUnionTarget::UserInputMention) {
                typed::MentionUserInput result;
                if (!requiredString(value, "name", result.name, Surface, diagnostic) ||
                    !requiredString(value, "path", result.path, Surface, diagnostic)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::UserInput>(std::move(result));
            }
            return malformed<typed::UserInput>(Surface, "$.type");
        } catch (const std::exception&) {
            return malformed<typed::UserInput>(Surface, "$");
        } catch (...) {
            return malformed<typed::UserInput>(Surface, "$");
        }
    }

    ConversationDecodeResult<typed::WebSearchAction> decodeWebSearchAction(const Json& value) noexcept {
        constexpr std::string_view Surface = "WebSearchAction";
        try {
            ResolvedDiscriminator resolved = resolveInternal(value, Surface);
            if (resolved.malformed()) {
                return {std::nullopt, std::move(resolved.diagnostic)};
            }
            if (resolved.unknown()) {
                return unknown<typed::WebSearchAction, typed::UnknownWebSearchAction>(value, resolved.discriminator, Surface, "$.type");
            }
            std::optional<typed::DecodeDiagnostic> diagnostic;
            const ConversationUnionTarget target = resolved.descriptor->target;
            if (target == ConversationUnionTarget::WebSearchActionSearch) {
                typed::SearchWebSearchAction result;
                if (!optionalNullable(value, "queries", result.queries, Surface, diagnostic, decodeStringArray) ||
                    !optionalNullable(value, "query", result.query, Surface, diagnostic, decodeStringValue)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::WebSearchAction>(std::move(result));
            }
            if (target == ConversationUnionTarget::WebSearchActionOpenPage) {
                typed::OpenPageWebSearchAction result;
                if (!optionalNullable(value, "url", result.url, Surface, diagnostic, decodeStringValue)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::WebSearchAction>(std::move(result));
            }
            if (target == ConversationUnionTarget::WebSearchActionFindInPage) {
                typed::FindInPageWebSearchAction result;
                if (!optionalNullable(value, "pattern", result.pattern, Surface, diagnostic, decodeStringValue) ||
                    !optionalNullable(value, "url", result.url, Surface, diagnostic, decodeStringValue)) {
                    return {std::nullopt, std::move(diagnostic)};
                }
                result.raw = value;
                return decoded<typed::WebSearchAction>(std::move(result));
            }
            if (target == ConversationUnionTarget::WebSearchActionOther) {
                return decoded<typed::WebSearchAction>(typed::OtherWebSearchAction{value, {}});
            }
            return malformed<typed::WebSearchAction>(Surface, "$.type");
        } catch (const std::exception&) {
            return malformed<typed::WebSearchAction>(Surface, "$");
        } catch (...) {
            return malformed<typed::WebSearchAction>(Surface, "$");
        }
    }

    std::optional<Json> encodeAskForApproval(const typed::AskForApproval& value, std::string& error) noexcept {
        try {
            error.clear();
            return std::visit(
                [&](const auto& alternative) -> std::optional<Json> {
                    using Alternative = std::decay_t<decltype(alternative)>;
                    if constexpr (std::is_same_v<Alternative, typed::ApprovalPolicy>) {
                        const ProtocolSurfaceEntry* entry =
                            findSurface(SurfaceCategory::TaggedUnionDiscriminator, "AskForApproval", VariantField, alternative.value);
                        if (entry == nullptr) {
                            error = "unknown AskForApproval scalar cannot be encoded; use the raw protocol API";
                            return std::nullopt;
                        }
                        const ConversationUnionTarget* target = std::get_if<ConversationUnionTarget>(&entry->runtimeTarget);
                        const ConversationUnionCodecDescriptor* descriptor =
                            target == nullptr ? nullptr : descriptorForEncoding(*target, error);
                        if (descriptor == nullptr || descriptor->shape != ConversationUnionCodecShape::ScalarString ||
                            descriptor->key.domain != "AskForApproval") {
                            if (error.empty()) {
                                error = "AskForApproval scalar registry target has the wrong encoding shape";
                            }
                            return std::nullopt;
                        }
                        return std::optional<Json>{Json(std::string(descriptor->key.name))};
                    } else if constexpr (std::is_same_v<Alternative, typed::GranularAskForApproval>) {
                        const ConversationUnionCodecDescriptor* descriptor =
                            descriptorForEncoding(ConversationUnionTarget::AskForApprovalGranular, error);
                        if (descriptor == nullptr || descriptor->shape != ConversationUnionCodecShape::ExternallyTaggedObject) {
                            if (error.empty()) {
                                error = "granular AskForApproval registry target has the wrong encoding shape";
                            }
                            return std::nullopt;
                        }
                        Json granular = Json::object({{"mcp_elicitations", alternative.granular.mcpElicitations},
                                                      {"rules", alternative.granular.rules},
                                                      {"sandbox_approval", alternative.granular.sandboxApproval}});
                        if (alternative.granular.requestPermissions) {
                            granular.emplace("request_permissions", *alternative.granular.requestPermissions);
                        }
                        if (alternative.granular.skillApproval) {
                            granular.emplace("skill_approval", *alternative.granular.skillApproval);
                        }
                        return std::optional<Json>{Json::object({{std::string(descriptor->key.name), std::move(granular)}})};
                    } else {
                        error = "unknown AskForApproval alternative cannot be encoded; use the raw protocol API";
                        return std::nullopt;
                    }
                },
                value);
        } catch (const std::exception&) {
            error = "failed to encode AskForApproval";
        } catch (...) {
            error = "failed to encode AskForApproval";
        }
        return std::nullopt;
    }

    std::optional<Json> encodeSandboxPolicy(const typed::SandboxPolicy& value, std::string& error) noexcept {
        try {
            error.clear();
            return std::visit(
                [&](const auto& alternative) -> std::optional<Json> {
                    using Alternative = std::decay_t<decltype(alternative)>;
                    ConversationUnionTarget target = ConversationUnionTarget::Count;
                    if constexpr (std::is_same_v<Alternative, typed::DangerFullAccessSandboxPolicy>) {
                        target = ConversationUnionTarget::SandboxPolicyDangerFullAccess;
                    } else if constexpr (std::is_same_v<Alternative, typed::ReadOnlySandboxPolicy>) {
                        target = ConversationUnionTarget::SandboxPolicyReadOnly;
                    } else if constexpr (std::is_same_v<Alternative, typed::ExternalSandboxSandboxPolicy>) {
                        target = ConversationUnionTarget::SandboxPolicyExternalSandbox;
                    } else if constexpr (std::is_same_v<Alternative, typed::WorkspaceWriteSandboxPolicy>) {
                        target = ConversationUnionTarget::SandboxPolicyWorkspaceWrite;
                    } else {
                        error = "unknown SandboxPolicy alternative cannot be encoded; use the raw protocol API";
                        return std::nullopt;
                    }

                    const ConversationUnionCodecDescriptor* descriptor = nullptr;
                    std::optional<Json> encoded = beginInternalEncoding(target, descriptor, error);
                    if (!encoded) {
                        return std::nullopt;
                    }
                    if constexpr (std::is_same_v<Alternative, typed::ReadOnlySandboxPolicy>) {
                        if (alternative.networkAccess) {
                            encoded->emplace("networkAccess", *alternative.networkAccess);
                        }
                    } else if constexpr (std::is_same_v<Alternative, typed::ExternalSandboxSandboxPolicy>) {
                        if (alternative.networkAccess) {
                            encoded->emplace("networkAccess", alternative.networkAccess->value);
                        }
                    } else if constexpr (std::is_same_v<Alternative, typed::WorkspaceWriteSandboxPolicy>) {
                        if (alternative.writableRoots) {
                            Json roots = Json::array();
                            for (const typed::AbsolutePathBuf& root : *alternative.writableRoots) {
                                roots.emplace_back(root.value);
                            }
                            encoded->emplace("writableRoots", std::move(roots));
                        }
                        if (alternative.networkAccess) {
                            encoded->emplace("networkAccess", *alternative.networkAccess);
                        }
                        if (alternative.excludeTmpdirEnvVar) {
                            encoded->emplace("excludeTmpdirEnvVar", *alternative.excludeTmpdirEnvVar);
                        }
                        if (alternative.excludeSlashTmp) {
                            encoded->emplace("excludeSlashTmp", *alternative.excludeSlashTmp);
                        }
                    }
                    return std::optional<Json>{std::move(*encoded)};
                },
                value);
        } catch (const std::exception&) {
            error = "failed to encode SandboxPolicy";
        } catch (...) {
            error = "failed to encode SandboxPolicy";
        }
        return std::nullopt;
    }

    std::optional<Json> encodeUserInput(const typed::UserInput& value, std::string& error) noexcept {
        try {
            error.clear();
            return std::visit(
                [&](const auto& alternative) -> std::optional<Json> {
                    using Alternative = std::decay_t<decltype(alternative)>;
                    ConversationUnionTarget target = ConversationUnionTarget::Count;
                    if constexpr (std::is_same_v<Alternative, typed::TextUserInput>) {
                        target = ConversationUnionTarget::UserInputText;
                    } else if constexpr (std::is_same_v<Alternative, typed::ImageUserInput>) {
                        target = ConversationUnionTarget::UserInputImage;
                    } else if constexpr (std::is_same_v<Alternative, typed::LocalImageUserInput>) {
                        target = ConversationUnionTarget::UserInputLocalImage;
                    } else if constexpr (std::is_same_v<Alternative, typed::SkillUserInput>) {
                        target = ConversationUnionTarget::UserInputSkill;
                    } else if constexpr (std::is_same_v<Alternative, typed::MentionUserInput>) {
                        target = ConversationUnionTarget::UserInputMention;
                    } else {
                        error = "unknown UserInput alternative cannot be encoded; use the raw protocol API";
                        return std::nullopt;
                    }

                    const ConversationUnionCodecDescriptor* descriptor = nullptr;
                    std::optional<Json> encoded = beginInternalEncoding(target, descriptor, error);
                    if (!encoded) {
                        return std::nullopt;
                    }
                    if constexpr (std::is_same_v<Alternative, typed::TextUserInput>) {
                        encoded->emplace("text", alternative.text);
                        if (alternative.textElements) {
                            Json elements = Json::array();
                            for (const typed::TextElement& element : *alternative.textElements) {
                                std::optional<Json> encodedElement = encodeTextElement(element, error);
                                if (!encodedElement) {
                                    return std::nullopt;
                                }
                                elements.emplace_back(std::move(*encodedElement));
                            }
                            encoded->emplace("text_elements", std::move(elements));
                        }
                    } else if constexpr (std::is_same_v<Alternative, typed::ImageUserInput>) {
                        encoded->emplace("url", alternative.url);
                        if (!encodeOptionalNullable(
                                *encoded, "detail", alternative.detail, error, [](const typed::ImageDetail& detail) -> std::optional<Json> {
                                    return std::optional<Json>{Json(detail.value)};
                                })) {
                            return std::nullopt;
                        }
                    } else if constexpr (std::is_same_v<Alternative, typed::LocalImageUserInput>) {
                        encoded->emplace("path", alternative.path);
                        if (!encodeOptionalNullable(
                                *encoded, "detail", alternative.detail, error, [](const typed::ImageDetail& detail) -> std::optional<Json> {
                                    return std::optional<Json>{Json(detail.value)};
                                })) {
                            return std::nullopt;
                        }
                    } else if constexpr (std::is_same_v<Alternative, typed::SkillUserInput> ||
                                         std::is_same_v<Alternative, typed::MentionUserInput>) {
                        encoded->emplace("name", alternative.name);
                        encoded->emplace("path", alternative.path);
                    }
                    return std::optional<Json>{std::move(*encoded)};
                },
                value);
        } catch (const std::exception&) {
            error = "failed to encode UserInput";
        } catch (...) {
            error = "failed to encode UserInput";
        }
        return std::nullopt;
    }

} // namespace ai::openai::codex::detail
