/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_TYPED_CONFIGURATION_H
#define AI_OPENAI_CODEX_TYPED_CONFIGURATION_H

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/typed/Conversation.h"
#include "ai/openai/codex/typed/Models.h"
#include "ai/openai/codex/typed/Results.h"
#include "ai/openai/codex/typed/Types.h"

#include <compare>
#include <cstdint>
#include <functional>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace ai::openai::codex::typed {

    struct ConfigKeyPath {
        std::string value;

        auto operator<=>(const ConfigKeyPath&) const = default;
    };

    struct ConfigLayerId {
        std::string value;

        auto operator<=>(const ConfigLayerId&) const = default;
    };

    struct ExperimentalFeatureId {
        std::string value;

        auto operator<=>(const ExperimentalFeatureId&) const = default;
    };

    struct ModelProviderId {
        std::string value;

        auto operator<=>(const ModelProviderId&) const = default;
    };

    struct PermissionProfileName {
        std::string value;

        auto operator<=>(const PermissionProfileName&) const = default;
    };

    struct AutoCompactTokenLimitScope {
        std::string value;

        static AutoCompactTokenLimitScope total();
        static AutoCompactTokenLimitScope bodyAfterPrefix();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const AutoCompactTokenLimitScope&) const = default;
    };

    struct ForcedLoginMethod {
        std::string value;

        static ForcedLoginMethod chatgpt();
        static ForcedLoginMethod api();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const ForcedLoginMethod&) const = default;
    };

    struct ResidencyRequirement {
        std::string value;

        static ResidencyRequirement us();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const ResidencyRequirement&) const = default;
    };

    struct Verbosity {
        std::string value;

        static Verbosity low();
        static Verbosity medium();
        static Verbosity high();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const Verbosity&) const = default;
    };

    struct WebSearchContextSize {
        std::string value;

        static WebSearchContextSize low();
        static WebSearchContextSize medium();
        static WebSearchContextSize high();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const WebSearchContextSize&) const = default;
    };

    struct WebSearchMode {
        std::string value;

        static WebSearchMode disabled();
        static WebSearchMode cached();
        static WebSearchMode indexed();
        static WebSearchMode live();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const WebSearchMode&) const = default;
    };

    struct WindowsSandboxSetupMode {
        std::string value;

        static WindowsSandboxSetupMode elevated();
        static WindowsSandboxSetupMode unelevated();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const WindowsSandboxSetupMode&) const = default;
    };

    struct MergeStrategy {
        std::string value;

        static MergeStrategy replace();
        static MergeStrategy upsert();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const MergeStrategy&) const = default;
    };

    struct WriteStatus {
        std::string value;

        static WriteStatus ok();
        static WriteStatus okOverridden();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const WriteStatus&) const = default;
    };

    struct ExperimentalFeatureStage {
        std::string value;

        static ExperimentalFeatureStage beta();
        static ExperimentalFeatureStage underDevelopment();
        static ExperimentalFeatureStage stable();
        static ExperimentalFeatureStage deprecated();
        static ExperimentalFeatureStage removed();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const ExperimentalFeatureStage&) const = default;
    };

    struct AnalyticsConfig {
        OptionalNullable<bool> enabled;
        // The pinned schema explicitly permits arbitrary additional analytics
        // keys. Known fields remain typed and are never duplicated here.
        std::map<std::string, Json> unknownProperties;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const AnalyticsConfig&) const = default;
    };

    struct WebSearchLocation {
        OptionalNullable<std::string> city;
        OptionalNullable<std::string> country;
        OptionalNullable<std::string> region;
        OptionalNullable<std::string> timezone;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const WebSearchLocation&) const = default;
    };

    struct WebSearchToolConfig {
        OptionalNullable<std::vector<std::string>> allowedDomains;
        OptionalNullable<WebSearchContextSize> contextSize;
        OptionalNullable<WebSearchLocation> location;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const WebSearchToolConfig&) const = default;
    };

    struct ToolsV2 {
        OptionalNullable<WebSearchToolConfig> webSearch;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ToolsV2&) const = default;
    };

    struct SandboxWorkspaceWrite {
        bool excludeSlashTmp = false;
        bool excludeTmpdirEnvVar = false;
        bool networkAccess = false;
        std::vector<std::string> writableRoots;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const SandboxWorkspaceWrite&) const = default;
    };

    struct ForcedChatgptWorkspaceIds {
        using Value = std::variant<std::string, std::vector<std::string>>;

        Value value;
        Json raw = nullptr;
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ForcedChatgptWorkspaceIds&) const = default;
    };

    struct MdmConfigLayerSource {
        std::string domain;
        std::string key;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const MdmConfigLayerSource&) const = default;
    };

    struct SystemConfigLayerSource {
        AbsolutePathBuf file;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const SystemConfigLayerSource&) const = default;
    };

    struct EnterpriseManagedConfigLayerSource {
        ConfigLayerId id;
        std::string name;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const EnterpriseManagedConfigLayerSource&) const = default;
    };

    struct UserConfigLayerSource {
        AbsolutePathBuf file;
        OptionalNullable<std::string> profile;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const UserConfigLayerSource&) const = default;
    };

    struct ProjectConfigLayerSource {
        AbsolutePathBuf dotCodexFolder;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ProjectConfigLayerSource&) const = default;
    };

    struct SessionFlagsConfigLayerSource {
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const SessionFlagsConfigLayerSource&) const = default;
    };

    struct LegacyManagedConfigTomlFromFileConfigLayerSource {
        AbsolutePathBuf file;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const LegacyManagedConfigTomlFromFileConfigLayerSource&) const = default;
    };

    struct LegacyManagedConfigTomlFromMdmConfigLayerSource {
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const LegacyManagedConfigTomlFromMdmConfigLayerSource&) const = default;
    };

    struct UnknownConfigLayerSource {
        std::string discriminator;
        Json raw = Json::object();
        DecodeDiagnostic diagnostic;

        bool operator==(const UnknownConfigLayerSource&) const = default;
    };

    using ConfigLayerSource = std::variant<MdmConfigLayerSource,
                                           SystemConfigLayerSource,
                                           EnterpriseManagedConfigLayerSource,
                                           UserConfigLayerSource,
                                           ProjectConfigLayerSource,
                                           SessionFlagsConfigLayerSource,
                                           LegacyManagedConfigTomlFromFileConfigLayerSource,
                                           LegacyManagedConfigTomlFromMdmConfigLayerSource,
                                           UnknownConfigLayerSource>;

    struct ConfigLayer {
        // This property is required on the wire but nullable. A disengaged
        // optional therefore means an explicitly null value, never omission.
        std::optional<Json> config;
        OptionalNullable<std::string> disabledReason;
        ConfigLayerSource name;
        std::string version;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ConfigLayer&) const = default;
    };

    struct ConfigLayerMetadata {
        ConfigLayerSource name;
        std::string version;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ConfigLayerMetadata&) const = default;
    };

    struct ConfigEdit {
        ConfigKeyPath keyPath;
        MergeStrategy mergeStrategy;
        // The pinned schema deliberately accepts any JSON value, including
        // null. A disengaged optional represents that required null value.
        std::optional<Json> value;

        bool operator==(const ConfigEdit&) const = default;
    };

    struct ConfigBatchWriteParams {
        std::vector<ConfigEdit> edits;
        OptionalNullable<std::string> expectedVersion;
        OptionalNullable<AbsolutePathBuf> filePath;
        std::optional<bool> reloadUserConfig;

        bool operator==(const ConfigBatchWriteParams&) const = default;
    };

    struct ConfigValueWriteParams {
        OptionalNullable<std::string> expectedVersion;
        OptionalNullable<AbsolutePathBuf> filePath;
        ConfigKeyPath keyPath;
        MergeStrategy mergeStrategy;
        // The pinned schema deliberately accepts any JSON value, including
        // null. A disengaged optional represents that required null value.
        std::optional<Json> value;

        bool operator==(const ConfigValueWriteParams&) const = default;
    };

    struct OverriddenMetadata {
        // This incoming property is required but schema-authorized arbitrary
        // JSON. A disengaged optional represents an explicit JSON null.
        std::optional<Json> effectiveValue;
        std::string message;
        ConfigLayerMetadata overridingLayer;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const OverriddenMetadata&) const = default;
    };

    struct ConfigWriteResponse {
        AbsolutePathBuf filePath;
        OptionalNullable<OverriddenMetadata> overriddenMetadata;
        WriteStatus status;
        std::string version;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ConfigWriteResponse&) const = default;
    };

    struct Config {
        OptionalNullable<AnalyticsConfig> analytics;
        OptionalNullable<AskForApproval> approvalPolicy;
        OptionalNullable<ApprovalsReviewer> approvalsReviewer;
        OptionalNullable<std::string> compactPrompt;
        OptionalNullable<std::map<std::string, Json>> desktop;
        OptionalNullable<std::string> developerInstructions;
        OptionalNullable<ForcedChatgptWorkspaceIds> forcedChatgptWorkspaceId;
        OptionalNullable<ForcedLoginMethod> forcedLoginMethod;
        OptionalNullable<std::string> instructions;
        OptionalNullable<ModelId> model;
        OptionalNullable<std::int64_t> modelAutoCompactTokenLimit;
        OptionalNullable<AutoCompactTokenLimitScope> modelAutoCompactTokenLimitScope;
        OptionalNullable<std::int64_t> modelContextWindow;
        OptionalNullable<ModelProviderId> modelProvider;
        OptionalNullable<ReasoningEffort> modelReasoningEffort;
        OptionalNullable<ReasoningSummary> modelReasoningSummary;
        OptionalNullable<Verbosity> modelVerbosity;
        OptionalNullable<ModelId> reviewModel;
        OptionalNullable<SandboxMode> sandboxMode;
        OptionalNullable<SandboxWorkspaceWrite> sandboxWorkspaceWrite;
        OptionalNullable<ModelServiceTierId> serviceTier;
        OptionalNullable<ToolsV2> tools;
        OptionalNullable<WebSearchMode> webSearch;
        // Config deliberately declares additionalProperties: true. Preserve
        // those properties separately while retaining every known field above.
        std::map<std::string, Json> unknownProperties;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const Config&) const = default;
    };

    struct ConfigReadParams {
        OptionalNullable<std::string> cwd;
        std::optional<bool> includeLayers;

        bool operator==(const ConfigReadParams&) const = default;
    };

    struct ConfigReadResponse {
        Config config;
        OptionalNullable<std::vector<ConfigLayer>> layers;
        std::map<ConfigKeyPath, ConfigLayerMetadata> origins;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ConfigReadResponse&) const = default;
    };

    struct ComputerUseRequirements {
        OptionalNullable<bool> allowLockedComputerUse;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ComputerUseRequirements&) const = default;
    };

    struct NewThreadModelDefaults {
        OptionalNullable<ModelId> model;
        OptionalNullable<ReasoningEffort> modelReasoningEffort;
        OptionalNullable<ModelServiceTierId> serviceTier;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const NewThreadModelDefaults&) const = default;
    };

    struct ModelsRequirements {
        OptionalNullable<NewThreadModelDefaults> newThread;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ModelsRequirements&) const = default;
    };

    struct ConfigRequirements {
        OptionalNullable<bool> allowAppshots;
        OptionalNullable<bool> allowManagedHooksOnly;
        OptionalNullable<bool> allowRemoteControl;
        OptionalNullable<std::vector<AskForApproval>> allowedApprovalPolicies;
        OptionalNullable<std::map<PermissionProfileName, bool>> allowedPermissionProfiles;
        OptionalNullable<std::vector<SandboxMode>> allowedSandboxModes;
        OptionalNullable<std::vector<WebSearchMode>> allowedWebSearchModes;
        OptionalNullable<std::vector<WindowsSandboxSetupMode>> allowedWindowsSandboxImplementations;
        OptionalNullable<ComputerUseRequirements> computerUse;
        OptionalNullable<std::string> defaultPermissions;
        OptionalNullable<ResidencyRequirement> enforceResidency;
        OptionalNullable<std::map<ExperimentalFeatureId, bool>> featureRequirements;
        OptionalNullable<ModelsRequirements> models;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ConfigRequirements&) const = default;
    };

    struct ConfigRequirementsReadResponse {
        OptionalNullable<ConfigRequirements> requirements;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ConfigRequirementsReadResponse&) const = default;
    };

    struct ExperimentalFeatureEnablementSetParams {
        std::map<ExperimentalFeatureId, bool> enablement;

        bool operator==(const ExperimentalFeatureEnablementSetParams&) const = default;
    };

    struct ExperimentalFeatureEnablementSetResponse {
        std::map<ExperimentalFeatureId, bool> enablement;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ExperimentalFeatureEnablementSetResponse&) const = default;
    };

    struct ExperimentalFeatureListParams {
        OptionalNullable<std::string> cursor;
        OptionalNullable<std::uint32_t> limit;
        OptionalNullable<ThreadId> threadId;

        bool operator==(const ExperimentalFeatureListParams&) const = default;
    };

    struct ExperimentalFeature {
        OptionalNullable<std::string> announcement;
        bool defaultEnabled = false;
        OptionalNullable<std::string> description;
        OptionalNullable<std::string> displayName;
        bool enabled = false;
        ExperimentalFeatureId name;
        ExperimentalFeatureStage stage;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ExperimentalFeature&) const = default;
    };

    struct ExperimentalFeatureListResponse {
        std::vector<ExperimentalFeature> data;
        OptionalNullable<std::string> nextCursor;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ExperimentalFeatureListResponse&) const = default;
    };

    struct TextPosition {
        std::uint64_t column = 0;
        std::uint64_t line = 0;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const TextPosition&) const = default;
    };

    struct TextRange {
        TextPosition end;
        TextPosition start;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const TextRange&) const = default;
    };

    // Incoming notification aggregates retain the complete JSON-RPC envelope.
    struct ConfigWarningNotification {
        OptionalNullable<std::string> details;
        OptionalNullable<AbsolutePathBuf> path;
        OptionalNullable<TextRange> range;
        std::string summary;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ConfigWarningNotification&) const = default;
    };

    class Configuration {
    public:
        using Submission = AppServerClient::RawProtocol::Submission;
        using BatchWriteResultHandler = std::function<void(const OperationResult<ConfigWriteResponse>&)>;
        using UnitResultHandler = std::function<void(const OperationResult<Unit>&)>;
        using ReadResultHandler = std::function<void(const OperationResult<ConfigReadResponse>&)>;
        using ReadRequirementsResultHandler = std::function<void(const OperationResult<ConfigRequirementsReadResponse>&)>;
        using WriteValueResultHandler = std::function<void(const OperationResult<ConfigWriteResponse>&)>;
        using SetExperimentalFeatureEnablementResultHandler =
            std::function<void(const OperationResult<ExperimentalFeatureEnablementSetResponse>&)>;
        using ListExperimentalFeaturesResultHandler = std::function<void(const OperationResult<ExperimentalFeatureListResponse>&)>;

        Submission batchWrite(ConfigBatchWriteParams params, BatchWriteResultHandler handler);
        Submission reloadMcpServers(Unit params, UnitResultHandler handler);
        Submission read(ConfigReadParams params, ReadResultHandler handler);
        Submission readRequirements(Unit params, ReadRequirementsResultHandler handler);
        Submission writeValue(ConfigValueWriteParams params, WriteValueResultHandler handler);
        Submission setExperimentalFeatureEnablement(ExperimentalFeatureEnablementSetParams params,
                                                    SetExperimentalFeatureEnablementResultHandler handler);
        Submission listExperimentalFeatures(ExperimentalFeatureListParams params, ListExperimentalFeaturesResultHandler handler);

    private:
        friend class ::ai::openai::codex::AppServerClient;

        explicit Configuration(AppServerClient::RawProtocol& protocol) noexcept;

        AppServerClient::RawProtocol* protocol;
    };

} // namespace ai::openai::codex::typed

#endif // AI_OPENAI_CODEX_TYPED_CONFIGURATION_H
