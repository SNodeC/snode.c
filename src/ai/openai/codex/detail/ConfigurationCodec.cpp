/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ConfigurationCodec.h"

#include "ai/openai/codex/detail/DecodeDiagnostic.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "ai/openai/codex/typed/Conversation.h"
#include "ai/openai/codex/typed/Models.h"
#include "ai/openai/codex/typed/Types.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <map>
#include <nlohmann/detail/iterators/iteration_proxy.hpp>
#include <nlohmann/detail/json_ref.hpp>
#include <nlohmann/json.hpp>
#include <span>
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

        std::string fieldPath(std::string_view base, std::string_view name) {
            std::string result(base.empty() ? "$" : base);
            if (!name.empty()) {
                result.push_back('.');
                result.append(name);
            }
            return result;
        }

        std::string indexedPath(std::string_view base, std::size_t index) {
            return std::string(base) + "[" + std::to_string(index) + "]";
        }

        std::string rebasedPath(std::string_view base, std::string_view nested) {
            if (nested.empty() || nested == "$") {
                return std::string(base);
            }
            if (nested.front() == '$') {
                return std::string(base) + std::string(nested.substr(1));
            }
            return std::string(base);
        }

        bool fail(std::string& error, std::string_view context, std::string_view path, std::string_view requirement) {
            error = std::string(context) + " field '" + std::string(path) + "' " + std::string(requirement);
            return false;
        }

        bool requireObject(const Json& value, std::string_view context, std::string& error, std::string_view path = "$") {
            return value.is_object() || fail(error, context, path, "must be an object");
        }

        bool decodeStringAt(const Json& value, std::string& result, std::string& error, std::string_view context, std::string_view path) {
            if (!value.is_string()) {
                return fail(error, context, path, "must be a string");
            }
            result = value.get_ref<const std::string&>();
            return true;
        }

        bool decodeBoolAt(const Json& value, bool& result, std::string& error, std::string_view context, std::string_view path) {
            if (!value.is_boolean()) {
                return fail(error, context, path, "must be a boolean");
            }
            result = value.get_ref<const Json::boolean_t&>();
            return true;
        }

        bool decodeInt64At(const Json& value, std::int64_t& result, std::string& error, std::string_view context, std::string_view path) {
            if (value.is_number_unsigned()) {
                const std::uint64_t number = value.get_ref<const Json::number_unsigned_t&>();
                if (number > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                    return fail(error, context, path, "is outside the int64 range");
                }
                result = static_cast<std::int64_t>(number);
                return true;
            }
            if (!value.is_number_integer()) {
                return fail(error, context, path, "must be an int64 integer");
            }
            result = value.get_ref<const Json::number_integer_t&>();
            return true;
        }

        bool decodeUint64At(const Json& value, std::uint64_t& result, std::string& error, std::string_view context, std::string_view path) {
            if (value.is_number_unsigned()) {
                result = value.get_ref<const Json::number_unsigned_t&>();
                return true;
            }
            if (!value.is_number_integer()) {
                return fail(error, context, path, "must be a non-negative integer");
            }
            const std::int64_t number = value.get_ref<const Json::number_integer_t&>();
            if (number < 0) {
                return fail(error, context, path, "must be a non-negative integer");
            }
            result = static_cast<std::uint64_t>(number);
            return true;
        }

        template <typename Strong>
        bool decodeStrongStringAt(const Json& value, Strong& result, std::string& error, std::string_view context, std::string_view path) {
            return decodeStringAt(value, result.value, error, context, path);
        }

        void appendDiagnostics(std::vector<typed::DecodeDiagnostic>& target, const std::vector<typed::DecodeDiagnostic>& source) {
            target.insert(target.end(), source.begin(), source.end());
        }

        template <typename OpenEnum>
        bool decodeOpenEnumAt(const Json& value,
                              OpenEnum& result,
                              std::vector<typed::DecodeDiagnostic>& diagnostics,
                              std::string_view surface,
                              std::string_view path,
                              std::string& error,
                              std::string_view context,
                              bool requireNonEmpty = false) {
            if (!decodeStringAt(value, result.value, error, context, path)) {
                return false;
            }
            if (requireNonEmpty && result.value.empty()) {
                return fail(error, context, path, "must be a non-empty string");
            }
            if (!result.isKnown()) {
                diagnostics.emplace_back(unknownEnumDiagnostic(std::string(surface), std::string(path)));
            }
            return true;
        }

        template <typename T, typename Decode>
        bool decodeRequired(const Json& object,
                            std::string_view name,
                            T& result,
                            Decode&& decode,
                            std::string& error,
                            std::string_view context,
                            std::string_view path = "$") {
            const std::string nestedPath = fieldPath(path, name);
            const Json* value = member(object, name);
            if (value == nullptr) {
                return fail(error, context, nestedPath, "is required");
            }
            return decode(*value, result, nestedPath);
        }

        template <typename T, typename Decode>
        bool decodeOptionalNullable(const Json& object,
                                    std::string_view name,
                                    typed::OptionalNullable<T>& result,
                                    Decode&& decode,
                                    std::string& error,
                                    std::string_view context,
                                    std::string_view path = "$") {
            result = typed::OptionalNullable<T>::omitted();
            const Json* value = member(object, name);
            if (value == nullptr) {
                return true;
            }
            if (value->is_null()) {
                result = typed::OptionalNullable<T>::explicitNull();
                return true;
            }
            T decoded;
            const std::string nestedPath = fieldPath(path, name);
            if (!decode(*value, decoded, nestedPath)) {
                if (error.empty()) {
                    return fail(error, context, nestedPath, "has the wrong type");
                }
                return false;
            }
            result = typed::OptionalNullable<T>::withValue(std::move(decoded));
            return true;
        }

        template <typename T, typename Decode>
        bool decodeArrayAt(const Json& value,
                           std::vector<T>& result,
                           Decode&& decode,
                           std::string& error,
                           std::string_view context,
                           std::string_view path) {
            if (!value.is_array()) {
                return fail(error, context, path, "must be an array");
            }
            result.clear();
            result.reserve(value.size());
            for (std::size_t index = 0; index < value.size(); ++index) {
                T decoded;
                const std::string itemPath = indexedPath(path, index);
                if (!decode(value[index], decoded, itemPath)) {
                    if (error.empty()) {
                        return fail(error, context, itemPath, "has the wrong type");
                    }
                    return false;
                }
                result.emplace_back(std::move(decoded));
            }
            return true;
        }

        bool rejectUnknownProperties(const Json& object,
                                     std::initializer_list<std::string_view> known,
                                     std::string& error,
                                     std::string_view context,
                                     std::string_view path) {
            for (const auto& [key, unused] : object.items()) {
                static_cast<void>(unused);
                if (std::find(known.begin(), known.end(), key) == known.end()) {
                    return fail(error, context, path, "contains an unsupported property");
                }
            }
            return true;
        }

        template <std::size_t Size>
        void
        retainUnknownProperties(const Json& object, const std::array<std::string_view, Size>& known, std::map<std::string, Json>& unknown) {
            unknown.clear();
            for (const auto& [key, value] : object.items()) {
                if (std::find(known.begin(), known.end(), key) == known.end()) {
                    unknown.emplace(key, value);
                }
            }
        }

        bool decodeStringVectorAt(
            const Json& value, std::vector<std::string>& result, std::string& error, std::string_view context, std::string_view path) {
            return decodeArrayAt(
                value,
                result,
                [&](const Json& item, std::string& decoded, const std::string& itemPath) {
                    return decodeStringAt(item, decoded, error, context, itemPath);
                },
                error,
                context,
                path);
        }

        bool decodeJsonMapAt(
            const Json& value, std::map<std::string, Json>& result, std::string& error, std::string_view context, std::string_view path) {
            if (!value.is_object()) {
                return fail(error, context, path, "must be an object");
            }
            result.clear();
            for (const auto& [key, nested] : value.items()) {
                result.emplace(key, nested);
            }
            return true;
        }

        template <typename Key>
        bool decodeBoolMapAt(
            const Json& value, std::map<Key, bool>& result, std::string& error, std::string_view context, std::string_view path) {
            if (!value.is_object()) {
                return fail(error, context, path, "must be an object");
            }
            result.clear();
            for (const auto& [key, nested] : value.items()) {
                bool decoded = false;
                if (!decodeBoolAt(nested, decoded, error, context, fieldPath(path, key))) {
                    return false;
                }
                result.emplace(Key{key}, decoded);
            }
            return true;
        }

        template <typename T>
        bool validateOptionalNullable(const typed::OptionalNullable<T>& value,
                                      std::string& error,
                                      std::string_view context,
                                      std::string_view path) {
            if (!value.present && value.value.has_value()) {
                return fail(error, context, path, "has an inconsistent omitted state");
            }
            return true;
        }

        template <typename T, typename Encode>
        void encodeOptionalNullable(Json& object, std::string_view name, const typed::OptionalNullable<T>& value, Encode&& encode) {
            if (!value.present) {
                return;
            }
            object[std::string(name)] = value.value ? Json(encode(*value.value)) : Json(nullptr);
        }

        template <typename Key>
        Json encodeBoolMap(const std::map<Key, bool>& value) {
            Json result = Json::object();
            for (const auto& [key, enabled] : value) {
                result[key.value] = enabled;
            }
            return result;
        }

        void appendAskForApprovalDiagnostics(std::vector<typed::DecodeDiagnostic>& diagnostics, const typed::AskForApproval& value) {
            std::visit(
                [&](const auto& alternative) {
                    using Alternative = std::decay_t<decltype(alternative)>;
                    if constexpr (std::is_same_v<Alternative, typed::GranularAskForApproval>) {
                        appendDiagnostics(diagnostics, alternative.diagnostics);
                    } else if constexpr (std::is_same_v<Alternative, typed::UnknownAskForApproval>) {
                        if (alternative.diagnostic) {
                            diagnostics.push_back(*alternative.diagnostic);
                        }
                    }
                },
                value);
        }

        bool decodeAskForApprovalAt(
            const Json& value, typed::AskForApproval& result, std::string& error, std::string_view context, std::string_view path) {
            ConversationDecodeResult<typed::AskForApproval> decoded = decodeAskForApproval(value);
            if (!decoded.value) {
                const std::string diagnosticPath =
                    decoded.diagnostic ? rebasedPath(path, decoded.diagnostic->fieldPath) : std::string(path);
                return fail(error, context, diagnosticPath, "does not match AskForApproval");
            }
            result = std::move(*decoded.value);
            if (decoded.diagnostic) {
                typed::DecodeDiagnostic diagnostic = *decoded.diagnostic;
                diagnostic.fieldPath = rebasedPath(path, diagnostic.fieldPath);
                if (auto* unknown = std::get_if<typed::UnknownAskForApproval>(&result)) {
                    unknown->diagnostic = diagnostic;
                } else if (auto* granular = std::get_if<typed::GranularAskForApproval>(&result)) {
                    granular->diagnostics.push_back(diagnostic);
                }
            }
            return true;
        }

        bool decodeAnalyticsConfig(const Json& value, typed::AnalyticsConfig& result, std::string& error, std::string_view path) {
            constexpr std::string_view Context = "AnalyticsConfig";
            if (!requireObject(value, Context, error, path)) {
                return false;
            }
            result = {};
            if (!decodeOptionalNullable(
                    value,
                    "enabled",
                    result.enabled,
                    [&](const Json& nested, bool& decoded, const std::string& nestedPath) {
                        return decodeBoolAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path)) {
                return false;
            }
            constexpr std::array<std::string_view, 1> Known{"enabled"};
            retainUnknownProperties(value, Known, result.unknownProperties);
            result.raw = value;
            return true;
        }

        bool decodeWebSearchLocation(const Json& value, typed::WebSearchLocation& result, std::string& error, std::string_view path) {
            constexpr std::string_view Context = "WebSearchLocation";
            if (!requireObject(value, Context, error, path)) {
                return false;
            }
            result = {};
            const auto decodeOptionalString = [&](std::string_view name, typed::OptionalNullable<std::string>& output) {
                return decodeOptionalNullable(
                    value,
                    name,
                    output,
                    [&](const Json& nested, std::string& decoded, const std::string& nestedPath) {
                        return decodeStringAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path);
            };
            if (!decodeOptionalString("city", result.city) || !decodeOptionalString("country", result.country) ||
                !decodeOptionalString("region", result.region) || !decodeOptionalString("timezone", result.timezone) ||
                !rejectUnknownProperties(value, {"city", "country", "region", "timezone"}, error, Context, path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool decodeWebSearchToolConfig(const Json& value, typed::WebSearchToolConfig& result, std::string& error, std::string_view path) {
            constexpr std::string_view Context = "WebSearchToolConfig";
            if (!requireObject(value, Context, error, path)) {
                return false;
            }
            result = {};
            if (!decodeOptionalNullable(
                    value,
                    "allowed_domains",
                    result.allowedDomains,
                    [&](const Json& nested, std::vector<std::string>& decoded, const std::string& nestedPath) {
                        return decodeStringVectorAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "context_size",
                    result.contextSize,
                    [&](const Json& nested, typed::WebSearchContextSize& decoded, const std::string& nestedPath) {
                        return decodeOpenEnumAt(nested, decoded, result.diagnostics, "WebSearchContextSize", nestedPath, error, Context);
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "location",
                    result.location,
                    [&](const Json& nested, typed::WebSearchLocation& decoded, const std::string& nestedPath) {
                        if (!decodeWebSearchLocation(nested, decoded, error, nestedPath)) {
                            return false;
                        }
                        appendDiagnostics(result.diagnostics, decoded.diagnostics);
                        return true;
                    },
                    error,
                    Context,
                    path) ||
                !rejectUnknownProperties(value, {"allowed_domains", "context_size", "location"}, error, Context, path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool decodeToolsV2(const Json& value, typed::ToolsV2& result, std::string& error, std::string_view path) {
            constexpr std::string_view Context = "ToolsV2";
            if (!requireObject(value, Context, error, path)) {
                return false;
            }
            result = {};
            if (!decodeOptionalNullable(
                    value,
                    "web_search",
                    result.webSearch,
                    [&](const Json& nested, typed::WebSearchToolConfig& decoded, const std::string& nestedPath) {
                        if (!decodeWebSearchToolConfig(nested, decoded, error, nestedPath)) {
                            return false;
                        }
                        appendDiagnostics(result.diagnostics, decoded.diagnostics);
                        return true;
                    },
                    error,
                    Context,
                    path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool
        decodeSandboxWorkspaceWrite(const Json& value, typed::SandboxWorkspaceWrite& result, std::string& error, std::string_view path) {
            constexpr std::string_view Context = "SandboxWorkspaceWrite";
            if (!requireObject(value, Context, error, path)) {
                return false;
            }
            result = {};
            const auto decodeDefaultedBool = [&](std::string_view name, bool& output) {
                const Json* nested = member(value, name);
                return nested == nullptr || decodeBoolAt(*nested, output, error, Context, fieldPath(path, name));
            };
            const Json* writableRoots = member(value, "writable_roots");
            if (!decodeDefaultedBool("exclude_slash_tmp", result.excludeSlashTmp) ||
                !decodeDefaultedBool("exclude_tmpdir_env_var", result.excludeTmpdirEnvVar) ||
                !decodeDefaultedBool("network_access", result.networkAccess) ||
                (writableRoots != nullptr &&
                 !decodeStringVectorAt(*writableRoots, result.writableRoots, error, Context, fieldPath(path, "writable_roots")))) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool decodeForcedChatgptWorkspaceIds(const Json& value,
                                             typed::ForcedChatgptWorkspaceIds& result,
                                             std::string& error,
                                             std::string_view path) {
            constexpr std::string_view Context = "ForcedChatgptWorkspaceIds";
            result = {};
            if (value.is_string()) {
                result.value = value.get_ref<const std::string&>();
            } else if (value.is_array()) {
                std::vector<std::string> identifiers;
                if (!decodeStringVectorAt(value, identifiers, error, Context, path)) {
                    return false;
                }
                result.value = std::move(identifiers);
            } else {
                return fail(error, Context, path, "must be a string or an array of strings");
            }
            result.raw = value;
            return true;
        }

        template <typename T>
        ConversationDecodeResult<T> malformedUnion(std::string_view surface, std::string path) {
            return {std::nullopt, malformedKnownDiagnostic(std::string(surface), std::move(path))};
        }

        struct ConfigLayerSourceResolution {
            const ProtocolSurfaceEntry* entry = nullptr;
            const AccountsModelsConfigurationUnionCodecDescriptor* descriptor = nullptr;
            std::optional<typed::DecodeDiagnostic> diagnostic;
        };

        ConfigLayerSourceResolution resolveConfigLayerSource(std::string_view discriminator, std::string path) {
            ConfigLayerSourceResolution result;
            result.entry = findSurface(SurfaceCategory::TaggedUnionDiscriminator, "ConfigLayerSource", "type", discriminator);
            if (result.entry == nullptr) {
                return result;
            }
            if (result.entry->runtimeDisposition != RuntimeDisposition::Typed ||
                result.entry->typedImplementation != TypedImplementationStatus::Implemented) {
                result.diagnostic = malformedKnownDiagnostic("ConfigLayerSource", std::move(path));
                return result;
            }
            const auto* target = std::get_if<AccountsModelsConfigurationUnionTarget>(&result.entry->runtimeTarget);
            if (target == nullptr) {
                result.diagnostic = malformedKnownDiagnostic("ConfigLayerSource", std::move(path));
                return result;
            }
            const std::span<const AccountsModelsConfigurationUnionCodecDescriptor> descriptors =
                accountsModelsConfigurationUnionCodecDescriptors();
            const auto descriptor = std::find_if(descriptors.begin(), descriptors.end(), [&](const auto& candidate) {
                return candidate.key == result.entry->key && candidate.target == *target;
            });
            if (descriptor == descriptors.end() || descriptor->shape != ConversationUnionCodecShape::InternallyTaggedObject ||
                descriptor->direction != ConversationUnionCodecDirection::DecodeOnly) {
                result.diagnostic = malformedKnownDiagnostic("ConfigLayerSource", std::move(path));
                return result;
            }
            result.descriptor = &*descriptor;
            return result;
        }

        ConversationDecodeResult<typed::ConfigLayerSource> decodeConfigLayerSourceAt(const Json& value, std::string_view path) {
            constexpr std::string_view Surface = "ConfigLayerSource";
            if (!value.is_object()) {
                return malformedUnion<typed::ConfigLayerSource>(Surface, std::string(path));
            }
            const Json* typeValue = member(value, "type");
            if (typeValue == nullptr || !typeValue->is_string()) {
                return malformedUnion<typed::ConfigLayerSource>(Surface, fieldPath(path, "type"));
            }

            const std::string discriminator = typeValue->get_ref<const std::string&>();
            const ConfigLayerSourceResolution resolution = resolveConfigLayerSource(discriminator, fieldPath(path, "type"));
            if (resolution.diagnostic) {
                return {std::nullopt, resolution.diagnostic};
            }
            if (resolution.entry == nullptr) {
                typed::DecodeDiagnostic diagnostic = unknownDiscriminatorDiagnostic(std::string(Surface), fieldPath(path, "type"));
                return {typed::ConfigLayerSource{typed::UnknownConfigLayerSource{discriminator, value, diagnostic}}, diagnostic};
            }

            std::string error;
            const auto target = std::get<AccountsModelsConfigurationUnionTarget>(resolution.entry->runtimeTarget);
            switch (target) {
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceMdm: {
                    typed::MdmConfigLayerSource result;
                    if (!decodeRequired(
                            value,
                            "domain",
                            result.domain,
                            [&](const Json& nested, std::string& decoded, const std::string& nestedPath) {
                                return decodeStringAt(nested, decoded, error, Surface, nestedPath);
                            },
                            error,
                            Surface,
                            path)) {
                        return malformedUnion<typed::ConfigLayerSource>(Surface, fieldPath(path, "domain"));
                    }
                    if (!decodeRequired(
                            value,
                            "key",
                            result.key,
                            [&](const Json& nested, std::string& decoded, const std::string& nestedPath) {
                                return decodeStringAt(nested, decoded, error, Surface, nestedPath);
                            },
                            error,
                            Surface,
                            path)) {
                        return malformedUnion<typed::ConfigLayerSource>(Surface, fieldPath(path, "key"));
                    }
                    result.raw = value;
                    return {typed::ConfigLayerSource{std::move(result)}, std::nullopt};
                }
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceSystem: {
                    typed::SystemConfigLayerSource result;
                    if (!decodeRequired(
                            value,
                            "file",
                            result.file,
                            [&](const Json& nested, typed::AbsolutePathBuf& decoded, const std::string& nestedPath) {
                                return decodeStrongStringAt(nested, decoded, error, Surface, nestedPath);
                            },
                            error,
                            Surface,
                            path)) {
                        return malformedUnion<typed::ConfigLayerSource>(Surface, fieldPath(path, "file"));
                    }
                    result.raw = value;
                    return {typed::ConfigLayerSource{std::move(result)}, std::nullopt};
                }
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceEnterpriseManaged: {
                    typed::EnterpriseManagedConfigLayerSource result;
                    if (!decodeRequired(
                            value,
                            "id",
                            result.id,
                            [&](const Json& nested, typed::ConfigLayerId& decoded, const std::string& nestedPath) {
                                return decodeStrongStringAt(nested, decoded, error, Surface, nestedPath);
                            },
                            error,
                            Surface,
                            path) ||
                        !decodeRequired(
                            value,
                            "name",
                            result.name,
                            [&](const Json& nested, std::string& decoded, const std::string& nestedPath) {
                                return decodeStringAt(nested, decoded, error, Surface, nestedPath);
                            },
                            error,
                            Surface,
                            path)) {
                        const std::string failedPath = member(value, "id") == nullptr || !member(value, "id")->is_string()
                                                           ? fieldPath(path, "id")
                                                           : fieldPath(path, "name");
                        return malformedUnion<typed::ConfigLayerSource>(Surface, failedPath);
                    }
                    result.raw = value;
                    return {typed::ConfigLayerSource{std::move(result)}, std::nullopt};
                }
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceUser: {
                    typed::UserConfigLayerSource result;
                    if (!decodeRequired(
                            value,
                            "file",
                            result.file,
                            [&](const Json& nested, typed::AbsolutePathBuf& decoded, const std::string& nestedPath) {
                                return decodeStrongStringAt(nested, decoded, error, Surface, nestedPath);
                            },
                            error,
                            Surface,
                            path) ||
                        !decodeOptionalNullable(
                            value,
                            "profile",
                            result.profile,
                            [&](const Json& nested, std::string& decoded, const std::string& nestedPath) {
                                return decodeStringAt(nested, decoded, error, Surface, nestedPath);
                            },
                            error,
                            Surface,
                            path)) {
                        const Json* file = member(value, "file");
                        return malformedUnion<typed::ConfigLayerSource>(
                            Surface, file == nullptr || !file->is_string() ? fieldPath(path, "file") : fieldPath(path, "profile"));
                    }
                    result.raw = value;
                    return {typed::ConfigLayerSource{std::move(result)}, std::nullopt};
                }
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceProject: {
                    typed::ProjectConfigLayerSource result;
                    if (!decodeRequired(
                            value,
                            "dotCodexFolder",
                            result.dotCodexFolder,
                            [&](const Json& nested, typed::AbsolutePathBuf& decoded, const std::string& nestedPath) {
                                return decodeStrongStringAt(nested, decoded, error, Surface, nestedPath);
                            },
                            error,
                            Surface,
                            path)) {
                        return malformedUnion<typed::ConfigLayerSource>(Surface, fieldPath(path, "dotCodexFolder"));
                    }
                    result.raw = value;
                    return {typed::ConfigLayerSource{std::move(result)}, std::nullopt};
                }
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceSessionFlags: {
                    typed::SessionFlagsConfigLayerSource result;
                    result.raw = value;
                    return {typed::ConfigLayerSource{std::move(result)}, std::nullopt};
                }
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceLegacyManagedConfigTomlFromFile: {
                    typed::LegacyManagedConfigTomlFromFileConfigLayerSource result;
                    if (!decodeRequired(
                            value,
                            "file",
                            result.file,
                            [&](const Json& nested, typed::AbsolutePathBuf& decoded, const std::string& nestedPath) {
                                return decodeStrongStringAt(nested, decoded, error, Surface, nestedPath);
                            },
                            error,
                            Surface,
                            path)) {
                        return malformedUnion<typed::ConfigLayerSource>(Surface, fieldPath(path, "file"));
                    }
                    result.raw = value;
                    return {typed::ConfigLayerSource{std::move(result)}, std::nullopt};
                }
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceLegacyManagedConfigTomlFromMdm: {
                    typed::LegacyManagedConfigTomlFromMdmConfigLayerSource result;
                    result.raw = value;
                    return {typed::ConfigLayerSource{std::move(result)}, std::nullopt};
                }
                case AccountsModelsConfigurationUnionTarget::AccountAmazonBedrock:
                case AccountsModelsConfigurationUnionTarget::AccountApiKey:
                case AccountsModelsConfigurationUnionTarget::AccountChatgpt:
                case AccountsModelsConfigurationUnionTarget::LoginAccountParamsApiKey:
                case AccountsModelsConfigurationUnionTarget::LoginAccountParamsChatgpt:
                case AccountsModelsConfigurationUnionTarget::LoginAccountParamsChatgptAuthTokens:
                case AccountsModelsConfigurationUnionTarget::LoginAccountParamsChatgptDeviceCode:
                case AccountsModelsConfigurationUnionTarget::LoginAccountResponseApiKey:
                case AccountsModelsConfigurationUnionTarget::LoginAccountResponseChatgpt:
                case AccountsModelsConfigurationUnionTarget::LoginAccountResponseChatgptAuthTokens:
                case AccountsModelsConfigurationUnionTarget::LoginAccountResponseChatgptDeviceCode:
                case AccountsModelsConfigurationUnionTarget::Count:
                    return malformedUnion<typed::ConfigLayerSource>(Surface, fieldPath(path, "type"));
            }
            return malformedUnion<typed::ConfigLayerSource>(Surface, fieldPath(path, "type"));
        }

        void appendConfigLayerSourceDiagnostics(std::vector<typed::DecodeDiagnostic>& diagnostics, const typed::ConfigLayerSource& source) {
            std::visit(
                [&](const auto& alternative) {
                    using Alternative = std::decay_t<decltype(alternative)>;
                    if constexpr (std::is_same_v<Alternative, typed::UnknownConfigLayerSource>) {
                        diagnostics.push_back(alternative.diagnostic);
                    } else {
                        appendDiagnostics(diagnostics, alternative.diagnostics);
                    }
                },
                source);
        }

        bool decodeConfigLayer(const Json& value, typed::ConfigLayer& result, std::string& error, std::string_view path) {
            constexpr std::string_view Context = "ConfigLayer";
            if (!requireObject(value, Context, error, path)) {
                return false;
            }
            result = {};

            const Json* config = member(value, "config");
            if (config == nullptr) {
                return fail(error, Context, fieldPath(path, "config"), "is required");
            }
            if (config->is_null()) {
                result.config.reset();
            } else {
                result.config = *config;
            }

            const Json* name = member(value, "name");
            if (name == nullptr) {
                return fail(error, Context, fieldPath(path, "name"), "is required");
            }
            ConversationDecodeResult<typed::ConfigLayerSource> decodedName = decodeConfigLayerSourceAt(*name, fieldPath(path, "name"));
            if (!decodedName.value) {
                const std::string diagnosticPath = decodedName.diagnostic ? decodedName.diagnostic->fieldPath : fieldPath(path, "name");
                return fail(error, Context, diagnosticPath, "does not match ConfigLayerSource");
            }
            result.name = std::move(*decodedName.value);
            appendConfigLayerSourceDiagnostics(result.diagnostics, result.name);

            if (!decodeRequired(
                    value,
                    "version",
                    result.version,
                    [&](const Json& nested, std::string& decoded, const std::string& nestedPath) {
                        return decodeStringAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "disabledReason",
                    result.disabledReason,
                    [&](const Json& nested, std::string& decoded, const std::string& nestedPath) {
                        return decodeStringAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path)) {
                return false;
            }

            result.raw = value;
            return true;
        }

        bool decodeConfigLayerMetadata(const Json& value, typed::ConfigLayerMetadata& result, std::string& error, std::string_view path) {
            constexpr std::string_view Context = "ConfigLayerMetadata";
            if (!requireObject(value, Context, error, path)) {
                return false;
            }
            result = {};
            const Json* name = member(value, "name");
            if (name == nullptr) {
                return fail(error, Context, fieldPath(path, "name"), "is required");
            }
            ConversationDecodeResult<typed::ConfigLayerSource> decodedName = decodeConfigLayerSourceAt(*name, fieldPath(path, "name"));
            if (!decodedName.value) {
                const std::string diagnosticPath = decodedName.diagnostic ? decodedName.diagnostic->fieldPath : fieldPath(path, "name");
                return fail(error, Context, diagnosticPath, "does not match ConfigLayerSource");
            }
            result.name = std::move(*decodedName.value);
            appendConfigLayerSourceDiagnostics(result.diagnostics, result.name);
            if (!decodeRequired(
                    value,
                    "version",
                    result.version,
                    [&](const Json& nested, std::string& decoded, const std::string& nestedPath) {
                        return decodeStringAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool decodeConfig(const Json& value, typed::Config& result, std::string& error, std::string_view path) {
            constexpr std::string_view Context = "Config";
            if (!requireObject(value, Context, error, path)) {
                return false;
            }
            result = {};

            const auto optionalString = [&](std::string_view name, typed::OptionalNullable<std::string>& output) {
                return decodeOptionalNullable(
                    value,
                    name,
                    output,
                    [&](const Json& nested, std::string& decoded, const std::string& nestedPath) {
                        return decodeStringAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path);
            };
            const auto optionalInt64 = [&](std::string_view name, typed::OptionalNullable<std::int64_t>& output) {
                return decodeOptionalNullable(
                    value,
                    name,
                    output,
                    [&](const Json& nested, std::int64_t& decoded, const std::string& nestedPath) {
                        return decodeInt64At(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path);
            };
            const auto optionalModelId = [&](std::string_view name, typed::OptionalNullable<typed::ModelId>& output) {
                return decodeOptionalNullable(
                    value,
                    name,
                    output,
                    [&](const Json& nested, typed::ModelId& decoded, const std::string& nestedPath) {
                        return decodeStrongStringAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path);
            };

            if (!decodeOptionalNullable(
                    value,
                    "analytics",
                    result.analytics,
                    [&](const Json& nested, typed::AnalyticsConfig& decoded, const std::string& nestedPath) {
                        if (!decodeAnalyticsConfig(nested, decoded, error, nestedPath)) {
                            return false;
                        }
                        appendDiagnostics(result.diagnostics, decoded.diagnostics);
                        return true;
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "approval_policy",
                    result.approvalPolicy,
                    [&](const Json& nested, typed::AskForApproval& decoded, const std::string& nestedPath) {
                        if (!decodeAskForApprovalAt(nested, decoded, error, Context, nestedPath)) {
                            return false;
                        }
                        appendAskForApprovalDiagnostics(result.diagnostics, decoded);
                        return true;
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "approvals_reviewer",
                    result.approvalsReviewer,
                    [&](const Json& nested, typed::ApprovalsReviewer& decoded, const std::string& nestedPath) {
                        return decodeOpenEnumAt(nested, decoded, result.diagnostics, "ApprovalsReviewer", nestedPath, error, Context);
                    },
                    error,
                    Context,
                    path) ||
                !optionalString("compact_prompt", result.compactPrompt) ||
                !decodeOptionalNullable(
                    value,
                    "desktop",
                    result.desktop,
                    [&](const Json& nested, std::map<std::string, Json>& decoded, const std::string& nestedPath) {
                        return decodeJsonMapAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path) ||
                !optionalString("developer_instructions", result.developerInstructions) ||
                !decodeOptionalNullable(
                    value,
                    "forced_chatgpt_workspace_id",
                    result.forcedChatgptWorkspaceId,
                    [&](const Json& nested, typed::ForcedChatgptWorkspaceIds& decoded, const std::string& nestedPath) {
                        if (!decodeForcedChatgptWorkspaceIds(nested, decoded, error, nestedPath)) {
                            return false;
                        }
                        appendDiagnostics(result.diagnostics, decoded.diagnostics);
                        return true;
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "forced_login_method",
                    result.forcedLoginMethod,
                    [&](const Json& nested, typed::ForcedLoginMethod& decoded, const std::string& nestedPath) {
                        return decodeOpenEnumAt(nested, decoded, result.diagnostics, "ForcedLoginMethod", nestedPath, error, Context);
                    },
                    error,
                    Context,
                    path) ||
                !optionalString("instructions", result.instructions) || !optionalModelId("model", result.model) ||
                !optionalInt64("model_auto_compact_token_limit", result.modelAutoCompactTokenLimit) ||
                !decodeOptionalNullable(
                    value,
                    "model_auto_compact_token_limit_scope",
                    result.modelAutoCompactTokenLimitScope,
                    [&](const Json& nested, typed::AutoCompactTokenLimitScope& decoded, const std::string& nestedPath) {
                        return decodeOpenEnumAt(
                            nested, decoded, result.diagnostics, "AutoCompactTokenLimitScope", nestedPath, error, Context);
                    },
                    error,
                    Context,
                    path) ||
                !optionalInt64("model_context_window", result.modelContextWindow) ||
                !decodeOptionalNullable(
                    value,
                    "model_provider",
                    result.modelProvider,
                    [&](const Json& nested, typed::ModelProviderId& decoded, const std::string& nestedPath) {
                        return decodeStrongStringAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "model_reasoning_effort",
                    result.modelReasoningEffort,
                    [&](const Json& nested, typed::ReasoningEffort& decoded, const std::string& nestedPath) {
                        return decodeOpenEnumAt(nested, decoded, result.diagnostics, "ReasoningEffort", nestedPath, error, Context, true);
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "model_reasoning_summary",
                    result.modelReasoningSummary,
                    [&](const Json& nested, typed::ReasoningSummary& decoded, const std::string& nestedPath) {
                        return decodeOpenEnumAt(nested, decoded, result.diagnostics, "ReasoningSummary", nestedPath, error, Context);
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "model_verbosity",
                    result.modelVerbosity,
                    [&](const Json& nested, typed::Verbosity& decoded, const std::string& nestedPath) {
                        return decodeOpenEnumAt(nested, decoded, result.diagnostics, "Verbosity", nestedPath, error, Context);
                    },
                    error,
                    Context,
                    path) ||
                !optionalModelId("review_model", result.reviewModel) ||
                !decodeOptionalNullable(
                    value,
                    "sandbox_mode",
                    result.sandboxMode,
                    [&](const Json& nested, typed::SandboxMode& decoded, const std::string& nestedPath) {
                        return decodeOpenEnumAt(nested, decoded, result.diagnostics, "SandboxMode", nestedPath, error, Context);
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "sandbox_workspace_write",
                    result.sandboxWorkspaceWrite,
                    [&](const Json& nested, typed::SandboxWorkspaceWrite& decoded, const std::string& nestedPath) {
                        if (!decodeSandboxWorkspaceWrite(nested, decoded, error, nestedPath)) {
                            return false;
                        }
                        appendDiagnostics(result.diagnostics, decoded.diagnostics);
                        return true;
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "service_tier",
                    result.serviceTier,
                    [&](const Json& nested, typed::ModelServiceTierId& decoded, const std::string& nestedPath) {
                        return decodeStrongStringAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "tools",
                    result.tools,
                    [&](const Json& nested, typed::ToolsV2& decoded, const std::string& nestedPath) {
                        if (!decodeToolsV2(nested, decoded, error, nestedPath)) {
                            return false;
                        }
                        appendDiagnostics(result.diagnostics, decoded.diagnostics);
                        return true;
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "web_search",
                    result.webSearch,
                    [&](const Json& nested, typed::WebSearchMode& decoded, const std::string& nestedPath) {
                        return decodeOpenEnumAt(nested, decoded, result.diagnostics, "WebSearchMode", nestedPath, error, Context);
                    },
                    error,
                    Context,
                    path)) {
                return false;
            }

            constexpr std::array<std::string_view, 23> KnownProperties{
                "analytics",
                "approval_policy",
                "approvals_reviewer",
                "compact_prompt",
                "desktop",
                "developer_instructions",
                "forced_chatgpt_workspace_id",
                "forced_login_method",
                "instructions",
                "model",
                "model_auto_compact_token_limit",
                "model_auto_compact_token_limit_scope",
                "model_context_window",
                "model_provider",
                "model_reasoning_effort",
                "model_reasoning_summary",
                "model_verbosity",
                "review_model",
                "sandbox_mode",
                "sandbox_workspace_write",
                "service_tier",
                "tools",
                "web_search",
            };
            retainUnknownProperties(value, KnownProperties, result.unknownProperties);
            result.raw = value;
            return true;
        }

        bool decodeConfigLayerVectorAt(const Json& value,
                                       std::vector<typed::ConfigLayer>& result,
                                       std::vector<typed::DecodeDiagnostic>& diagnostics,
                                       std::string& error,
                                       std::string_view path) {
            return decodeArrayAt(
                value,
                result,
                [&](const Json& nested, typed::ConfigLayer& decoded, const std::string& nestedPath) {
                    if (!decodeConfigLayer(nested, decoded, error, nestedPath)) {
                        return false;
                    }
                    appendDiagnostics(diagnostics, decoded.diagnostics);
                    return true;
                },
                error,
                "ConfigReadResponse",
                path);
        }

        bool decodeConfigOriginsAt(const Json& value,
                                   std::map<typed::ConfigKeyPath, typed::ConfigLayerMetadata>& result,
                                   std::vector<typed::DecodeDiagnostic>& diagnostics,
                                   std::string& error,
                                   std::string_view path) {
            constexpr std::string_view Context = "ConfigReadResponse";
            if (!value.is_object()) {
                return fail(error, Context, path, "must be an object");
            }
            result.clear();
            for (const auto& [key, nested] : value.items()) {
                typed::ConfigLayerMetadata decoded;
                if (!decodeConfigLayerMetadata(nested, decoded, error, fieldPath(path, key))) {
                    return false;
                }
                appendDiagnostics(diagnostics, decoded.diagnostics);
                result.emplace(typed::ConfigKeyPath{key}, std::move(decoded));
            }
            return true;
        }

        bool decodeComputerUseRequirements(const Json& value,
                                           typed::ComputerUseRequirements& result,
                                           std::string& error,
                                           std::string_view path) {
            constexpr std::string_view Context = "ComputerUseRequirements";
            if (!requireObject(value, Context, error, path)) {
                return false;
            }
            result = {};
            if (!decodeOptionalNullable(
                    value,
                    "allowLockedComputerUse",
                    result.allowLockedComputerUse,
                    [&](const Json& nested, bool& decoded, const std::string& nestedPath) {
                        return decodeBoolAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool
        decodeNewThreadModelDefaults(const Json& value, typed::NewThreadModelDefaults& result, std::string& error, std::string_view path) {
            constexpr std::string_view Context = "NewThreadModelDefaults";
            if (!requireObject(value, Context, error, path)) {
                return false;
            }
            result = {};
            if (!decodeOptionalNullable(
                    value,
                    "model",
                    result.model,
                    [&](const Json& nested, typed::ModelId& decoded, const std::string& nestedPath) {
                        return decodeStrongStringAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "modelReasoningEffort",
                    result.modelReasoningEffort,
                    [&](const Json& nested, typed::ReasoningEffort& decoded, const std::string& nestedPath) {
                        return decodeOpenEnumAt(nested, decoded, result.diagnostics, "ReasoningEffort", nestedPath, error, Context, true);
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "serviceTier",
                    result.serviceTier,
                    [&](const Json& nested, typed::ModelServiceTierId& decoded, const std::string& nestedPath) {
                        return decodeStrongStringAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool decodeModelsRequirements(const Json& value, typed::ModelsRequirements& result, std::string& error, std::string_view path) {
            constexpr std::string_view Context = "ModelsRequirements";
            if (!requireObject(value, Context, error, path)) {
                return false;
            }
            result = {};
            if (!decodeOptionalNullable(
                    value,
                    "newThread",
                    result.newThread,
                    [&](const Json& nested, typed::NewThreadModelDefaults& decoded, const std::string& nestedPath) {
                        if (!decodeNewThreadModelDefaults(nested, decoded, error, nestedPath)) {
                            return false;
                        }
                        appendDiagnostics(result.diagnostics, decoded.diagnostics);
                        return true;
                    },
                    error,
                    Context,
                    path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool decodeConfigRequirements(const Json& value, typed::ConfigRequirements& result, std::string& error, std::string_view path) {
            constexpr std::string_view Context = "ConfigRequirements";
            if (!requireObject(value, Context, error, path)) {
                return false;
            }
            result = {};

            const auto optionalBool = [&](std::string_view name, typed::OptionalNullable<bool>& output) {
                return decodeOptionalNullable(
                    value,
                    name,
                    output,
                    [&](const Json& nested, bool& decoded, const std::string& nestedPath) {
                        return decodeBoolAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path);
            };

            if (!optionalBool("allowAppshots", result.allowAppshots) ||
                !optionalBool("allowManagedHooksOnly", result.allowManagedHooksOnly) ||
                !optionalBool("allowRemoteControl", result.allowRemoteControl) ||
                !decodeOptionalNullable(
                    value,
                    "allowedApprovalPolicies",
                    result.allowedApprovalPolicies,
                    [&](const Json& nested, std::vector<typed::AskForApproval>& decoded, const std::string& nestedPath) {
                        return decodeArrayAt(
                            nested,
                            decoded,
                            [&](const Json& item, typed::AskForApproval& approval, const std::string& itemPath) {
                                if (!decodeAskForApprovalAt(item, approval, error, Context, itemPath)) {
                                    return false;
                                }
                                appendAskForApprovalDiagnostics(result.diagnostics, approval);
                                return true;
                            },
                            error,
                            Context,
                            nestedPath);
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "allowedPermissionProfiles",
                    result.allowedPermissionProfiles,
                    [&](const Json& nested, std::map<typed::PermissionProfileName, bool>& decoded, const std::string& nestedPath) {
                        return decodeBoolMapAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "allowedSandboxModes",
                    result.allowedSandboxModes,
                    [&](const Json& nested, std::vector<typed::SandboxMode>& decoded, const std::string& nestedPath) {
                        return decodeArrayAt(
                            nested,
                            decoded,
                            [&](const Json& item, typed::SandboxMode& mode, const std::string& itemPath) {
                                return decodeOpenEnumAt(item, mode, result.diagnostics, "SandboxMode", itemPath, error, Context);
                            },
                            error,
                            Context,
                            nestedPath);
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "allowedWebSearchModes",
                    result.allowedWebSearchModes,
                    [&](const Json& nested, std::vector<typed::WebSearchMode>& decoded, const std::string& nestedPath) {
                        return decodeArrayAt(
                            nested,
                            decoded,
                            [&](const Json& item, typed::WebSearchMode& mode, const std::string& itemPath) {
                                return decodeOpenEnumAt(item, mode, result.diagnostics, "WebSearchMode", itemPath, error, Context);
                            },
                            error,
                            Context,
                            nestedPath);
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "allowedWindowsSandboxImplementations",
                    result.allowedWindowsSandboxImplementations,
                    [&](const Json& nested, std::vector<typed::WindowsSandboxSetupMode>& decoded, const std::string& nestedPath) {
                        return decodeArrayAt(
                            nested,
                            decoded,
                            [&](const Json& item, typed::WindowsSandboxSetupMode& mode, const std::string& itemPath) {
                                return decodeOpenEnumAt(
                                    item, mode, result.diagnostics, "WindowsSandboxSetupMode", itemPath, error, Context);
                            },
                            error,
                            Context,
                            nestedPath);
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "computerUse",
                    result.computerUse,
                    [&](const Json& nested, typed::ComputerUseRequirements& decoded, const std::string& nestedPath) {
                        if (!decodeComputerUseRequirements(nested, decoded, error, nestedPath)) {
                            return false;
                        }
                        appendDiagnostics(result.diagnostics, decoded.diagnostics);
                        return true;
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "defaultPermissions",
                    result.defaultPermissions,
                    [&](const Json& nested, std::string& decoded, const std::string& nestedPath) {
                        return decodeStringAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "enforceResidency",
                    result.enforceResidency,
                    [&](const Json& nested, typed::ResidencyRequirement& decoded, const std::string& nestedPath) {
                        return decodeOpenEnumAt(nested, decoded, result.diagnostics, "ResidencyRequirement", nestedPath, error, Context);
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "featureRequirements",
                    result.featureRequirements,
                    [&](const Json& nested, std::map<typed::ExperimentalFeatureId, bool>& decoded, const std::string& nestedPath) {
                        return decodeBoolMapAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "models",
                    result.models,
                    [&](const Json& nested, typed::ModelsRequirements& decoded, const std::string& nestedPath) {
                        if (!decodeModelsRequirements(nested, decoded, error, nestedPath)) {
                            return false;
                        }
                        appendDiagnostics(result.diagnostics, decoded.diagnostics);
                        return true;
                    },
                    error,
                    Context,
                    path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool decodeOverriddenMetadata(const Json& value, typed::OverriddenMetadata& result, std::string& error, std::string_view path) {
            constexpr std::string_view Context = "OverriddenMetadata";
            if (!requireObject(value, Context, error, path)) {
                return false;
            }
            result = {};

            const Json* effectiveValue = member(value, "effectiveValue");
            if (effectiveValue == nullptr) {
                return fail(error, Context, fieldPath(path, "effectiveValue"), "is required");
            }
            if (effectiveValue->is_null()) {
                result.effectiveValue.reset();
            } else {
                result.effectiveValue = *effectiveValue;
            }

            if (!decodeRequired(
                    value,
                    "message",
                    result.message,
                    [&](const Json& nested, std::string& decoded, const std::string& nestedPath) {
                        return decodeStringAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path) ||
                !decodeRequired(
                    value,
                    "overridingLayer",
                    result.overridingLayer,
                    [&](const Json& nested, typed::ConfigLayerMetadata& decoded, const std::string& nestedPath) {
                        if (!decodeConfigLayerMetadata(nested, decoded, error, nestedPath)) {
                            return false;
                        }
                        appendDiagnostics(result.diagnostics, decoded.diagnostics);
                        return true;
                    },
                    error,
                    Context,
                    path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool decodeExperimentalFeature(const Json& value, typed::ExperimentalFeature& result, std::string& error, std::string_view path) {
            constexpr std::string_view Context = "ExperimentalFeature";
            if (!requireObject(value, Context, error, path)) {
                return false;
            }
            result = {};
            const auto optionalString = [&](std::string_view name, typed::OptionalNullable<std::string>& output) {
                return decodeOptionalNullable(
                    value,
                    name,
                    output,
                    [&](const Json& nested, std::string& decoded, const std::string& nestedPath) {
                        return decodeStringAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path);
            };

            if (!optionalString("announcement", result.announcement) ||
                !decodeRequired(
                    value,
                    "defaultEnabled",
                    result.defaultEnabled,
                    [&](const Json& nested, bool& decoded, const std::string& nestedPath) {
                        return decodeBoolAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path) ||
                !optionalString("description", result.description) || !optionalString("displayName", result.displayName) ||
                !decodeRequired(
                    value,
                    "enabled",
                    result.enabled,
                    [&](const Json& nested, bool& decoded, const std::string& nestedPath) {
                        return decodeBoolAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path) ||
                !decodeRequired(
                    value,
                    "name",
                    result.name,
                    [&](const Json& nested, typed::ExperimentalFeatureId& decoded, const std::string& nestedPath) {
                        return decodeStrongStringAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path) ||
                !decodeRequired(
                    value,
                    "stage",
                    result.stage,
                    [&](const Json& nested, typed::ExperimentalFeatureStage& decoded, const std::string& nestedPath) {
                        return decodeOpenEnumAt(
                            nested, decoded, result.diagnostics, "ExperimentalFeatureStage", nestedPath, error, Context);
                    },
                    error,
                    Context,
                    path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool decodeTextPosition(const Json& value, typed::TextPosition& result, std::string& error, std::string_view path) {
            constexpr std::string_view Context = "TextPosition";
            if (!requireObject(value, Context, error, path)) {
                return false;
            }
            result = {};
            if (!decodeRequired(
                    value,
                    "column",
                    result.column,
                    [&](const Json& nested, std::uint64_t& decoded, const std::string& nestedPath) {
                        return decodeUint64At(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path) ||
                !decodeRequired(
                    value,
                    "line",
                    result.line,
                    [&](const Json& nested, std::uint64_t& decoded, const std::string& nestedPath) {
                        return decodeUint64At(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool decodeTextRange(const Json& value, typed::TextRange& result, std::string& error, std::string_view path) {
            constexpr std::string_view Context = "TextRange";
            if (!requireObject(value, Context, error, path)) {
                return false;
            }
            result = {};
            if (!decodeRequired(
                    value,
                    "end",
                    result.end,
                    [&](const Json& nested, typed::TextPosition& decoded, const std::string& nestedPath) {
                        if (!decodeTextPosition(nested, decoded, error, nestedPath)) {
                            return false;
                        }
                        appendDiagnostics(result.diagnostics, decoded.diagnostics);
                        return true;
                    },
                    error,
                    Context,
                    path) ||
                !decodeRequired(
                    value,
                    "start",
                    result.start,
                    [&](const Json& nested, typed::TextPosition& decoded, const std::string& nestedPath) {
                        if (!decodeTextPosition(nested, decoded, error, nestedPath)) {
                            return false;
                        }
                        appendDiagnostics(result.diagnostics, decoded.diagnostics);
                        return true;
                    },
                    error,
                    Context,
                    path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

    } // namespace

    ConversationDecodeResult<typed::ConfigLayerSource> decodeConfigLayerSource(const Json& value) noexcept {
        try {
            return decodeConfigLayerSourceAt(value, "$");
        } catch (...) {
            return malformedUnion<typed::ConfigLayerSource>("ConfigLayerSource", "$");
        }
    }

    std::optional<Json> encodeConfigBatchWriteParams(const typed::ConfigBatchWriteParams& params, std::string& error) {
        constexpr std::string_view Context = "ConfigBatchWriteParams";
        try {
            if (!validateOptionalNullable(params.expectedVersion, error, Context, "$.expectedVersion") ||
                !validateOptionalNullable(params.filePath, error, Context, "$.filePath")) {
                return std::nullopt;
            }

            Json edits = Json::array();
            for (const typed::ConfigEdit& edit : params.edits) {
                edits.push_back({
                    {"keyPath", edit.keyPath.value},
                    {"mergeStrategy", edit.mergeStrategy.value},
                    {"value", edit.value ? *edit.value : Json(nullptr)},
                });
            }

            Json result{{"edits", std::move(edits)}};
            encodeOptionalNullable(result, "expectedVersion", params.expectedVersion, [](const std::string& value) {
                return value;
            });
            encodeOptionalNullable(result, "filePath", params.filePath, [](const typed::AbsolutePathBuf& value) {
                return value.value;
            });
            if (params.reloadUserConfig.has_value()) {
                result["reloadUserConfig"] = *params.reloadUserConfig;
            }
            error.clear();
            return std::optional<Json>{std::move(result)};
        } catch (...) {
            error = "ConfigBatchWriteParams field '$' could not be encoded";
            return std::nullopt;
        }
    }

    std::optional<Json> encodeConfigReadParams(const typed::ConfigReadParams& params, std::string& error) {
        try {
            if (!validateOptionalNullable(params.cwd, error, "ConfigReadParams", "$.cwd")) {
                return std::nullopt;
            }
            Json result = Json::object();
            if (params.cwd.present) {
                result["cwd"] = params.cwd.value ? Json(*params.cwd.value) : Json(nullptr);
            }
            if (params.includeLayers) {
                result["includeLayers"] = *params.includeLayers;
            }
            error.clear();
            return std::optional<Json>{std::move(result)};
        } catch (...) {
            error = "ConfigReadParams could not be encoded";
            return std::nullopt;
        }
    }

    std::optional<Json> encodeConfigValueWriteParams(const typed::ConfigValueWriteParams& params, std::string& error) {
        constexpr std::string_view Context = "ConfigValueWriteParams";
        try {
            if (!validateOptionalNullable(params.expectedVersion, error, Context, "$.expectedVersion") ||
                !validateOptionalNullable(params.filePath, error, Context, "$.filePath")) {
                return std::nullopt;
            }

            Json result{
                {"keyPath", params.keyPath.value},
                {"mergeStrategy", params.mergeStrategy.value},
                {"value", params.value ? *params.value : Json(nullptr)},
            };
            encodeOptionalNullable(result, "expectedVersion", params.expectedVersion, [](const std::string& value) {
                return value;
            });
            encodeOptionalNullable(result, "filePath", params.filePath, [](const typed::AbsolutePathBuf& value) {
                return value.value;
            });
            error.clear();
            return std::optional<Json>{std::move(result)};
        } catch (...) {
            error = "ConfigValueWriteParams field '$' could not be encoded";
            return std::nullopt;
        }
    }

    std::optional<Json> encodeExperimentalFeatureEnablementSetParams(const typed::ExperimentalFeatureEnablementSetParams& params,
                                                                     std::string& error) {
        try {
            Json result{{"enablement", encodeBoolMap(params.enablement)}};
            error.clear();
            return std::optional<Json>{std::move(result)};
        } catch (...) {
            error = "ExperimentalFeatureEnablementSetParams field '$' could not be encoded";
            return std::nullopt;
        }
    }

    std::optional<Json> encodeExperimentalFeatureListParams(const typed::ExperimentalFeatureListParams& params, std::string& error) {
        constexpr std::string_view Context = "ExperimentalFeatureListParams";
        try {
            if (!validateOptionalNullable(params.cursor, error, Context, "$.cursor") ||
                !validateOptionalNullable(params.limit, error, Context, "$.limit") ||
                !validateOptionalNullable(params.threadId, error, Context, "$.threadId")) {
                return std::nullopt;
            }

            Json result = Json::object();
            encodeOptionalNullable(result, "cursor", params.cursor, [](const std::string& value) {
                return value;
            });
            encodeOptionalNullable(result, "limit", params.limit, [](std::uint32_t value) {
                return value;
            });
            encodeOptionalNullable(result, "threadId", params.threadId, [](const typed::ThreadId& value) {
                return value.value;
            });
            error.clear();
            return std::optional<Json>{std::move(result)};
        } catch (...) {
            error = "ExperimentalFeatureListParams field '$' could not be encoded";
            return std::nullopt;
        }
    }

    std::optional<typed::ConfigReadResponse> decodeConfigReadResponse(const Json& value, std::string& error) {
        constexpr std::string_view Context = "ConfigReadResponse";
        try {
            if (!requireObject(value, Context, error)) {
                return std::nullopt;
            }
            typed::ConfigReadResponse result;
            if (!decodeRequired(
                    value,
                    "config",
                    result.config,
                    [&](const Json& nested, typed::Config& decoded, const std::string& nestedPath) {
                        if (!decodeConfig(nested, decoded, error, nestedPath)) {
                            return false;
                        }
                        appendDiagnostics(result.diagnostics, decoded.diagnostics);
                        return true;
                    },
                    error,
                    Context) ||
                !decodeOptionalNullable(
                    value,
                    "layers",
                    result.layers,
                    [&](const Json& nested, std::vector<typed::ConfigLayer>& decoded, const std::string& nestedPath) {
                        return decodeConfigLayerVectorAt(nested, decoded, result.diagnostics, error, nestedPath);
                    },
                    error,
                    Context) ||
                !decodeRequired(
                    value,
                    "origins",
                    result.origins,
                    [&](const Json& nested,
                        std::map<typed::ConfigKeyPath, typed::ConfigLayerMetadata>& decoded,
                        const std::string& nestedPath) {
                        return decodeConfigOriginsAt(nested, decoded, result.diagnostics, error, nestedPath);
                    },
                    error,
                    Context)) {
                return std::nullopt;
            }
            result.raw = value;
            error.clear();
            return std::optional<typed::ConfigReadResponse>{std::move(result)};
        } catch (...) {
            error = "ConfigReadResponse field '$' could not be decoded";
            return std::nullopt;
        }
    }

    std::optional<typed::ConfigRequirementsReadResponse> decodeConfigRequirementsReadResponse(const Json& value, std::string& error) {
        constexpr std::string_view Context = "ConfigRequirementsReadResponse";
        try {
            if (!requireObject(value, Context, error)) {
                return std::nullopt;
            }
            typed::ConfigRequirementsReadResponse result;
            if (!decodeOptionalNullable(
                    value,
                    "requirements",
                    result.requirements,
                    [&](const Json& nested, typed::ConfigRequirements& decoded, const std::string& nestedPath) {
                        if (!decodeConfigRequirements(nested, decoded, error, nestedPath)) {
                            return false;
                        }
                        appendDiagnostics(result.diagnostics, decoded.diagnostics);
                        return true;
                    },
                    error,
                    Context)) {
                return std::nullopt;
            }
            result.raw = value;
            error.clear();
            return std::optional<typed::ConfigRequirementsReadResponse>{std::move(result)};
        } catch (...) {
            error = "ConfigRequirementsReadResponse field '$' could not be decoded";
            return std::nullopt;
        }
    }

    std::optional<typed::ConfigWriteResponse> decodeConfigWriteResponse(const Json& value, std::string& error) {
        constexpr std::string_view Context = "ConfigWriteResponse";
        try {
            if (!requireObject(value, Context, error)) {
                return std::nullopt;
            }
            typed::ConfigWriteResponse result;
            if (!decodeRequired(
                    value,
                    "filePath",
                    result.filePath,
                    [&](const Json& nested, typed::AbsolutePathBuf& decoded, const std::string& nestedPath) {
                        return decodeStrongStringAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context) ||
                !decodeOptionalNullable(
                    value,
                    "overriddenMetadata",
                    result.overriddenMetadata,
                    [&](const Json& nested, typed::OverriddenMetadata& decoded, const std::string& nestedPath) {
                        if (!decodeOverriddenMetadata(nested, decoded, error, nestedPath)) {
                            return false;
                        }
                        appendDiagnostics(result.diagnostics, decoded.diagnostics);
                        return true;
                    },
                    error,
                    Context) ||
                !decodeRequired(
                    value,
                    "status",
                    result.status,
                    [&](const Json& nested, typed::WriteStatus& decoded, const std::string& nestedPath) {
                        return decodeOpenEnumAt(nested, decoded, result.diagnostics, "WriteStatus", nestedPath, error, Context);
                    },
                    error,
                    Context) ||
                !decodeRequired(
                    value,
                    "version",
                    result.version,
                    [&](const Json& nested, std::string& decoded, const std::string& nestedPath) {
                        return decodeStringAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context)) {
                return std::nullopt;
            }
            result.raw = value;
            error.clear();
            return std::optional<typed::ConfigWriteResponse>{std::move(result)};
        } catch (...) {
            error = "ConfigWriteResponse field '$' could not be decoded";
            return std::nullopt;
        }
    }

    std::optional<typed::ExperimentalFeatureEnablementSetResponse> decodeExperimentalFeatureEnablementSetResponse(const Json& value,
                                                                                                                  std::string& error) {
        constexpr std::string_view Context = "ExperimentalFeatureEnablementSetResponse";
        try {
            if (!requireObject(value, Context, error)) {
                return std::nullopt;
            }
            typed::ExperimentalFeatureEnablementSetResponse result;
            if (!decodeRequired(
                    value,
                    "enablement",
                    result.enablement,
                    [&](const Json& nested, std::map<typed::ExperimentalFeatureId, bool>& decoded, const std::string& nestedPath) {
                        return decodeBoolMapAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context)) {
                return std::nullopt;
            }
            result.raw = value;
            error.clear();
            return std::optional<typed::ExperimentalFeatureEnablementSetResponse>{std::move(result)};
        } catch (...) {
            error = "ExperimentalFeatureEnablementSetResponse field '$' could not be decoded";
            return std::nullopt;
        }
    }

    std::optional<typed::ExperimentalFeatureListResponse> decodeExperimentalFeatureListResponse(const Json& value, std::string& error) {
        constexpr std::string_view Context = "ExperimentalFeatureListResponse";
        try {
            if (!requireObject(value, Context, error)) {
                return std::nullopt;
            }
            typed::ExperimentalFeatureListResponse result;
            if (!decodeRequired(
                    value,
                    "data",
                    result.data,
                    [&](const Json& nested, std::vector<typed::ExperimentalFeature>& decoded, const std::string& nestedPath) {
                        return decodeArrayAt(
                            nested,
                            decoded,
                            [&](const Json& item, typed::ExperimentalFeature& feature, const std::string& itemPath) {
                                if (!decodeExperimentalFeature(item, feature, error, itemPath)) {
                                    return false;
                                }
                                appendDiagnostics(result.diagnostics, feature.diagnostics);
                                return true;
                            },
                            error,
                            Context,
                            nestedPath);
                    },
                    error,
                    Context) ||
                !decodeOptionalNullable(
                    value,
                    "nextCursor",
                    result.nextCursor,
                    [&](const Json& nested, std::string& decoded, const std::string& nestedPath) {
                        return decodeStringAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context)) {
                return std::nullopt;
            }
            result.raw = value;
            error.clear();
            return std::optional<typed::ExperimentalFeatureListResponse>{std::move(result)};
        } catch (...) {
            error = "ExperimentalFeatureListResponse field '$' could not be decoded";
            return std::nullopt;
        }
    }

    std::optional<typed::ConfigWarningNotification> decodeConfigWarningNotification(const Notification& notification, std::string& error) {
        constexpr std::string_view Context = "ConfigWarningNotification";
        constexpr std::string_view ParamsPath = "$.params";
        try {
            if (!requireObject(notification.params, Context, error, ParamsPath)) {
                return std::nullopt;
            }
            typed::ConfigWarningNotification result;
            if (!decodeOptionalNullable(
                    notification.params,
                    "details",
                    result.details,
                    [&](const Json& nested, std::string& decoded, const std::string& nestedPath) {
                        return decodeStringAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    ParamsPath) ||
                !decodeOptionalNullable(
                    notification.params,
                    "path",
                    result.path,
                    [&](const Json& nested, typed::AbsolutePathBuf& decoded, const std::string& nestedPath) {
                        return decodeStrongStringAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    ParamsPath) ||
                !decodeOptionalNullable(
                    notification.params,
                    "range",
                    result.range,
                    [&](const Json& nested, typed::TextRange& decoded, const std::string& nestedPath) {
                        if (!decodeTextRange(nested, decoded, error, nestedPath)) {
                            return false;
                        }
                        appendDiagnostics(result.diagnostics, decoded.diagnostics);
                        return true;
                    },
                    error,
                    Context,
                    ParamsPath) ||
                !decodeRequired(
                    notification.params,
                    "summary",
                    result.summary,
                    [&](const Json& nested, std::string& decoded, const std::string& nestedPath) {
                        return decodeStringAt(nested, decoded, error, Context, nestedPath);
                    },
                    error,
                    Context,
                    ParamsPath)) {
                return std::nullopt;
            }
            result.raw = notification.raw;
            error.clear();
            return std::optional<typed::ConfigWarningNotification>{std::move(result)};
        } catch (...) {
            error = "ConfigWarningNotification field '$.params' could not be decoded";
            return std::nullopt;
        }
    }

} // namespace ai::openai::codex::detail
