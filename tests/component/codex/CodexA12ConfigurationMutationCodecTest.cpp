/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ClientOperationCodec.h"
#include "ai/openai/codex/detail/ConfigurationCodec.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "ai/openai/codex/typed/Configuration.h"
#include "support/TestResult.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
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

    void testOpenEnums(tests::support::TestResult& result) {
        result.expectTrue(typed::MergeStrategy::replace().value == "replace" && typed::MergeStrategy::replace().isKnown() &&
                              typed::MergeStrategy::upsert().value == "upsert" && typed::MergeStrategy::upsert().isKnown() &&
                              typed::WriteStatus::ok().value == "ok" && typed::WriteStatus::ok().isKnown() &&
                              typed::WriteStatus::okOverridden().value == "okOverridden" && typed::WriteStatus::okOverridden().isKnown() &&
                              typed::ExperimentalFeatureStage::beta().isKnown() &&
                              typed::ExperimentalFeatureStage::underDevelopment().value == "underDevelopment" &&
                              typed::ExperimentalFeatureStage::stable().isKnown() &&
                              typed::ExperimentalFeatureStage::deprecated().isKnown() &&
                              typed::ExperimentalFeatureStage::removed().isKnown(),
                          "B5 open-enum factories retain every pinned known value");
        result.expectTrue(!typed::MergeStrategy{"futureMerge"}.isKnown() && !typed::WriteStatus{"futureWrite"}.isKnown() &&
                              !typed::ExperimentalFeatureStage{"futureStage"}.isKnown(),
                          "B5 merge, write-status, and feature-stage domains remain open");
    }

    void testWriteEncoding(tests::support::TestResult& result) {
        std::string error = "stale";
        const codex::Json arbitraryObject{
            {"array", codex::Json::array({1, "two", nullptr, false})},
            {"nested", {{"exact", true}}},
        };
        const typed::ConfigBatchWriteParams batch{
            .edits =
                {
                    {typed::ConfigKeyPath{"alpha"}, typed::MergeStrategy::upsert(), std::optional<codex::Json>{arbitraryObject}},
                    {typed::ConfigKeyPath{"beta"}, typed::MergeStrategy{"futureMerge"}, std::nullopt},
                },
            .expectedVersion = typed::OptionalNullable<std::string>::explicitNull(),
            .filePath = typed::OptionalNullable<typed::AbsolutePathBuf>::withValue(typed::AbsolutePathBuf{"/synthetic/config.toml"}),
            .reloadUserConfig = false,
        };
        const codex::Json expectedBatch{
            {"edits",
             codex::Json::array({{{"keyPath", "alpha"}, {"mergeStrategy", "upsert"}, {"value", arbitraryObject}},
                                 {{"keyPath", "beta"}, {"mergeStrategy", "futureMerge"}, {"value", nullptr}}})},
            {"expectedVersion", nullptr},
            {"filePath", "/synthetic/config.toml"},
            {"reloadUserConfig", false},
        };
        const auto encodedBatch = detail::encodeConfigBatchWriteParams(batch, error);
        result.expectTrue(encodedBatch == expectedBatch && error.empty(),
                          "config/batchWrite preserves edit order, arbitrary JSON, required null, future merge values, and present false");

        const auto emptyBatch = detail::encodeConfigBatchWriteParams({}, error);
        result.expectTrue(emptyBatch == codex::Json{{"edits", codex::Json::array()}},
                          "config/batchWrite omits all omitted optional fields and preserves an empty edit array");

        const typed::ConfigValueWriteParams value{
            .expectedVersion = typed::OptionalNullable<std::string>::withValue("version-1"),
            .filePath = typed::OptionalNullable<typed::AbsolutePathBuf>::explicitNull(),
            .keyPath = typed::ConfigKeyPath{"features.synthetic"},
            .mergeStrategy = typed::MergeStrategy::replace(),
            .value = std::optional<codex::Json>{codex::Json::array({true, nullptr, {{"nested", "exact"}}})},
        };
        const codex::Json expectedValue{
            {"expectedVersion", "version-1"},
            {"filePath", nullptr},
            {"keyPath", "features.synthetic"},
            {"mergeStrategy", "replace"},
            {"value", codex::Json::array({true, nullptr, {{"nested", "exact"}}})},
        };
        const auto encodedValue = detail::encodeConfigValueWriteParams(value, error);
        result.expectTrue(encodedValue == expectedValue && error.empty(),
                          "config/value/write retains arbitrary JSON and exact omitted/null/value semantics");

        typed::ConfigValueWriteParams nullValue{
            .keyPath = typed::ConfigKeyPath{"features.null"},
            .mergeStrategy = typed::MergeStrategy::upsert(),
            .value = std::nullopt,
        };
        const auto encodedNullValue = detail::encodeConfigValueWriteParams(nullValue, error);
        result.expectTrue(encodedNullValue == codex::Json{{"keyPath", "features.null"}, {"mergeStrategy", "upsert"}, {"value", nullptr}},
                          "config/value/write represents the required nullable JSON value as explicit null, never omission");

        typed::ConfigBatchWriteParams inconsistentBatch;
        inconsistentBatch.expectedVersion.value = std::string("must-not-encode");
        const auto rejectedBatch = detail::encodeConfigBatchWriteParams(inconsistentBatch, error);
        result.expectTrue(!rejectedBatch && error.find("$.expectedVersion") != std::string::npos,
                          "config/batchWrite rejects an inconsistent omitted expectedVersion at its exact path");

        typed::ConfigValueWriteParams inconsistentValue{
            .keyPath = typed::ConfigKeyPath{"features.invalid"},
            .mergeStrategy = typed::MergeStrategy::replace(),
        };
        inconsistentValue.filePath.value = typed::AbsolutePathBuf{"/must-not-encode"};
        const auto rejectedValue = detail::encodeConfigValueWriteParams(inconsistentValue, error);
        result.expectTrue(!rejectedValue && error.find("$.filePath") != std::string::npos,
                          "config/value/write rejects an inconsistent omitted filePath at its exact path");
    }

    codex::Json fullConfigWriteResponse() {
        return {
            {"filePath", "/synthetic/config.toml"},
            {"overriddenMetadata",
             {{"effectiveValue", codex::Json::array({1, nullptr, {{"exact", true}}})},
              {"message", "Synthetic managed override"},
              {"overridingLayer", {{"name", {{"type", "sessionFlags"}}}, {"version", "managed-v1"}}}}},
            {"status", "futureWriteStatus"},
            {"version", "write-v2"},
            {"futureResultField", true},
        };
    }

    void testWriteResponse(tests::support::TestResult& result) {
        const codex::Json wire = fullConfigWriteResponse();
        std::string error = "stale";
        const auto decoded = detail::decodeConfigWriteResponse(wire, error);
        result.expectTrue(
            decoded && error.empty() && decoded->raw == wire && decoded->filePath.value == "/synthetic/config.toml" &&
                decoded->overriddenMetadata.hasValue() && decoded->overriddenMetadata.value->effectiveValue &&
                *decoded->overriddenMetadata.value->effectiveValue == codex::Json::array({1, nullptr, {{"exact", true}}}) &&
                decoded->overriddenMetadata.value->message == "Synthetic managed override" &&
                std::holds_alternative<typed::SessionFlagsConfigLayerSource>(decoded->overriddenMetadata.value->overridingLayer.name) &&
                decoded->status.value == "futureWriteStatus" && !decoded->status.isKnown() && decoded->version == "write-v2" &&
                hasDiagnostic(decoded->diagnostics,
                              typed::DecodeIssueKind::UnknownEnumValue,
                              typed::DecodeIssueSeverity::ForwardCompatibility,
                              "WriteStatus",
                              "$.status"),
            "ConfigWriteResponse types all known fields, retains authorized JSON/raw data, and keeps a future status");

        const codex::Json omittedWire{
            {"filePath", "/synthetic/config.toml"},
            {"status", "ok"},
            {"version", "v1"},
        };
        const codex::Json nullWire{
            {"filePath", "/synthetic/config.toml"},
            {"overriddenMetadata", nullptr},
            {"status", "okOverridden"},
            {"version", "v2"},
        };
        const codex::Json requiredNullWire{
            {"filePath", "/synthetic/config.toml"},
            {"overriddenMetadata",
             {{"effectiveValue", nullptr},
              {"message", "null effective value"},
              {"overridingLayer", {{"name", {{"type", "legacyManagedConfigTomlFromMdm"}}}, {"version", "v3"}}}}},
            {"status", "okOverridden"},
            {"version", "v3"},
        };
        const auto omitted = detail::decodeConfigWriteResponse(omittedWire, error);
        const auto explicitNull = detail::decodeConfigWriteResponse(nullWire, error);
        const auto requiredNull = detail::decodeConfigWriteResponse(requiredNullWire, error);
        result.expectTrue(omitted && omitted->overriddenMetadata.isOmitted() && explicitNull && explicitNull->overriddenMetadata.isNull() &&
                              requiredNull && requiredNull->overriddenMetadata.hasValue() &&
                              !requiredNull->overriddenMetadata.value->effectiveValue.has_value(),
                          "ConfigWriteResponse distinguishes omitted/null/value metadata and required-null effectiveValue");

        const auto missingStatus = detail::decodeConfigWriteResponse({{"filePath", "/synthetic/config.toml"}, {"version", "v1"}}, error);
        result.expectTrue(!missingStatus && error.find("$.status") != std::string::npos,
                          "ConfigWriteResponse rejects a missing required status at its exact path");
        const auto wrongMetadata = detail::decodeConfigWriteResponse(
            {{"filePath", "/synthetic/config.toml"}, {"overriddenMetadata", codex::Json::array()}, {"status", "ok"}, {"version", "v1"}},
            error);
        result.expectTrue(!wrongMetadata && error.find("$.overriddenMetadata") != std::string::npos,
                          "ConfigWriteResponse rejects a wrong-shaped nested metadata value");
    }

    void testExperimentalFeatureCodecs(tests::support::TestResult& result) {
        std::string error = "stale";
        const typed::ExperimentalFeatureEnablementSetParams setParams{
            .enablement = {{typed::ExperimentalFeatureId{"feature-alpha"}, true}, {typed::ExperimentalFeatureId{"feature-beta"}, false}},
        };
        const codex::Json expectedSet{{"enablement", {{"feature-alpha", true}, {"feature-beta", false}}}};
        result.expectTrue(detail::encodeExperimentalFeatureEnablementSetParams(setParams, error) == expectedSet && error.empty(),
                          "experimental feature enablement encodes the exact typed Boolean map");

        const auto setResponse = detail::decodeExperimentalFeatureEnablementSetResponse(expectedSet, error);
        result.expectTrue(setResponse && error.empty() && setResponse->raw == expectedSet &&
                              setResponse->enablement.at(typed::ExperimentalFeatureId{"feature-alpha"}) &&
                              !setResponse->enablement.at(typed::ExperimentalFeatureId{"feature-beta"}),
                          "experimental feature enablement response decodes the complete exact typed map");
        const auto malformedSet =
            detail::decodeExperimentalFeatureEnablementSetResponse({{"enablement", {{"feature-alpha", "yes"}}}}, error);
        result.expectTrue(!malformedSet && error.find("$.enablement.feature-alpha") != std::string::npos,
                          "experimental feature enablement rejects non-Boolean map values at the exact key path");

        const auto omittedList = detail::encodeExperimentalFeatureListParams({}, error);
        result.expectTrue(omittedList == codex::Json::object(), "experimentalFeature/list omits all three omitted optional fields");
        const typed::ExperimentalFeatureListParams listParams{
            .cursor = typed::OptionalNullable<std::string>::explicitNull(),
            .limit = typed::OptionalNullable<std::uint32_t>::withValue(std::numeric_limits<std::uint32_t>::max()),
            .threadId = typed::OptionalNullable<typed::ThreadId>::withValue(typed::ThreadId{"thread-feature"}),
        };
        const codex::Json expectedListParams{
            {"cursor", nullptr},
            {"limit", std::numeric_limits<std::uint32_t>::max()},
            {"threadId", "thread-feature"},
        };
        result.expectTrue(detail::encodeExperimentalFeatureListParams(listParams, error) == expectedListParams,
                          "experimentalFeature/list encodes null, uint32 boundary, and strong thread ID exactly");

        const codex::Json listWire{
            {"data",
             codex::Json::array({{{"announcement", nullptr},
                                  {"defaultEnabled", false},
                                  {"description", "Synthetic feature"},
                                  {"displayName", nullptr},
                                  {"enabled", true},
                                  {"name", "feature-alpha"},
                                  {"stage", "futureStage"},
                                  {"futureFeatureField", true}}})},
            {"nextCursor", "cursor-next"},
            {"futureResultField", true},
        };
        const auto list = detail::decodeExperimentalFeatureListResponse(listWire, error);
        const typed::ExperimentalFeature* feature = list && list->data.size() == 1 ? &list->data.front() : nullptr;
        result.expectTrue(feature && error.empty() && list->raw == listWire && feature->raw == listWire.at("data").front() &&
                              feature->announcement.isNull() && !feature->defaultEnabled &&
                              feature->description.value == "Synthetic feature" && feature->displayName.isNull() && feature->enabled &&
                              feature->name.value == "feature-alpha" && feature->stage.value == "futureStage" &&
                              !feature->stage.isKnown() && list->nextCursor.value == "cursor-next" &&
                              hasDiagnostic(list->diagnostics,
                                            typed::DecodeIssueKind::UnknownEnumValue,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "ExperimentalFeatureStage",
                                            "$.data[0].stage"),
                          "experimentalFeature/list types full metadata, retains raw values, and keeps a future stage");

        const auto omittedCursor = detail::decodeExperimentalFeatureListResponse({{"data", codex::Json::array()}}, error);
        const auto nullCursor =
            detail::decodeExperimentalFeatureListResponse({{"data", codex::Json::array()}, {"nextCursor", nullptr}}, error);
        result.expectTrue(omittedCursor && omittedCursor->nextCursor.isOmitted() && nullCursor && nullCursor->nextCursor.isNull(),
                          "experimental feature list distinguishes omitted and explicit-null next cursor");
        const auto missingData = detail::decodeExperimentalFeatureListResponse(codex::Json::object(), error);
        result.expectTrue(!missingData && error.find("$.data") != std::string::npos,
                          "experimental feature list rejects a missing required data array");
    }

    void testOperationAssociations(tests::support::TestResult& result) {
        const codex::Json write = fullConfigWriteResponse();
        const codex::Json set{{"enablement", {{"feature-alpha", true}}}};
        const codex::Json list{{"data", codex::Json::array()}, {"nextCursor", nullptr}};

        const auto batch = detail::decodeClientOperationResult(detail::ClientRequestTarget::ConfigBatchWrite, write);
        const auto reload = detail::decodeClientOperationResult(detail::ClientRequestTarget::ConfigMcpServerReload, codex::Json::object());
        const auto value = detail::decodeClientOperationResult(detail::ClientRequestTarget::ConfigValueWrite, write);
        const auto featureSet = detail::decodeClientOperationResult(detail::ClientRequestTarget::ExperimentalFeatureEnablementSet, set);
        const auto featureList = detail::decodeClientOperationResult(detail::ClientRequestTarget::ExperimentalFeatureList, list);

        result.expectTrue(batch && std::holds_alternative<typed::ConfigWriteResponse>(*batch.value) && reload &&
                              std::holds_alternative<typed::Unit>(*reload.value) && value &&
                              std::holds_alternative<typed::ConfigWriteResponse>(*value.value) && featureSet &&
                              std::holds_alternative<typed::ExperimentalFeatureEnablementSetResponse>(*featureSet.value) && featureList &&
                              std::holds_alternative<typed::ExperimentalFeatureListResponse>(*featureList.value),
                          "all five B5 operation targets select their authoritative Concrete/Unit result decoders");

        const auto malformedUnit =
            detail::decodeClientOperationResult(detail::ClientRequestTarget::ConfigMcpServerReload, {{"unexpected", true}});
        result.expectTrue(!malformedUnit && malformedUnit.diagnostic.code == detail::ClientOperationDecodeCode::MalformedKnownPayload &&
                              malformedUnit.diagnostic.message == "Unit successful result must be the exact empty object",
                          "config/mcpServer/reload retains the reviewed exact-empty-object Unit invariant");

        const auto wrongAssociation = detail::decodeClientOperationResult(detail::ClientRequestTarget::ExperimentalFeatureList, set);
        result.expectTrue(!wrongAssociation && wrongAssociation.diagnostic.code == detail::ClientOperationDecodeCode::MalformedKnownPayload,
                          "a stale B5 result association fails through its exact target-specific decoder");
    }
} // namespace

int main() {
    tests::support::TestResult result;
    testOpenEnums(result);
    testWriteEncoding(result);
    testWriteResponse(result);
    testExperimentalFeatureCodecs(result);
    testOperationAssociations(result);
    return result.processResult();
}
