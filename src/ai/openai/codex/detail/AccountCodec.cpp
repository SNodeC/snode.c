/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/AccountCodec.h"

#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "ai/openai/codex/typed/Types.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <nlohmann/detail/iterators/iteration_proxy.hpp>
#include <nlohmann/detail/json_ref.hpp>
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

        std::string fieldPath(std::string_view base, std::string_view name) {
            std::string result(base.empty() ? "$" : base);
            if (!name.empty()) {
                result += ".";
                result += name;
            }
            return result;
        }

        typed::DecodeDiagnostic malformedDiagnostic(std::string surface, std::string path) {
            return {typed::DecodeIssueKind::MalformedKnownPayload,
                    typed::DecodeIssueSeverity::ProtocolWarning,
                    std::move(surface),
                    std::move(path),
                    "known protocol surface contains an invalid value"};
        }

        typed::DecodeDiagnostic unknownDiscriminatorDiagnostic(std::string surface, std::string path) {
            return {typed::DecodeIssueKind::UnknownDiscriminator,
                    typed::DecodeIssueSeverity::ForwardCompatibility,
                    std::move(surface),
                    std::move(path),
                    "protocol surface contains a future discriminator"};
        }

        typed::DecodeDiagnostic unknownEnumDiagnostic(std::string surface, std::string path) {
            return {typed::DecodeIssueKind::UnknownEnumValue,
                    typed::DecodeIssueSeverity::ForwardCompatibility,
                    std::move(surface),
                    std::move(path),
                    "protocol surface contains a future string-enum value"};
        }

        void appendDiagnostics(std::vector<typed::DecodeDiagnostic>& target, const std::vector<typed::DecodeDiagnostic>& source) {
            target.insert(target.end(), source.begin(), source.end());
        }

        bool requireObject(const Json& value, std::string_view context, std::string& error, std::string_view path = "$") {
            return value.is_object() || fail(error, std::string(context) + " at '" + std::string(path) + "' must be an object");
        }

        bool decodeString(const Json& value, std::string& result) {
            if (!value.is_string()) {
                return false;
            }
            result = value.get_ref<const std::string&>();
            return true;
        }

        bool decodeBool(const Json& value, bool& result) {
            if (!value.is_boolean()) {
                return false;
            }
            result = value.get_ref<const Json::boolean_t&>();
            return true;
        }

        bool decodeInt32(const Json& value, std::int32_t& result) noexcept {
            if (value.is_number_unsigned()) {
                const auto number = value.get_ref<const Json::number_unsigned_t&>();
                if (number > static_cast<Json::number_unsigned_t>(std::numeric_limits<std::int32_t>::max())) {
                    return false;
                }
                result = static_cast<std::int32_t>(number);
                return true;
            }
            if (!value.is_number_integer()) {
                return false;
            }
            const auto number = value.get_ref<const Json::number_integer_t&>();
            if (number < std::numeric_limits<std::int32_t>::min() || number > std::numeric_limits<std::int32_t>::max()) {
                return false;
            }
            result = static_cast<std::int32_t>(number);
            return true;
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

        template <typename Strong>
        bool decodeStrongString(const Json& value, Strong& result) {
            return decodeString(value, result.value);
        }

        template <typename OpenEnum>
        bool decodeOpenEnum(const Json& value,
                            OpenEnum& result,
                            std::vector<typed::DecodeDiagnostic>& diagnostics,
                            std::string_view surface,
                            std::string path) {
            if (!decodeString(value, result.value)) {
                return false;
            }
            if (!result.isKnown()) {
                diagnostics.emplace_back(unknownEnumDiagnostic(std::string(surface), std::move(path)));
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
            const Json* value = member(object, name);
            if (value == nullptr || !decode(*value, result)) {
                if (!error.empty()) {
                    return false;
                }
                return fail(error, std::string(context) + " field '" + fieldPath(path, name) + "' has the wrong type or is missing");
            }
            return true;
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
            if (!decode(*value, decoded)) {
                if (!error.empty()) {
                    return false;
                }
                return fail(error, std::string(context) + " field '" + fieldPath(path, name) + "' has the wrong type");
            }
            result = typed::OptionalNullable<T>::withValue(std::move(decoded));
            return true;
        }

        template <typename T, typename Encode>
        void encodeOptionalNullable(Json& object, std::string_view name, const typed::OptionalNullable<T>& value, Encode&& encode) {
            if (!value.present) {
                return;
            }
            object[std::string(name)] = value.value ? Json(encode(*value.value)) : Json(nullptr);
        }

        struct UnionResolution {
            const ProtocolSurfaceEntry* entry = nullptr;
            const AccountsModelsConfigurationUnionCodecDescriptor* descriptor = nullptr;
            std::optional<typed::DecodeDiagnostic> diagnostic;
        };

        UnionResolution resolveUnion(std::string_view surface,
                                     std::string_view discriminator,
                                     ConversationUnionCodecDirection direction,
                                     std::string path) {
            UnionResolution result;
            result.entry = findSurface(SurfaceCategory::TaggedUnionDiscriminator, surface, "type", discriminator);
            if (result.entry == nullptr) {
                return result;
            }
            if (result.entry->runtimeDisposition != RuntimeDisposition::Typed ||
                result.entry->typedImplementation != TypedImplementationStatus::Implemented) {
                result.diagnostic = malformedDiagnostic(std::string(surface), std::move(path));
                return result;
            }
            const auto* target = std::get_if<AccountsModelsConfigurationUnionTarget>(&result.entry->runtimeTarget);
            if (target == nullptr) {
                result.diagnostic = malformedDiagnostic(std::string(surface), std::move(path));
                return result;
            }
            const auto descriptors = accountsModelsConfigurationUnionCodecDescriptors();
            const auto descriptor = std::find_if(descriptors.begin(), descriptors.end(), [&](const auto& candidate) {
                return candidate.key == result.entry->key && candidate.target == *target;
            });
            if (descriptor == descriptors.end() || descriptor->shape != ConversationUnionCodecShape::InternallyTaggedObject ||
                descriptor->direction != direction) {
                result.diagnostic = malformedDiagnostic(std::string(surface), std::move(path));
                return result;
            }
            result.descriptor = &*descriptor;
            return result;
        }

        template <typename T>
        ConversationDecodeResult<T> malformedUnion(std::string_view surface, std::string path) {
            return {std::nullopt, malformedDiagnostic(std::string(surface), std::move(path))};
        }

        template <typename OpenEnum>
        bool decodeOptionalNullableOpenEnum(const Json& object,
                                            std::string_view name,
                                            typed::OptionalNullable<OpenEnum>& result,
                                            std::vector<typed::DecodeDiagnostic>& diagnostics,
                                            std::string_view enumName,
                                            std::string path,
                                            std::string& error,
                                            std::string_view context) {
            const std::size_t separator = path.rfind('.');
            const std::string parentPath = separator == std::string::npos ? "$" : path.substr(0, separator);
            return decodeOptionalNullable(
                object,
                name,
                result,
                [&](const Json& value, OpenEnum& decoded) {
                    return decodeOpenEnum(value, decoded, diagnostics, enumName, std::move(path));
                },
                error,
                context,
                parentPath);
        }

        bool decodeRateLimitWindow(const Json& value, typed::RateLimitWindow& result, std::string& error, std::string_view path) {
            if (!requireObject(value, "RateLimitWindow", error, path) ||
                !decodeOptionalNullable(value, "resetsAt", result.resetsAt, decodeInt64, error, "RateLimitWindow", path) ||
                !decodeRequired(value, "usedPercent", result.usedPercent, decodeInt32, error, "RateLimitWindow", path) ||
                !decodeOptionalNullable(
                    value, "windowDurationMins", result.windowDurationMins, decodeInt64, error, "RateLimitWindow", path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool decodeCreditsSnapshot(const Json& value, typed::CreditsSnapshot& result, std::string& error, std::string_view path) {
            if (!requireObject(value, "CreditsSnapshot", error, path) ||
                !decodeOptionalNullable(value, "balance", result.balance, decodeString, error, "CreditsSnapshot", path) ||
                !decodeRequired(value, "hasCredits", result.hasCredits, decodeBool, error, "CreditsSnapshot", path) ||
                !decodeRequired(value, "unlimited", result.unlimited, decodeBool, error, "CreditsSnapshot", path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool decodeSpendControlLimitSnapshot(const Json& value,
                                             typed::SpendControlLimitSnapshot& result,
                                             std::string& error,
                                             std::string_view path) {
            if (!requireObject(value, "SpendControlLimitSnapshot", error, path) ||
                !decodeRequired(value, "limit", result.limit, decodeString, error, "SpendControlLimitSnapshot", path) ||
                !decodeRequired(
                    value, "remainingPercent", result.remainingPercent, decodeInt32, error, "SpendControlLimitSnapshot", path) ||
                !decodeRequired(value, "resetsAt", result.resetsAt, decodeInt64, error, "SpendControlLimitSnapshot", path) ||
                !decodeRequired(value, "used", result.used, decodeString, error, "SpendControlLimitSnapshot", path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool decodeRateLimitSnapshot(const Json& value, typed::RateLimitSnapshot& result, std::string& error, std::string_view path) {
            if (!requireObject(value, "RateLimitSnapshot", error, path) ||
                !decodeOptionalNullable(
                    value,
                    "credits",
                    result.credits,
                    [&](const Json& item, typed::CreditsSnapshot& decoded) {
                        return decodeCreditsSnapshot(item, decoded, error, std::string(path) + ".credits");
                    },
                    error,
                    "RateLimitSnapshot",
                    path) ||
                !decodeOptionalNullable(
                    value,
                    "individualLimit",
                    result.individualLimit,
                    [&](const Json& item, typed::SpendControlLimitSnapshot& decoded) {
                        return decodeSpendControlLimitSnapshot(item, decoded, error, std::string(path) + ".individualLimit");
                    },
                    error,
                    "RateLimitSnapshot",
                    path) ||
                !decodeOptionalNullable(
                    value, "limitId", result.limitId, decodeStrongString<typed::RateLimitId>, error, "RateLimitSnapshot", path) ||
                !decodeOptionalNullable(value, "limitName", result.limitName, decodeString, error, "RateLimitSnapshot", path) ||
                !decodeOptionalNullableOpenEnum(value,
                                                "planType",
                                                result.planType,
                                                result.diagnostics,
                                                "PlanType",
                                                std::string(path) + ".planType",
                                                error,
                                                "RateLimitSnapshot") ||
                !decodeOptionalNullable(
                    value,
                    "primary",
                    result.primary,
                    [&](const Json& item, typed::RateLimitWindow& decoded) {
                        return decodeRateLimitWindow(item, decoded, error, std::string(path) + ".primary");
                    },
                    error,
                    "RateLimitSnapshot",
                    path) ||
                !decodeOptionalNullableOpenEnum(value,
                                                "rateLimitReachedType",
                                                result.rateLimitReachedType,
                                                result.diagnostics,
                                                "RateLimitReachedType",
                                                std::string(path) + ".rateLimitReachedType",
                                                error,
                                                "RateLimitSnapshot") ||
                !decodeOptionalNullable(
                    value,
                    "secondary",
                    result.secondary,
                    [&](const Json& item, typed::RateLimitWindow& decoded) {
                        return decodeRateLimitWindow(item, decoded, error, std::string(path) + ".secondary");
                    },
                    error,
                    "RateLimitSnapshot",
                    path)) {
                return false;
            }
            if (result.credits.value) {
                appendDiagnostics(result.diagnostics, result.credits.value->diagnostics);
            }
            if (result.individualLimit.value) {
                appendDiagnostics(result.diagnostics, result.individualLimit.value->diagnostics);
            }
            if (result.primary.value) {
                appendDiagnostics(result.diagnostics, result.primary.value->diagnostics);
            }
            if (result.secondary.value) {
                appendDiagnostics(result.diagnostics, result.secondary.value->diagnostics);
            }
            result.raw = value;
            return true;
        }

        bool decodeRateLimitResetCredit(const Json& value, typed::RateLimitResetCredit& result, std::string& error, std::string_view path) {
            if (!requireObject(value, "RateLimitResetCredit", error, path) ||
                !decodeOptionalNullable(value, "description", result.description, decodeString, error, "RateLimitResetCredit", path) ||
                !decodeOptionalNullable(value, "expiresAt", result.expiresAt, decodeInt64, error, "RateLimitResetCredit", path) ||
                !decodeRequired(value, "grantedAt", result.grantedAt, decodeInt64, error, "RateLimitResetCredit", path) ||
                !decodeRequired(
                    value, "id", result.id, decodeStrongString<typed::RateLimitResetCreditId>, error, "RateLimitResetCredit", path)) {
                return false;
            }
            const Json* resetType = member(value, "resetType");
            if (resetType == nullptr ||
                !decodeOpenEnum(*resetType, result.resetType, result.diagnostics, "RateLimitResetType", fieldPath(path, "resetType"))) {
                return fail(error, "RateLimitResetCredit field '" + fieldPath(path, "resetType") + "' has the wrong type or is missing");
            }
            const Json* status = member(value, "status");
            if (status == nullptr ||
                !decodeOpenEnum(*status, result.status, result.diagnostics, "RateLimitResetCreditStatus", fieldPath(path, "status"))) {
                return fail(error, "RateLimitResetCredit field '" + fieldPath(path, "status") + "' has the wrong type or is missing");
            }
            if (!decodeOptionalNullable(value, "title", result.title, decodeString, error, "RateLimitResetCredit", path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool decodeRateLimitResetCreditsSummary(const Json& value,
                                                typed::RateLimitResetCreditsSummary& result,
                                                std::string& error,
                                                std::string_view path) {
            if (!requireObject(value, "RateLimitResetCreditsSummary", error, path) ||
                !decodeRequired(value, "availableCount", result.availableCount, decodeInt64, error, "RateLimitResetCreditsSummary", path)) {
                return false;
            }
            if (!decodeOptionalNullable(
                    value,
                    "credits",
                    result.credits,
                    [&](const Json& array, std::vector<typed::RateLimitResetCredit>& decoded) {
                        if (!array.is_array()) {
                            return false;
                        }
                        decoded.reserve(array.size());
                        for (std::size_t index = 0; index < array.size(); ++index) {
                            typed::RateLimitResetCredit credit;
                            if (!decodeRateLimitResetCredit(
                                    array[index], credit, error, std::string(path) + ".credits[" + std::to_string(index) + "]")) {
                                return false;
                            }
                            appendDiagnostics(result.diagnostics, credit.diagnostics);
                            decoded.emplace_back(std::move(credit));
                        }
                        return true;
                    },
                    error,
                    "RateLimitResetCreditsSummary",
                    path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool decodeAccountTokenUsageSummary(const Json& value,
                                            typed::AccountTokenUsageSummary& result,
                                            std::string& error,
                                            std::string_view path) {
            if (!requireObject(value, "AccountTokenUsageSummary", error, path) ||
                !decodeOptionalNullable(
                    value, "currentStreakDays", result.currentStreakDays, decodeInt64, error, "AccountTokenUsageSummary", path) ||
                !decodeOptionalNullable(
                    value, "lifetimeTokens", result.lifetimeTokens, decodeInt64, error, "AccountTokenUsageSummary", path) ||
                !decodeOptionalNullable(
                    value, "longestRunningTurnSec", result.longestRunningTurnSec, decodeInt64, error, "AccountTokenUsageSummary", path) ||
                !decodeOptionalNullable(
                    value, "longestStreakDays", result.longestStreakDays, decodeInt64, error, "AccountTokenUsageSummary", path) ||
                !decodeOptionalNullable(
                    value, "peakDailyTokens", result.peakDailyTokens, decodeInt64, error, "AccountTokenUsageSummary", path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool decodeAccountTokenUsageDailyBucket(const Json& value,
                                                typed::AccountTokenUsageDailyBucket& result,
                                                std::string& error,
                                                std::string_view path) {
            if (!requireObject(value, "AccountTokenUsageDailyBucket", error, path) ||
                !decodeRequired(value, "startDate", result.startDate, decodeString, error, "AccountTokenUsageDailyBucket", path) ||
                !decodeRequired(value, "tokens", result.tokens, decodeInt64, error, "AccountTokenUsageDailyBucket", path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool decodeWorkspaceMessage(const Json& value, typed::WorkspaceMessage& result, std::string& error, std::string_view path) {
            if (!requireObject(value, "WorkspaceMessage", error, path) ||
                !decodeOptionalNullable(value, "archivedAt", result.archivedAt, decodeInt64, error, "WorkspaceMessage", path) ||
                !decodeOptionalNullable(value, "createdAt", result.createdAt, decodeInt64, error, "WorkspaceMessage", path) ||
                !decodeRequired(value, "messageBody", result.messageBody, decodeString, error, "WorkspaceMessage", path) ||
                !decodeRequired(
                    value, "messageId", result.messageId, decodeStrongString<typed::WorkspaceMessageId>, error, "WorkspaceMessage", path)) {
                return false;
            }
            const Json* messageType = member(value, "messageType");
            if (messageType == nullptr ||
                !decodeOpenEnum(
                    *messageType, result.messageType, result.diagnostics, "WorkspaceMessageType", std::string(path) + ".messageType")) {
                return fail(error, "WorkspaceMessage field '" + fieldPath(path, "messageType") + "' has the wrong type or is missing");
            }
            result.raw = value;
            return true;
        }

        bool serverRequestDescriptorMatches() noexcept {
            try {
                const ProtocolSurfaceEntry& entry = entryFor(ServerRequestTarget::ChatgptAuthTokensRefresh);
                const auto descriptors = serverRequestCodecDescriptors();
                const auto descriptor = std::find_if(descriptors.begin(), descriptors.end(), [&](const auto& candidate) {
                    return candidate.target == ServerRequestTarget::ChatgptAuthTokensRefresh;
                });
                return descriptor != descriptors.end() && descriptor->key == entry.key &&
                       descriptor->parameterTypeIdentity == entry.operationContract.parameterTypeIdentity &&
                       descriptor->resultTypeIdentity == entry.operationContract.resultTypeIdentity &&
                       descriptor->resultKind == entry.operationContract.resultKind;
            } catch (...) {
                return false;
            }
        }

    } // namespace

    ConversationDecodeResult<typed::Account> decodeAccount(const Json& value) noexcept {
        try {
            constexpr std::string_view Surface = "Account";
            if (!value.is_object()) {
                return malformedUnion<typed::Account>(Surface, "$");
            }
            const Json* typeValue = member(value, "type");
            if (typeValue == nullptr || !typeValue->is_string()) {
                return malformedUnion<typed::Account>(Surface, "$.type");
            }
            const std::string type = typeValue->get_ref<const std::string&>();
            const UnionResolution resolution = resolveUnion(Surface, type, ConversationUnionCodecDirection::DecodeOnly, "$.type");
            if (resolution.diagnostic) {
                return {std::nullopt, resolution.diagnostic};
            }
            if (resolution.entry == nullptr) {
                typed::DecodeDiagnostic diagnostic = unknownDiscriminatorDiagnostic(std::string(Surface), "$.type");
                return {typed::Account{typed::UnknownAccount{type, value, diagnostic}}, diagnostic};
            }

            const auto target = std::get<AccountsModelsConfigurationUnionTarget>(resolution.entry->runtimeTarget);
            switch (target) {
                case AccountsModelsConfigurationUnionTarget::AccountApiKey: {
                    typed::ApiKeyAccount result;
                    result.raw = value;
                    return {typed::Account{std::move(result)}, std::nullopt};
                }
                case AccountsModelsConfigurationUnionTarget::AccountChatgpt: {
                    typed::ChatgptAccount result;
                    const Json* email = member(value, "email");
                    const Json* planType = member(value, "planType");
                    if (email == nullptr || (!email->is_null() && !email->is_string())) {
                        return malformedUnion<typed::Account>(Surface, "$.email");
                    }
                    if (planType == nullptr || !decodeOpenEnum(*planType, result.planType, result.diagnostics, "PlanType", "$.planType")) {
                        return malformedUnion<typed::Account>(Surface, "$.planType");
                    }
                    if (email->is_string()) {
                        result.email = email->get_ref<const std::string&>();
                    }
                    result.raw = value;
                    const std::optional<typed::DecodeDiagnostic> diagnostic =
                        result.diagnostics.empty() ? std::nullopt : std::optional{result.diagnostics.front()};
                    return {typed::Account{std::move(result)}, diagnostic};
                }
                case AccountsModelsConfigurationUnionTarget::AccountAmazonBedrock: {
                    typed::AmazonBedrockAccount result;
                    const Json* credentialSource = member(value, "credentialSource");
                    if (credentialSource == nullptr) {
                        result.credentialSource = typed::AmazonBedrockCredentialSource::awsManaged();
                    } else if (!decodeOpenEnum(*credentialSource,
                                               result.credentialSource,
                                               result.diagnostics,
                                               "AmazonBedrockCredentialSource",
                                               "$.credentialSource")) {
                        return malformedUnion<typed::Account>(Surface, "$.credentialSource");
                    }
                    result.raw = value;
                    const std::optional<typed::DecodeDiagnostic> diagnostic =
                        result.diagnostics.empty() ? std::nullopt : std::optional{result.diagnostics.front()};
                    return {typed::Account{std::move(result)}, diagnostic};
                }
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceEnterpriseManaged:
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceLegacyManagedConfigTomlFromFile:
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceLegacyManagedConfigTomlFromMdm:
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceMdm:
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceProject:
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceSessionFlags:
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceSystem:
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceUser:
                case AccountsModelsConfigurationUnionTarget::LoginAccountParamsApiKey:
                case AccountsModelsConfigurationUnionTarget::LoginAccountParamsChatgpt:
                case AccountsModelsConfigurationUnionTarget::LoginAccountParamsChatgptAuthTokens:
                case AccountsModelsConfigurationUnionTarget::LoginAccountParamsChatgptDeviceCode:
                case AccountsModelsConfigurationUnionTarget::LoginAccountResponseApiKey:
                case AccountsModelsConfigurationUnionTarget::LoginAccountResponseChatgpt:
                case AccountsModelsConfigurationUnionTarget::LoginAccountResponseChatgptAuthTokens:
                case AccountsModelsConfigurationUnionTarget::LoginAccountResponseChatgptDeviceCode:
                case AccountsModelsConfigurationUnionTarget::Count:
                    return malformedUnion<typed::Account>(Surface, "$.type");
            }
            return malformedUnion<typed::Account>(Surface, "$.type");
        } catch (...) {
            return malformedUnion<typed::Account>("Account", "$");
        }
    }

    ConversationDecodeResult<typed::LoginAccountResponse> decodeLoginAccountResponseUnion(const Json& value) noexcept {
        try {
            constexpr std::string_view Surface = "LoginAccountResponse";
            if (!value.is_object()) {
                return malformedUnion<typed::LoginAccountResponse>(Surface, "$");
            }
            const Json* typeValue = member(value, "type");
            if (typeValue == nullptr || !typeValue->is_string()) {
                return malformedUnion<typed::LoginAccountResponse>(Surface, "$.type");
            }
            const std::string type = typeValue->get_ref<const std::string&>();
            const UnionResolution resolution = resolveUnion(Surface, type, ConversationUnionCodecDirection::DecodeOnly, "$.type");
            if (resolution.diagnostic) {
                return {std::nullopt, resolution.diagnostic};
            }
            if (resolution.entry == nullptr) {
                typed::DecodeDiagnostic diagnostic = unknownDiscriminatorDiagnostic(std::string(Surface), "$.type");
                return {typed::LoginAccountResponse{typed::UnknownLoginAccountResponse{type, value, diagnostic}}, diagnostic};
            }

            std::string error;
            const auto target = std::get<AccountsModelsConfigurationUnionTarget>(resolution.entry->runtimeTarget);
            switch (target) {
                case AccountsModelsConfigurationUnionTarget::LoginAccountResponseApiKey: {
                    typed::ApiKeyLoginAccountResponse result;
                    result.raw = value;
                    return {typed::LoginAccountResponse{std::move(result)}, std::nullopt};
                }
                case AccountsModelsConfigurationUnionTarget::LoginAccountResponseChatgpt: {
                    typed::ChatgptLoginAccountResponse result;
                    if (!decodeRequired(value, "authUrl", result.authUrl, decodeString, error, Surface)) {
                        return malformedUnion<typed::LoginAccountResponse>(Surface, "$.authUrl");
                    }
                    if (!decodeRequired(value, "loginId", result.loginId, decodeStrongString<typed::LoginId>, error, Surface)) {
                        return malformedUnion<typed::LoginAccountResponse>(Surface, "$.loginId");
                    }
                    result.raw = value;
                    return {typed::LoginAccountResponse{std::move(result)}, std::nullopt};
                }
                case AccountsModelsConfigurationUnionTarget::LoginAccountResponseChatgptDeviceCode: {
                    typed::ChatgptDeviceCodeLoginAccountResponse result;
                    if (!decodeRequired(value, "loginId", result.loginId, decodeStrongString<typed::LoginId>, error, Surface)) {
                        return malformedUnion<typed::LoginAccountResponse>(Surface, "$.loginId");
                    }
                    if (!decodeRequired(value, "userCode", result.userCode, decodeString, error, Surface)) {
                        return malformedUnion<typed::LoginAccountResponse>(Surface, "$.userCode");
                    }
                    if (!decodeRequired(value, "verificationUrl", result.verificationUrl, decodeString, error, Surface)) {
                        return malformedUnion<typed::LoginAccountResponse>(Surface, "$.verificationUrl");
                    }
                    result.raw = value;
                    return {typed::LoginAccountResponse{std::move(result)}, std::nullopt};
                }
                case AccountsModelsConfigurationUnionTarget::LoginAccountResponseChatgptAuthTokens: {
                    typed::ChatgptAuthTokensLoginAccountResponse result;
                    result.raw = value;
                    return {typed::LoginAccountResponse{std::move(result)}, std::nullopt};
                }
                case AccountsModelsConfigurationUnionTarget::AccountAmazonBedrock:
                case AccountsModelsConfigurationUnionTarget::AccountApiKey:
                case AccountsModelsConfigurationUnionTarget::AccountChatgpt:
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceEnterpriseManaged:
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceLegacyManagedConfigTomlFromFile:
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceLegacyManagedConfigTomlFromMdm:
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceMdm:
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceProject:
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceSessionFlags:
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceSystem:
                case AccountsModelsConfigurationUnionTarget::ConfigLayerSourceUser:
                case AccountsModelsConfigurationUnionTarget::LoginAccountParamsApiKey:
                case AccountsModelsConfigurationUnionTarget::LoginAccountParamsChatgpt:
                case AccountsModelsConfigurationUnionTarget::LoginAccountParamsChatgptAuthTokens:
                case AccountsModelsConfigurationUnionTarget::LoginAccountParamsChatgptDeviceCode:
                case AccountsModelsConfigurationUnionTarget::Count:
                    return malformedUnion<typed::LoginAccountResponse>(Surface, "$.type");
            }
            return malformedUnion<typed::LoginAccountResponse>(Surface, "$.type");
        } catch (...) {
            return malformedUnion<typed::LoginAccountResponse>("LoginAccountResponse", "$");
        }
    }

    std::optional<Json> encodeCancelLoginAccountParams(const typed::CancelLoginAccountParams& params, std::string& error) {
        error.clear();
        return std::optional<Json>{Json{{"loginId", params.loginId.value}}};
    }

    std::optional<Json> encodeLoginAccountParams(const typed::LoginAccountParams& params, std::string& error) {
        return std::visit(
            [&](const auto& alternative) -> std::optional<Json> {
                using Alternative = std::decay_t<decltype(alternative)>;
                if constexpr (std::is_same_v<Alternative, typed::UnknownLoginAccountParams>) {
                    error = "LoginAccountParams future discriminator cannot be encoded";
                    return std::nullopt;
                } else {
                    const std::string discriminator = typed::loginAccountParamsDiscriminator(params);
                    const UnionResolution resolution =
                        resolveUnion("LoginAccountParams", discriminator, ConversationUnionCodecDirection::EncodeOnly, "$.type");
                    if (resolution.entry == nullptr || resolution.descriptor == nullptr || resolution.diagnostic) {
                        error = "LoginAccountParams has no exact canonical registry/descriptor binding";
                        return std::nullopt;
                    }
                    Json result{{"type", discriminator}};
                    if constexpr (std::is_same_v<Alternative, typed::ApiKeyLoginAccountParams>) {
                        result["apiKey"] = alternative.apiKey;
                    } else if constexpr (std::is_same_v<Alternative, typed::ChatgptLoginAccountParams>) {
                        encodeOptionalNullable(result, "appBrand", alternative.appBrand, [](const auto& value) {
                            return value.value;
                        });
                        if (alternative.codexStreamlinedLogin) {
                            result["codexStreamlinedLogin"] = *alternative.codexStreamlinedLogin;
                        }
                        if (alternative.useHostedLoginSuccessPage) {
                            result["useHostedLoginSuccessPage"] = *alternative.useHostedLoginSuccessPage;
                        }
                    } else if constexpr (std::is_same_v<Alternative, typed::ChatgptAuthTokensLoginAccountParams>) {
                        result["accessToken"] = alternative.accessToken;
                        result["chatgptAccountId"] = alternative.chatgptAccountId.value;
                        encodeOptionalNullable(result, "chatgptPlanType", alternative.chatgptPlanType, [](const auto& value) {
                            return value.value;
                        });
                    }
                    error.clear();
                    return std::optional<Json>{std::move(result)};
                }
            },
            params);
    }

    std::optional<Json> encodeConsumeAccountRateLimitResetCreditParams(const typed::ConsumeAccountRateLimitResetCreditParams& params,
                                                                       std::string& error) {
        Json result{{"idempotencyKey", params.idempotencyKey.value}};
        encodeOptionalNullable(result, "creditId", params.creditId, [](const auto& value) {
            return value.value;
        });
        error.clear();
        return std::optional<Json>{std::move(result)};
    }

    std::optional<Json> encodeGetAccountParams(const typed::GetAccountParams& params, std::string& error) {
        Json result = Json::object();
        if (params.refreshToken) {
            result["refreshToken"] = *params.refreshToken;
        }
        error.clear();
        return std::optional<Json>{std::move(result)};
    }

    std::optional<Json> encodeSendAddCreditsNudgeEmailParams(const typed::SendAddCreditsNudgeEmailParams& params, std::string& error) {
        error.clear();
        return std::optional<Json>{Json{{"creditType", params.creditType.value}}};
    }

    std::optional<typed::CancelLoginAccountResponse> decodeCancelLoginAccountResponse(const Json& value, std::string& error) {
        error.clear();
        typed::CancelLoginAccountResponse result;
        if (!requireObject(value, "CancelLoginAccountResponse", error)) {
            return std::nullopt;
        }
        const Json* status = member(value, "status");
        if (status == nullptr || !decodeOpenEnum(*status, result.status, result.diagnostics, "CancelLoginAccountStatus", "$.status")) {
            fail(error, "CancelLoginAccountResponse field '$.status' has the wrong type or is missing");
            return std::nullopt;
        }
        result.raw = value;
        error.clear();
        return result;
    }

    std::optional<typed::LoginAccountResponse> decodeLoginAccountResponse(const Json& value, std::string& error) {
        error.clear();
        auto decoded = decodeLoginAccountResponseUnion(value);
        if (!decoded.value) {
            error = "LoginAccountResponse field '" + (decoded.diagnostic ? decoded.diagnostic->fieldPath : std::string("$")) +
                    "' contains a malformed known payload";
            return std::nullopt;
        }
        error.clear();
        return std::move(*decoded.value);
    }

    std::optional<typed::ConsumeAccountRateLimitResetCreditResponse> decodeConsumeAccountRateLimitResetCreditResponse(const Json& value,
                                                                                                                      std::string& error) {
        error.clear();
        typed::ConsumeAccountRateLimitResetCreditResponse result;
        if (!requireObject(value, "ConsumeAccountRateLimitResetCreditResponse", error)) {
            return std::nullopt;
        }
        const Json* outcome = member(value, "outcome");
        if (outcome == nullptr ||
            !decodeOpenEnum(*outcome, result.outcome, result.diagnostics, "ConsumeAccountRateLimitResetCreditOutcome", "$.outcome")) {
            fail(error, "ConsumeAccountRateLimitResetCreditResponse field '$.outcome' has the wrong type or is missing");
            return std::nullopt;
        }
        result.raw = value;
        error.clear();
        return result;
    }

    std::optional<typed::GetAccountRateLimitsResponse> decodeGetAccountRateLimitsResponse(const Json& value, std::string& error) {
        error.clear();
        typed::GetAccountRateLimitsResponse result;
        if (!requireObject(value, "GetAccountRateLimitsResponse", error) ||
            !decodeOptionalNullable(
                value,
                "rateLimitResetCredits",
                result.rateLimitResetCredits,
                [&](const Json& item, typed::RateLimitResetCreditsSummary& decoded) {
                    return decodeRateLimitResetCreditsSummary(item, decoded, error, "$.rateLimitResetCredits");
                },
                error,
                "GetAccountRateLimitsResponse")) {
            return std::nullopt;
        }
        const Json* rateLimits = member(value, "rateLimits");
        if (rateLimits == nullptr || !decodeRateLimitSnapshot(*rateLimits, result.rateLimits, error, "$.rateLimits")) {
            if (error.empty()) {
                fail(error, "GetAccountRateLimitsResponse field '$.rateLimits' has the wrong type or is missing");
            }
            return std::nullopt;
        }
        if (!decodeOptionalNullable(
                value,
                "rateLimitsByLimitId",
                result.rateLimitsByLimitId,
                [&](const Json& object, std::map<typed::RateLimitId, typed::RateLimitSnapshot>& decoded) {
                    if (!object.is_object()) {
                        return false;
                    }
                    for (const auto& [key, item] : object.items()) {
                        typed::RateLimitSnapshot snapshot;
                        if (!decodeRateLimitSnapshot(item, snapshot, error, "$.rateLimitsByLimitId[*]")) {
                            return false;
                        }
                        appendDiagnostics(result.diagnostics, snapshot.diagnostics);
                        decoded.emplace(typed::RateLimitId{key}, std::move(snapshot));
                    }
                    return true;
                },
                error,
                "GetAccountRateLimitsResponse")) {
            return std::nullopt;
        }
        if (result.rateLimitResetCredits.value) {
            appendDiagnostics(result.diagnostics, result.rateLimitResetCredits.value->diagnostics);
        }
        appendDiagnostics(result.diagnostics, result.rateLimits.diagnostics);
        result.raw = value;
        error.clear();
        return result;
    }

    std::optional<typed::GetAccountResponse> decodeGetAccountResponse(const Json& value, std::string& error) {
        error.clear();
        typed::GetAccountResponse result;
        if (!requireObject(value, "GetAccountResponse", error) ||
            !decodeRequired(value, "requiresOpenaiAuth", result.requiresOpenaiAuth, decodeBool, error, "GetAccountResponse")) {
            return std::nullopt;
        }
        const Json* account = member(value, "account");
        if (account == nullptr) {
            result.account = typed::OptionalNullable<typed::Account>::omitted();
        } else if (account->is_null()) {
            result.account = typed::OptionalNullable<typed::Account>::explicitNull();
        } else {
            auto decoded = decodeAccount(*account);
            if (!decoded.value) {
                std::string path = "$.account";
                if (decoded.diagnostic && decoded.diagnostic->fieldPath.starts_with("$")) {
                    path += decoded.diagnostic->fieldPath.substr(1);
                }
                fail(error, "GetAccountResponse field '" + path + "' contains a malformed known Account payload");
                return std::nullopt;
            }
            result.account = typed::OptionalNullable<typed::Account>::withValue(std::move(*decoded.value));
            if (decoded.diagnostic) {
                typed::DecodeDiagnostic diagnostic = std::move(*decoded.diagnostic);
                if (diagnostic.fieldPath.starts_with("$")) {
                    diagnostic.fieldPath = "$.account" + diagnostic.fieldPath.substr(1);
                }
                result.diagnostics.emplace_back(std::move(diagnostic));
            }
        }
        result.raw = value;
        error.clear();
        return result;
    }

    std::optional<typed::SendAddCreditsNudgeEmailResponse> decodeSendAddCreditsNudgeEmailResponse(const Json& value, std::string& error) {
        error.clear();
        typed::SendAddCreditsNudgeEmailResponse result;
        if (!requireObject(value, "SendAddCreditsNudgeEmailResponse", error)) {
            return std::nullopt;
        }
        const Json* status = member(value, "status");
        if (status == nullptr || !decodeOpenEnum(*status, result.status, result.diagnostics, "AddCreditsNudgeEmailStatus", "$.status")) {
            fail(error, "SendAddCreditsNudgeEmailResponse field '$.status' has the wrong type or is missing");
            return std::nullopt;
        }
        result.raw = value;
        error.clear();
        return result;
    }

    std::optional<typed::GetAccountTokenUsageResponse> decodeGetAccountTokenUsageResponse(const Json& value, std::string& error) {
        error.clear();
        typed::GetAccountTokenUsageResponse result;
        if (!requireObject(value, "GetAccountTokenUsageResponse", error)) {
            return std::nullopt;
        }
        if (!decodeOptionalNullable(
                value,
                "dailyUsageBuckets",
                result.dailyUsageBuckets,
                [&](const Json& array, std::vector<typed::AccountTokenUsageDailyBucket>& decoded) {
                    if (!array.is_array()) {
                        return false;
                    }
                    decoded.reserve(array.size());
                    for (std::size_t index = 0; index < array.size(); ++index) {
                        typed::AccountTokenUsageDailyBucket bucket;
                        if (!decodeAccountTokenUsageDailyBucket(
                                array[index], bucket, error, "$.dailyUsageBuckets[" + std::to_string(index) + "]")) {
                            return false;
                        }
                        decoded.emplace_back(std::move(bucket));
                    }
                    return true;
                },
                error,
                "GetAccountTokenUsageResponse")) {
            return std::nullopt;
        }
        const Json* summary = member(value, "summary");
        if (summary == nullptr || !decodeAccountTokenUsageSummary(*summary, result.summary, error, "$.summary")) {
            if (error.empty()) {
                fail(error, "GetAccountTokenUsageResponse field '$.summary' has the wrong type or is missing");
            }
            return std::nullopt;
        }
        result.raw = value;
        error.clear();
        return result;
    }

    std::optional<typed::GetWorkspaceMessagesResponse> decodeGetWorkspaceMessagesResponse(const Json& value, std::string& error) {
        error.clear();
        typed::GetWorkspaceMessagesResponse result;
        if (!requireObject(value, "GetWorkspaceMessagesResponse", error) ||
            !decodeRequired(value, "featureEnabled", result.featureEnabled, decodeBool, error, "GetWorkspaceMessagesResponse")) {
            return std::nullopt;
        }
        const Json* messages = member(value, "messages");
        if (messages == nullptr || !messages->is_array()) {
            fail(error, "GetWorkspaceMessagesResponse field '$.messages' must be an array");
            return std::nullopt;
        }
        result.messages.reserve(messages->size());
        for (std::size_t index = 0; index < messages->size(); ++index) {
            typed::WorkspaceMessage message;
            if (!decodeWorkspaceMessage((*messages)[index], message, error, "$.messages[" + std::to_string(index) + "]")) {
                return std::nullopt;
            }
            appendDiagnostics(result.diagnostics, message.diagnostics);
            result.messages.emplace_back(std::move(message));
        }
        result.raw = value;
        error.clear();
        return result;
    }

    std::optional<typed::AccountLoginCompletedNotification> decodeAccountLoginCompletedNotification(const Notification& notification,
                                                                                                    std::string& error) {
        error.clear();
        typed::AccountLoginCompletedNotification result;
        if (!requireObject(notification.params, "account/login/completed params", error, "$.params") ||
            !decodeOptionalNullable(
                notification.params, "error", result.error, decodeString, error, "account/login/completed", "$.params") ||
            !decodeOptionalNullable(notification.params,
                                    "loginId",
                                    result.loginId,
                                    decodeStrongString<typed::LoginId>,
                                    error,
                                    "account/login/completed",
                                    "$.params") ||
            !decodeRequired(notification.params, "success", result.success, decodeBool, error, "account/login/completed", "$.params")) {
            return std::nullopt;
        }
        result.raw = notification.raw;
        error.clear();
        return result;
    }

    std::optional<typed::AccountRateLimitsUpdatedNotification> decodeAccountRateLimitsUpdatedNotification(const Notification& notification,
                                                                                                          std::string& error) {
        error.clear();
        typed::AccountRateLimitsUpdatedNotification result;
        if (!requireObject(notification.params, "account/rateLimits/updated params", error, "$.params")) {
            return std::nullopt;
        }
        const Json* rateLimits = member(notification.params, "rateLimits");
        if (rateLimits == nullptr || !decodeRateLimitSnapshot(*rateLimits, result.rateLimits, error, "$.params.rateLimits")) {
            if (error.empty()) {
                fail(error, "account/rateLimits/updated field '$.params.rateLimits' has the wrong type or is missing");
            }
            return std::nullopt;
        }
        appendDiagnostics(result.diagnostics, result.rateLimits.diagnostics);
        result.raw = notification.raw;
        error.clear();
        return result;
    }

    std::optional<typed::AccountUpdatedNotification> decodeAccountUpdatedNotification(const Notification& notification,
                                                                                      std::string& error) {
        error.clear();
        typed::AccountUpdatedNotification result;
        if (!requireObject(notification.params, "account/updated params", error, "$.params") ||
            !decodeOptionalNullableOpenEnum(notification.params,
                                            "authMode",
                                            result.authMode,
                                            result.diagnostics,
                                            "AuthMode",
                                            "$.params.authMode",
                                            error,
                                            "account/updated") ||
            !decodeOptionalNullableOpenEnum(notification.params,
                                            "planType",
                                            result.planType,
                                            result.diagnostics,
                                            "PlanType",
                                            "$.params.planType",
                                            error,
                                            "account/updated")) {
            return std::nullopt;
        }
        result.raw = notification.raw;
        error.clear();
        return result;
    }

    std::optional<typed::ChatgptAuthTokensRefreshParams> decodeChatgptAuthTokensRefreshParams(const Json& value, std::string& error) {
        error.clear();
        typed::ChatgptAuthTokensRefreshParams result;
        if (!serverRequestDescriptorMatches()) {
            fail(error, "account/chatgptAuthTokens/refresh has no exact canonical registry/descriptor binding");
            return std::nullopt;
        }
        if (!requireObject(value, "account/chatgptAuthTokens/refresh params", error, "$.params") ||
            !decodeOptionalNullable(value,
                                    "previousAccountId",
                                    result.previousAccountId,
                                    decodeStrongString<typed::AccountId>,
                                    error,
                                    "account/chatgptAuthTokens/refresh",
                                    "$.params")) {
            return std::nullopt;
        }
        const Json* reason = member(value, "reason");
        if (reason == nullptr ||
            !decodeOpenEnum(*reason, result.reason, result.diagnostics, "ChatgptAuthTokensRefreshReason", "$.params.reason")) {
            fail(error, "account/chatgptAuthTokens/refresh field '$.params.reason' has the wrong type or is missing");
            return std::nullopt;
        }
        result.raw = value;
        error.clear();
        return result;
    }

    std::optional<Json> encodeChatgptAuthTokensRefreshResponse(typed::ChatgptAuthTokensRefreshResponse response, std::string& error) {
        if (!serverRequestDescriptorMatches()) {
            fail(error, "account/chatgptAuthTokens/refresh has no exact canonical registry/descriptor binding");
            return std::nullopt;
        }
        Json result{{"accessToken", std::move(response.accessToken)}, {"chatgptAccountId", std::move(response.chatgptAccountId.value)}};
        encodeOptionalNullable(result, "chatgptPlanType", response.chatgptPlanType, [](const auto& value) {
            return value.value;
        });
        error.clear();
        return std::optional<Json>{std::move(result)};
    }

} // namespace ai::openai::codex::detail
