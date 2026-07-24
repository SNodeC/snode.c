/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/backend/Reducer.h"
#include "ai/openai/codex/backend/Snapshot.h"
#include "ai/openai/codex/typed/Events.h"
#include "support/TestResult.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace {
    namespace backend = ai::openai::codex::backend;
    namespace typed = ai::openai::codex::typed;

    using ai::openai::codex::Json;

    typed::DecodeDiagnostic diagnosticFor(const std::string& method) {
        return {typed::DecodeIssueKind::UnknownEnumValue,
                typed::DecodeIssueSeverity::ForwardCompatibility,
                method,
                "$.params.futureMode",
                "future open-enum value retained by the typed notification"};
    }

    Json envelopeFor(const std::string& method, std::size_t sequence, Json additionalParams = Json::object()) {
        Json params = Json::object({{"sequence", sequence}, {"futureMode", "future-mode"}});
        params.update(std::move(additionalParams));
        return Json::object({{"jsonrpc", "2.0"}, {"method", method}, {"params", std::move(params)}, {"futureEnvelopeField", sequence}});
    }

    backend::Snapshot withoutExtensions(backend::Snapshot snapshot) {
        snapshot.recentExtensions.clear();
        snapshot.omittedRecentExtensions = 0;
        return snapshot;
    }

    template <typename Notification>
    typed::Event notificationEvent(Json raw, bool withDiagnostic = true) {
        static_assert(std::is_default_constructible_v<Notification>);
        Notification notification{};
        notification.raw = std::move(raw);
        if (withDiagnostic) {
            const std::string method = notification.raw.value("method", std::string{});
            notification.diagnostics = {diagnosticFor(method),
                                        typed::DecodeDiagnostic{typed::DecodeIssueKind::UnknownEnumValue,
                                                                typed::DecodeIssueSeverity::ForwardCompatibility,
                                                                method,
                                                                "$.params.secondFutureMode",
                                                                "second diagnostic remains available in the public typed value"}};
        }
        return typed::Event{std::move(notification)};
    }

    template <typename Notification>
    bool verifyPreservedNotification(tests::support::TestResult& result, const std::string& method, std::size_t sequence) {
        backend::Reducer reducer;
        backend::BackendState state;
        const backend::Snapshot before = backend::makeSnapshot(state);
        const Json raw = envelopeFor(method, sequence);
        const std::vector<backend::BackendEvent> translated = reducer.translate(notificationEvent<Notification>(raw));
        const auto* extension = translated.size() == 1 ? std::get_if<backend::CodexExtensionReceived>(&translated.front()) : nullptr;

        const bool translatedExactly =
            extension && extension->method == method && extension->payload == raw.at("params") && !extension->decodingError &&
            extension->diagnostic && extension->diagnostic->kind == typed::DecodeIssueKind::UnknownEnumValue &&
            extension->diagnostic->severity == typed::DecodeIssueSeverity::ForwardCompatibility &&
            extension->diagnostic->surface == method && extension->diagnostic->fieldPath == "$.params.futureMode";
        result.expectTrue(translatedExactly, method + " translates to one exact structured Codex extension without a legacy decode error");

        backend::Reduction reduction;
        if (extension) {
            reduction = reducer.apply(state, *extension);
        }
        const backend::ExtensionRecord* retained = state.recentExtensions.size() == 1 ? &state.recentExtensions.front() : nullptr;
        const bool retainedExactly = extension && reduction.changed && !reduction.flushImmediately && retained &&
                                     retained->method == method && retained->payload == raw.at("params") && !retained->decodingError &&
                                     !retained->originalMethodBytes && !retained->originalPayloadBytes &&
                                     !retained->originalDecodingErrorBytes && retained->diagnostic &&
                                     retained->diagnostic->kind == typed::DecodeIssueKind::UnknownEnumValue &&
                                     retained->diagnostic->severity == typed::DecodeIssueSeverity::ForwardCompatibility &&
                                     retained->diagnostic->surface == method && retained->diagnostic->fieldPath == "$.params.futureMode";
        result.expectTrue(retainedExactly,
                          method + " remains observable in the bounded recent-extension state with raw and diagnostic data");

        const backend::Snapshot after = backend::makeSnapshot(state);
        result.expectTrue(withoutExtensions(after) == before,
                          method + " adds no canonical BackendState or snapshot semantics outside the existing extension path");
        return translatedExactly && retainedExactly;
    }

    void testAllNewNotificationsRemainObservable(tests::support::TestResult& result) {
        std::size_t exact = 0;
        std::size_t sequence = 0;

#define CODEX_EXPECT_PRESERVED_NOTIFICATION(type, method)                                                                                  \
    exact += verifyPreservedNotification<typed::type>(result, method, sequence++) ? 1U : 0U

        CODEX_EXPECT_PRESERVED_NOTIFICATION(TerminalInteractionNotification, "item/commandExecution/terminalInteraction");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(FileChangeOutputDeltaNotification, "item/fileChange/outputDelta");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(McpToolCallProgressNotification, "item/mcpToolCall/progress");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(PlanDeltaNotification, "item/plan/delta");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(ReasoningSummaryPartAddedNotification, "item/reasoning/summaryPartAdded");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(ThreadArchivedNotification, "thread/archived");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(ThreadClosedNotification, "thread/closed");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(ContextCompactedNotification, "thread/compacted");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(ThreadDeletedNotification, "thread/deleted");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(ThreadGoalClearedNotification, "thread/goal/cleared");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(ThreadGoalUpdatedNotification, "thread/goal/updated");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(ThreadNameUpdatedNotification, "thread/name/updated");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(ThreadRealtimeClosedNotification, "thread/realtime/closed");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(ThreadRealtimeErrorNotification, "thread/realtime/error");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(ThreadRealtimeItemAddedNotification, "thread/realtime/itemAdded");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(ThreadRealtimeOutputAudioDeltaNotification, "thread/realtime/outputAudio/delta");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(ThreadRealtimeSdpNotification, "thread/realtime/sdp");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(ThreadRealtimeStartedNotification, "thread/realtime/started");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(ThreadRealtimeTranscriptDeltaNotification, "thread/realtime/transcript/delta");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(ThreadRealtimeTranscriptDoneNotification, "thread/realtime/transcript/done");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(ThreadSettingsUpdatedNotification, "thread/settings/updated");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(ThreadUnarchivedNotification, "thread/unarchived");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(TurnDiffUpdatedNotification, "turn/diff/updated");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(TurnModerationMetadataNotification, "turn/moderationMetadata");
        CODEX_EXPECT_PRESERVED_NOTIFICATION(TurnPlanUpdatedNotification, "turn/plan/updated");

#undef CODEX_EXPECT_PRESERVED_NOTIFICATION

        result.expectTrue(sequence == 25 && exact == 25,
                          "all 25 newly typed A1.1 notifications remain observable through the exact preservation path");
    }

    void applyTranslated(backend::Reducer& reducer, backend::BackendState& state, typed::Event event) {
        const std::vector<backend::BackendEvent> translated = reducer.translate(event);
        if (translated.size() == 1) {
            reducer.apply(state, translated.front());
        }
    }

    void testRealtimeRetentionIsBounded(tests::support::TestResult& result) {
        backend::ReducerOptions options;
        options.retainedExtensions = 7;
        options.maxExtensionBytes = 256;
        backend::Reducer reducer(options);
        backend::BackendState state;

        for (std::size_t sequence = 0; sequence < 100; ++sequence) {
            if (sequence % 2 == 0) {
                applyTranslated(reducer,
                                state,
                                notificationEvent<typed::ThreadRealtimeOutputAudioDeltaNotification>(
                                    envelopeFor("thread/realtime/outputAudio/delta", sequence), false));
            } else {
                applyTranslated(reducer,
                                state,
                                notificationEvent<typed::ThreadRealtimeTranscriptDeltaNotification>(
                                    envelopeFor("thread/realtime/transcript/delta", sequence), false));
            }
        }

        result.expectTrue(state.recentExtensions.size() == 7 &&
                              state.recentExtensions.front().method == "thread/realtime/transcript/delta" &&
                              state.recentExtensions.front().payload.at("sequence") == 93 &&
                              state.recentExtensions.back().method == "thread/realtime/transcript/delta" &&
                              state.recentExtensions.back().payload.at("sequence") == 99,
                          "high-volume realtime audio and transcript notifications retain only the configured deterministic newest suffix");

        const std::string largeAudio(4096, 'a');
        const Json audioRaw = envelopeFor("thread/realtime/outputAudio/delta", 100, Json{{"audio", {{"data", largeAudio}}}});
        const std::uint64_t audioBytes = static_cast<std::uint64_t>(audioRaw.at("params").dump().size());
        applyTranslated(reducer, state, notificationEvent<typed::ThreadRealtimeOutputAudioDeltaNotification>(audioRaw, false));
        const backend::ExtensionRecord& boundedAudio = state.recentExtensions.back();
        result.expectTrue(boundedAudio.method == "thread/realtime/outputAudio/delta" && boundedAudio.originalPayloadBytes == audioBytes &&
                              boundedAudio.payload.value("truncated", false) && boundedAudio.payload.value("omitted", false) &&
                              boundedAudio.payload.dump().find(largeAudio) == std::string::npos,
                          "oversized realtime audio is replaced by a bounded omission record with exact original-size accounting");

        const std::string largeTranscript(4096, 't');
        const Json transcriptRaw = envelopeFor("thread/realtime/transcript/delta", 101, Json{{"delta", largeTranscript}});
        const std::uint64_t transcriptBytes = static_cast<std::uint64_t>(transcriptRaw.at("params").dump().size());
        applyTranslated(reducer, state, notificationEvent<typed::ThreadRealtimeTranscriptDeltaNotification>(transcriptRaw, false));
        const backend::ExtensionRecord& boundedTranscript = state.recentExtensions.back();
        result.expectTrue(boundedTranscript.method == "thread/realtime/transcript/delta" &&
                              boundedTranscript.originalPayloadBytes == transcriptBytes &&
                              boundedTranscript.payload.value("truncated", false) && boundedTranscript.payload.value("omitted", false) &&
                              boundedTranscript.payload.dump().find(largeTranscript) == std::string::npos,
                          "oversized realtime transcripts are bounded independently with exact original-size accounting");
    }

    void testExistingSnapshotRedactionBoundaryIsReused(tests::support::TestResult& result) {
        backend::Reducer reducer;
        backend::BackendState state;
        const std::string accessToken = "typed-realtime-access-token-must-not-leak";
        const std::string secretAnswer = "typed-realtime-answer-must-not-leak";
        const Json raw = envelopeFor("thread/realtime/transcript/done",
                                     1,
                                     Json{{"safe", "visible"}, {"accessToken", accessToken}, {"nested", {{"answer", secretAnswer}}}});

        applyTranslated(reducer, state, notificationEvent<typed::ThreadRealtimeTranscriptDoneNotification>(raw, false));
        const backend::ExtensionRecord* canonical = state.recentExtensions.size() == 1 ? &state.recentExtensions.front() : nullptr;
        const backend::Snapshot snapshot = backend::makeSnapshot(state);
        const backend::ExtensionSnapshot* frontendSafe =
            snapshot.recentExtensions.size() == 1 ? &snapshot.recentExtensions.front() : nullptr;
        const std::string encodedSafe = frontendSafe ? frontendSafe->payload.dump() : std::string{};

        result.expectTrue(canonical && canonical->payload == raw.at("params") && canonical->payload.dump().find(accessToken) != std::string::npos,
                          "canonical bounded preservation retains the complete typed realtime params before frontend redaction");
        result.expectTrue(frontendSafe && frontendSafe->method == "thread/realtime/transcript/done" &&
                              frontendSafe->sensitiveFieldsRedacted && frontendSafe->payload.at("safe") == "visible" &&
                              encodedSafe.find(accessToken) == std::string::npos && encodedSafe.find(secretAnswer) == std::string::npos,
                          "new typed notifications reuse the unchanged frontend-safe recursive redaction boundary");
    }
} // namespace

int main() {
    tests::support::TestResult result;

    testAllNewNotificationsRemainObservable(result);
    testRealtimeRetentionIsBounded(result);
    testExistingSnapshotRedactionBoundaryIsReused(result);

    return result.processResult();
}
