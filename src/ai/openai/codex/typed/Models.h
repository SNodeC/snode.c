/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_TYPED_MODELS_H
#define AI_OPENAI_CODEX_TYPED_MODELS_H

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/typed/Results.h"
#include "ai/openai/codex/typed/Types.h"

#include <compare>
#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace ai::openai::codex::typed {

    struct ModelServiceTierId {
        std::string value;

        auto operator<=>(const ModelServiceTierId&) const = default;
    };

    struct InputModality {
        std::string value;

        static InputModality text();
        static InputModality image();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const InputModality&) const = default;
    };

    struct ModelRerouteReason {
        std::string value;

        static ModelRerouteReason highRiskCyberActivity();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const ModelRerouteReason&) const = default;
    };

    struct ModelVerification {
        std::string value;

        static ModelVerification trustedAccessForCyber();
        [[nodiscard]] bool isKnown() const noexcept;

        auto operator<=>(const ModelVerification&) const = default;
    };

    struct ModelAvailabilityNux {
        std::string message;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ModelAvailabilityNux&) const = default;
    };

    struct ModelServiceTier {
        std::string description;
        ModelServiceTierId id;
        std::string name;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ModelServiceTier&) const = default;
    };

    struct ModelUpgradeInfo {
        OptionalNullable<std::string> migrationMarkdown;
        ModelId model;
        OptionalNullable<std::string> modelLink;
        OptionalNullable<std::string> upgradeCopy;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ModelUpgradeInfo&) const = default;
    };

    struct ReasoningEffortOption {
        std::string description;
        ReasoningEffort reasoningEffort;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ReasoningEffortOption&) const = default;
    };

    struct Model {
        std::vector<ModelServiceTierId> additionalSpeedTiers;
        OptionalNullable<ModelAvailabilityNux> availabilityNux;
        ReasoningEffort defaultReasoningEffort;
        OptionalNullable<ModelServiceTierId> defaultServiceTier;
        std::string description;
        std::string displayName;
        bool hidden = false;
        ModelId id;
        std::vector<InputModality> inputModalities{InputModality::text(), InputModality::image()};
        bool isDefault = false;
        ModelId model;
        std::vector<ModelServiceTier> serviceTiers;
        std::vector<ReasoningEffortOption> supportedReasoningEfforts;
        bool supportsPersonality = false;
        OptionalNullable<std::string> upgrade;
        OptionalNullable<ModelUpgradeInfo> upgradeInfo;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const Model&) const = default;
    };

    struct ModelListParams {
        OptionalNullable<std::string> cursor;
        OptionalNullable<bool> includeHidden;
        OptionalNullable<std::uint32_t> limit;

        bool operator==(const ModelListParams&) const = default;
    };

    struct ModelListResponse {
        std::vector<Model> data;
        OptionalNullable<std::string> nextCursor;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ModelListResponse&) const = default;
    };

    struct ModelProviderCapabilitiesReadParams {
        bool operator==(const ModelProviderCapabilitiesReadParams&) const = default;
    };

    struct ModelProviderCapabilitiesReadResponse {
        bool imageGeneration = false;
        bool namespaceTools = false;
        bool webSearch = false;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ModelProviderCapabilitiesReadResponse&) const = default;
    };

    // Incoming notification aggregates retain the complete JSON-RPC envelope.
    struct ModelReroutedNotification {
        ModelId fromModel;
        ModelRerouteReason reason;
        ThreadId threadId;
        ModelId toModel;
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ModelReroutedNotification&) const = default;
    };

    struct ModelSafetyBufferingUpdatedNotification {
        OptionalNullable<ModelId> fasterModel;
        ModelId model;
        std::vector<std::string> reasons;
        bool showBufferingUi = false;
        ThreadId threadId;
        TurnId turnId;
        std::vector<std::string> useCases;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ModelSafetyBufferingUpdatedNotification&) const = default;
    };

    struct ModelVerificationNotification {
        ThreadId threadId;
        TurnId turnId;
        std::vector<ModelVerification> verifications;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const ModelVerificationNotification&) const = default;
    };

    class Models {
    public:
        using Submission = AppServerClient::RawProtocol::Submission;
        using ListResultHandler = std::function<void(const OperationResult<ModelListResponse>&)>;
        using ReadProviderCapabilitiesResultHandler =
            std::function<void(const OperationResult<ModelProviderCapabilitiesReadResponse>&)>;

        Submission list(ModelListParams params, ListResultHandler handler);
        Submission readProviderCapabilities(ModelProviderCapabilitiesReadParams params,
                                            ReadProviderCapabilitiesResultHandler handler);

    private:
        friend class ::ai::openai::codex::AppServerClient;

        explicit Models(AppServerClient::RawProtocol& protocol) noexcept;

        AppServerClient::RawProtocol* protocol;
    };

} // namespace ai::openai::codex::typed

#endif // AI_OPENAI_CODEX_TYPED_MODELS_H
