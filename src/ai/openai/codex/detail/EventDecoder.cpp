/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/EventDecoder.h"

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/detail/AccountCodec.h"
#include "ai/openai/codex/detail/CodexErrorInfoCodec.h"
#include "ai/openai/codex/detail/ConversationCodec.h"
#include "ai/openai/codex/detail/DecodeDiagnostic.h"
#include "ai/openai/codex/detail/ItemDecoder.h"
#include "ai/openai/codex/detail/ModelCodec.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "ai/openai/codex/detail/ThreadCodec.h"
#include "ai/openai/codex/detail/TurnCodec.h"
#include "ai/openai/codex/typed/Models.h"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace ai::openai::codex::detail {

    namespace {

        const Json* findMember(const Json& object, const char* name) {
            if (!object.is_object()) {
                return nullptr;
            }

            const auto member = object.find(name);
            return member == object.end() ? nullptr : &*member;
        }

        bool requireParamsObject(const Notification& notification, std::string& error) {
            if (!notification.params.is_object()) {
                error = "notification params must be an object";
                return false;
            }
            return true;
        }

        bool requireString(const Json& object, const char* name, std::string& value, std::string& error) {
            const Json* member = findMember(object, name);
            if (member == nullptr) {
                error = std::string("notification params are missing required '") + name + "' field";
                return false;
            }
            if (!member->is_string()) {
                error = std::string("notification param '") + name + "' must be a string";
                return false;
            }

            value = member->get<std::string>();
            return true;
        }

        bool requireBoolean(const Json& object, const char* name, bool& value, std::string& error) {
            const Json* member = findMember(object, name);
            if (member == nullptr) {
                error = std::string("notification params are missing required '") + name + "' field";
                return false;
            }
            if (!member->is_boolean()) {
                error = std::string("notification param '") + name + "' must be a boolean";
                return false;
            }

            value = member->get<bool>();
            return true;
        }

        bool requireObject(const Json& object, const char* name, const Json*& value, std::string& error) {
            value = findMember(object, name);
            if (value == nullptr) {
                error = std::string("notification params are missing required '") + name + "' field";
                return false;
            }
            if (!value->is_object()) {
                error = std::string("notification param '") + name + "' must be an object";
                return false;
            }
            return true;
        }

        bool requireArray(const Json& object, const char* name, const Json*& value, std::string& error) {
            value = findMember(object, name);
            if (value == nullptr) {
                error = std::string("notification params are missing required '") + name + "' field";
                return false;
            }
            if (!value->is_array()) {
                error = std::string("notification param '") + name + "' must be an array";
                return false;
            }
            return true;
        }

        bool requireInt64(const Json& object, const char* name, std::int64_t& value, std::string& error) {
            const Json* member = findMember(object, name);
            if (member == nullptr) {
                error = std::string("notification params are missing required '") + name + "' field";
                return false;
            }

            if (member->is_number_unsigned()) {
                const std::uint64_t integer = member->get<std::uint64_t>();
                if (integer > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                    error = std::string("notification param '") + name + "' is outside the int64 range";
                    return false;
                }
                value = static_cast<std::int64_t>(integer);
                return true;
            }
            if (member->is_number_integer()) {
                value = member->get<std::int64_t>();
                return true;
            }

            error = std::string("notification param '") + name + "' must be an integer";
            return false;
        }

        bool decodeStringValue(const Json& value, std::string& output, std::string& error) {
            if (!value.is_string()) {
                error = "value must be a string";
                return false;
            }
            output = value.get<std::string>();
            return true;
        }

        bool decodeInt64Value(const Json& value, std::int64_t& output, std::string& error) {
            if (value.is_number_unsigned()) {
                const auto number = value.get<std::uint64_t>();
                if (number > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
                    error = "value is outside the int64 range";
                    return false;
                }
                output = static_cast<std::int64_t>(number);
                return true;
            }
            if (value.is_number_integer()) {
                output = value.get<std::int64_t>();
                return true;
            }
            error = "value must be an integer";
            return false;
        }

        template <typename Unsigned>
        bool decodeUnsignedValue(const Json& value, Unsigned& output, std::string& error) {
            std::uint64_t number = 0;
            if (value.is_number_unsigned()) {
                number = value.get<std::uint64_t>();
            } else if (value.is_number_integer()) {
                const std::int64_t signedNumber = value.get<std::int64_t>();
                if (signedNumber < 0) {
                    error = "value must be a non-negative integer";
                    return false;
                }
                number = static_cast<std::uint64_t>(signedNumber);
            } else {
                error = "value must be a non-negative integer";
                return false;
            }
            if (number > static_cast<std::uint64_t>(std::numeric_limits<Unsigned>::max())) {
                error = "value is outside its unsigned integer range";
                return false;
            }
            output = static_cast<Unsigned>(number);
            return true;
        }

        template <typename T, typename Decoder>
        bool decodeOptionalNullable(const Json& object,
                                    const char* name,
                                    typed::OptionalNullable<T>& output,
                                    Decoder&& decoder,
                                    std::string& error) {
            const Json* value = findMember(object, name);
            if (value == nullptr) {
                output = typed::OptionalNullable<T>::omitted();
                return true;
            }
            if (value->is_null()) {
                output = typed::OptionalNullable<T>::explicitNull();
                return true;
            }
            T decoded;
            std::string nestedError;
            if (!decoder(*value, decoded, nestedError)) {
                error = std::string("notification param '") + name + "' " + nestedError;
                return false;
            }
            output = typed::OptionalNullable<T>::withValue(std::move(decoded));
            return true;
        }

        template <typename StrongId>
        bool decodeStrongId(const Json& value, StrongId& output, std::string& error) {
            std::string decoded;
            if (!decodeStringValue(value, decoded, error)) {
                return false;
            }
            output.value = std::move(decoded);
            return true;
        }

        typed::DecodeDiagnostic prefixedDiagnostic(typed::DecodeDiagnostic diagnostic, std::string_view prefix) {
            if (diagnostic.fieldPath.empty() || diagnostic.fieldPath == "$") {
                diagnostic.fieldPath = std::string(prefix);
            } else if (diagnostic.fieldPath.starts_with("$")) {
                diagnostic.fieldPath = std::string(prefix) + diagnostic.fieldPath.substr(1);
            }
            return diagnostic;
        }

        template <typename T>
        void appendDiagnostic(std::vector<typed::DecodeDiagnostic>& diagnostics,
                              const ConversationDecodeResult<T>& decoded,
                              std::string_view prefix) {
            if (decoded.diagnostic) {
                diagnostics.emplace_back(prefixedDiagnostic(*decoded.diagnostic, prefix));
            }
        }

        bool requireAny(const Json& object, const char* name, std::optional<Json>& value, std::string& error) {
            const Json* member = findMember(object, name);
            if (member == nullptr) {
                error = std::string("notification params are missing required '") + name + "' field";
                return false;
            }
            if (member->is_null()) {
                value = std::nullopt;
            } else {
                value = *member;
            }
            return true;
        }

        bool decodeThreadGoalValue(const Json& value,
                                   typed::ThreadGoal& output,
                                   std::vector<typed::DecodeDiagnostic>& diagnostics,
                                   std::string& error) {
            if (!value.is_object()) {
                error = "must be an object";
                return false;
            }
            std::string threadId;
            std::string status;
            if (!requireInt64(value, "createdAt", output.createdAt, error) ||
                !requireString(value, "objective", output.objective, error) ||
                !requireString(value, "status", status, error) || !requireString(value, "threadId", threadId, error) ||
                !requireInt64(value, "timeUsedSeconds", output.timeUsedSeconds, error) ||
                !decodeOptionalNullable(value, "tokenBudget", output.tokenBudget, decodeInt64Value, error) ||
                !requireInt64(value, "tokensUsed", output.tokensUsed, error) ||
                !requireInt64(value, "updatedAt", output.updatedAt, error)) {
                return false;
            }
            output.threadId.value = std::move(threadId);
            output.status.value = std::move(status);
            if (!output.status.isKnown()) {
                auto diagnostic = unknownEnumDiagnostic("ThreadGoalStatus", "$.status");
                output.diagnostics.emplace_back(diagnostic);
                diagnostics.emplace_back(std::move(diagnostic));
            }
            output.raw = value;
            return true;
        }

        bool decodeFileUpdateChangeValue(const Json& value,
                                         typed::FileUpdateChange& output,
                                         std::vector<typed::DecodeDiagnostic>& diagnostics,
                                         std::string& error,
                                         std::size_t index) {
            if (!value.is_object()) {
                error = "notification param 'changes[" + std::to_string(index) + "]' must be an object";
                return false;
            }
            const Json* kind = nullptr;
            if (!requireString(value, "diff", output.diff, error) || !requireObject(value, "kind", kind, error) ||
                !requireString(value, "path", output.path, error)) {
                return false;
            }
            auto decoded = decodePatchChangeKind(*kind);
            if (!decoded.value) {
                error = "notification param 'changes[" + std::to_string(index) + "].kind' does not match PatchChangeKind";
                return false;
            }
            output.kind = std::move(*decoded.value);
            appendDiagnostic(diagnostics, decoded, "$.params.changes[" + std::to_string(index) + "].kind");
            return true;
        }

        bool decodeTokenUsageBreakdownValue(const Json& value,
                                            typed::TokenUsageBreakdown& output,
                                            std::string& error,
                                            std::string_view field) {
            if (!value.is_object()) {
                error = std::string("notification param '") + std::string(field) + "' must be an object";
                return false;
            }
            if (!requireInt64(value, "cachedInputTokens", output.cachedInputTokens, error) ||
                !requireInt64(value, "inputTokens", output.inputTokens, error) ||
                !requireInt64(value, "outputTokens", output.outputTokens, error) ||
                !requireInt64(value, "reasoningOutputTokens", output.reasoningOutputTokens, error) ||
                !requireInt64(value, "totalTokens", output.totalTokens, error)) {
                return false;
            }
            output.raw = value;
            return true;
        }

        bool decodeThreadTokenUsageValue(const Json& value, typed::ThreadTokenUsage& output, std::string& error) {
            if (!value.is_object()) {
                error = "notification param 'tokenUsage' must be an object";
                return false;
            }
            const Json* last = nullptr;
            const Json* total = nullptr;
            if (!requireObject(value, "last", last, error) || !requireObject(value, "total", total, error) ||
                !decodeTokenUsageBreakdownValue(*last, output.last, error, "tokenUsage.last") ||
                !decodeOptionalNullable(value, "modelContextWindow", output.modelContextWindow, decodeInt64Value, error) ||
                !decodeTokenUsageBreakdownValue(*total, output.total, error, "tokenUsage.total")) {
                return false;
            }
            output.raw = value;
            return true;
        }

        bool decodeActivePermissionProfileValue(const Json& value, typed::ActivePermissionProfile& output, std::string& error) {
            if (!value.is_object()) {
                error = "must be an object";
                return false;
            }
            if (!decodeOptionalNullable(value, "extends", output.extends, decodeStringValue, error) ||
                !requireString(value, "id", output.id, error)) {
                return false;
            }
            output.raw = value;
            return true;
        }

        bool decodeSettingsValue(const Json& value, typed::Settings& output, std::string& error) {
            if (!value.is_object()) {
                error = "must be an object";
                return false;
            }
            std::string model;
            if (!decodeOptionalNullable(value, "developer_instructions", output.developerInstructions, decodeStringValue, error) ||
                !requireString(value, "model", model, error) ||
                !decodeOptionalNullable(
                    value,
                    "reasoning_effort",
                    output.reasoningEffort,
                    [](const Json& input, typed::ReasoningEffort& decoded, std::string& nestedError) {
                        std::string raw;
                        if (!decodeStringValue(input, raw, nestedError)) {
                            return false;
                        }
                        decoded.value = std::move(raw);
                        return true;
                    },
                    error)) {
                return false;
            }
            output.model.value = std::move(model);
            if (output.reasoningEffort.hasValue() && !output.reasoningEffort->isKnown()) {
                output.diagnostics.emplace_back(unknownEnumDiagnostic("ReasoningEffort", "$.reasoning_effort"));
            }
            output.raw = value;
            return true;
        }

        bool decodeCollaborationModeValue(const Json& value, typed::CollaborationMode& output, std::string& error) {
            if (!value.is_object()) {
                error = "must be an object";
                return false;
            }
            std::string mode;
            const Json* settings = nullptr;
            if (!requireString(value, "mode", mode, error) || !requireObject(value, "settings", settings, error) ||
                !decodeSettingsValue(*settings, output.settings, error)) {
                return false;
            }
            output.mode.value = std::move(mode);
            if (!output.mode.isKnown()) {
                output.diagnostics.emplace_back(unknownEnumDiagnostic("ModeKind", "$.mode"));
            }
            for (const auto& diagnostic : output.settings.diagnostics) {
                output.diagnostics.emplace_back(prefixedDiagnostic(diagnostic, "$.settings"));
            }
            output.raw = value;
            return true;
        }

        template <typename OpenValue>
        bool decodeOpenValue(const Json& value, OpenValue& output, std::string& error) {
            return decodeStringValue(value, output.value, error);
        }

        bool decodeThreadSettingsValue(const Json& value, typed::ThreadSettings& output, std::string& error) {
            if (!value.is_object()) {
                error = "notification param 'threadSettings' must be an object";
                return false;
            }
            const Json* approvalPolicy = findMember(value, "approvalPolicy");
            const Json* sandboxPolicy = findMember(value, "sandboxPolicy");
            const Json* collaborationMode = nullptr;
            std::string reviewer;
            std::string cwd;
            std::string model;
            if (approvalPolicy == nullptr) {
                error = "notification params are missing required 'approvalPolicy' field";
                return false;
            }
            if (sandboxPolicy == nullptr) {
                error = "notification params are missing required 'sandboxPolicy' field";
                return false;
            }
            if (!requireString(value, "approvalsReviewer", reviewer, error) ||
                !requireObject(value, "collaborationMode", collaborationMode, error) ||
                !requireString(value, "cwd", cwd, error) || !requireString(value, "model", model, error) ||
                !requireString(value, "modelProvider", output.modelProvider, error) ||
                !decodeOptionalNullable(
                    value, "activePermissionProfile", output.activePermissionProfile, decodeActivePermissionProfileValue, error) ||
                !decodeCollaborationModeValue(*collaborationMode, output.collaborationMode, error) ||
                !decodeOptionalNullable(value, "effort", output.effort, decodeOpenValue<typed::ReasoningEffort>, error) ||
                !decodeOptionalNullable(value, "summary", output.summary, decodeOpenValue<typed::ReasoningSummary>, error) ||
                !decodeOptionalNullable(value, "personality", output.personality, decodeOpenValue<typed::Personality>, error) ||
                !decodeOptionalNullable(value, "serviceTier", output.serviceTier, decodeStringValue, error)) {
                return false;
            }
            auto approval = decodeAskForApproval(*approvalPolicy);
            if (!approval.value) {
                error = "notification param 'approvalPolicy' does not match AskForApproval";
                return false;
            }
            auto sandbox = decodeSandboxPolicy(*sandboxPolicy);
            if (!sandbox.value) {
                error = "notification param 'sandboxPolicy' does not match SandboxPolicy";
                return false;
            }
            output.approvalPolicy = std::move(*approval.value);
            output.sandboxPolicy = std::move(*sandbox.value);
            output.approvalsReviewer.value = std::move(reviewer);
            output.cwd.value = std::move(cwd);
            output.model.value = std::move(model);
            appendDiagnostic(output.diagnostics, approval, "$.approvalPolicy");
            appendDiagnostic(output.diagnostics, sandbox, "$.sandboxPolicy");
            for (const auto& diagnostic : output.collaborationMode.diagnostics) {
                output.diagnostics.emplace_back(prefixedDiagnostic(diagnostic, "$.collaborationMode"));
            }
            if (!output.approvalsReviewer.isKnown()) {
                output.diagnostics.emplace_back(unknownEnumDiagnostic("ApprovalsReviewer", "$.approvalsReviewer"));
            }
            if (output.effort.hasValue() && !output.effort->isKnown()) {
                output.diagnostics.emplace_back(unknownEnumDiagnostic("ReasoningEffort", "$.effort"));
            }
            if (output.summary.hasValue() && !output.summary->isKnown()) {
                output.diagnostics.emplace_back(unknownEnumDiagnostic("ReasoningSummary", "$.summary"));
            }
            if (output.personality.hasValue() && !output.personality->isKnown()) {
                output.diagnostics.emplace_back(unknownEnumDiagnostic("Personality", "$.personality"));
            }
            output.raw = value;
            return true;
        }

        bool decodeAudioChunkValue(const Json& value, typed::ThreadRealtimeAudioChunk& output, std::string& error) {
            if (!value.is_object()) {
                error = "notification param 'audio' must be an object";
                return false;
            }
            const Json* numChannels = findMember(value, "numChannels");
            const Json* sampleRate = findMember(value, "sampleRate");
            if (numChannels == nullptr || sampleRate == nullptr || !requireString(value, "data", output.data, error) ||
                !decodeOptionalNullable(value, "itemId", output.itemId, decodeStrongId<typed::ItemId>, error) ||
                !decodeUnsignedValue(*numChannels, output.numChannels, error) ||
                !decodeUnsignedValue(*sampleRate, output.sampleRate, error) ||
                !decodeOptionalNullable(
                    value, "samplesPerChannel", output.samplesPerChannel, decodeUnsignedValue<std::uint32_t>, error)) {
                if (error.empty()) {
                    error = numChannels == nullptr ? "notification params are missing required 'numChannels' field"
                                                  : "notification params are missing required 'sampleRate' field";
                }
                return false;
            }
            output.raw = value;
            return true;
        }

        bool decodeTurnPlanStepValue(const Json& value,
                                     typed::TurnPlanStep& output,
                                     std::vector<typed::DecodeDiagnostic>& diagnostics,
                                     std::string& error,
                                     std::size_t index) {
            if (!value.is_object()) {
                error = "notification param 'plan[" + std::to_string(index) + "]' must be an object";
                return false;
            }
            std::string status;
            if (!requireString(value, "status", status, error) || !requireString(value, "step", output.step, error)) {
                return false;
            }
            output.status.value = std::move(status);
            if (!output.status.isKnown()) {
                auto diagnostic = unknownEnumDiagnostic(
                    "TurnPlanStepStatus", "$.params.plan[" + std::to_string(index) + "].status");
                output.diagnostics.emplace_back(diagnostic);
                diagnostics.emplace_back(std::move(diagnostic));
            }
            output.raw = value;
            return true;
        }

        typed::Event unknownEvent(const Notification& notification, std::optional<std::string> decodingError = std::nullopt) {
            const bool malformed = decodingError.has_value();
            return typed::Event{typed::UnknownEvent{
                notification.method,
                notification.params,
                notification.raw,
                std::move(decodingError),
                malformed ? std::optional<typed::DecodeDiagnostic>{
                                malformedKnownDiagnostic(notification.method, "$.params")}
                          : std::optional<typed::DecodeDiagnostic>{
                                unknownMethodDiagnostic(notification.method)}}};
        }

        typed::Event malformedEvent(const Notification& notification, std::string error) {
            if (error.empty()) {
                error = "notification payload could not be decoded";
            }
            const bool exactPathNotification = notification.method == "account/login/completed" ||
                                               notification.method == "account/rateLimits/updated" ||
                                               notification.method == "account/updated" ||
                                               notification.method == "model/rerouted" ||
                                               notification.method == "model/safetyBuffering/updated" ||
                                               notification.method == "model/verification";
            std::string fieldPath = "$.params";
            if (exactPathNotification) {
                const std::size_t begin = error.find("'$");
                if (begin != std::string::npos) {
                    const std::size_t end = error.find('\'', begin + 1);
                    if (end != std::string::npos) {
                        fieldPath = error.substr(begin + 1, end - begin - 1);
                    }
                }
            }
            typed::Event event = unknownEvent(notification, notification.method + ": " + std::move(error));
            if (auto* unknown = std::get_if<typed::UnknownEvent>(&event); unknown != nullptr && unknown->diagnostic) {
                unknown->diagnostic->fieldPath = std::move(fieldPath);
            }
            return event;
        }

        typed::Event decodeThreadStarted(const Notification& notification) {
            std::string error;
            const Json* threadValue = nullptr;
            if (!requireParamsObject(notification, error) || !requireObject(notification.params, "thread", threadValue, error)) {
                return malformedEvent(notification, std::move(error));
            }

            auto thread = decodeThread(*threadValue, error);
            if (!thread.has_value()) {
                return malformedEvent(notification, "thread: " + std::move(error));
            }
            typed::ThreadStartedNotification canonical;
            canonical.thread = *thread;
            canonical.raw = notification.raw;
            for (const auto& diagnostic : thread->diagnostics) {
                canonical.diagnostics.emplace_back(prefixedDiagnostic(diagnostic, "$.params.thread"));
            }
            typed::ThreadStarted event;
            event.thread = std::move(*thread);
            event.raw = notification.raw;
            event.canonical = std::move(canonical);
            return typed::Event{std::move(event)};
        }

        typed::Event decodeThreadStatusChanged(const Notification& notification) {
            std::string error;
            std::string threadId;
            const Json* statusValue = nullptr;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "threadId", threadId, error) ||
                !requireObject(notification.params, "status", statusValue, error)) {
                return malformedEvent(notification, std::move(error));
            }

            auto status = ::ai::openai::codex::detail::decodeThreadStatus(*statusValue);
            if (!status.value) {
                error = "notification param 'status' does not match ThreadStatus";
                if (status.diagnostic) {
                    error += " at " + status.diagnostic->fieldPath;
                }
                return malformedEvent(notification, std::move(error));
            }
            typed::ThreadStatusChangedNotification canonical;
            canonical.threadId.value = threadId;
            canonical.status = *status.value;
            canonical.raw = notification.raw;
            appendDiagnostic(canonical.diagnostics, status, "$.params.status");

            typed::ThreadStatusChanged event;
            event.threadId.value = std::move(threadId);
            event.status = std::move(*status.value);
            event.raw = notification.raw;
            event.canonical = std::move(canonical);
            return typed::Event{std::move(event)};
        }

        typed::Event decodeTurnStarted(const Notification& notification) {
            std::string error;
            std::string threadIdValue;
            const Json* turnValue = nullptr;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "threadId", threadIdValue, error) ||
                !requireObject(notification.params, "turn", turnValue, error)) {
                return malformedEvent(notification, std::move(error));
            }

            const typed::ThreadId threadId{std::move(threadIdValue)};
            auto turn = decodeTurn(*turnValue, threadId, error);
            if (!turn.has_value()) {
                return malformedEvent(notification, "turn: " + std::move(error));
            }
            typed::TurnStartedNotification canonical;
            canonical.threadId = threadId;
            canonical.turn = *turn;
            canonical.raw = notification.raw;
            for (const auto& diagnostic : turn->diagnostics) {
                canonical.diagnostics.emplace_back(prefixedDiagnostic(diagnostic, "$.params.turn"));
            }
            typed::TurnStarted event;
            event.turn = std::move(*turn);
            event.raw = notification.raw;
            event.canonical = std::move(canonical);
            return typed::Event{std::move(event)};
        }

        typed::Event decodeTurnCompleted(const Notification& notification) {
            std::string error;
            std::string threadIdValue;
            const Json* turnValue = nullptr;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "threadId", threadIdValue, error) ||
                !requireObject(notification.params, "turn", turnValue, error)) {
                return malformedEvent(notification, std::move(error));
            }

            const typed::ThreadId threadId{std::move(threadIdValue)};
            auto turn = decodeTurn(*turnValue, threadId, error);
            if (!turn.has_value()) {
                return malformedEvent(notification, "turn: " + std::move(error));
            }
            typed::TurnCompletedNotification canonical;
            canonical.threadId = threadId;
            canonical.turn = *turn;
            canonical.raw = notification.raw;
            for (const auto& diagnostic : turn->diagnostics) {
                canonical.diagnostics.emplace_back(prefixedDiagnostic(diagnostic, "$.params.turn"));
            }
            if (turn->status.value == "failed") {
                Json turnError = turn->error.hasValue() ? turn->error->raw : Json(nullptr);
                typed::TurnFailed event;
                event.turn = std::move(*turn);
                event.error = std::move(turnError);
                event.raw = notification.raw;
                event.canonical = std::move(canonical);
                return typed::Event{std::move(event)};
            }
            typed::TurnCompleted event;
            event.turn = std::move(*turn);
            event.raw = notification.raw;
            event.canonical = std::move(canonical);
            return typed::Event{std::move(event)};
        }

        typed::Event decodeItemLifecycle(const Notification& notification, bool started) {
            std::string error;
            std::string threadIdValue;
            std::string turnIdValue;
            const Json* itemValue = nullptr;
            const char* timestampName = started ? "startedAtMs" : "completedAtMs";
            std::int64_t timestamp = 0;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "threadId", threadIdValue, error) ||
                !requireString(notification.params, "turnId", turnIdValue, error) ||
                !requireObject(notification.params, "item", itemValue, error) ||
                !requireInt64(notification.params, timestampName, timestamp, error)) {
                return malformedEvent(notification, std::move(error));
            }

            const typed::ThreadId threadId{std::move(threadIdValue)};
            const typed::TurnId turnId{std::move(turnIdValue)};
            auto item = decodeItem(*itemValue, threadId, turnId, error);
            if (!item.has_value()) {
                return malformedEvent(notification, "item: " + std::move(error));
            }
            if (started) {
                typed::ItemStartedNotification canonical;
                canonical.item = *item;
                canonical.startedAtMs = timestamp;
                canonical.threadId = threadId;
                canonical.turnId = turnId;
                canonical.raw = notification.raw;
                typed::ItemStarted event;
                event.item = std::move(*item);
                event.startedAtMs = timestamp;
                event.raw = notification.raw;
                event.canonical = std::move(canonical);
                return typed::Event{std::move(event)};
            }
            typed::ItemCompletedNotification canonical;
            canonical.completedAtMs = timestamp;
            canonical.item = *item;
            canonical.threadId = threadId;
            canonical.turnId = turnId;
            canonical.raw = notification.raw;
            typed::ItemCompleted event;
            event.item = std::move(*item);
            event.completedAtMs = timestamp;
            event.raw = notification.raw;
            event.canonical = std::move(canonical);
            return typed::Event{std::move(event)};
        }

        bool decodeDeltaMetadata(const Notification& notification,
                                 std::string& threadId,
                                 std::string& turnId,
                                 std::string& itemId,
                                 std::string& delta,
                                 std::string& error) {
            return requireParamsObject(notification, error) && requireString(notification.params, "threadId", threadId, error) &&
                   requireString(notification.params, "turnId", turnId, error) &&
                   requireString(notification.params, "itemId", itemId, error) && requireString(notification.params, "delta", delta, error);
        }

        typed::Event decodeAgentMessageDelta(const Notification& notification) {
            std::string error;
            std::string threadId;
            std::string turnId;
            std::string itemId;
            std::string delta;
            if (!decodeDeltaMetadata(notification, threadId, turnId, itemId, delta, error)) {
                return malformedEvent(notification, std::move(error));
            }

            typed::AgentMessageDeltaNotification canonical;
            canonical.delta = delta;
            canonical.itemId.value = itemId;
            canonical.threadId.value = threadId;
            canonical.turnId.value = turnId;
            canonical.raw = notification.raw;
            typed::AgentMessageDelta event;
            event.threadId.value = std::move(threadId);
            event.turnId.value = std::move(turnId);
            event.itemId.value = std::move(itemId);
            event.text = std::move(delta);
            event.raw = notification.raw;
            event.canonical = std::move(canonical);
            return typed::Event{std::move(event)};
        }

        typed::Event decodeReasoningDelta(const Notification& notification, typed::ReasoningDelta::Kind kind) {
            std::string error;
            std::string threadId;
            std::string turnId;
            std::string itemId;
            std::string delta;
            std::int64_t index = 0;
            const char* indexName = kind == typed::ReasoningDelta::Kind::Text ? "contentIndex" : "summaryIndex";
            if (!decodeDeltaMetadata(notification, threadId, turnId, itemId, delta, error) ||
                !requireInt64(notification.params, indexName, index, error)) {
                return malformedEvent(notification, std::move(error));
            }

            typed::ReasoningDelta event;
            event.threadId.value = threadId;
            event.turnId.value = turnId;
            event.itemId.value = itemId;
            event.text = delta;
            event.kind = kind;
            event.index = index;
            event.raw = notification.raw;
            if (kind == typed::ReasoningDelta::Kind::Text) {
                typed::ReasoningTextDeltaNotification canonical;
                canonical.contentIndex = index;
                canonical.delta = std::move(delta);
                canonical.itemId.value = std::move(itemId);
                canonical.threadId.value = std::move(threadId);
                canonical.turnId.value = std::move(turnId);
                canonical.raw = notification.raw;
                event.canonical = std::move(canonical);
            } else {
                typed::ReasoningSummaryTextDeltaNotification canonical;
                canonical.delta = std::move(delta);
                canonical.itemId.value = std::move(itemId);
                canonical.summaryIndex = index;
                canonical.threadId.value = std::move(threadId);
                canonical.turnId.value = std::move(turnId);
                canonical.raw = notification.raw;
                event.canonical = std::move(canonical);
            }
            return typed::Event{std::move(event)};
        }

        typed::Event decodeCommandOutputDelta(const Notification& notification) {
            std::string error;
            std::string threadId;
            std::string turnId;
            std::string itemId;
            std::string delta;
            if (!decodeDeltaMetadata(notification, threadId, turnId, itemId, delta, error)) {
                return malformedEvent(notification, std::move(error));
            }

            typed::CommandExecutionOutputDeltaNotification canonical;
            canonical.delta = delta;
            canonical.itemId.value = itemId;
            canonical.threadId.value = threadId;
            canonical.turnId.value = turnId;
            canonical.raw = notification.raw;
            typed::CommandOutputDelta event;
            event.threadId.value = std::move(threadId);
            event.turnId.value = std::move(turnId);
            event.itemId.value = std::move(itemId);
            event.output = std::move(delta);
            event.raw = notification.raw;
            event.canonical = std::move(canonical);
            return typed::Event{std::move(event)};
        }

        typed::Event decodeFileChangeUpdated(const Notification& notification) {
            std::string error;
            std::string threadId;
            std::string turnId;
            std::string itemId;
            const Json* changes = nullptr;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "threadId", threadId, error) ||
                !requireString(notification.params, "turnId", turnId, error) ||
                !requireString(notification.params, "itemId", itemId, error) ||
                !requireArray(notification.params, "changes", changes, error)) {
                return malformedEvent(notification, std::move(error));
            }

            typed::FileChangePatchUpdatedNotification canonical;
            canonical.itemId.value = itemId;
            canonical.threadId.value = threadId;
            canonical.turnId.value = turnId;
            canonical.raw = notification.raw;
            canonical.changes.reserve(changes->size());
            for (std::size_t index = 0; index < changes->size(); ++index) {
                typed::FileUpdateChange change;
                if (!decodeFileUpdateChangeValue((*changes)[index], change, canonical.diagnostics, error, index)) {
                    return malformedEvent(notification, std::move(error));
                }
                canonical.changes.emplace_back(std::move(change));
            }
            typed::FileChangeUpdated event;
            event.threadId.value = std::move(threadId);
            event.turnId.value = std::move(turnId);
            event.itemId.value = std::move(itemId);
            event.changes = *changes;
            event.raw = notification.raw;
            event.canonical = std::move(canonical);
            return typed::Event{std::move(event)};
        }

        typed::Event decodeTokenUsageUpdated(const Notification& notification) {
            std::string error;
            std::string threadId;
            std::string turnId;
            const Json* tokenUsage = nullptr;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "threadId", threadId, error) ||
                !requireString(notification.params, "turnId", turnId, error) ||
                !requireObject(notification.params, "tokenUsage", tokenUsage, error)) {
                return malformedEvent(notification, std::move(error));
            }

            typed::ThreadTokenUsageUpdatedNotification canonical;
            canonical.threadId.value = threadId;
            canonical.turnId.value = turnId;
            canonical.raw = notification.raw;
            if (!decodeThreadTokenUsageValue(*tokenUsage, canonical.tokenUsage, error)) {
                return malformedEvent(notification, std::move(error));
            }
            typed::TokenUsageUpdated event;
            event.threadId.value = std::move(threadId);
            event.turnId.value = std::move(turnId);
            event.usage = *tokenUsage;
            event.raw = notification.raw;
            event.canonical = std::move(canonical);
            return typed::Event{std::move(event)};
        }

        template <typename EventType>
        typed::Event decodeSimpleDelta(const Notification& notification) {
            std::string error;
            std::string threadId;
            std::string turnId;
            std::string itemId;
            std::string delta;
            if (!decodeDeltaMetadata(notification, threadId, turnId, itemId, delta, error)) {
                return malformedEvent(notification, std::move(error));
            }
            EventType event;
            event.delta = std::move(delta);
            event.itemId.value = std::move(itemId);
            event.threadId.value = std::move(threadId);
            event.turnId.value = std::move(turnId);
            event.raw = notification.raw;
            return typed::Event{std::move(event)};
        }

        template <typename EventType>
        typed::Event decodeThreadIdentityOnly(const Notification& notification) {
            std::string error;
            std::string threadId;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "threadId", threadId, error)) {
                return malformedEvent(notification, std::move(error));
            }
            EventType event;
            event.threadId.value = std::move(threadId);
            event.raw = notification.raw;
            return typed::Event{std::move(event)};
        }

        typed::Event decodeTerminalInteraction(const Notification& notification) {
            std::string error;
            std::string threadId;
            std::string turnId;
            std::string itemId;
            typed::TerminalInteractionNotification event;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "itemId", itemId, error) ||
                !requireString(notification.params, "processId", event.processId, error) ||
                !requireString(notification.params, "stdin", event.stdin, error) ||
                !requireString(notification.params, "threadId", threadId, error) ||
                !requireString(notification.params, "turnId", turnId, error)) {
                return malformedEvent(notification, std::move(error));
            }
            event.itemId.value = std::move(itemId);
            event.threadId.value = std::move(threadId);
            event.turnId.value = std::move(turnId);
            event.raw = notification.raw;
            return typed::Event{std::move(event)};
        }

        typed::Event decodeMcpToolCallProgress(const Notification& notification) {
            std::string error;
            std::string threadId;
            std::string turnId;
            std::string itemId;
            typed::McpToolCallProgressNotification event;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "itemId", itemId, error) ||
                !requireString(notification.params, "message", event.message, error) ||
                !requireString(notification.params, "threadId", threadId, error) ||
                !requireString(notification.params, "turnId", turnId, error)) {
                return malformedEvent(notification, std::move(error));
            }
            event.itemId.value = std::move(itemId);
            event.threadId.value = std::move(threadId);
            event.turnId.value = std::move(turnId);
            event.raw = notification.raw;
            return typed::Event{std::move(event)};
        }

        typed::Event decodeReasoningSummaryPartAdded(const Notification& notification) {
            std::string error;
            std::string threadId;
            std::string turnId;
            std::string itemId;
            typed::ReasoningSummaryPartAddedNotification event;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "itemId", itemId, error) ||
                !requireInt64(notification.params, "summaryIndex", event.summaryIndex, error) ||
                !requireString(notification.params, "threadId", threadId, error) ||
                !requireString(notification.params, "turnId", turnId, error)) {
                return malformedEvent(notification, std::move(error));
            }
            event.itemId.value = std::move(itemId);
            event.threadId.value = std::move(threadId);
            event.turnId.value = std::move(turnId);
            event.raw = notification.raw;
            return typed::Event{std::move(event)};
        }

        typed::Event decodeContextCompacted(const Notification& notification) {
            std::string error;
            std::string threadId;
            std::string turnId;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "threadId", threadId, error) ||
                !requireString(notification.params, "turnId", turnId, error)) {
                return malformedEvent(notification, std::move(error));
            }
            typed::ContextCompactedNotification event;
            event.threadId.value = std::move(threadId);
            event.turnId.value = std::move(turnId);
            event.raw = notification.raw;
            return typed::Event{std::move(event)};
        }

        typed::Event decodeThreadGoalUpdated(const Notification& notification) {
            std::string error;
            std::string threadId;
            const Json* goal = nullptr;
            typed::ThreadGoalUpdatedNotification event;
            if (!requireParamsObject(notification, error) || !requireObject(notification.params, "goal", goal, error) ||
                !requireString(notification.params, "threadId", threadId, error) ||
                !decodeOptionalNullable(notification.params, "turnId", event.turnId, decodeStrongId<typed::TurnId>, error) ||
                !decodeThreadGoalValue(*goal, event.goal, event.diagnostics, error)) {
                return malformedEvent(notification, std::move(error));
            }
            event.threadId.value = std::move(threadId);
            event.raw = notification.raw;
            for (auto& diagnostic : event.diagnostics) {
                diagnostic = prefixedDiagnostic(std::move(diagnostic), "$.params.goal");
            }
            return typed::Event{std::move(event)};
        }

        typed::Event decodeThreadNameUpdated(const Notification& notification) {
            std::string error;
            std::string threadId;
            typed::ThreadNameUpdatedNotification event;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "threadId", threadId, error) ||
                !decodeOptionalNullable(notification.params, "threadName", event.threadName, decodeStringValue, error)) {
                return malformedEvent(notification, std::move(error));
            }
            event.threadId.value = std::move(threadId);
            event.raw = notification.raw;
            return typed::Event{std::move(event)};
        }

        typed::Event decodeThreadRealtimeClosed(const Notification& notification) {
            std::string error;
            std::string threadId;
            typed::ThreadRealtimeClosedNotification event;
            if (!requireParamsObject(notification, error) ||
                !decodeOptionalNullable(notification.params, "reason", event.reason, decodeStringValue, error) ||
                !requireString(notification.params, "threadId", threadId, error)) {
                return malformedEvent(notification, std::move(error));
            }
            event.threadId.value = std::move(threadId);
            event.raw = notification.raw;
            return typed::Event{std::move(event)};
        }

        typed::Event decodeThreadRealtimeError(const Notification& notification) {
            std::string error;
            std::string threadId;
            typed::ThreadRealtimeErrorNotification event;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "message", event.message, error) ||
                !requireString(notification.params, "threadId", threadId, error)) {
                return malformedEvent(notification, std::move(error));
            }
            event.threadId.value = std::move(threadId);
            event.raw = notification.raw;
            return typed::Event{std::move(event)};
        }

        typed::Event decodeThreadRealtimeItemAdded(const Notification& notification) {
            std::string error;
            std::string threadId;
            typed::ThreadRealtimeItemAddedNotification event;
            if (!requireParamsObject(notification, error) || !requireAny(notification.params, "item", event.item, error) ||
                !requireString(notification.params, "threadId", threadId, error)) {
                return malformedEvent(notification, std::move(error));
            }
            event.threadId.value = std::move(threadId);
            event.raw = notification.raw;
            return typed::Event{std::move(event)};
        }

        typed::Event decodeThreadRealtimeOutputAudioDelta(const Notification& notification) {
            std::string error;
            std::string threadId;
            const Json* audio = nullptr;
            typed::ThreadRealtimeOutputAudioDeltaNotification event;
            if (!requireParamsObject(notification, error) || !requireObject(notification.params, "audio", audio, error) ||
                !requireString(notification.params, "threadId", threadId, error) ||
                !decodeAudioChunkValue(*audio, event.audio, error)) {
                return malformedEvent(notification, std::move(error));
            }
            event.threadId.value = std::move(threadId);
            event.raw = notification.raw;
            return typed::Event{std::move(event)};
        }

        typed::Event decodeThreadRealtimeSdp(const Notification& notification) {
            std::string error;
            std::string threadId;
            typed::ThreadRealtimeSdpNotification event;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "sdp", event.sdp, error) ||
                !requireString(notification.params, "threadId", threadId, error)) {
                return malformedEvent(notification, std::move(error));
            }
            event.threadId.value = std::move(threadId);
            event.raw = notification.raw;
            return typed::Event{std::move(event)};
        }

        typed::Event decodeThreadRealtimeStarted(const Notification& notification) {
            std::string error;
            std::string threadId;
            std::string version;
            typed::ThreadRealtimeStartedNotification event;
            if (!requireParamsObject(notification, error) ||
                !decodeOptionalNullable(
                    notification.params, "realtimeSessionId", event.realtimeSessionId, decodeStringValue, error) ||
                !requireString(notification.params, "threadId", threadId, error) ||
                !requireString(notification.params, "version", version, error)) {
                return malformedEvent(notification, std::move(error));
            }
            event.threadId.value = std::move(threadId);
            event.version.value = std::move(version);
            if (!event.version.isKnown()) {
                event.diagnostics.emplace_back(
                    unknownEnumDiagnostic("RealtimeConversationVersion", "$.params.version"));
            }
            event.raw = notification.raw;
            return typed::Event{std::move(event)};
        }

        typed::Event decodeThreadRealtimeTranscriptDelta(const Notification& notification) {
            std::string error;
            std::string threadId;
            typed::ThreadRealtimeTranscriptDeltaNotification event;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "delta", event.delta, error) ||
                !requireString(notification.params, "role", event.role, error) ||
                !requireString(notification.params, "threadId", threadId, error)) {
                return malformedEvent(notification, std::move(error));
            }
            event.threadId.value = std::move(threadId);
            event.raw = notification.raw;
            return typed::Event{std::move(event)};
        }

        typed::Event decodeThreadRealtimeTranscriptDone(const Notification& notification) {
            std::string error;
            std::string threadId;
            typed::ThreadRealtimeTranscriptDoneNotification event;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "role", event.role, error) ||
                !requireString(notification.params, "text", event.text, error) ||
                !requireString(notification.params, "threadId", threadId, error)) {
                return malformedEvent(notification, std::move(error));
            }
            event.threadId.value = std::move(threadId);
            event.raw = notification.raw;
            return typed::Event{std::move(event)};
        }

        typed::Event decodeThreadSettingsUpdated(const Notification& notification) {
            std::string error;
            std::string threadId;
            const Json* threadSettings = nullptr;
            typed::ThreadSettingsUpdatedNotification event;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "threadId", threadId, error) ||
                !requireObject(notification.params, "threadSettings", threadSettings, error) ||
                !decodeThreadSettingsValue(*threadSettings, event.threadSettings, error)) {
                return malformedEvent(notification, std::move(error));
            }
            event.threadId.value = std::move(threadId);
            for (const auto& diagnostic : event.threadSettings.diagnostics) {
                event.diagnostics.emplace_back(prefixedDiagnostic(diagnostic, "$.params.threadSettings"));
            }
            event.raw = notification.raw;
            return typed::Event{std::move(event)};
        }

        typed::Event decodeTurnDiffUpdated(const Notification& notification) {
            std::string error;
            std::string threadId;
            std::string turnId;
            typed::TurnDiffUpdatedNotification event;
            if (!requireParamsObject(notification, error) || !requireString(notification.params, "diff", event.diff, error) ||
                !requireString(notification.params, "threadId", threadId, error) ||
                !requireString(notification.params, "turnId", turnId, error)) {
                return malformedEvent(notification, std::move(error));
            }
            event.threadId.value = std::move(threadId);
            event.turnId.value = std::move(turnId);
            event.raw = notification.raw;
            return typed::Event{std::move(event)};
        }

        typed::Event decodeTurnModerationMetadata(const Notification& notification) {
            std::string error;
            std::string threadId;
            std::string turnId;
            typed::TurnModerationMetadataNotification event;
            if (!requireParamsObject(notification, error) || !requireAny(notification.params, "metadata", event.metadata, error) ||
                !requireString(notification.params, "threadId", threadId, error) ||
                !requireString(notification.params, "turnId", turnId, error)) {
                return malformedEvent(notification, std::move(error));
            }
            event.threadId.value = std::move(threadId);
            event.turnId.value = std::move(turnId);
            event.raw = notification.raw;
            return typed::Event{std::move(event)};
        }

        typed::Event decodeTurnPlanUpdated(const Notification& notification) {
            std::string error;
            std::string threadId;
            std::string turnId;
            const Json* plan = nullptr;
            typed::TurnPlanUpdatedNotification event;
            if (!requireParamsObject(notification, error) ||
                !decodeOptionalNullable(notification.params, "explanation", event.explanation, decodeStringValue, error) ||
                !requireArray(notification.params, "plan", plan, error) ||
                !requireString(notification.params, "threadId", threadId, error) ||
                !requireString(notification.params, "turnId", turnId, error)) {
                return malformedEvent(notification, std::move(error));
            }
            event.plan.reserve(plan->size());
            for (std::size_t index = 0; index < plan->size(); ++index) {
                typed::TurnPlanStep step;
                if (!decodeTurnPlanStepValue((*plan)[index], step, event.diagnostics, error, index)) {
                    return malformedEvent(notification, std::move(error));
                }
                event.plan.emplace_back(std::move(step));
            }
            event.threadId.value = std::move(threadId);
            event.turnId.value = std::move(turnId);
            event.raw = notification.raw;
            return typed::Event{std::move(event)};
        }

        typed::Event decodeModelRerouted(const Notification& notification) {
            std::string error;
            auto decoded = decodeModelReroutedNotification(notification, error);
            if (!decoded) {
                return malformedEvent(notification, std::move(error));
            }
            typed::ModelRerouted event{decoded->threadId,
                                       decoded->turnId,
                                       decoded->fromModel,
                                       decoded->toModel,
                                       decoded->reason.value,
                                       notification.raw,
                                       std::move(*decoded)};
            return typed::Event{std::move(event)};
        }

        typed::Event decodeModelSafetyBufferingUpdated(const Notification& notification) {
            std::string error;
            auto decoded = decodeModelSafetyBufferingUpdatedNotification(notification, error);
            return decoded ? typed::Event{std::move(*decoded)} : malformedEvent(notification, std::move(error));
        }

        typed::Event decodeModelVerification(const Notification& notification) {
            std::string error;
            auto decoded = decodeModelVerificationNotification(notification, error);
            return decoded ? typed::Event{std::move(*decoded)} : malformedEvent(notification, std::move(error));
        }

        typed::Event decodeAccountLoginCompleted(const Notification& notification) {
            std::string error;
            auto decoded = decodeAccountLoginCompletedNotification(notification, error);
            return decoded ? typed::Event{std::move(*decoded)} : malformedEvent(notification, std::move(error));
        }

        typed::Event decodeAccountRateLimitsUpdated(const Notification& notification) {
            std::string error;
            auto decoded = decodeAccountRateLimitsUpdatedNotification(notification, error);
            return decoded ? typed::Event{std::move(*decoded)} : malformedEvent(notification, std::move(error));
        }

        typed::Event decodeAccountUpdated(const Notification& notification) {
            std::string error;
            auto decoded = decodeAccountUpdatedNotification(notification, error);
            return decoded ? typed::Event{std::move(*decoded)} : malformedEvent(notification, std::move(error));
        }

        typed::Event decodeTurnError(const Notification& notification) {
            std::string decodingError;
            std::string threadId;
            std::string turnId;
            const Json* turnError = nullptr;
            bool willRetry = false;
            if (!requireParamsObject(notification, decodingError) ||
                !requireString(notification.params, "threadId", threadId, decodingError) ||
                !requireString(notification.params, "turnId", turnId, decodingError) ||
                !requireObject(notification.params, "error", turnError, decodingError) ||
                !requireBoolean(notification.params, "willRetry", willRetry, decodingError)) {
                return malformedEvent(notification, std::move(decodingError));
            }

            std::optional<typed::DecodeDiagnostic> turnDiagnostic;
            std::optional<typed::TurnError> typedError = detail::decodeTurnError(*turnError, turnDiagnostic);
            if (!typedError) {
                return typed::Event{typed::UnknownEvent{notification.method,
                                                        notification.params,
                                                        notification.raw,
                                                        notification.method + ": error payload could not be decoded",
                                                        std::move(turnDiagnostic)}};
            }
            return typed::Event{typed::TurnErrorEvent{
                typed::ThreadId{std::move(threadId)},
                typed::TurnId{std::move(turnId)},
                *turnError,
                willRetry,
                notification.raw,
                std::move(typedError)}};
        }

    } // namespace

    typed::Event decodeEvent(const Notification& notification) noexcept {
        try {
            const ProtocolSurfaceEntry* entry =
                findSurface(SurfaceCategory::ServerNotification, "ServerNotification", "method", notification.method);
            const ServerNotificationTarget* target = entry == nullptr || entry->runtimeDisposition != RuntimeDisposition::Typed
                                                         ? nullptr
                                                         : std::get_if<ServerNotificationTarget>(&entry->runtimeTarget);
            if (target == nullptr) {
                return unknownEvent(notification);
            }
            const auto descriptors = serverNotificationCodecDescriptors();
            const auto descriptor = std::find_if(
                descriptors.begin(), descriptors.end(), [&](const ServerNotificationCodecDescriptor& candidate) {
                    return candidate.target == *target;
                });
            if (descriptor == descriptors.end() || descriptor->key != entry->key) {
                return malformedEvent(notification, "canonical server-notification descriptor mismatch");
            }

            switch (*target) {
                case ServerNotificationTarget::Error:
                    return decodeTurnError(notification);
                case ServerNotificationTarget::AccountLoginCompleted:
                    return decodeAccountLoginCompleted(notification);
                case ServerNotificationTarget::AccountRateLimitsUpdated:
                    return decodeAccountRateLimitsUpdated(notification);
                case ServerNotificationTarget::AccountUpdated:
                    return decodeAccountUpdated(notification);
                case ServerNotificationTarget::ModelSafetyBufferingUpdated:
                    return decodeModelSafetyBufferingUpdated(notification);
                case ServerNotificationTarget::ModelVerification:
                    return decodeModelVerification(notification);
                case ServerNotificationTarget::ThreadStarted:
                    return decodeThreadStarted(notification);
                case ServerNotificationTarget::ThreadStatusChanged:
                    return decodeThreadStatusChanged(notification);
                case ServerNotificationTarget::TurnStarted:
                    return decodeTurnStarted(notification);
                case ServerNotificationTarget::TurnCompleted:
                    return decodeTurnCompleted(notification);
                case ServerNotificationTarget::ItemStarted:
                    return decodeItemLifecycle(notification, true);
                case ServerNotificationTarget::ItemCompleted:
                    return decodeItemLifecycle(notification, false);
                case ServerNotificationTarget::AgentMessageDelta:
                    return decodeAgentMessageDelta(notification);
                case ServerNotificationTarget::TerminalInteraction:
                    return decodeTerminalInteraction(notification);
                case ServerNotificationTarget::ReasoningTextDelta:
                    return decodeReasoningDelta(notification, typed::ReasoningDelta::Kind::Text);
                case ServerNotificationTarget::ReasoningSummaryPartAdded:
                    return decodeReasoningSummaryPartAdded(notification);
                case ServerNotificationTarget::ReasoningSummaryTextDelta:
                    return decodeReasoningDelta(notification, typed::ReasoningDelta::Kind::Summary);
                case ServerNotificationTarget::CommandExecutionOutputDelta:
                    return decodeCommandOutputDelta(notification);
                case ServerNotificationTarget::FileChangeOutputDelta:
                    return decodeSimpleDelta<typed::FileChangeOutputDeltaNotification>(notification);
                case ServerNotificationTarget::FileChangePatchUpdated:
                    return decodeFileChangeUpdated(notification);
                case ServerNotificationTarget::McpToolCallProgress:
                    return decodeMcpToolCallProgress(notification);
                case ServerNotificationTarget::PlanDelta:
                    return decodeSimpleDelta<typed::PlanDeltaNotification>(notification);
                case ServerNotificationTarget::ThreadArchived:
                    return decodeThreadIdentityOnly<typed::ThreadArchivedNotification>(notification);
                case ServerNotificationTarget::ThreadClosed:
                    return decodeThreadIdentityOnly<typed::ThreadClosedNotification>(notification);
                case ServerNotificationTarget::ContextCompacted:
                    return decodeContextCompacted(notification);
                case ServerNotificationTarget::ThreadDeleted:
                    return decodeThreadIdentityOnly<typed::ThreadDeletedNotification>(notification);
                case ServerNotificationTarget::ThreadGoalCleared:
                    return decodeThreadIdentityOnly<typed::ThreadGoalClearedNotification>(notification);
                case ServerNotificationTarget::ThreadGoalUpdated:
                    return decodeThreadGoalUpdated(notification);
                case ServerNotificationTarget::ThreadNameUpdated:
                    return decodeThreadNameUpdated(notification);
                case ServerNotificationTarget::ThreadRealtimeClosed:
                    return decodeThreadRealtimeClosed(notification);
                case ServerNotificationTarget::ThreadRealtimeError:
                    return decodeThreadRealtimeError(notification);
                case ServerNotificationTarget::ThreadRealtimeItemAdded:
                    return decodeThreadRealtimeItemAdded(notification);
                case ServerNotificationTarget::ThreadRealtimeOutputAudioDelta:
                    return decodeThreadRealtimeOutputAudioDelta(notification);
                case ServerNotificationTarget::ThreadRealtimeSdp:
                    return decodeThreadRealtimeSdp(notification);
                case ServerNotificationTarget::ThreadRealtimeStarted:
                    return decodeThreadRealtimeStarted(notification);
                case ServerNotificationTarget::ThreadRealtimeTranscriptDelta:
                    return decodeThreadRealtimeTranscriptDelta(notification);
                case ServerNotificationTarget::ThreadRealtimeTranscriptDone:
                    return decodeThreadRealtimeTranscriptDone(notification);
                case ServerNotificationTarget::ThreadSettingsUpdated:
                    return decodeThreadSettingsUpdated(notification);
                case ServerNotificationTarget::ThreadTokenUsageUpdated:
                    return decodeTokenUsageUpdated(notification);
                case ServerNotificationTarget::ThreadUnarchived:
                    return decodeThreadIdentityOnly<typed::ThreadUnarchivedNotification>(notification);
                case ServerNotificationTarget::TurnDiffUpdated:
                    return decodeTurnDiffUpdated(notification);
                case ServerNotificationTarget::TurnModerationMetadata:
                    return decodeTurnModerationMetadata(notification);
                case ServerNotificationTarget::TurnPlanUpdated:
                    return decodeTurnPlanUpdated(notification);
                case ServerNotificationTarget::ModelRerouted:
                    return decodeModelRerouted(notification);
                case ServerNotificationTarget::Count:
                    break;
            }

            return unknownEvent(notification);
        } catch (const std::exception& exception) {
            try {
                return unknownEvent(notification, std::string("event decoding failed: ") + exception.what());
            } catch (...) {
                return typed::Event{typed::UnknownEvent{
                    "", nullptr, nullptr, "event decoding failed", malformedKnownDiagnostic("ServerNotification", "$")}};
            }
        } catch (...) {
            try {
                return unknownEvent(notification, "event decoding failed with an unknown exception");
            } catch (...) {
                return typed::Event{typed::UnknownEvent{
                    "", nullptr, nullptr, "event decoding failed", malformedKnownDiagnostic("ServerNotification", "$")}};
            }
        }
    }

} // namespace ai::openai::codex::detail
