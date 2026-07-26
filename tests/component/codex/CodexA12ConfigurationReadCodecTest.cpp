/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ClientOperationCodec.h"
#include "ai/openai/codex/detail/ConfigurationCodec.h"
#include "ai/openai/codex/detail/EventDecoder.h"
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

    codex::Notification notification(std::string method, codex::Json params) {
        codex::Json raw{
            {"jsonrpc", "2.0"},
            {"method", method},
            {"params", params},
            {"futureEnvelopeOnly", true},
        };
        return {std::move(method), std::move(params), std::move(raw)};
    }

    void testOpenEnums(tests::support::TestResult& result) {
        result.expectTrue(typed::AutoCompactTokenLimitScope::total().isKnown() &&
                              typed::AutoCompactTokenLimitScope::bodyAfterPrefix().value == "body_after_prefix" &&
                              typed::ForcedLoginMethod::chatgpt().isKnown() && typed::ForcedLoginMethod::api().isKnown() &&
                              typed::ResidencyRequirement::us().isKnown() && typed::Verbosity::low().isKnown() &&
                              typed::Verbosity::medium().isKnown() && typed::Verbosity::high().isKnown() &&
                              typed::WebSearchContextSize::low().isKnown() && typed::WebSearchContextSize::medium().isKnown() &&
                              typed::WebSearchContextSize::high().isKnown() && typed::WebSearchMode::disabled().isKnown() &&
                              typed::WebSearchMode::cached().isKnown() && typed::WebSearchMode::indexed().isKnown() &&
                              typed::WebSearchMode::live().isKnown() && typed::WindowsSandboxSetupMode::elevated().isKnown() &&
                              typed::WindowsSandboxSetupMode::unelevated().isKnown(),
                          "configuration read open-enum factories retain every pinned known value");
        result.expectTrue(!typed::AutoCompactTokenLimitScope{"futureScope"}.isKnown() &&
                              !typed::ForcedLoginMethod{"futureLogin"}.isKnown() && !typed::ResidencyRequirement{"eu"}.isKnown() &&
                              !typed::Verbosity{"futureVerbosity"}.isKnown() && !typed::WebSearchContextSize{"futureContext"}.isKnown() &&
                              !typed::WebSearchMode{"futureSearch"}.isKnown() && !typed::WindowsSandboxSetupMode{"futureSandbox"}.isKnown(),
                          "configuration read string domains remain open to future values");
    }

    void testRequestEncoding(tests::support::TestResult& result) {
        std::string error = "stale";
        const auto omitted = detail::encodeConfigReadParams({}, error);
        result.expectTrue(omitted == codex::Json::object() && error.empty(), "config/read omits both omitted parameters");

        typed::ConfigReadParams nullCwd{
            .cwd = typed::OptionalNullable<std::string>::explicitNull(),
            .includeLayers = false,
        };
        const auto encodedNull = detail::encodeConfigReadParams(nullCwd, error);
        result.expectTrue(encodedNull == codex::Json{{"cwd", nullptr}, {"includeLayers", false}},
                          "config/read preserves explicit null and present false");

        typed::ConfigReadParams values{
            .cwd = typed::OptionalNullable<std::string>::withValue("/synthetic/project"),
            .includeLayers = true,
        };
        const auto encodedValues = detail::encodeConfigReadParams(values, error);
        result.expectTrue(encodedValues == codex::Json{{"cwd", "/synthetic/project"}, {"includeLayers", true}},
                          "config/read encodes the exact reviewed parameter names and values");

        typed::ConfigReadParams inconsistent;
        inconsistent.cwd.value = std::string("/must-not-encode");
        const auto rejected = detail::encodeConfigReadParams(inconsistent, error);
        result.expectTrue(!rejected && error.find("$.cwd") != std::string::npos,
                          "config/read rejects an inconsistent omitted tri-state synchronously");
    }

    void testLayerSourceUnion(tests::support::TestResult& result) {
        struct Case {
            codex::Json wire;
            std::size_t variantIndex;
        };
        const std::vector<Case> cases{
            {{{"type", "mdm"}, {"domain", "test.domain"}, {"key", "test-key"}}, 0},
            {{{"type", "system"}, {"file", "/synthetic/system.toml"}}, 1},
            {{{"type", "enterpriseManaged"}, {"id", "layer-id"}, {"name", "Managed"}}, 2},
            {{{"type", "user"}, {"file", "/synthetic/user.toml"}, {"profile", nullptr}}, 3},
            {{{"type", "project"}, {"dotCodexFolder", "/synthetic/.codex"}}, 4},
            {{{"type", "sessionFlags"}}, 5},
            {{{"type", "legacyManagedConfigTomlFromFile"}, {"file", "/synthetic/legacy.toml"}}, 6},
            {{{"type", "legacyManagedConfigTomlFromMdm"}}, 7},
        };

        std::size_t exact = 0;
        for (const Case& testCase : cases) {
            const auto decoded = detail::decodeConfigLayerSource(testCase.wire);
            if (decoded.value && decoded.value->index() == testCase.variantIndex && !decoded.diagnostic) {
                ++exact;
            }
        }
        result.expectTrue(exact == cases.size(),
                          "all eight ConfigLayerSource alternatives decode through their exact registry descriptors");

        const auto unknown = detail::decodeConfigLayerSource({{"type", "futureLayer"}, {"futureField", codex::Json{{"retained", true}}}});
        const auto* future = unknown.value ? std::get_if<typed::UnknownConfigLayerSource>(&*unknown.value) : nullptr;
        result.expectTrue(future && future->discriminator == "futureLayer" && future->raw.at("futureField").at("retained") == true &&
                              unknown.diagnostic && unknown.diagnostic->kind == typed::DecodeIssueKind::UnknownDiscriminator &&
                              unknown.diagnostic->severity == typed::DecodeIssueSeverity::ForwardCompatibility &&
                              unknown.diagnostic->fieldPath == "$.type",
                          "future ConfigLayerSource retains raw JSON in its explicit incoming unknown alternative");

        const auto malformedKnown = detail::decodeConfigLayerSource({{"type", "user"}});
        const auto wrongType = detail::decodeConfigLayerSource({{"type", 7}});
        const auto missingType = detail::decodeConfigLayerSource({{"file", "/synthetic/user.toml"}});
        result.expectTrue(!malformedKnown.value && malformedKnown.diagnostic &&
                              malformedKnown.diagnostic->kind == typed::DecodeIssueKind::MalformedKnownPayload &&
                              malformedKnown.diagnostic->fieldPath == "$.file" && !wrongType.value && wrongType.diagnostic &&
                              wrongType.diagnostic->kind == typed::DecodeIssueKind::MalformedKnownPayload &&
                              wrongType.diagnostic->fieldPath == "$.type" && !missingType.value && missingType.diagnostic &&
                              missingType.diagnostic->kind == typed::DecodeIssueKind::MalformedKnownPayload &&
                              missingType.diagnostic->fieldPath == "$.type",
                          "malformed known, wrong discriminator type, and missing discriminator never masquerade as future branches");
    }

    codex::Json fullConfigReadResult() {
        return {
            {"config",
             {{"analytics", {{"enabled", nullptr}, {"futureAnalytics", {{"exact", 1}}}}},
              {"approval_policy", "on-request"},
              {"approvals_reviewer", "auto_review"},
              {"compact_prompt", nullptr},
              {"desktop", {{"arbitrary", codex::Json::array({1, true, nullptr})}}},
              {"developer_instructions", "synthetic instructions"},
              {"forced_chatgpt_workspace_id", codex::Json::array({"workspace-a", "workspace-b"})},
              {"forced_login_method", "futureLogin"},
              {"instructions", "synthetic root instructions"},
              {"model", "model-config"},
              {"model_auto_compact_token_limit", std::numeric_limits<std::int64_t>::max()},
              {"model_auto_compact_token_limit_scope", "futureScope"},
              {"model_context_window", std::numeric_limits<std::int64_t>::min()},
              {"model_provider", "provider-config"},
              {"model_reasoning_effort", "futureEffort"},
              {"model_reasoning_summary", "concise"},
              {"model_verbosity", "futureVerbosity"},
              {"review_model", "model-review"},
              {"sandbox_mode", "workspace-write"},
              {"sandbox_workspace_write",
               {{"exclude_slash_tmp", true},
                {"exclude_tmpdir_env_var", false},
                {"network_access", true},
                {"writable_roots", codex::Json::array({"/synthetic/a", "/synthetic/b"})}}},
              {"service_tier", "priority"},
              {"tools",
               {{"web_search",
                 {{"allowed_domains", codex::Json::array({"example.test"})},
                  {"context_size", "futureContext"},
                  {"location", {{"city", "Synthetic City"}, {"country", nullptr}, {"region", "Synthetic Region"}, {"timezone", "UTC"}}}}}}},
              {"web_search", "futureSearch"},
              {"futureConfig", {{"retained", true}}}}},
            {"layers",
             codex::Json::array({{{"config", {{"arbitrary", codex::Json::array({1, "two"})}}},
                                  {"disabledReason", nullptr},
                                  {"name", {{"type", "user"}, {"file", "/synthetic/user.toml"}, {"profile", nullptr}}},
                                  {"version", "1"}}})},
            {"origins",
             {{"model", {{"name", {{"type", "enterpriseManaged"}, {"id", "enterprise-id"}, {"name", "Managed"}}}, {"version", "2"}}}}},
            {"futureResultField", true},
        };
    }

    void testConfigReadResponse(tests::support::TestResult& result) {
        const codex::Json wire = fullConfigReadResult();
        std::string error = "stale";
        const auto decoded = detail::decodeConfigReadResponse(wire, error);
        const typed::Config* config = decoded ? &decoded->config : nullptr;
        const typed::ConfigLayer* layer =
            decoded && decoded->layers.hasValue() && decoded->layers.value->size() == 1 ? &decoded->layers.value->front() : nullptr;
        result.expectTrue(
            config && layer && decoded->raw == wire && error.empty() && config->raw == wire.at("config") && config->analytics.hasValue() &&
                config->analytics.value->enabled.isNull() &&
                config->analytics.value->unknownProperties.at("futureAnalytics").at("exact") == 1 && config->compactPrompt.isNull() &&
                config->desktop.hasValue() && config->desktop.value->at("arbitrary") == codex::Json::array({1, true, nullptr}) &&
                config->forcedChatgptWorkspaceId.hasValue() &&
                std::get<std::vector<std::string>>(config->forcedChatgptWorkspaceId.value->value).size() == 2 &&
                config->forcedLoginMethod.value->value == "futureLogin" &&
                config->modelAutoCompactTokenLimit.value == std::numeric_limits<std::int64_t>::max() &&
                config->modelContextWindow.value == std::numeric_limits<std::int64_t>::min() &&
                config->sandboxWorkspaceWrite.value->writableRoots.size() == 2 &&
                config->unknownProperties.at("futureConfig").at("retained") == true && layer->config &&
                layer->config->at("arbitrary") == codex::Json::array({1, "two"}) && layer->disabledReason.isNull() &&
                std::holds_alternative<typed::UserConfigLayerSource>(layer->name) && decoded->origins.size() == 1 &&
                std::holds_alternative<typed::EnterpriseManagedConfigLayerSource>(decoded->origins.begin()->second.name),
            "config/read maps all known fields, authorized JSON paths, strong identifiers, layers, origins, and exact raw JSON");

        result.expectTrue(decoded &&
                              hasDiagnostic(decoded->diagnostics,
                                            typed::DecodeIssueKind::UnknownEnumValue,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "ForcedLoginMethod",
                                            "$.config.forced_login_method") &&
                              hasDiagnostic(decoded->diagnostics,
                                            typed::DecodeIssueKind::UnknownEnumValue,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "AutoCompactTokenLimitScope",
                                            "$.config.model_auto_compact_token_limit_scope") &&
                              hasDiagnostic(decoded->diagnostics,
                                            typed::DecodeIssueKind::UnknownEnumValue,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "WebSearchMode",
                                            "$.config.web_search"),
                          "future config enums retain the known outer aggregate with exact forward-compatibility paths");

        const auto omitted =
            detail::decodeConfigReadResponse({{"config", codex::Json::object()}, {"origins", codex::Json::object()}}, error);
        const auto nullLayers = detail::decodeConfigReadResponse(
            {{"config", codex::Json::object()}, {"layers", nullptr}, {"origins", codex::Json::object()}}, error);
        const auto nullLayerConfig = detail::decodeConfigReadResponse(
            {{"config", codex::Json::object()},
             {"layers", codex::Json::array({{{"config", nullptr}, {"name", {{"type", "sessionFlags"}}}, {"version", "null-config"}}})},
             {"origins", codex::Json::object()}},
            error);
        result.expectTrue(omitted && omitted->layers.isOmitted() && nullLayers && nullLayers->layers.isNull() && nullLayerConfig &&
                              nullLayerConfig->layers.hasValue() && !nullLayerConfig->layers.value->front().config.has_value(),
                          "config/read distinguishes omitted layers, null layers, and required-nullable layer config");

        const auto missingOrigins = detail::decodeConfigReadResponse({{"config", codex::Json::object()}}, error);
        result.expectTrue(!missingOrigins && error.find("$.origins") != std::string::npos,
                          "config/read rejects a missing required origins map at the exact field path");
        const auto fractional =
            detail::decodeConfigReadResponse({{"config", {{"model_context_window", 1.5}}}, {"origins", codex::Json::object()}}, error);
        result.expectTrue(!fractional && error.find("$.config.model_context_window") != std::string::npos,
                          "config/read rejects fractional int64 values without coercion");
        const auto emptyReasoningEffort =
            detail::decodeConfigReadResponse({{"config", {{"model_reasoning_effort", ""}}}, {"origins", codex::Json::object()}}, error);
        result.expectTrue(!emptyReasoningEffort && error.find("$.config.model_reasoning_effort") != std::string::npos,
                          "config/read rejects an empty ReasoningEffort instead of treating it as a future enum value");

        const auto operation = detail::decodeClientOperationResult(detail::ClientRequestTarget::ConfigRead, wire);
        result.expectTrue(operation && std::holds_alternative<typed::ConfigReadResponse>(*operation.value) &&
                              operation.diagnostic.code == detail::ClientOperationDecodeCode::Decoded &&
                              detail::entryFor(detail::ClientRequestTarget::ConfigRead).key.name == "config/read",
                          "config/read successful results traverse the exact canonical registry/descriptor decoder");
    }

    void testRequirementsAndWarning(tests::support::TestResult& result) {
        const codex::Json wire{
            {"requirements",
             {{"allowAppshots", true},
              {"allowManagedHooksOnly", nullptr},
              {"allowRemoteControl", false},
              {"allowedApprovalPolicies", codex::Json::array({"on-request"})},
              {"allowedPermissionProfiles", {{"default", true}}},
              {"allowedSandboxModes", codex::Json::array({"workspace-write"})},
              {"allowedWebSearchModes", codex::Json::array({"cached", "futureSearch"})},
              {"allowedWindowsSandboxImplementations", codex::Json::array({"elevated"})},
              {"computerUse", {{"allowLockedComputerUse", nullptr}}},
              {"defaultPermissions", "synthetic-permissions"},
              {"enforceResidency", "us"},
              {"featureRequirements", {{"synthetic-feature", false}}},
              {"models", {{"newThread", {{"model", "model-required"}, {"modelReasoningEffort", "high"}, {"serviceTier", "priority"}}}}}}},
        };
        std::string error;
        const auto decoded = detail::decodeConfigRequirementsReadResponse(wire, error);
        const typed::ConfigRequirements* requirements =
            decoded && decoded->requirements.hasValue() ? &*decoded->requirements.value : nullptr;
        result.expectTrue(requirements && decoded->raw == wire && requirements->allowAppshots.value == true &&
                              requirements->allowManagedHooksOnly.isNull() && requirements->allowRemoteControl.value == false &&
                              requirements->allowedApprovalPolicies.value->size() == 1 &&
                              requirements->allowedPermissionProfiles.value->begin()->first.value == "default" &&
                              requirements->allowedWebSearchModes.value->at(1).value == "futureSearch" &&
                              requirements->computerUse.value->allowLockedComputerUse.isNull() &&
                              requirements->featureRequirements.value->begin()->first.value == "synthetic-feature" &&
                              requirements->models.value->newThread.value->model.value->value == "model-required",
                          "configRequirements/read decodes its complete nested requirements closure and tri-state fields");
        result.expectTrue(decoded && hasDiagnostic(decoded->diagnostics,
                                                   typed::DecodeIssueKind::UnknownEnumValue,
                                                   typed::DecodeIssueSeverity::ForwardCompatibility,
                                                   "WebSearchMode",
                                                   "$.requirements.allowedWebSearchModes[1]"),
                          "a future requirements enum remains observable without losing its known outer aggregate");

        const auto omitted = detail::decodeConfigRequirementsReadResponse(codex::Json::object(), error);
        const auto explicitNull = detail::decodeConfigRequirementsReadResponse({{"requirements", nullptr}}, error);
        result.expectTrue(omitted && omitted->requirements.isOmitted() && explicitNull && explicitNull->requirements.isNull(),
                          "configRequirements/read preserves omitted versus explicit-null requirements");
        const auto emptyReasoningEffort = detail::decodeConfigRequirementsReadResponse(
            {{"requirements", {{"models", {{"newThread", {{"modelReasoningEffort", ""}}}}}}}}, error);
        result.expectTrue(!emptyReasoningEffort && error.find("$.requirements.models.newThread.modelReasoningEffort") != std::string::npos,
                          "configRequirements/read rejects an empty ReasoningEffort instead of treating it as a future enum value");

        const auto requirementsOperation = detail::decodeClientOperationResult(detail::ClientRequestTarget::ConfigRequirementsRead, wire);
        result.expectTrue(requirementsOperation &&
                              std::holds_alternative<typed::ConfigRequirementsReadResponse>(*requirementsOperation.value) &&
                              detail::entryFor(detail::ClientRequestTarget::ConfigRequirementsRead).key.name == "configRequirements/read",
                          "configRequirements/read result decoding is bound to its exact registry contract");

        const codex::Notification warning = notification(
            "configWarning",
            {{"details", "synthetic potentially sensitive guidance"},
             {"path", "/synthetic/config.toml"},
             {"range",
              {{"start", {{"line", 0}, {"column", 1}}}, {"end", {{"line", std::numeric_limits<std::uint32_t>::max()}, {"column", 2}}}}},
             {"summary", "Synthetic warning"}});
        const auto decodedWarning = detail::decodeConfigWarningNotification(warning, error);
        const typed::Event event = detail::decodeEvent(warning);
        const auto* typedEvent = std::get_if<typed::ConfigWarningNotification>(&event);
        result.expectTrue(decodedWarning && typedEvent && decodedWarning->raw == warning.raw && typedEvent->raw == warning.raw &&
                              typedEvent->details.value == "synthetic potentially sensitive guidance" &&
                              typedEvent->path.value->value == "/synthetic/config.toml" && typedEvent->range.value->start.line == 0 &&
                              typedEvent->range.value->end.line == std::numeric_limits<std::uint32_t>::max() &&
                              typedEvent->summary == "Synthetic warning",
                          "configWarning types all fields and retains the complete internal JSON-RPC envelope");

        codex::Notification malformed = warning;
        malformed.params["details"] = codex::Json::object({{"never", "include-this-value-in-diagnostics"}});
        malformed.raw["params"] = malformed.params;
        const typed::Event malformedEvent = detail::decodeEvent(malformed);
        const auto* unknown = std::get_if<typed::UnknownEvent>(&malformedEvent);
        result.expectTrue(unknown && unknown->diagnostic && unknown->diagnostic->kind == typed::DecodeIssueKind::MalformedKnownPayload &&
                              unknown->diagnostic->severity == typed::DecodeIssueSeverity::ProtocolWarning &&
                              unknown->diagnostic->fieldPath == "$.params.details" && unknown->decodingError &&
                              unknown->decodingError->find("include-this-value-in-diagnostics") == std::string::npos,
                          "malformed configWarning diagnostics report only stable codes and field paths, never field values");
    }
} // namespace

int main() {
    tests::support::TestResult result;
    testOpenEnums(result);
    testRequestEncoding(result);
    testLayerSourceUnion(result);
    testConfigReadResponse(result);
    testRequirementsAndWarning(result);
    return result.processResult();
}
