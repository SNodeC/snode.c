/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ModelCodec.h"

#include "ai/openai/codex/detail/DecodeDiagnostic.h"
#include "ai/openai/codex/typed/Types.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <utility>
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

        std::string indexedPath(std::string_view base, std::size_t index) {
            return std::string(base) + "[" + std::to_string(index) + "]";
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

        template <typename Strong>
        bool decodeStrongString(const Json& value, Strong& result) {
            return decodeString(value, result.value);
        }

        template <typename OpenEnum>
        bool decodeOpenEnum(const Json& value,
                            OpenEnum& result,
                            std::vector<typed::DecodeDiagnostic>& diagnostics,
                            std::string_view surface,
                            std::string path,
                            bool requireNonEmpty = false) {
            if (!decodeString(value, result.value) || (requireNonEmpty && result.value.empty())) {
                return false;
            }
            if (!result.isKnown()) {
                diagnostics.emplace_back(unknownEnumDiagnostic(std::string(surface), std::move(path)));
            }
            return true;
        }

        void appendDiagnostics(std::vector<typed::DecodeDiagnostic>& target, const std::vector<typed::DecodeDiagnostic>& source) {
            target.insert(target.end(), source.begin(), source.end());
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

        template <typename T, typename Decode>
        bool decodeArray(const Json& value,
                         std::vector<T>& result,
                         Decode&& decode,
                         std::string& error,
                         std::string_view context,
                         std::string_view path) {
            if (!value.is_array()) {
                return fail(error, std::string(context) + " field '" + std::string(path) + "' must be an array");
            }
            result.clear();
            result.reserve(value.size());
            for (std::size_t index = 0; index < value.size(); ++index) {
                T decoded;
                const std::string itemPath = indexedPath(path, index);
                if (!decode(value[index], decoded, itemPath)) {
                    if (!error.empty()) {
                        return false;
                    }
                    return fail(error, std::string(context) + " field '" + itemPath + "' has the wrong type");
                }
                result.emplace_back(std::move(decoded));
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

        bool decodeModelAvailabilityNux(const Json& value,
                                        typed::ModelAvailabilityNux& result,
                                        std::string& error,
                                        std::string_view path) {
            if (!requireObject(value, "ModelAvailabilityNux", error, path) ||
                !decodeRequired(value, "message", result.message, decodeString, error, "ModelAvailabilityNux", path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool decodeModelServiceTier(const Json& value,
                                    typed::ModelServiceTier& result,
                                    std::string& error,
                                    std::string_view path) {
            if (!requireObject(value, "ModelServiceTier", error, path) ||
                !decodeRequired(value, "description", result.description, decodeString, error, "ModelServiceTier", path) ||
                !decodeRequired(
                    value, "id", result.id, decodeStrongString<typed::ModelServiceTierId>, error, "ModelServiceTier", path) ||
                !decodeRequired(value, "name", result.name, decodeString, error, "ModelServiceTier", path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool decodeModelUpgradeInfo(const Json& value,
                                    typed::ModelUpgradeInfo& result,
                                    std::string& error,
                                    std::string_view path) {
            if (!requireObject(value, "ModelUpgradeInfo", error, path) ||
                !decodeOptionalNullable(
                    value, "migrationMarkdown", result.migrationMarkdown, decodeString, error, "ModelUpgradeInfo", path) ||
                !decodeRequired(value, "model", result.model, decodeStrongString<typed::ModelId>, error, "ModelUpgradeInfo", path) ||
                !decodeOptionalNullable(value, "modelLink", result.modelLink, decodeString, error, "ModelUpgradeInfo", path) ||
                !decodeOptionalNullable(value, "upgradeCopy", result.upgradeCopy, decodeString, error, "ModelUpgradeInfo", path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool decodeReasoningEffortOption(const Json& value,
                                         typed::ReasoningEffortOption& result,
                                         std::string& error,
                                         std::string_view path) {
            if (!requireObject(value, "ReasoningEffortOption", error, path) ||
                !decodeRequired(value, "description", result.description, decodeString, error, "ReasoningEffortOption", path)) {
                return false;
            }
            const Json* effort = member(value, "reasoningEffort");
            const std::string effortPath = fieldPath(path, "reasoningEffort");
            if (effort == nullptr ||
                !decodeOpenEnum(
                    *effort, result.reasoningEffort, result.diagnostics, "ReasoningEffort", effortPath, true)) {
                return fail(error, "ReasoningEffortOption field '" + effortPath + "' has the wrong type or is missing");
            }
            result.raw = value;
            return true;
        }

        bool decodeModel(const Json& value, typed::Model& result, std::string& error, std::string_view path) {
            if (!requireObject(value, "Model", error, path)) {
                return false;
            }

            if (const Json* additionalSpeedTiers = member(value, "additionalSpeedTiers")) {
                if (!decodeArray(
                        *additionalSpeedTiers,
                        result.additionalSpeedTiers,
                        [](const Json& item, typed::ModelServiceTierId& decoded, const std::string&) {
                            return decodeStrongString(item, decoded);
                        },
                        error,
                        "Model",
                        fieldPath(path, "additionalSpeedTiers"))) {
                    return false;
                }
            }

            if (!decodeOptionalNullable(
                    value,
                    "availabilityNux",
                    result.availabilityNux,
                    [&](const Json& input, typed::ModelAvailabilityNux& decoded) {
                        return decodeModelAvailabilityNux(input, decoded, error, fieldPath(path, "availabilityNux"));
                    },
                    error,
                    "Model",
                    path)) {
                return false;
            }

            const Json* defaultReasoningEffort = member(value, "defaultReasoningEffort");
            const std::string defaultReasoningEffortPath = fieldPath(path, "defaultReasoningEffort");
            if (defaultReasoningEffort == nullptr ||
                !decodeOpenEnum(*defaultReasoningEffort,
                                result.defaultReasoningEffort,
                                result.diagnostics,
                                "ReasoningEffort",
                                defaultReasoningEffortPath,
                                true)) {
                return fail(error, "Model field '" + defaultReasoningEffortPath + "' has the wrong type or is missing");
            }

            if (!decodeOptionalNullable(value,
                                        "defaultServiceTier",
                                        result.defaultServiceTier,
                                        decodeStrongString<typed::ModelServiceTierId>,
                                        error,
                                        "Model",
                                        path) ||
                !decodeRequired(value, "description", result.description, decodeString, error, "Model", path) ||
                !decodeRequired(value, "displayName", result.displayName, decodeString, error, "Model", path) ||
                !decodeRequired(value, "hidden", result.hidden, decodeBool, error, "Model", path) ||
                !decodeRequired(value, "id", result.id, decodeStrongString<typed::ModelId>, error, "Model", path)) {
                return false;
            }

            if (const Json* inputModalities = member(value, "inputModalities")) {
                const std::string modalitiesPath = fieldPath(path, "inputModalities");
                if (!decodeArray(
                        *inputModalities,
                        result.inputModalities,
                        [&](const Json& item, typed::InputModality& decoded, const std::string& itemPath) {
                            return decodeOpenEnum(item, decoded, result.diagnostics, "InputModality", itemPath);
                        },
                        error,
                        "Model",
                        modalitiesPath)) {
                    return false;
                }
            }

            if (!decodeRequired(value, "isDefault", result.isDefault, decodeBool, error, "Model", path) ||
                !decodeRequired(value, "model", result.model, decodeStrongString<typed::ModelId>, error, "Model", path)) {
                return false;
            }

            if (const Json* serviceTiers = member(value, "serviceTiers")) {
                const std::string tiersPath = fieldPath(path, "serviceTiers");
                if (!decodeArray(
                        *serviceTiers,
                        result.serviceTiers,
                        [&](const Json& item, typed::ModelServiceTier& decoded, const std::string& itemPath) {
                            if (!decodeModelServiceTier(item, decoded, error, itemPath)) {
                                return false;
                            }
                            appendDiagnostics(result.diagnostics, decoded.diagnostics);
                            return true;
                        },
                        error,
                        "Model",
                        tiersPath)) {
                    return false;
                }
            }

            const Json* supportedReasoningEfforts = member(value, "supportedReasoningEfforts");
            if (supportedReasoningEfforts == nullptr ||
                !decodeArray(
                    *supportedReasoningEfforts,
                    result.supportedReasoningEfforts,
                    [&](const Json& item, typed::ReasoningEffortOption& decoded, const std::string& itemPath) {
                        if (!decodeReasoningEffortOption(item, decoded, error, itemPath)) {
                            return false;
                        }
                        appendDiagnostics(result.diagnostics, decoded.diagnostics);
                        return true;
                    },
                    error,
                    "Model",
                    fieldPath(path, "supportedReasoningEfforts"))) {
                if (error.empty()) {
                    fail(error,
                         "Model field '" + fieldPath(path, "supportedReasoningEfforts") + "' has the wrong type or is missing");
                }
                return false;
            }

            if (const Json* supportsPersonality = member(value, "supportsPersonality")) {
                if (!decodeBool(*supportsPersonality, result.supportsPersonality)) {
                    return fail(error, "Model field '" + fieldPath(path, "supportsPersonality") + "' has the wrong type");
                }
            }

            if (!decodeOptionalNullable(value, "upgrade", result.upgrade, decodeString, error, "Model", path) ||
                !decodeOptionalNullable(
                    value,
                    "upgradeInfo",
                    result.upgradeInfo,
                    [&](const Json& input, typed::ModelUpgradeInfo& decoded) {
                        return decodeModelUpgradeInfo(input, decoded, error, fieldPath(path, "upgradeInfo"));
                    },
                    error,
                    "Model",
                    path)) {
                return false;
            }

            if (result.availabilityNux.value) {
                appendDiagnostics(result.diagnostics, result.availabilityNux.value->diagnostics);
            }
            if (result.upgradeInfo.value) {
                appendDiagnostics(result.diagnostics, result.upgradeInfo.value->diagnostics);
            }
            result.raw = value;
            return true;
        }

        bool decodeRequiredStringArray(const Json& object,
                                       std::string_view name,
                                       std::vector<std::string>& result,
                                       std::string& error,
                                       std::string_view context,
                                       std::string_view basePath) {
            const Json* value = member(object, name);
            const std::string path = fieldPath(basePath, name);
            if (value == nullptr ||
                !decodeArray(
                    *value,
                    result,
                    [](const Json& item, std::string& decoded, const std::string&) {
                        return decodeString(item, decoded);
                    },
                    error,
                    context,
                    path)) {
                if (error.empty()) {
                    fail(error, std::string(context) + " field '" + path + "' has the wrong type or is missing");
                }
                return false;
            }
            return true;
        }

    } // namespace

    std::optional<Json> encodeModelListParams(const typed::ModelListParams& params, std::string& error) {
        error.clear();
        Json result = Json::object();
        encodeOptionalNullable(result, "cursor", params.cursor, [](const std::string& value) {
            return Json(value);
        });
        encodeOptionalNullable(result, "includeHidden", params.includeHidden, [](bool value) {
            return Json(value);
        });
        encodeOptionalNullable(result, "limit", params.limit, [](std::uint32_t value) {
            return Json(value);
        });
        return std::optional<Json>{std::move(result)};
    }

    std::optional<Json>
    encodeModelProviderCapabilitiesReadParams(const typed::ModelProviderCapabilitiesReadParams&, std::string& error) {
        error.clear();
        return std::optional<Json>{Json::object()};
    }

    std::optional<typed::ModelListResponse> decodeModelListResponse(const Json& value, std::string& error) {
        error.clear();
        typed::ModelListResponse result;
        const Json* data = member(value, "data");
        if (!requireObject(value, "ModelListResponse", error) || data == nullptr ||
            !decodeArray(
                *data,
                result.data,
                [&](const Json& item, typed::Model& decoded, const std::string& itemPath) {
                    if (!decodeModel(item, decoded, error, itemPath)) {
                        return false;
                    }
                    appendDiagnostics(result.diagnostics, decoded.diagnostics);
                    return true;
                },
                error,
                "ModelListResponse",
                "$.data") ||
            !decodeOptionalNullable(
                value, "nextCursor", result.nextCursor, decodeString, error, "ModelListResponse")) {
            if (error.empty()) {
                fail(error, "ModelListResponse field '$.data' has the wrong type or is missing");
            }
            return std::nullopt;
        }
        result.raw = value;
        error.clear();
        return result;
    }

    std::optional<typed::ModelProviderCapabilitiesReadResponse>
    decodeModelProviderCapabilitiesReadResponse(const Json& value, std::string& error) {
        error.clear();
        typed::ModelProviderCapabilitiesReadResponse result;
        if (!requireObject(value, "ModelProviderCapabilitiesReadResponse", error) ||
            !decodeRequired(
                value, "imageGeneration", result.imageGeneration, decodeBool, error, "ModelProviderCapabilitiesReadResponse") ||
            !decodeRequired(
                value, "namespaceTools", result.namespaceTools, decodeBool, error, "ModelProviderCapabilitiesReadResponse") ||
            !decodeRequired(value, "webSearch", result.webSearch, decodeBool, error, "ModelProviderCapabilitiesReadResponse")) {
            return std::nullopt;
        }
        result.raw = value;
        error.clear();
        return result;
    }

    std::optional<typed::ModelReroutedNotification>
    decodeModelReroutedNotification(const Notification& notification, std::string& error) {
        error.clear();
        typed::ModelReroutedNotification result;
        if (!requireObject(notification.params, "ModelReroutedNotification", error, "$.params") ||
            !decodeRequired(notification.params,
                            "fromModel",
                            result.fromModel,
                            decodeStrongString<typed::ModelId>,
                            error,
                            "ModelReroutedNotification",
                            "$.params")) {
            return std::nullopt;
        }

        const Json* reason = member(notification.params, "reason");
        if (reason == nullptr ||
            !decodeOpenEnum(*reason, result.reason, result.diagnostics, "ModelRerouteReason", "$.params.reason")) {
            fail(error, "ModelReroutedNotification field '$.params.reason' has the wrong type or is missing");
            return std::nullopt;
        }

        if (!decodeRequired(notification.params,
                            "threadId",
                            result.threadId,
                            decodeStrongString<typed::ThreadId>,
                            error,
                            "ModelReroutedNotification",
                            "$.params") ||
            !decodeRequired(notification.params,
                            "toModel",
                            result.toModel,
                            decodeStrongString<typed::ModelId>,
                            error,
                            "ModelReroutedNotification",
                            "$.params") ||
            !decodeRequired(notification.params,
                            "turnId",
                            result.turnId,
                            decodeStrongString<typed::TurnId>,
                            error,
                            "ModelReroutedNotification",
                            "$.params")) {
            return std::nullopt;
        }
        result.raw = notification.raw;
        error.clear();
        return result;
    }

    std::optional<typed::ModelSafetyBufferingUpdatedNotification>
    decodeModelSafetyBufferingUpdatedNotification(const Notification& notification, std::string& error) {
        error.clear();
        typed::ModelSafetyBufferingUpdatedNotification result;
        if (!requireObject(notification.params, "ModelSafetyBufferingUpdatedNotification", error, "$.params") ||
            !decodeOptionalNullable(notification.params,
                                    "fasterModel",
                                    result.fasterModel,
                                    decodeStrongString<typed::ModelId>,
                                    error,
                                    "ModelSafetyBufferingUpdatedNotification",
                                    "$.params") ||
            !decodeRequired(notification.params,
                            "model",
                            result.model,
                            decodeStrongString<typed::ModelId>,
                            error,
                            "ModelSafetyBufferingUpdatedNotification",
                            "$.params") ||
            !decodeRequiredStringArray(
                notification.params, "reasons", result.reasons, error, "ModelSafetyBufferingUpdatedNotification", "$.params") ||
            !decodeRequired(notification.params,
                            "showBufferingUi",
                            result.showBufferingUi,
                            decodeBool,
                            error,
                            "ModelSafetyBufferingUpdatedNotification",
                            "$.params") ||
            !decodeRequired(notification.params,
                            "threadId",
                            result.threadId,
                            decodeStrongString<typed::ThreadId>,
                            error,
                            "ModelSafetyBufferingUpdatedNotification",
                            "$.params") ||
            !decodeRequired(notification.params,
                            "turnId",
                            result.turnId,
                            decodeStrongString<typed::TurnId>,
                            error,
                            "ModelSafetyBufferingUpdatedNotification",
                            "$.params") ||
            !decodeRequiredStringArray(
                notification.params, "useCases", result.useCases, error, "ModelSafetyBufferingUpdatedNotification", "$.params")) {
            return std::nullopt;
        }
        result.raw = notification.raw;
        error.clear();
        return result;
    }

    std::optional<typed::ModelVerificationNotification>
    decodeModelVerificationNotification(const Notification& notification, std::string& error) {
        error.clear();
        typed::ModelVerificationNotification result;
        if (!requireObject(notification.params, "ModelVerificationNotification", error, "$.params") ||
            !decodeRequired(notification.params,
                            "threadId",
                            result.threadId,
                            decodeStrongString<typed::ThreadId>,
                            error,
                            "ModelVerificationNotification",
                            "$.params") ||
            !decodeRequired(notification.params,
                            "turnId",
                            result.turnId,
                            decodeStrongString<typed::TurnId>,
                            error,
                            "ModelVerificationNotification",
                            "$.params")) {
            return std::nullopt;
        }

        const Json* verifications = member(notification.params, "verifications");
        if (verifications == nullptr ||
            !decodeArray(
                *verifications,
                result.verifications,
                [&](const Json& item, typed::ModelVerification& decoded, const std::string& itemPath) {
                    return decodeOpenEnum(item, decoded, result.diagnostics, "ModelVerification", itemPath);
                },
                error,
                "ModelVerificationNotification",
                "$.params.verifications")) {
            if (error.empty()) {
                fail(error, "ModelVerificationNotification field '$.params.verifications' has the wrong type or is missing");
            }
            return std::nullopt;
        }
        result.raw = notification.raw;
        error.clear();
        return result;
    }

} // namespace ai::openai::codex::detail
