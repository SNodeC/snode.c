/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/EventDecoder.h"
#include "ai/openai/codex/detail/ProtocolCodec.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "support/TestResult.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#ifndef CODEX_A1_FIXTURE_ROOT
#error "CODEX_A1_FIXTURE_ROOT must name the checked-in App Server fixture corpus"
#endif

#ifndef CODEX_A1_NOTIFICATION_PRODUCTION_COVERAGE
#error "CODEX_A1_NOTIFICATION_PRODUCTION_COVERAGE must name the checked notification production-coverage table"
#endif

namespace {
    namespace codex = ai::openai::codex;
    namespace detail = ai::openai::codex::detail;
    namespace typed = ai::openai::codex::typed;

    codex::Json readJson(const std::filesystem::path& path) {
        std::ifstream input(path);
        if (!input) {
            throw std::runtime_error("unable to open " + path.string());
        }
        return codex::Json::parse(input);
    }

    std::vector<std::string> stringArray(const codex::Json& value) {
        std::vector<std::string> result;
        for (const codex::Json& element : value) {
            result.emplace_back(element.get<std::string>());
        }
        return result;
    }

    void appendDiagnosticCodes(std::vector<std::string>& codes, const typed::DecodeDiagnostic& diagnostic) {
        switch (diagnostic.kind) {
            case typed::DecodeIssueKind::UnknownMethod:
                codes.emplace_back("UnknownMethod");
                break;
            case typed::DecodeIssueKind::UnknownDiscriminator:
                codes.emplace_back("UnknownDiscriminator");
                break;
            case typed::DecodeIssueKind::UnknownEnumValue:
                codes.emplace_back("UnknownEnumValue");
                break;
            case typed::DecodeIssueKind::MalformedKnownPayload:
                codes.emplace_back("MalformedKnownPayload");
                break;
        }
        switch (diagnostic.severity) {
            case typed::DecodeIssueSeverity::ForwardCompatibility:
                codes.emplace_back("ForwardCompatibility");
                break;
            case typed::DecodeIssueSeverity::ProtocolWarning:
                codes.emplace_back("ProtocolWarning");
                break;
        }
    }

    template <typename T>
    void appendValueDiagnostics(std::vector<std::string>& codes, const T& value) {
        if constexpr (requires { value.diagnostics; }) {
            for (const typed::DecodeDiagnostic& diagnostic : value.diagnostics) {
                appendDiagnosticCodes(codes, diagnostic);
            }
        }
    }

    void appendItemDiagnostics(std::vector<std::string>& codes, const typed::ThreadItem& item) {
        std::visit(
            [&](const auto& value) {
                using Value = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<Value, typed::UnknownItem>) {
                    if (value.diagnostic) {
                        appendDiagnosticCodes(codes, *value.diagnostic);
                    }
                } else {
                    appendValueDiagnostics(codes, value);
                }
            },
            item);
    }

    void appendTurnDiagnostics(std::vector<std::string>& codes, const typed::Turn& turn) {
        appendValueDiagnostics(codes, turn);
        if (turn.error.hasValue() && turn.error->codexErrorDiagnostic) {
            appendDiagnosticCodes(codes, *turn.error->codexErrorDiagnostic);
        }
        for (const typed::ThreadItem& item : turn.items) {
            appendItemDiagnostics(codes, item);
        }
    }

    void appendThreadDiagnostics(std::vector<std::string>& codes, const typed::Thread& thread) {
        appendValueDiagnostics(codes, thread);
        for (const typed::Turn& turn : thread.turns) {
            appendTurnDiagnostics(codes, turn);
        }
    }

    std::vector<std::string> eventCodes(const typed::Event& event) {
        std::vector<std::string> result;
        std::visit(
            [&](const auto& value) {
                using Value = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<Value, typed::UnknownEvent>) {
                    if (value.diagnostic) {
                        appendDiagnosticCodes(result, *value.diagnostic);
                    }
                } else {
                    result.emplace_back("Decoded");
                    if constexpr (std::is_same_v<Value, typed::ItemStarted> ||
                                  std::is_same_v<Value, typed::ItemCompleted>) {
                        appendItemDiagnostics(result, value.item);
                    } else if constexpr (std::is_same_v<Value, typed::ThreadStarted>) {
                        appendThreadDiagnostics(result, value.thread);
                    } else if constexpr (std::is_same_v<Value, typed::TurnStarted> ||
                                         std::is_same_v<Value, typed::TurnCompleted> ||
                                         std::is_same_v<Value, typed::TurnFailed>) {
                        appendTurnDiagnostics(result, value.turn);
                    } else if constexpr (std::is_same_v<Value, typed::ReasoningDelta>) {
                        std::visit(
                            [&](const auto& canonical) {
                                appendValueDiagnostics(result, canonical);
                            },
                            value.canonical);
                    } else if constexpr (requires { value.canonical.has_value(); }) {
                        if (value.canonical) {
                            appendValueDiagnostics(result, *value.canonical);
                        }
                    } else {
                        appendValueDiagnostics(result, value);
                    }
                }
            },
            event);
        return result;
    }

    std::optional<typed::CanonicalServerNotification> canonicalNotification(const typed::Event& event) {
        return std::visit(
            [](const auto& value) -> std::optional<typed::CanonicalServerNotification> {
                using Value = std::decay_t<decltype(value)>;
                if constexpr (std::is_constructible_v<typed::CanonicalServerNotification, Value>) {
                    return typed::CanonicalServerNotification{value};
                } else if constexpr (std::is_same_v<Value, typed::ReasoningDelta>) {
                    return std::visit(
                        [](const auto& canonical) -> std::optional<typed::CanonicalServerNotification> {
                            using Canonical = std::decay_t<decltype(canonical)>;
                            if constexpr (std::is_constructible_v<typed::CanonicalServerNotification, Canonical>) {
                                return typed::CanonicalServerNotification{canonical};
                            }
                            return std::nullopt;
                        },
                        value.canonical);
                } else if constexpr (requires { value.canonical.has_value(); }) {
                    if (value.canonical) {
                        return typed::CanonicalServerNotification{*value.canonical};
                    }
                    return std::nullopt;
                }
                return std::nullopt;
            },
            event);
    }

    std::string_view eventAlternativeName(const typed::Event& event) {
        return std::visit(
            [](const auto& value) -> std::string_view {
                using Value = std::decay_t<decltype(value)>;
#define CODEX_EVENT_ALTERNATIVE(type)                                                                                                      \
    if constexpr (std::is_same_v<Value, typed::type>) {                                                                                    \
        return #type;                                                                                                                       \
    } else
                CODEX_EVENT_ALTERNATIVE(AgentMessageDelta)
                CODEX_EVENT_ALTERNATIVE(CommandOutputDelta)
                CODEX_EVENT_ALTERNATIVE(ItemCompleted)
                CODEX_EVENT_ALTERNATIVE(FileChangeUpdated)
                CODEX_EVENT_ALTERNATIVE(ReasoningDelta)
                CODEX_EVENT_ALTERNATIVE(ItemStarted)
                CODEX_EVENT_ALTERNATIVE(ThreadStarted)
                CODEX_EVENT_ALTERNATIVE(ThreadStatusChanged)
                CODEX_EVENT_ALTERNATIVE(TokenUsageUpdated)
                CODEX_EVENT_ALTERNATIVE(TurnCompleted)
                CODEX_EVENT_ALTERNATIVE(TurnFailed)
                CODEX_EVENT_ALTERNATIVE(TurnStarted)
                CODEX_EVENT_ALTERNATIVE(TerminalInteractionNotification)
                CODEX_EVENT_ALTERNATIVE(FileChangeOutputDeltaNotification)
                CODEX_EVENT_ALTERNATIVE(McpToolCallProgressNotification)
                CODEX_EVENT_ALTERNATIVE(PlanDeltaNotification)
                CODEX_EVENT_ALTERNATIVE(ReasoningSummaryPartAddedNotification)
                CODEX_EVENT_ALTERNATIVE(ThreadArchivedNotification)
                CODEX_EVENT_ALTERNATIVE(ThreadClosedNotification)
                CODEX_EVENT_ALTERNATIVE(ContextCompactedNotification)
                CODEX_EVENT_ALTERNATIVE(ThreadDeletedNotification)
                CODEX_EVENT_ALTERNATIVE(ThreadGoalClearedNotification)
                CODEX_EVENT_ALTERNATIVE(ThreadGoalUpdatedNotification)
                CODEX_EVENT_ALTERNATIVE(ThreadNameUpdatedNotification)
                CODEX_EVENT_ALTERNATIVE(ThreadRealtimeClosedNotification)
                CODEX_EVENT_ALTERNATIVE(ThreadRealtimeErrorNotification)
                CODEX_EVENT_ALTERNATIVE(ThreadRealtimeItemAddedNotification)
                CODEX_EVENT_ALTERNATIVE(ThreadRealtimeOutputAudioDeltaNotification)
                CODEX_EVENT_ALTERNATIVE(ThreadRealtimeSdpNotification)
                CODEX_EVENT_ALTERNATIVE(ThreadRealtimeStartedNotification)
                CODEX_EVENT_ALTERNATIVE(ThreadRealtimeTranscriptDeltaNotification)
                CODEX_EVENT_ALTERNATIVE(ThreadRealtimeTranscriptDoneNotification)
                CODEX_EVENT_ALTERNATIVE(ThreadSettingsUpdatedNotification)
                CODEX_EVENT_ALTERNATIVE(ThreadUnarchivedNotification)
                CODEX_EVENT_ALTERNATIVE(TurnDiffUpdatedNotification)
                CODEX_EVENT_ALTERNATIVE(TurnModerationMetadataNotification)
                CODEX_EVENT_ALTERNATIVE(TurnPlanUpdatedNotification) {
                    return "non-A1.1";
                }
#undef CODEX_EVENT_ALTERNATIVE
            },
            event);
    }

    std::string_view canonicalTypeIdentity(const typed::CanonicalServerNotification& notification) {
        return std::visit(
            [](const auto& value) -> std::string_view {
                using Value = std::decay_t<decltype(value)>;
#define CODEX_CANONICAL_IDENTITY(type)                                                                                                     \
    if constexpr (std::is_same_v<Value, typed::type>) {                                                                                    \
        return "typed::" #type;                                                                                                             \
    } else
                CODEX_CANONICAL_IDENTITY(AgentMessageDeltaNotification)
                CODEX_CANONICAL_IDENTITY(CommandExecutionOutputDeltaNotification)
                CODEX_CANONICAL_IDENTITY(TerminalInteractionNotification)
                CODEX_CANONICAL_IDENTITY(ItemCompletedNotification)
                CODEX_CANONICAL_IDENTITY(FileChangeOutputDeltaNotification)
                CODEX_CANONICAL_IDENTITY(FileChangePatchUpdatedNotification)
                CODEX_CANONICAL_IDENTITY(McpToolCallProgressNotification)
                CODEX_CANONICAL_IDENTITY(PlanDeltaNotification)
                CODEX_CANONICAL_IDENTITY(ReasoningSummaryPartAddedNotification)
                CODEX_CANONICAL_IDENTITY(ReasoningSummaryTextDeltaNotification)
                CODEX_CANONICAL_IDENTITY(ReasoningTextDeltaNotification)
                CODEX_CANONICAL_IDENTITY(ItemStartedNotification)
                CODEX_CANONICAL_IDENTITY(ThreadArchivedNotification)
                CODEX_CANONICAL_IDENTITY(ThreadClosedNotification)
                CODEX_CANONICAL_IDENTITY(ContextCompactedNotification)
                CODEX_CANONICAL_IDENTITY(ThreadDeletedNotification)
                CODEX_CANONICAL_IDENTITY(ThreadGoalClearedNotification)
                CODEX_CANONICAL_IDENTITY(ThreadGoalUpdatedNotification)
                CODEX_CANONICAL_IDENTITY(ThreadNameUpdatedNotification)
                CODEX_CANONICAL_IDENTITY(ThreadRealtimeClosedNotification)
                CODEX_CANONICAL_IDENTITY(ThreadRealtimeErrorNotification)
                CODEX_CANONICAL_IDENTITY(ThreadRealtimeItemAddedNotification)
                CODEX_CANONICAL_IDENTITY(ThreadRealtimeOutputAudioDeltaNotification)
                CODEX_CANONICAL_IDENTITY(ThreadRealtimeSdpNotification)
                CODEX_CANONICAL_IDENTITY(ThreadRealtimeStartedNotification)
                CODEX_CANONICAL_IDENTITY(ThreadRealtimeTranscriptDeltaNotification)
                CODEX_CANONICAL_IDENTITY(ThreadRealtimeTranscriptDoneNotification)
                CODEX_CANONICAL_IDENTITY(ThreadSettingsUpdatedNotification)
                CODEX_CANONICAL_IDENTITY(ThreadStartedNotification)
                CODEX_CANONICAL_IDENTITY(ThreadStatusChangedNotification)
                CODEX_CANONICAL_IDENTITY(ThreadTokenUsageUpdatedNotification)
                CODEX_CANONICAL_IDENTITY(ThreadUnarchivedNotification)
                CODEX_CANONICAL_IDENTITY(TurnCompletedNotification)
                CODEX_CANONICAL_IDENTITY(TurnDiffUpdatedNotification)
                CODEX_CANONICAL_IDENTITY(TurnModerationMetadataNotification)
                CODEX_CANONICAL_IDENTITY(TurnPlanUpdatedNotification)
                CODEX_CANONICAL_IDENTITY(TurnStartedNotification) {
                    return "";
                }
#undef CODEX_CANONICAL_IDENTITY
            },
            notification);
    }

    template <typename T>
    bool optionalStringMatches(const typed::OptionalNullable<T>& value, const codex::Json& object, std::string_view field) {
        const auto member = object.find(field);
        if (member == object.end()) {
            return value.isOmitted();
        }
        if (member->is_null()) {
            return value.isNull();
        }
        if (!value.hasValue() || !member->is_string()) {
            return false;
        }
        if constexpr (std::is_same_v<T, std::string>) {
            return *value == member->get<std::string>();
        } else {
            return value->value == member->get<std::string>();
        }
    }

    template <typename T>
    bool optionalIntegerMatches(const typed::OptionalNullable<T>& value, const codex::Json& object, std::string_view field) {
        const auto member = object.find(field);
        if (member == object.end()) {
            return value.isOmitted();
        }
        if (member->is_null()) {
            return value.isNull();
        }
        return value.hasValue() && member->is_number_integer() && *value == member->get<T>();
    }

    const codex::Json& itemRaw(const typed::ThreadItem& item) {
        return std::visit(
            [](const auto& value) -> const codex::Json& {
                using Value = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<Value, typed::UnknownItem>) {
                    return value.raw;
                } else {
                    return value.metadata.raw;
                }
            },
            item);
    }

    std::string patchChangeKind(const typed::PatchChangeKind& kind) {
        return std::visit(
            [](const auto& value) {
                return value.raw.at("type").template get<std::string>();
            },
            kind);
    }

    template <typename Variant>
    const codex::Json& unionRaw(const Variant& value) {
        return std::visit(
            [](const auto& alternative) -> const codex::Json& {
                return alternative.raw;
            },
            value);
    }

    bool canonicalFieldsMatch(const typed::CanonicalServerNotification& notification, const codex::Json& envelope) {
        const codex::Json& params = envelope.at("params");
        return std::visit(
            [&](const auto& value) {
                using Value = std::decay_t<decltype(value)>;
                const auto stringAt = [](const codex::Json& object, std::string_view field) {
                    return object.at(field).get<std::string>();
                };
                const auto integerAt = [](const codex::Json& object, std::string_view field) {
                    return object.at(field).get<std::int64_t>();
                };
                const auto threadMatches = [&]() {
                    if constexpr (requires { value.threadId.value; }) {
                        return value.threadId.value == stringAt(params, "threadId");
                    }
                    return false;
                };
                const auto turnMatches = [&](bool required) {
                    if (!required) {
                        return true;
                    }
                    if constexpr (requires { value.turnId; }) {
                        using TurnField = std::remove_cvref_t<decltype(value.turnId)>;
                        if constexpr (std::is_same_v<TurnField, typed::TurnId>) {
                            return value.turnId.value == stringAt(params, "turnId");
                        }
                    }
                    return false;
                };
                const auto itemMatches = [&](bool required) {
                    if (!required) {
                        return true;
                    }
                    if constexpr (requires { value.itemId.value; }) {
                        return value.itemId.value == stringAt(params, "itemId");
                    }
                    return false;
                };
                const auto ids = [&](bool item, bool turn = true) {
                    return threadMatches() && turnMatches(turn) && itemMatches(item);
                };
                if constexpr (std::is_same_v<Value, typed::AgentMessageDeltaNotification> ||
                              std::is_same_v<Value, typed::CommandExecutionOutputDeltaNotification> ||
                              std::is_same_v<Value, typed::FileChangeOutputDeltaNotification> ||
                              std::is_same_v<Value, typed::PlanDeltaNotification>) {
                    return ids(true) && value.delta == stringAt(params, "delta");
                } else if constexpr (std::is_same_v<Value, typed::TerminalInteractionNotification>) {
                    return ids(true) && value.processId == stringAt(params, "processId") &&
                           value.stdin == stringAt(params, "stdin");
                } else if constexpr (std::is_same_v<Value, typed::ItemCompletedNotification>) {
                    return ids(false) && value.completedAtMs == integerAt(params, "completedAtMs") &&
                           itemRaw(value.item) == params.at("item");
                } else if constexpr (std::is_same_v<Value, typed::FileChangePatchUpdatedNotification>) {
                    if (!ids(true) || value.changes.size() != params.at("changes").size()) {
                        return false;
                    }
                    for (std::size_t index = 0; index < value.changes.size(); ++index) {
                        const auto& raw = params.at("changes").at(index);
                        if (value.changes[index].diff != stringAt(raw, "diff") ||
                            value.changes[index].path != stringAt(raw, "path") ||
                            patchChangeKind(value.changes[index].kind) != stringAt(raw.at("kind"), "type")) {
                            return false;
                        }
                    }
                    return true;
                } else if constexpr (std::is_same_v<Value, typed::McpToolCallProgressNotification>) {
                    return ids(true) && value.message == stringAt(params, "message");
                } else if constexpr (std::is_same_v<Value, typed::ReasoningSummaryPartAddedNotification>) {
                    return ids(true) && value.summaryIndex == integerAt(params, "summaryIndex");
                } else if constexpr (std::is_same_v<Value, typed::ReasoningSummaryTextDeltaNotification>) {
                    return ids(true) && value.summaryIndex == integerAt(params, "summaryIndex") &&
                           value.delta == stringAt(params, "delta");
                } else if constexpr (std::is_same_v<Value, typed::ReasoningTextDeltaNotification>) {
                    return ids(true) && value.contentIndex == integerAt(params, "contentIndex") &&
                           value.delta == stringAt(params, "delta");
                } else if constexpr (std::is_same_v<Value, typed::ItemStartedNotification>) {
                    return ids(false) && value.startedAtMs == integerAt(params, "startedAtMs") &&
                           itemRaw(value.item) == params.at("item");
                } else if constexpr (std::is_same_v<Value, typed::ThreadArchivedNotification> ||
                                     std::is_same_v<Value, typed::ThreadClosedNotification> ||
                                     std::is_same_v<Value, typed::ThreadDeletedNotification> ||
                                     std::is_same_v<Value, typed::ThreadGoalClearedNotification> ||
                                     std::is_same_v<Value, typed::ThreadUnarchivedNotification>) {
                    return value.threadId.value == stringAt(params, "threadId");
                } else if constexpr (std::is_same_v<Value, typed::ContextCompactedNotification>) {
                    return ids(false);
                } else if constexpr (std::is_same_v<Value, typed::ThreadGoalUpdatedNotification>) {
                    const auto& goal = params.at("goal");
                    return value.threadId.value == stringAt(params, "threadId") &&
                           optionalStringMatches(value.turnId, params, "turnId") && value.goal.raw == goal &&
                           value.goal.createdAt == integerAt(goal, "createdAt") &&
                           value.goal.objective == stringAt(goal, "objective") &&
                           value.goal.status.value == stringAt(goal, "status") &&
                           value.goal.threadId.value == stringAt(goal, "threadId") &&
                           value.goal.timeUsedSeconds == integerAt(goal, "timeUsedSeconds") &&
                           optionalIntegerMatches(value.goal.tokenBudget, goal, "tokenBudget") &&
                           value.goal.tokensUsed == integerAt(goal, "tokensUsed") &&
                           value.goal.updatedAt == integerAt(goal, "updatedAt");
                } else if constexpr (std::is_same_v<Value, typed::ThreadNameUpdatedNotification>) {
                    return value.threadId.value == stringAt(params, "threadId") &&
                           optionalStringMatches(value.threadName, params, "threadName");
                } else if constexpr (std::is_same_v<Value, typed::ThreadRealtimeClosedNotification>) {
                    return value.threadId.value == stringAt(params, "threadId") &&
                           optionalStringMatches(value.reason, params, "reason");
                } else if constexpr (std::is_same_v<Value, typed::ThreadRealtimeErrorNotification>) {
                    return value.threadId.value == stringAt(params, "threadId") &&
                           value.message == stringAt(params, "message");
                } else if constexpr (std::is_same_v<Value, typed::ThreadRealtimeItemAddedNotification>) {
                    const auto& raw = params.at("item");
                    return value.threadId.value == stringAt(params, "threadId") &&
                           (raw.is_null() ? !value.item : value.item && *value.item == raw);
                } else if constexpr (std::is_same_v<Value, typed::ThreadRealtimeOutputAudioDeltaNotification>) {
                    const auto& audio = params.at("audio");
                    return value.threadId.value == stringAt(params, "threadId") && value.audio.raw == audio &&
                           value.audio.data == stringAt(audio, "data") &&
                           optionalStringMatches(value.audio.itemId, audio, "itemId") &&
                           value.audio.numChannels == audio.at("numChannels").get<std::uint16_t>() &&
                           value.audio.sampleRate == audio.at("sampleRate").get<std::uint32_t>() &&
                           optionalIntegerMatches(value.audio.samplesPerChannel, audio, "samplesPerChannel");
                } else if constexpr (std::is_same_v<Value, typed::ThreadRealtimeSdpNotification>) {
                    return value.threadId.value == stringAt(params, "threadId") && value.sdp == stringAt(params, "sdp");
                } else if constexpr (std::is_same_v<Value, typed::ThreadRealtimeStartedNotification>) {
                    return value.threadId.value == stringAt(params, "threadId") &&
                           optionalStringMatches(value.realtimeSessionId, params, "realtimeSessionId") &&
                           value.version.value == stringAt(params, "version");
                } else if constexpr (std::is_same_v<Value, typed::ThreadRealtimeTranscriptDeltaNotification>) {
                    return value.threadId.value == stringAt(params, "threadId") &&
                           value.delta == stringAt(params, "delta") && value.role == stringAt(params, "role");
                } else if constexpr (std::is_same_v<Value, typed::ThreadRealtimeTranscriptDoneNotification>) {
                    return value.threadId.value == stringAt(params, "threadId") &&
                           value.role == stringAt(params, "role") && value.text == stringAt(params, "text");
                } else if constexpr (std::is_same_v<Value, typed::ThreadSettingsUpdatedNotification>) {
                    const auto& settings = params.at("threadSettings");
                    const auto& collaboration = settings.at("collaborationMode");
                    const auto& nestedSettings = collaboration.at("settings");
                    const auto activeProfileMember = settings.find("activePermissionProfile");
                    const bool activeProfile =
                        activeProfileMember == settings.end()
                            ? value.threadSettings.activePermissionProfile.isOmitted()
                            : (activeProfileMember->is_null()
                                   ? value.threadSettings.activePermissionProfile.isNull()
                                   : value.threadSettings.activePermissionProfile.hasValue() &&
                                         value.threadSettings.activePermissionProfile->raw == *activeProfileMember &&
                                         optionalStringMatches(value.threadSettings.activePermissionProfile->extends,
                                                               *activeProfileMember,
                                                               "extends") &&
                                         value.threadSettings.activePermissionProfile->id ==
                                             stringAt(*activeProfileMember, "id"));
                    const bool scalarApproval =
                        std::holds_alternative<typed::ApprovalPolicy>(value.threadSettings.approvalPolicy) &&
                        std::get<typed::ApprovalPolicy>(value.threadSettings.approvalPolicy).value ==
                            stringAt(settings, "approvalPolicy");
                    return value.threadId.value == stringAt(params, "threadId") &&
                           value.threadSettings.raw == settings && activeProfile && scalarApproval &&
                           unionRaw(value.threadSettings.sandboxPolicy) == settings.at("sandboxPolicy") &&
                           value.threadSettings.model.value == stringAt(settings, "model") &&
                           value.threadSettings.modelProvider == stringAt(settings, "modelProvider") &&
                           value.threadSettings.cwd.value == stringAt(settings, "cwd") &&
                           value.threadSettings.approvalsReviewer.value == stringAt(settings, "approvalsReviewer") &&
                           value.threadSettings.collaborationMode.raw == collaboration &&
                           value.threadSettings.collaborationMode.mode.value ==
                               stringAt(collaboration, "mode") &&
                           value.threadSettings.collaborationMode.settings.raw == nestedSettings &&
                           optionalStringMatches(value.threadSettings.collaborationMode.settings.developerInstructions,
                                                 nestedSettings,
                                                 "developer_instructions") &&
                           value.threadSettings.collaborationMode.settings.model.value ==
                               stringAt(nestedSettings, "model") &&
                           optionalStringMatches(value.threadSettings.collaborationMode.settings.reasoningEffort,
                                                 nestedSettings,
                                                 "reasoning_effort") &&
                           optionalStringMatches(value.threadSettings.effort, settings, "effort") &&
                           optionalStringMatches(value.threadSettings.summary, settings, "summary") &&
                           optionalStringMatches(value.threadSettings.personality, settings, "personality") &&
                           optionalStringMatches(value.threadSettings.serviceTier, settings, "serviceTier");
                } else if constexpr (std::is_same_v<Value, typed::ThreadStartedNotification>) {
                    return value.thread.raw == params.at("thread") &&
                           value.thread.id.value == stringAt(params.at("thread"), "id");
                } else if constexpr (std::is_same_v<Value, typed::ThreadStatusChangedNotification>) {
                    return value.threadId.value == stringAt(params, "threadId") &&
                           typed::threadStatusRaw(value.status) == params.at("status");
                } else if constexpr (std::is_same_v<Value, typed::ThreadTokenUsageUpdatedNotification>) {
                    const auto& usage = params.at("tokenUsage");
                    const auto breakdown = [](const typed::TokenUsageBreakdown& decoded, const codex::Json& raw) {
                        return decoded.raw == raw &&
                               decoded.cachedInputTokens == raw.at("cachedInputTokens").get<std::int64_t>() &&
                               decoded.inputTokens == raw.at("inputTokens").get<std::int64_t>() &&
                               decoded.outputTokens == raw.at("outputTokens").get<std::int64_t>() &&
                               decoded.reasoningOutputTokens == raw.at("reasoningOutputTokens").get<std::int64_t>() &&
                               decoded.totalTokens == raw.at("totalTokens").get<std::int64_t>();
                    };
                    return ids(false) && value.tokenUsage.raw == usage && breakdown(value.tokenUsage.last, usage.at("last")) &&
                           optionalIntegerMatches(value.tokenUsage.modelContextWindow, usage, "modelContextWindow") &&
                           breakdown(value.tokenUsage.total, usage.at("total"));
                } else if constexpr (std::is_same_v<Value, typed::TurnCompletedNotification> ||
                                     std::is_same_v<Value, typed::TurnStartedNotification>) {
                    return value.threadId.value == stringAt(params, "threadId") && value.turn.raw == params.at("turn") &&
                           value.turn.id.value == stringAt(params.at("turn"), "id");
                } else if constexpr (std::is_same_v<Value, typed::TurnDiffUpdatedNotification>) {
                    return ids(false) && value.diff == stringAt(params, "diff");
                } else if constexpr (std::is_same_v<Value, typed::TurnModerationMetadataNotification>) {
                    const auto& raw = params.at("metadata");
                    return ids(false) && (raw.is_null() ? !value.metadata : value.metadata && *value.metadata == raw);
                } else if constexpr (std::is_same_v<Value, typed::TurnPlanUpdatedNotification>) {
                    if (!ids(false) || !optionalStringMatches(value.explanation, params, "explanation") ||
                        value.plan.size() != params.at("plan").size()) {
                        return false;
                    }
                    for (std::size_t index = 0; index < value.plan.size(); ++index) {
                        const auto& raw = params.at("plan").at(index);
                        if (value.plan[index].raw != raw ||
                            value.plan[index].status.value != stringAt(raw, "status") ||
                            value.plan[index].step != stringAt(raw, "step")) {
                            return false;
                        }
                    }
                    return true;
                }
                return false;
            },
            notification);
    }

    const detail::ServerNotificationCodecDescriptor*
    descriptorFor(const detail::ProtocolSurfaceEntry& row,
                  std::span<const detail::ServerNotificationCodecDescriptor> descriptors) {
        const auto* target = std::get_if<detail::ServerNotificationTarget>(&row.runtimeTarget);
        if (target == nullptr) {
            return nullptr;
        }
        const auto descriptor = std::find_if(descriptors.begin(), descriptors.end(), [&](const auto& candidate) {
            return candidate.target == *target;
        });
        return descriptor == descriptors.end() || descriptor->key != row.key ? nullptr : &*descriptor;
    }

    struct Observation {
        std::vector<std::string> codes;
        std::optional<typed::Event> event;
        bool protocolEnvelopeAccepted = false;
    };

    Observation decodeFixture(const codex::Json& raw) {
        std::string error;
        const auto protocol = detail::ProtocolCodec::decode(raw.dump(), error);
        if (!protocol || protocol->kind != detail::ProtocolMessage::Kind::Notification) {
            return {{"ProtocolEnvelopeRejected"}, std::nullopt, false};
        }
        typed::Event event =
            detail::decodeEvent(codex::Notification{protocol->method, protocol->params, protocol->raw});
        return {eventCodes(event), std::move(event), true};
    }
} // namespace

int main() {
    tests::support::TestResult result;
    const std::filesystem::path fixtureRoot = CODEX_A1_FIXTURE_ROOT;
    const codex::Json index = readJson(fixtureRoot / "index.json");
    const codex::Json coverage = readJson(CODEX_A1_NOTIFICATION_PRODUCTION_COVERAGE);

    std::map<std::string, const codex::Json*> indexedById;
    for (const codex::Json& record : index.at("fixtures")) {
        indexedById.emplace(record.at("id").get<std::string>(), &record);
    }
    std::map<std::string, const codex::Json*> notificationCoverageByMethod;
    for (const codex::Json& notification : coverage.at("notifications")) {
        notificationCoverageByMethod.emplace(
            notification.at("surface_key").at("name").get<std::string>(), &notification);
    }

    const auto descriptors = detail::serverNotificationCodecDescriptors();
    const std::size_t a11Descriptors =
        static_cast<std::size_t>(std::count_if(descriptors.begin(), descriptors.end(), [](const auto& descriptor) {
            return descriptor.a11ConversationDomain;
        }));
    result.expectTrue(a11Descriptors == 37, "exactly 37 descriptor rows belong to the A1.1 conversation-domain slice");
    const std::size_t residualPartialDescriptors =
        static_cast<std::size_t>(std::count_if(descriptors.begin(), descriptors.end(), [](const auto& descriptor) {
            const auto& row = detail::entryFor(descriptor.target);
            return !descriptor.a11ConversationDomain && row.typedSchemaStatus == detail::TypedSchemaStatus::Partial &&
                   descriptor.key.name == "error";
        }));
    result.expectTrue(residualPartialDescriptors == 1,
                      "error remains the exact residual partial descriptor row while B3 completes model/rerouted");

    std::map<std::string, std::size_t> roles;
    std::set<detail::ServerNotificationTarget> baseTargets;
    bool exactIndexAgreement = true;
    bool exactRegistryAgreement = true;
    bool exactOutcomeAgreement = true;
    bool exactRawAgreement = true;
    bool exactFieldAgreement = true;
    bool exactAlternativeAgreement = true;

    for (const codex::Json& record : coverage.at("records")) {
        const std::string id = record.at("id").get<std::string>();
        const std::string method = record.at("surface_key").at("name").get<std::string>();
        const std::string role = record.at("role").get<std::string>();
        ++roles[role];

        const auto indexed = indexedById.find(id);
        exactIndexAgreement =
            exactIndexAgreement && indexed != indexedById.end() && indexed->second->at("file") == record.at("file") &&
            indexed->second->at("file_sha256") == record.at("file_sha256") &&
            indexed->second->at("protocol_surface_key") == record.at("surface_key");

        const auto* row =
            detail::findSurface(detail::SurfaceCategory::ServerNotification, "ServerNotification", "method", method);
        const auto* target =
            row == nullptr ? nullptr : std::get_if<detail::ServerNotificationTarget>(&row->runtimeTarget);
        const auto* descriptor = row == nullptr ? nullptr : descriptorFor(*row, descriptors);
        exactRegistryAgreement =
            exactRegistryAgreement && row && target && descriptor && descriptor->a11ConversationDomain &&
            row->stability == detail::Stability::Stable && row->a1Slice == detail::A1Slice::A1_1 &&
            row->runtimeDisposition == detail::RuntimeDisposition::Typed &&
            row->typedImplementation == detail::TypedImplementationStatus::Implemented;
        if (row == nullptr || target == nullptr || descriptor == nullptr || indexed == indexedById.end()) {
            continue;
        }

        const codex::Json raw = readJson(fixtureRoot / record.at("file").get<std::string>());
        Observation observation = decodeFixture(raw);
        const std::vector<std::string> expectedCodes = stringArray(record.at("expected_intrinsic_codes"));
        exactOutcomeAgreement = exactOutcomeAgreement && observation.codes == expectedCodes;
        result.expectTrue(observation.codes == expectedCodes, id + " emits the exact intrinsic diagnostic-code multiset");

        const bool positive = role == "base" || role == "optional_omitted" || role == "nullable_null" ||
                              role == "required_nullable_null";
        if (positive && observation.event) {
            const auto canonical = canonicalNotification(*observation.event);
            exactRawAgreement = exactRawAgreement && canonical && std::visit(
                                                                      [&](const auto& value) {
                                                                          return value.raw == raw;
                                                                      },
                                                                      *canonical);
            exactFieldAgreement = exactFieldAgreement && canonical && canonicalFieldsMatch(*canonical, raw);
            if (role == "base") {
                baseTargets.emplace(*target);
                const auto expectedNotification = notificationCoverageByMethod.find(method);
                const std::string expectedAlternative =
                    expectedNotification == notificationCoverageByMethod.end()
                        ? ""
                        : expectedNotification->second->at("event_alternative").get<std::string>();
                const std::string actualAlternative{eventAlternativeName(*observation.event)};
                const bool adapterAlternative =
                    expectedAlternative == "TurnCompleted|TurnFailed"
                        ? actualAlternative == "TurnCompleted" || actualAlternative == "TurnFailed"
                        : actualAlternative == expectedAlternative;
                exactAlternativeAgreement = exactAlternativeAgreement && canonical &&
                                            canonicalTypeIdentity(*canonical) == descriptor->payloadTypeIdentity &&
                                            adapterAlternative;
            }
        } else if (positive) {
            exactRawAgreement = false;
            exactFieldAgreement = false;
        }
    }

    result.expectTrue(exactIndexAgreement, "all 798 rows retain exact fixture id/path/hash/surface-index agreement");
    result.expectTrue(exactRegistryAgreement,
                      "every corpus row begins at one exact stable A1.1 canonical registry row and generated descriptor");
    result.expectTrue(exactOutcomeAgreement,
                      "all valid, malformed-known, and envelope-rejected fixtures emit only their exact reviewed intrinsic codes");
    result.expectTrue(exactRawAgreement, "every accepted notification retains the complete raw envelope");
    result.expectTrue(exactFieldAgreement, "every accepted notification exposes typed field values matching its schema fixture");
    result.expectTrue(baseTargets.size() == 37 && exactAlternativeAgreement,
                      "all 37 base roots select their exact distinct canonical payload type");
    result.expectTrue(
        coverage.at("records").size() == 798 &&
            roles ==
                std::map<std::string, std::size_t>{
                    {"base", 37},
                    {"missing_required", 279},
                    {"nullable_null", 57},
                    {"optional_omitted", 65},
                    {"required_nullable_null", 2},
                    {"wrong_type", 358},
                },
        "the exact corpus ratchet remains 37 base, 65 omissions, 59 nulls, 279 missing-required, and 358 wrong-type");

    std::size_t enumKnown = 0;
    std::size_t enumUnknown = 0;
    bool enumOutcomes = true;
    for (const codex::Json& record : coverage.at("open_enum_records")) {
        const codex::Json scalar = readJson(fixtureRoot / record.at("file").get<std::string>());
        const std::string family = record.at("family").get<std::string>();
        const std::string method = record.at("owner_surface_key").at("name").get<std::string>();
        const auto notification = std::find_if(coverage.at("notifications").begin(),
                                               coverage.at("notifications").end(),
                                               [&](const codex::Json& candidate) {
                                                   return candidate.at("surface_key").at("name") == method;
                                               });
        if (notification == coverage.at("notifications").end()) {
            enumOutcomes = false;
            continue;
        }
        const auto indexed = indexedById.find(notification->at("base_fixture_id").get<std::string>());
        if (indexed == indexedById.end()) {
            enumOutcomes = false;
            continue;
        }
        codex::Json envelope = readJson(fixtureRoot / indexed->second->at("file").get<std::string>());
        if (family == "ModeKind") {
            envelope["params"]["threadSettings"]["collaborationMode"]["mode"] = scalar;
        } else if (family == "RealtimeConversationVersion") {
            envelope["params"]["version"] = scalar;
        } else if (family == "TurnPlanStepStatus") {
            envelope["params"]["plan"][0]["status"] = scalar;
        } else {
            enumOutcomes = false;
            continue;
        }
        Observation observation = decodeFixture(envelope);
        const auto expected = stringArray(record.at("expected_intrinsic_codes"));
        enumOutcomes = enumOutcomes && observation.codes == expected;
        expected.size() == 1 ? ++enumKnown : ++enumUnknown;
        result.expectTrue(observation.codes == expected,
                          record.at("id").get<std::string>() + " emits the exact open-enum code multiset");
    }
    result.expectTrue(enumKnown == 7 && enumUnknown == 6 && enumOutcomes,
                      "all seven known and six future/empty open-enum fixtures use the production notification path");

    const codex::Json sensitive = {
        {"method", "thread/realtime/error"},
        {"params", {{"threadId", "thread-secret"}, {"message", codex::Json{{"secret", "sensitive-notification-value"}}}}},
    };
    const Observation sensitiveObservation = decodeFixture(sensitive);
    const auto* unknown =
        sensitiveObservation.event ? std::get_if<typed::UnknownEvent>(&*sensitiveObservation.event) : nullptr;
    result.expectTrue(
        sensitiveObservation.codes == std::vector<std::string>{"MalformedKnownPayload", "ProtocolWarning"} && unknown &&
            unknown->diagnostic && unknown->diagnostic->surface == "thread/realtime/error" &&
            unknown->diagnostic->fieldPath == "$.params" &&
            unknown->diagnostic->message.find("sensitive-notification-value") == std::string::npos &&
            unknown->decodingError &&
            unknown->decodingError->find("sensitive-notification-value") == std::string::npos,
        "malformed-known notification diagnostics contain identity/path but never secret payload values");

    return result.processResult();
}
