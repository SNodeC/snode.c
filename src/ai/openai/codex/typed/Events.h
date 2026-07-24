/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_TYPED_EVENTS_H
#define AI_OPENAI_CODEX_TYPED_EVENTS_H

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/typed/Accounts.h"
#include "ai/openai/codex/typed/CodexErrorInfo.h"
#include "ai/openai/codex/typed/Models.h"
#include "ai/openai/codex/typed/Threads.h"

#include <compare>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace ai::openai::codex::typed {

    struct ModeKind {
        std::string value;

        static ModeKind plan() {
            return {"plan"};
        }

        static ModeKind defaultMode() {
            return {"default"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "plan" || value == "default";
        }

        auto operator<=>(const ModeKind&) const = default;
    };

    struct RealtimeConversationVersion {
        std::string value;

        static RealtimeConversationVersion v1() {
            return {"v1"};
        }

        static RealtimeConversationVersion v2() {
            return {"v2"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "v1" || value == "v2";
        }

        auto operator<=>(const RealtimeConversationVersion&) const = default;
    };

    struct TurnPlanStepStatus {
        std::string value;

        static TurnPlanStepStatus pending() {
            return {"pending"};
        }

        static TurnPlanStepStatus inProgress() {
            return {"inProgress"};
        }

        static TurnPlanStepStatus completed() {
            return {"completed"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "pending" || value == "inProgress" || value == "completed";
        }

        auto operator<=>(const TurnPlanStepStatus&) const = default;
    };

    struct ActivePermissionProfile {
        OptionalNullable<std::string> extends;
        std::string id;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct Settings {
        OptionalNullable<std::string> developerInstructions;
        ModelId model;
        OptionalNullable<ReasoningEffort> reasoningEffort;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct CollaborationMode {
        ModeKind mode;
        Settings settings;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ThreadSettings {
        OptionalNullable<ActivePermissionProfile> activePermissionProfile;
        AskForApproval approvalPolicy;
        ApprovalsReviewer approvalsReviewer;
        CollaborationMode collaborationMode;
        AbsolutePathBuf cwd;
        OptionalNullable<ReasoningEffort> effort;
        ModelId model;
        std::string modelProvider;
        OptionalNullable<ReasoningSummary> summary;
        OptionalNullable<Personality> personality;
        SandboxPolicy sandboxPolicy;
        OptionalNullable<std::string> serviceTier;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct TokenUsageBreakdown {
        std::int64_t cachedInputTokens = 0;
        std::int64_t inputTokens = 0;
        std::int64_t outputTokens = 0;
        std::int64_t reasoningOutputTokens = 0;
        std::int64_t totalTokens = 0;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ThreadTokenUsage {
        TokenUsageBreakdown last;
        OptionalNullable<std::int64_t> modelContextWindow;
        TokenUsageBreakdown total;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ThreadRealtimeAudioChunk {
        std::string data;
        OptionalNullable<ItemId> itemId;
        std::uint16_t numChannels = 0;
        std::uint32_t sampleRate = 0;
        OptionalNullable<std::uint32_t> samplesPerChannel;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct TurnPlanStep {
        TurnPlanStepStatus status;
        std::string step;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    // Canonical incoming payload aggregates. `raw` retains the complete JSON-RPC
    // notification envelope so a newly typed event never loses observability.
    struct AgentMessageDeltaNotification {
        std::string delta;
        ItemId itemId;
        ThreadId threadId;
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct CommandExecutionOutputDeltaNotification {
        std::string delta;
        ItemId itemId;
        ThreadId threadId;
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct TerminalInteractionNotification {
        ItemId itemId;
        std::string processId;
        std::string stdin;
        ThreadId threadId;
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ItemCompletedNotification {
        std::int64_t completedAtMs = 0;
        ThreadItem item;
        ThreadId threadId;
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct FileChangeOutputDeltaNotification {
        std::string delta;
        ItemId itemId;
        ThreadId threadId;
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct FileChangePatchUpdatedNotification {
        std::vector<FileUpdateChange> changes;
        ItemId itemId;
        ThreadId threadId;
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct McpToolCallProgressNotification {
        ItemId itemId;
        std::string message;
        ThreadId threadId;
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct PlanDeltaNotification {
        std::string delta;
        ItemId itemId;
        ThreadId threadId;
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ReasoningSummaryPartAddedNotification {
        ItemId itemId;
        std::int64_t summaryIndex = 0;
        ThreadId threadId;
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ReasoningSummaryTextDeltaNotification {
        std::string delta;
        ItemId itemId;
        std::int64_t summaryIndex = 0;
        ThreadId threadId;
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ReasoningTextDeltaNotification {
        std::int64_t contentIndex = 0;
        std::string delta;
        ItemId itemId;
        ThreadId threadId;
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ItemStartedNotification {
        ThreadItem item;
        std::int64_t startedAtMs = 0;
        ThreadId threadId;
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ThreadArchivedNotification {
        ThreadId threadId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ThreadClosedNotification {
        ThreadId threadId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ContextCompactedNotification {
        ThreadId threadId;
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ThreadDeletedNotification {
        ThreadId threadId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ThreadGoalClearedNotification {
        ThreadId threadId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ThreadGoalUpdatedNotification {
        ThreadGoal goal;
        ThreadId threadId;
        OptionalNullable<TurnId> turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ThreadNameUpdatedNotification {
        ThreadId threadId;
        OptionalNullable<std::string> threadName;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ThreadRealtimeClosedNotification {
        OptionalNullable<std::string> reason;
        ThreadId threadId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ThreadRealtimeErrorNotification {
        std::string message;
        ThreadId threadId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ThreadRealtimeItemAddedNotification {
        // ProtocolDefinedOpaqueJson: the upstream schema intentionally accepts
        // any JSON value for a realtime response item.
        std::optional<Json> item;
        ThreadId threadId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ThreadRealtimeOutputAudioDeltaNotification {
        ThreadRealtimeAudioChunk audio;
        ThreadId threadId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ThreadRealtimeSdpNotification {
        std::string sdp;
        ThreadId threadId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ThreadRealtimeStartedNotification {
        OptionalNullable<std::string> realtimeSessionId;
        ThreadId threadId;
        RealtimeConversationVersion version;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ThreadRealtimeTranscriptDeltaNotification {
        std::string delta;
        std::string role;
        ThreadId threadId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ThreadRealtimeTranscriptDoneNotification {
        std::string role;
        std::string text;
        ThreadId threadId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ThreadSettingsUpdatedNotification {
        ThreadId threadId;
        ThreadSettings threadSettings;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ThreadStartedNotification {
        Thread thread;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ThreadStatusChangedNotification {
        ThreadStatus status;
        ThreadId threadId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ThreadTokenUsageUpdatedNotification {
        ThreadId threadId;
        ThreadTokenUsage tokenUsage;
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct ThreadUnarchivedNotification {
        ThreadId threadId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct TurnCompletedNotification {
        ThreadId threadId;
        Turn turn;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct TurnDiffUpdatedNotification {
        std::string diff;
        ThreadId threadId;
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct TurnModerationMetadataNotification {
        // ProtocolDefinedOpaqueJson: the protocol deliberately leaves
        // moderation metadata unconstrained.
        std::optional<Json> metadata;
        ThreadId threadId;
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct TurnPlanUpdatedNotification {
        OptionalNullable<std::string> explanation;
        std::vector<TurnPlanStep> plan;
        ThreadId threadId;
        TurnId turnId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    struct TurnStartedNotification {
        ThreadId threadId;
        Turn turn;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;
    };

    using CanonicalServerNotification = std::variant<AccountLoginCompletedNotification,
                                                     AccountRateLimitsUpdatedNotification,
                                                     AccountUpdatedNotification,
                                                     ModelReroutedNotification,
                                                     ModelSafetyBufferingUpdatedNotification,
                                                     ModelVerificationNotification,
                                                     AgentMessageDeltaNotification,
                                                     CommandExecutionOutputDeltaNotification,
                                                     TerminalInteractionNotification,
                                                     ItemCompletedNotification,
                                                     FileChangeOutputDeltaNotification,
                                                     FileChangePatchUpdatedNotification,
                                                     McpToolCallProgressNotification,
                                                     PlanDeltaNotification,
                                                     ReasoningSummaryPartAddedNotification,
                                                     ReasoningSummaryTextDeltaNotification,
                                                     ReasoningTextDeltaNotification,
                                                     ItemStartedNotification,
                                                     ThreadArchivedNotification,
                                                     ThreadClosedNotification,
                                                     ContextCompactedNotification,
                                                     ThreadDeletedNotification,
                                                     ThreadGoalClearedNotification,
                                                     ThreadGoalUpdatedNotification,
                                                     ThreadNameUpdatedNotification,
                                                     ThreadRealtimeClosedNotification,
                                                     ThreadRealtimeErrorNotification,
                                                     ThreadRealtimeItemAddedNotification,
                                                     ThreadRealtimeOutputAudioDeltaNotification,
                                                     ThreadRealtimeSdpNotification,
                                                     ThreadRealtimeStartedNotification,
                                                     ThreadRealtimeTranscriptDeltaNotification,
                                                     ThreadRealtimeTranscriptDoneNotification,
                                                     ThreadSettingsUpdatedNotification,
                                                     ThreadStartedNotification,
                                                     ThreadStatusChangedNotification,
                                                     ThreadTokenUsageUpdatedNotification,
                                                     ThreadUnarchivedNotification,
                                                     TurnCompletedNotification,
                                                     TurnDiffUpdatedNotification,
                                                     TurnModerationMetadataNotification,
                                                     TurnPlanUpdatedNotification,
                                                     TurnStartedNotification>;

    struct ThreadStarted {
        Thread thread;
        Json raw;
        std::optional<ThreadStartedNotification> canonical = std::nullopt;
    };

    struct ThreadStatusChanged {
        ThreadId threadId;
        ThreadStatus status;
        Json raw;
        std::optional<ThreadStatusChangedNotification> canonical = std::nullopt;
    };

    struct TurnStarted {
        Turn turn;
        Json raw;
        std::optional<TurnStartedNotification> canonical = std::nullopt;
    };

    struct TurnCompleted {
        Turn turn;
        Json raw;
        std::optional<TurnCompletedNotification> canonical = std::nullopt;
    };

    struct TurnFailed {
        Turn turn;
        Json error = nullptr;
        Json raw;
        std::optional<TurnCompletedNotification> canonical = std::nullopt;
    };

    struct ItemStarted {
        Item item;
        std::int64_t startedAtMs = 0;
        Json raw;
        std::optional<ItemStartedNotification> canonical = std::nullopt;
    };

    struct ItemCompleted {
        Item item;
        std::int64_t completedAtMs = 0;
        Json raw;
        std::optional<ItemCompletedNotification> canonical = std::nullopt;
    };

    struct AgentMessageDelta {
        ThreadId threadId;
        TurnId turnId;
        ItemId itemId;
        std::string text;
        Json raw;
        std::optional<AgentMessageDeltaNotification> canonical = std::nullopt;
    };

    struct ReasoningDelta {
        enum class Kind { Text, Summary };

        ThreadId threadId;
        TurnId turnId;
        ItemId itemId;
        std::string text;
        Kind kind = Kind::Text;
        std::int64_t index = 0;
        Json raw;
        std::variant<std::monostate, ReasoningTextDeltaNotification, ReasoningSummaryTextDeltaNotification> canonical =
            std::monostate{};
    };

    struct CommandOutputDelta {
        ThreadId threadId;
        TurnId turnId;
        ItemId itemId;
        std::string output;
        Json raw;
        std::optional<CommandExecutionOutputDeltaNotification> canonical = std::nullopt;
    };

    struct FileChangeUpdated {
        ThreadId threadId;
        TurnId turnId;
        ItemId itemId;
        Json changes = Json::array();
        Json raw;
        std::optional<FileChangePatchUpdatedNotification> canonical = std::nullopt;
    };

    struct TokenUsageUpdated {
        ThreadId threadId;
        TurnId turnId;
        Json usage;
        Json raw;
        std::optional<ThreadTokenUsageUpdatedNotification> canonical = std::nullopt;
    };

    struct ModelRerouted {
        ThreadId threadId;
        TurnId turnId;
        ModelId from;
        ModelId to;
        std::string reason;
        Json raw;
        std::optional<ModelReroutedNotification> canonical = std::nullopt;
    };

    struct TurnErrorEvent {
        ThreadId threadId;
        TurnId turnId;
        Json error;
        bool willRetry = false;
        Json raw;
        std::optional<TurnError> typedError;
    };

    struct UnknownEvent {
        std::string method;
        Json params;
        Json raw;
        std::optional<std::string> decodingError;
        std::optional<DecodeDiagnostic> diagnostic;
    };

    using Event = std::variant<ThreadStarted,
                               ThreadStatusChanged,
                               TurnStarted,
                               TurnCompleted,
                               TurnFailed,
                               ItemStarted,
                               ItemCompleted,
                               AgentMessageDelta,
                               ReasoningDelta,
                               CommandOutputDelta,
                               FileChangeUpdated,
                               TokenUsageUpdated,
                               TerminalInteractionNotification,
                               FileChangeOutputDeltaNotification,
                               McpToolCallProgressNotification,
                               PlanDeltaNotification,
                               ReasoningSummaryPartAddedNotification,
                               ThreadArchivedNotification,
                               ThreadClosedNotification,
                               ContextCompactedNotification,
                               ThreadDeletedNotification,
                               ThreadGoalClearedNotification,
                               ThreadGoalUpdatedNotification,
                               ThreadNameUpdatedNotification,
                               ThreadRealtimeClosedNotification,
                               ThreadRealtimeErrorNotification,
                               ThreadRealtimeItemAddedNotification,
                               ThreadRealtimeOutputAudioDeltaNotification,
                               ThreadRealtimeSdpNotification,
                               ThreadRealtimeStartedNotification,
                               ThreadRealtimeTranscriptDeltaNotification,
                               ThreadRealtimeTranscriptDoneNotification,
                               ThreadSettingsUpdatedNotification,
                               ThreadUnarchivedNotification,
                               TurnDiffUpdatedNotification,
                               TurnModerationMetadataNotification,
                               TurnPlanUpdatedNotification,
                               AccountLoginCompletedNotification,
                               AccountRateLimitsUpdatedNotification,
                               AccountUpdatedNotification,
                               ModelRerouted,
                               ModelSafetyBufferingUpdatedNotification,
                               ModelVerificationNotification,
                               TurnErrorEvent,
                               UnknownEvent>;

    class Events {
    public:
        using EventHandler = std::function<void(const Event&)>;

        void setOnEvent(EventHandler handler);

    private:
        friend class ::ai::openai::codex::AppServerClient;

        explicit Events(AppServerClient::RawProtocol& protocol) noexcept;

        AppServerClient::RawProtocol* protocol;
    };

} // namespace ai::openai::codex::typed

#endif // AI_OPENAI_CODEX_TYPED_EVENTS_H
