/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ClientOperationCodec.h"
#include "ai/openai/codex/detail/ModelCodec.h"
#include "ai/openai/codex/detail/EventDecoder.h"
#include "ai/openai/codex/typed/Models.h"
#include "support/TestResult.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace {
    namespace codex = ai::openai::codex;
    namespace detail = ai::openai::codex::detail;
    namespace typed = ai::openai::codex::typed;

    bool hasDiagnostic(const std::vector<typed::DecodeDiagnostic>& diagnostics,
                       typed::DecodeIssueKind kind,
                       typed::DecodeIssueSeverity severity,
                       std::string_view surface,
                       std::string_view path) {
        for (const typed::DecodeDiagnostic& diagnostic : diagnostics) {
            if (diagnostic.kind == kind && diagnostic.severity == severity && diagnostic.surface == surface &&
                diagnostic.fieldPath == path) {
                return true;
            }
        }
        return false;
    }

    bool hasMalformedEventDiagnostic(const typed::Event& event,
                                     const codex::Notification& notification,
                                     std::string_view path) {
        const auto* unknown = std::get_if<typed::UnknownEvent>(&event);
        return unknown && unknown->method == notification.method &&
               unknown->params == notification.params &&
               unknown->raw == notification.raw && unknown->decodingError &&
               unknown->diagnostic &&
               unknown->diagnostic->kind == typed::DecodeIssueKind::MalformedKnownPayload &&
               unknown->diagnostic->severity == typed::DecodeIssueSeverity::ProtocolWarning &&
               unknown->diagnostic->surface == notification.method &&
               unknown->diagnostic->fieldPath == path;
    }

    codex::Notification notification(std::string method, codex::Json params) {
        codex::Json raw{
            {"jsonrpc", "2.0"},
            {"method", method},
            {"params", params},
            {"futureEnvelopeOnly", true},
        };
        return {std::move(method), std::move(params), std::move(raw)};
    }

    codex::Json requiredModel() {
        return {
            {"defaultReasoningEffort", "medium"},
            {"description", "A complete synthetic model"},
            {"displayName", "Synthetic Model"},
            {"hidden", false},
            {"id", "model-id"},
            {"isDefault", true},
            {"model", "model-wire"},
            {"supportedReasoningEfforts",
             codex::Json::array(
                 {{{"description", "Balanced"}, {"reasoningEffort", "medium"}}})},
        };
    }

    void testOpenStringEnums(tests::support::TestResult& result) {
        result.expectTrue(typed::InputModality::text().value == "text" &&
                              typed::InputModality::text().isKnown() &&
                              typed::InputModality::image().value == "image" &&
                              typed::InputModality::image().isKnown(),
                          "model input-modality known factories are exact");
        result.expectTrue(typed::ModelRerouteReason::highRiskCyberActivity().value == "highRiskCyberActivity" &&
                              typed::ModelRerouteReason::highRiskCyberActivity().isKnown(),
                          "model reroute-reason known factory is exact");
        result.expectTrue(typed::ModelVerification::trustedAccessForCyber().value == "trustedAccessForCyber" &&
                              typed::ModelVerification::trustedAccessForCyber().isKnown(),
                          "model-verification known factory is exact");
        result.expectTrue(!typed::InputModality{"futureModality"}.isKnown() &&
                              !typed::ModelRerouteReason{"futureReroute"}.isKnown() &&
                              !typed::ModelVerification{"futureVerification"}.isKnown(),
                          "all B3 string domains remain open to future values");
    }

    void testRequestEncoding(tests::support::TestResult& result) {
        std::string error = "stale";
        const auto omitted = detail::encodeModelListParams({}, error);
        result.expectTrue(omitted == codex::Json::object() && error.empty(),
                          "model/list omits all omitted pagination fields");

        typed::ModelListParams nulls{
            .cursor = typed::OptionalNullable<std::string>::explicitNull(),
            .includeHidden = typed::OptionalNullable<bool>::explicitNull(),
            .limit = typed::OptionalNullable<std::uint32_t>::explicitNull(),
        };
        const auto encodedNulls = detail::encodeModelListParams(nulls, error);
        result.expectTrue(encodedNulls ==
                                  codex::Json{{"cursor", nullptr}, {"includeHidden", nullptr}, {"limit", nullptr}},
                          "model/list distinguishes explicit null from omission for every parameter");

        typed::ModelListParams values{
            .cursor = typed::OptionalNullable<std::string>::withValue("cursor-wire"),
            .includeHidden = typed::OptionalNullable<bool>::withValue(false),
            .limit =
                typed::OptionalNullable<std::uint32_t>::withValue(std::numeric_limits<std::uint32_t>::max()),
        };
        const auto encodedValues = detail::encodeModelListParams(values, error);
        result.expectTrue(encodedValues ==
                                  codex::Json{{"cursor", "cursor-wire"},
                                              {"includeHidden", false},
                                              {"limit", std::numeric_limits<std::uint32_t>::max()}},
                          "model/list encodes false and the uint32 boundary without coercion");

        values.limit = typed::OptionalNullable<std::uint32_t>::withValue(0);
        const auto encodedZero = detail::encodeModelListParams(values, error);
        result.expectTrue(encodedZero && encodedZero->at("limit") == 0,
                          "model/list preserves the exact zero uint32 boundary");

        const auto provider = detail::encodeModelProviderCapabilitiesReadParams({}, error);
        result.expectTrue(provider == codex::Json::object() && error.empty(),
                          "modelProvider/capabilities/read encodes its reviewed empty-object params");
    }

    void testModelListResponse(tests::support::TestResult& result) {
        codex::Json model = requiredModel();
        model.update(
            {{"additionalSpeedTiers", codex::Json::array({"priority"})},
             {"availabilityNux", {{"message", "Now available"}, {"futureNux", 1}}},
             {"defaultReasoningEffort", "future-effort"},
             {"defaultServiceTier", "priority"},
             {"inputModalities", codex::Json::array({"text", "image", "futureModality"})},
             {"serviceTiers",
              codex::Json::array(
                  {{{"description", "Priority service"}, {"id", "priority"}, {"name", "Priority"}, {"futureTier", 2}}})},
             {"supportedReasoningEfforts",
              codex::Json::array(
                  {{{"description", "Future reasoning"}, {"reasoningEffort", "future-effort"}}})},
             {"supportsPersonality", true},
             {"upgrade", nullptr},
             {"upgradeInfo",
              {{"migrationMarkdown", nullptr},
               {"model", "model-upgrade"},
               {"modelLink", "https://example.test/model"},
               {"upgradeCopy", "Upgrade"},
               {"futureUpgrade", 3}}},
             {"futureModelField", {{"retained", true}}}});
        const codex::Json wire{
            {"data", codex::Json::array({model})},
            {"nextCursor", "next-wire"},
            {"futureResultField", true},
        };

        std::string error = "stale";
        const auto decoded = detail::decodeModelListResponse(wire, error);
        const typed::Model* value = decoded && decoded->data.size() == 1 ? &decoded->data.front() : nullptr;
        result.expectTrue(
            value && decoded->raw == wire && error.empty() &&
                decoded->nextCursor.hasValue() && decoded->nextCursor.value.value() == "next-wire" &&
                value->additionalSpeedTiers == std::vector<typed::ModelServiceTierId>{{"priority"}} &&
                value->availabilityNux.hasValue() && value->availabilityNux.value->message == "Now available" &&
                value->availabilityNux.value->raw == model.at("availabilityNux") &&
                value->defaultReasoningEffort.value == "future-effort" &&
                value->defaultServiceTier.hasValue() && value->defaultServiceTier.value->value == "priority" &&
                value->id.value == "model-id" && value->model.value == "model-wire" &&
                value->inputModalities.size() == 3 && value->inputModalities[2].value == "futureModality" &&
                value->serviceTiers.size() == 1 && value->serviceTiers[0].id.value == "priority" &&
                value->supportedReasoningEfforts.size() == 1 &&
                value->supportedReasoningEfforts[0].reasoningEffort.value == "future-effort" &&
                value->supportsPersonality && value->upgrade.isNull() && value->upgradeInfo.hasValue() &&
                value->upgradeInfo.value->migrationMarkdown.isNull() &&
                value->upgradeInfo.value->model.value == "model-upgrade" &&
                value->upgradeInfo.value->modelLink.hasValue() &&
                value->upgradeInfo.value->upgradeCopy.hasValue() && value->raw == model,
            "model/list decodes the full nested result, strong identifiers, tri-state fields, and exact raw JSON");

        result.expectTrue(
            decoded &&
                hasDiagnostic(decoded->diagnostics,
                              typed::DecodeIssueKind::UnknownEnumValue,
                              typed::DecodeIssueSeverity::ForwardCompatibility,
                              "ReasoningEffort",
                              "$.data[0].defaultReasoningEffort") &&
                hasDiagnostic(decoded->diagnostics,
                              typed::DecodeIssueKind::UnknownEnumValue,
                              typed::DecodeIssueSeverity::ForwardCompatibility,
                              "InputModality",
                              "$.data[0].inputModalities[2]") &&
                hasDiagnostic(decoded->diagnostics,
                              typed::DecodeIssueKind::UnknownEnumValue,
                              typed::DecodeIssueSeverity::ForwardCompatibility,
                              "ReasoningEffort",
                              "$.data[0].supportedReasoningEfforts[0].reasoningEffort"),
            "future nested model enums retain the known outer model and emit exact forward-compatibility paths");

        const codex::Json minimalWire{{"data", codex::Json::array({requiredModel()})}};
        const auto minimal = detail::decodeModelListResponse(minimalWire, error);
        const typed::Model* minimalModel =
            minimal && minimal->data.size() == 1 ? &minimal->data.front() : nullptr;
        result.expectTrue(
            minimalModel && minimal->nextCursor.isOmitted() &&
                minimalModel->additionalSpeedTiers.empty() &&
                minimalModel->availabilityNux.isOmitted() &&
                minimalModel->defaultServiceTier.isOmitted() &&
                minimalModel->inputModalities ==
                    std::vector<typed::InputModality>{typed::InputModality::text(), typed::InputModality::image()} &&
                minimalModel->serviceTiers.empty() && !minimalModel->supportsPersonality &&
                minimalModel->upgrade.isOmitted() && minimalModel->upgradeInfo.isOmitted(),
            "omitted optional/defaulted model fields retain the schema defaults without collapsing tri-state values");

        codex::Json nullModel = requiredModel();
        nullModel.update(
            {{"availabilityNux", nullptr},
             {"defaultServiceTier", nullptr},
             {"upgrade", nullptr},
             {"upgradeInfo", nullptr}});
        const auto explicitNulls = detail::decodeModelListResponse(
            {{"data", codex::Json::array({nullModel})}, {"nextCursor", nullptr}}, error);
        const typed::Model* nullValue =
            explicitNulls && explicitNulls->data.size() == 1 ? &explicitNulls->data.front() : nullptr;
        result.expectTrue(nullValue && explicitNulls->nextCursor.isNull() &&
                              nullValue->availabilityNux.isNull() &&
                              nullValue->defaultServiceTier.isNull() && nullValue->upgrade.isNull() &&
                              nullValue->upgradeInfo.isNull(),
                          "all nullable model/result paths preserve the explicit-null state");

        codex::Json emptyArraysModel = requiredModel();
        emptyArraysModel["additionalSpeedTiers"] = codex::Json::array();
        emptyArraysModel["inputModalities"] = codex::Json::array();
        emptyArraysModel["serviceTiers"] = codex::Json::array();
        emptyArraysModel["supportedReasoningEfforts"] = codex::Json::array();
        const auto explicitEmptyArrays = detail::decodeModelListResponse(
            {{"data", codex::Json::array({emptyArraysModel})}}, error);
        const typed::Model* emptyArraysValue =
            explicitEmptyArrays && explicitEmptyArrays->data.size() == 1
                ? &explicitEmptyArrays->data.front()
                : nullptr;
        const auto emptyData = detail::decodeModelListResponse(
            {{"data", codex::Json::array()}}, error);
        result.expectTrue(
            emptyArraysValue && emptyArraysValue->additionalSpeedTiers.empty() &&
                emptyArraysValue->inputModalities.empty() &&
                emptyArraysValue->serviceTiers.empty() &&
                emptyArraysValue->supportedReasoningEfforts.empty() &&
                emptyData && emptyData->data.empty(),
            "model/list preserves every explicitly present empty array without substituting schema defaults");
    }

    void testMalformedResults(tests::support::TestResult& result) {
        std::string error;
        const auto missingData = detail::decodeModelListResponse(codex::Json::object(), error);
        result.expectTrue(!missingData && error.find("$.data") != std::string::npos,
                          "model/list rejects missing required data at the stable field path");

        codex::Json missingNested = requiredModel();
        missingNested.erase("id");
        const auto malformedNested =
            detail::decodeModelListResponse({{"data", codex::Json::array({missingNested})}}, error);
        result.expectTrue(!malformedNested && error.find("$.data[0].id") != std::string::npos,
                          "model/list rejects a missing required nested model identifier");

        codex::Json emptyEffort = requiredModel();
        emptyEffort["defaultReasoningEffort"] = "";
        const auto malformedEffort =
            detail::decodeModelListResponse({{"data", codex::Json::array({emptyEffort})}}, error);
        result.expectTrue(!malformedEffort && error.find("$.data[0].defaultReasoningEffort") != std::string::npos,
                          "model/list enforces the non-empty ReasoningEffort scalar alias");

        codex::Json wrongModality = requiredModel();
        wrongModality["inputModalities"] = codex::Json::array({1});
        const auto malformedModality =
            detail::decodeModelListResponse({{"data", codex::Json::array({wrongModality})}}, error);
        result.expectTrue(!malformedModality && error.find("$.data[0].inputModalities[0]") != std::string::npos,
                          "model/list rejects a wrong primitive in its nested modality array");

        const auto missingProvider =
            detail::decodeModelProviderCapabilitiesReadResponse({{"imageGeneration", true}, {"webSearch", false}}, error);
        result.expectTrue(!missingProvider && error.find("$.namespaceTools") != std::string::npos,
                          "provider capabilities reject a missing required Boolean");
        const auto wrongProvider = detail::decodeModelProviderCapabilitiesReadResponse(
            {{"imageGeneration", true}, {"namespaceTools", "false"}, {"webSearch", false}}, error);
        result.expectTrue(!wrongProvider && error.find("$.namespaceTools") != std::string::npos,
                          "provider capabilities reject a wrong Boolean primitive");

        const auto malformedModelOperation = detail::decodeClientOperationResult(
            detail::ClientRequestTarget::ModelList,
            {{"data", codex::Json::array({missingNested})}});
        const detail::ProtocolSurfaceKey expectedModelKey =
            detail::entryFor(detail::ClientRequestTarget::ModelList).key;
        result.expectTrue(
            !malformedModelOperation &&
                malformedModelOperation.diagnostic.code ==
                    detail::ClientOperationDecodeCode::MalformedKnownPayload &&
                malformedModelOperation.diagnostic.target ==
                    detail::ClientRequestTarget::ModelList &&
                malformedModelOperation.diagnostic.surfaceKey == expectedModelKey &&
                expectedModelKey.name == "model/list" &&
                malformedModelOperation.diagnostic.fieldPath == "$.data[0].id",
            "model/list malformed results traverse the exact registry target and emit the exact structured diagnostic");

        const auto malformedProviderOperation = detail::decodeClientOperationResult(
            detail::ClientRequestTarget::ModelProviderCapabilitiesRead,
            {{"imageGeneration", true},
             {"namespaceTools", "false"},
             {"webSearch", false}});
        const detail::ProtocolSurfaceKey expectedProviderKey =
            detail::entryFor(
                detail::ClientRequestTarget::ModelProviderCapabilitiesRead)
                .key;
        result.expectTrue(
            !malformedProviderOperation &&
                malformedProviderOperation.diagnostic.code ==
                    detail::ClientOperationDecodeCode::MalformedKnownPayload &&
                malformedProviderOperation.diagnostic.target ==
                    detail::ClientRequestTarget::ModelProviderCapabilitiesRead &&
                malformedProviderOperation.diagnostic.surfaceKey ==
                    expectedProviderKey &&
                expectedProviderKey.name ==
                    "modelProvider/capabilities/read" &&
                malformedProviderOperation.diagnostic.fieldPath ==
                    "$.namespaceTools",
            "provider-capabilities malformed results traverse the exact registry target and emit the exact structured diagnostic");
    }

    void testProviderAndNotifications(tests::support::TestResult& result) {
        std::string error;
        const codex::Json providerWire{
            {"imageGeneration", true},
            {"namespaceTools", false},
            {"webSearch", true},
            {"futureCapability", true},
        };
        const auto provider = detail::decodeModelProviderCapabilitiesReadResponse(providerWire, error);
        result.expectTrue(provider && provider->imageGeneration && !provider->namespaceTools &&
                              provider->webSearch && provider->raw == providerWire && error.empty(),
                          "provider capabilities decode every known field and retain the exact raw result");

        const codex::Notification rerouted =
            notification("model/rerouted",
                         {{"fromModel", "model-old"},
                          {"reason", "futureReroute"},
                          {"threadId", "thread-model"},
                          {"toModel", "model-new"},
                          {"turnId", "turn-model"},
                          {"futureParam", 1}});
        const auto decodedRerouted = detail::decodeModelReroutedNotification(rerouted, error);
        result.expectTrue(
            decodedRerouted && decodedRerouted->fromModel.value == "model-old" &&
                decodedRerouted->toModel.value == "model-new" &&
                decodedRerouted->threadId.value == "thread-model" &&
                decodedRerouted->turnId.value == "turn-model" &&
                decodedRerouted->reason.value == "futureReroute" && !decodedRerouted->reason.isKnown() &&
                decodedRerouted->raw == rerouted.raw &&
                hasDiagnostic(decodedRerouted->diagnostics,
                              typed::DecodeIssueKind::UnknownEnumValue,
                              typed::DecodeIssueSeverity::ForwardCompatibility,
                              "ModelRerouteReason",
                              "$.params.reason"),
            "model/rerouted preserves its full envelope and future reason inside the known notification");

        const codex::Notification safety =
            notification("model/safetyBuffering/updated",
                         {{"fasterModel", nullptr},
                          {"model", "model-safe"},
                          {"reasons", codex::Json::array({"reason-a", "reason-b"})},
                          {"showBufferingUi", false},
                          {"threadId", "thread-model"},
                          {"turnId", "turn-model"},
                          {"useCases", codex::Json::array({"use-a"})},
                          {"futureParam", 2}});
        const auto decodedSafety = detail::decodeModelSafetyBufferingUpdatedNotification(safety, error);
        result.expectTrue(decodedSafety && decodedSafety->fasterModel.isNull() &&
                              decodedSafety->model.value == "model-safe" &&
                              decodedSafety->reasons == std::vector<std::string>{"reason-a", "reason-b"} &&
                              !decodedSafety->showBufferingUi &&
                              decodedSafety->useCases == std::vector<std::string>{"use-a"} &&
                              decodedSafety->raw == safety.raw,
                          "model safety buffering decodes ordered arrays, false, null, strong IDs, and full raw envelope");

        codex::Notification safetyOmitted = safety;
        safetyOmitted.params.erase("fasterModel");
        safetyOmitted.raw["params"].erase("fasterModel");
        const auto omittedSafety = detail::decodeModelSafetyBufferingUpdatedNotification(safetyOmitted, error);
        const codex::Notification safetyValue =
            notification("model/safetyBuffering/updated",
                         {{"fasterModel", "model-faster"},
                          {"model", "model-safe"},
                          {"reasons", codex::Json::array({"reason-a", "reason-b"})},
                          {"showBufferingUi", false},
                          {"threadId", "thread-model"},
                          {"turnId", "turn-model"},
                          {"useCases", codex::Json::array({"use-a"})}});
        const auto valueSafety = detail::decodeModelSafetyBufferingUpdatedNotification(safetyValue, error);
        result.expectTrue(omittedSafety && omittedSafety->fasterModel.isOmitted() &&
                              valueSafety && valueSafety->fasterModel.hasValue() &&
                              valueSafety->fasterModel.value->value == "model-faster",
                          "model safety buffering distinguishes omitted, null, and value fasterModel states");

        codex::Notification emptySafety = safety;
        emptySafety.params["reasons"] = codex::Json::array();
        emptySafety.params["useCases"] = codex::Json::array();
        emptySafety.raw["params"]["reasons"] = codex::Json::array();
        emptySafety.raw["params"]["useCases"] = codex::Json::array();
        const auto decodedEmptySafety =
            detail::decodeModelSafetyBufferingUpdatedNotification(emptySafety, error);
        result.expectTrue(
            decodedEmptySafety && decodedEmptySafety->reasons.empty() &&
                decodedEmptySafety->useCases.empty() &&
                decodedEmptySafety->raw == emptySafety.raw,
            "model safety buffering preserves explicitly present empty reason and use-case arrays");

        const codex::Notification verification =
            notification("model/verification",
                         {{"threadId", "thread-model"},
                          {"turnId", "turn-model"},
                          {"verifications",
                           codex::Json::array({"trustedAccessForCyber", "futureVerification"})},
                          {"futureParam", 3}});
        const auto decodedVerification = detail::decodeModelVerificationNotification(verification, error);
        result.expectTrue(
            decodedVerification && decodedVerification->verifications.size() == 2 &&
                decodedVerification->verifications[0].isKnown() &&
                decodedVerification->verifications[1].value == "futureVerification" &&
                !decodedVerification->verifications[1].isKnown() &&
                decodedVerification->raw == verification.raw &&
                hasDiagnostic(decodedVerification->diagnostics,
                              typed::DecodeIssueKind::UnknownEnumValue,
                              typed::DecodeIssueSeverity::ForwardCompatibility,
                              "ModelVerification",
                              "$.params.verifications[1]"),
            "model verification retains future values in its known notification with exact diagnostics");

        codex::Notification emptyVerification = verification;
        emptyVerification.params["verifications"] = codex::Json::array();
        emptyVerification.raw["params"]["verifications"] =
            codex::Json::array();
        const auto decodedEmptyVerification =
            detail::decodeModelVerificationNotification(
                emptyVerification, error);
        result.expectTrue(
            decodedEmptyVerification &&
                decodedEmptyVerification->verifications.empty() &&
                decodedEmptyVerification->raw == emptyVerification.raw,
            "model verification preserves an explicitly present empty verification array");

        codex::Notification malformedReroute = rerouted;
        malformedReroute.params["reason"] = 7;
        malformedReroute.raw["params"]["reason"] = 7;
        const auto wrongReroute = detail::decodeModelReroutedNotification(malformedReroute, error);
        result.expectTrue(!wrongReroute && error.find("$.params.reason") != std::string::npos,
                          "a malformed known reroute reason is rejected rather than classified as future");
        result.expectTrue(hasMalformedEventDiagnostic(
                              detail::decodeEvent(malformedReroute), malformedReroute, "$.params.reason"),
                          "malformed model/rerouted becomes a ProtocolWarning at the exact intrinsic reason path");

        codex::Notification malformedSafety = safety;
        malformedSafety.params["reasons"] = codex::Json::object();
        malformedSafety.raw["params"]["reasons"] = codex::Json::object();
        const auto wrongSafety = detail::decodeModelSafetyBufferingUpdatedNotification(malformedSafety, error);
        result.expectTrue(!wrongSafety && error.find("$.params.reasons") != std::string::npos,
                          "a malformed known safety reasons array reports its exact path");
        result.expectTrue(hasMalformedEventDiagnostic(
                              detail::decodeEvent(malformedSafety), malformedSafety, "$.params.reasons"),
                          "malformed model safety buffering becomes a ProtocolWarning at the exact intrinsic reasons path");

        codex::Notification malformedVerification = verification;
        malformedVerification.params["verifications"][0] = false;
        malformedVerification.raw["params"]["verifications"][0] = false;
        const auto wrongVerification = detail::decodeModelVerificationNotification(malformedVerification, error);
        result.expectTrue(!wrongVerification && error.find("$.params.verifications[0]") != std::string::npos,
                          "a malformed known verification value reports its exact array path");
        result.expectTrue(hasMalformedEventDiagnostic(
                              detail::decodeEvent(malformedVerification),
                              malformedVerification,
                              "$.params.verifications[0]"),
                          "malformed model verification becomes a ProtocolWarning at the exact intrinsic array path");
    }
} // namespace

int main() {
    static_assert(std::is_same_v<decltype(typed::ModelListParams{}.limit),
                                 typed::OptionalNullable<std::uint32_t>>,
                  "negative, fractional, and overflowing model limits must be unrepresentable in the typed facade");
    tests::support::TestResult result;
    testOpenStringEnums(result);
    testRequestEncoding(result);
    testModelListResponse(result);
    testMalformedResults(result);
    testProviderAndNotifications(result);
    return result.processResult();
}
