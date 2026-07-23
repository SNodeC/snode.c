/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/EventDecoder.h"
#include "support/TestResult.h"

#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <variant>

namespace {
    using ai::openai::codex::Json;
    using ai::openai::codex::Notification;
    using ai::openai::codex::detail::decodeEvent;
    using ai::openai::codex::typed::AgentMessageDelta;
    using ai::openai::codex::typed::CommandOutputDelta;
    using ai::openai::codex::typed::Event;
    using ai::openai::codex::typed::FileChangeUpdated;
    using ai::openai::codex::typed::ItemCompleted;
    using ai::openai::codex::typed::ItemStarted;
    using ai::openai::codex::typed::LocalImageUserInput;
    using ai::openai::codex::typed::ModelRerouted;
    using ai::openai::codex::typed::ReasoningDelta;
    using ai::openai::codex::typed::ThreadStarted;
    using ai::openai::codex::typed::ThreadStatusChanged;
    using ai::openai::codex::typed::TextUserInput;
    using ai::openai::codex::typed::TokenUsageUpdated;
    using ai::openai::codex::typed::TurnCompleted;
    using ai::openai::codex::typed::TurnErrorEvent;
    using ai::openai::codex::typed::TurnFailed;
    using ai::openai::codex::typed::TurnStarted;
    using ai::openai::codex::typed::UnknownEvent;
    using ai::openai::codex::typed::UnknownItem;
    using ai::openai::codex::typed::UnknownUserInput;
    using ai::openai::codex::typed::UserMessageItem;

    Notification makeNotification(std::string method, Json params, Json envelopeExtension = Json::object()) {
        Json raw = {
            {"method", method},
            {"params", params},
            {"futureEnvelope", std::move(envelopeExtension)},
        };
        return {std::move(method), std::move(params), std::move(raw)};
    }

    Json validThread(std::string id = "thread-event") {
        return {
            {"id", std::move(id)},
            {"cwd", "/tmp/project"},
            {"modelProvider", "openai"},
            {"preview", "Inspect the branch"},
            {"cliVersion", "0.144.6"},
            {"sessionId", "session-event"},
            {"createdAt", std::numeric_limits<std::int64_t>::min()},
            {"updatedAt", std::numeric_limits<std::int64_t>::max()},
            {"ephemeral", false},
            {"source", "appServer"},
            {"status", Json{{"type", "idle"}, {"futureStatus", true}}},
            {"name", nullptr},
            {"turns", Json::array()},
            {"futureThreadField", Json::array({1, 2})},
        };
    }

    Json validTurn(std::string id, std::string status = "inProgress") {
        return {
            {"id", std::move(id)},
            {"status", std::move(status)},
            {"items", Json::array()},
            {"futureTurnField", true},
        };
    }

    Json agentItem(std::string id = "agent-event") {
        return {
            {"type", "agentMessage"},
            {"id", std::move(id)},
            {"text", "event text"},
            {"futureItemField", 7},
        };
    }

    Json userMessageItem(std::string id = "user-message-event", Json clientId = nullptr) {
        return {
            {"type", "userMessage"},
            {"id", std::move(id)},
            {"clientId", std::move(clientId)},
            {"content",
             Json::array({
                 Json{{"type", "text"}, {"text", "Answer just with OK!"}, {"text_elements", Json::array()}},
                 Json{{"type", "futureInput"}, {"futurePayload", Json{{"kept", true}}}},
             })},
            {"futureItemField", Json::array({1, 2})},
        };
    }

    template <typename T>
    const T* as(const Event& event) {
        return std::get_if<T>(&event);
    }

    void testThreadEvents(tests::support::TestResult& testResult) {
        const Json thread = validThread();
        const Notification startedNotification =
            makeNotification("thread/started", Json{{"thread", thread}, {"futureParam", "kept"}}, Json{{"version", 3}});
        const Event startedEvent = decodeEvent(startedNotification);
        const ThreadStarted* started = as<ThreadStarted>(startedEvent);

        testResult.expectTrue(started && started->thread.id.value == "thread-event" && started->thread.cwd == "/tmp/project" &&
                                  started->thread.status && started->thread.status->value == "idle",
                              "thread/started decodes the current schema Thread payload");
        testResult.expectTrue(started && started->thread.raw == thread && started->raw == startedNotification.raw,
                              "thread/started preserves exact nested thread raw and complete notification envelope");

        const Json status = {{"type", "active"}, {"activeFlags", Json::array({"waitingOnApproval"})}, {"future", 1}};
        const Notification statusNotification =
            makeNotification("thread/status/changed", Json{{"threadId", "thread-event"}, {"status", status}});
        const Event statusEvent = decodeEvent(statusNotification);
        const ThreadStatusChanged* changed = as<ThreadStatusChanged>(statusEvent);
        testResult.expectTrue(changed && changed->threadId.value == "thread-event" && changed->status.value == "active" &&
                                  changed->status.raw == status && changed->raw == statusNotification.raw,
                              "thread/status/changed decodes IDs, extensible status, status raw, and event raw");
    }

    void testTurnEvents(tests::support::TestResult& testResult) {
        const Json startedTurn = validTurn("turn-started");
        const Notification startedNotification =
            makeNotification("turn/started", Json{{"threadId", "thread-event"}, {"turn", startedTurn}});
        const Event startedEvent = decodeEvent(startedNotification);
        const TurnStarted* started = as<TurnStarted>(startedEvent);
        testResult.expectTrue(started && started->turn.id.value == "turn-started" && started->turn.threadId.value == "thread-event" &&
                                  started->turn.status.value == "inProgress" && started->turn.raw == startedTurn &&
                                  started->raw == startedNotification.raw,
                              "turn/started decodes typed turn IDs and preserves nested and envelope raw JSON");

        const Json completedTurn = validTurn("turn-completed", "future_terminal_status");
        const Notification completedNotification =
            makeNotification("turn/completed", Json{{"threadId", "thread-event"}, {"turn", completedTurn}});
        const Event completedEvent = decodeEvent(completedNotification);
        const TurnCompleted* completed = as<TurnCompleted>(completedEvent);
        testResult.expectTrue(completed && completed->turn.status.value == "future_terminal_status" &&
                                  completed->raw == completedNotification.raw,
                              "turn/completed preserves an unknown future status as an extensible TurnCompleted");

        Json failedTurn = validTurn("turn-failed", "failed");
        failedTurn["error"] = Json{{"message", "model failed"}, {"futureErrorField", 9}};
        const Notification failedNotification =
            makeNotification("turn/completed", Json{{"threadId", "thread-event"}, {"turn", failedTurn}});
        const Event failedEvent = decodeEvent(failedNotification);
        const TurnFailed* failed = as<TurnFailed>(failedEvent);
        testResult.expectTrue(failed && failed->turn.id.value == "turn-failed" && failed->error == failedTurn["error"] &&
                                  failed->raw == failedNotification.raw,
                              "turn/completed with failed status is classified as TurnFailed and preserves its error");
    }

    void testItemEvents(tests::support::TestResult& testResult) {
        const Json startedItem = agentItem("agent-started");
        const Notification startedNotification = makeNotification("item/started",
                                                                  Json{{"threadId", "thread-event"},
                                                                       {"turnId", "turn-event"},
                                                                       {"item", startedItem},
                                                                       {"startedAtMs", std::numeric_limits<std::int64_t>::min()}});
        const Event startedEvent = decodeEvent(startedNotification);
        const ItemStarted* started = as<ItemStarted>(startedEvent);
        testResult.expectTrue(started && started->startedAtMs == std::numeric_limits<std::int64_t>::min() &&
                                  started->raw == startedNotification.raw,
                              "item/started preserves the full signed int64 timestamp range and complete event raw");
        const auto* startedAgent = started ? std::get_if<ai::openai::codex::typed::AgentMessageItem>(&started->item) : nullptr;
        testResult.expectTrue(startedAgent && startedAgent->metadata.threadId && startedAgent->metadata.threadId->value == "thread-event" &&
                                  startedAgent->metadata.turnId && startedAgent->metadata.turnId->value == "turn-event" &&
                                  startedAgent->metadata.raw == startedItem,
                              "item/started supplies event thread and turn IDs to item metadata and preserves item raw");

        const Json completedItem = agentItem("agent-completed");
        const Notification completedNotification = makeNotification("item/completed",
                                                                    Json{{"threadId", "thread-event"},
                                                                         {"turnId", "turn-event"},
                                                                         {"item", completedItem},
                                                                         {"completedAtMs", std::numeric_limits<std::int64_t>::max()}});
        const Event completedEvent = decodeEvent(completedNotification);
        const ItemCompleted* completed = as<ItemCompleted>(completedEvent);
        testResult.expectTrue(completed && completed->completedAtMs == std::numeric_limits<std::int64_t>::max() &&
                                  completed->raw == completedNotification.raw,
                              "item/completed preserves maximum int64 timestamp and complete event raw");

        const Json futureItem = {{"type", "futureItem"}, {"id", "future-item"}, {"future", Json::array({1, 2, 3})}};
        const Notification futureItemNotification = makeNotification(
            "item/started", Json{{"threadId", "thread-event"}, {"turnId", "turn-event"}, {"item", futureItem}, {"startedAtMs", 17}});
        const Event futureItemEvent = decodeEvent(futureItemNotification);
        const ItemStarted* futureStarted = as<ItemStarted>(futureItemEvent);
        const UnknownItem* unknownItem = futureStarted ? std::get_if<UnknownItem>(&futureStarted->item) : nullptr;
        testResult.expectTrue(unknownItem && unknownItem->type == "futureItem" && unknownItem->raw == futureItem &&
                                  unknownItem->metadata.id && unknownItem->metadata.id->value == "future-item" &&
                                  unknownItem->metadata.threadId && unknownItem->metadata.threadId->value == "thread-event" &&
                                  unknownItem->metadata.turnId && unknownItem->metadata.turnId->value == "turn-event" &&
                                  !unknownItem->decodingError,
                              "unknown item discriminator retains all common metadata inside a typed lifecycle event");

        const Json missingIdItem = {{"type", "futureItem"}, {"future", true}};
        const Notification missingIdNotification = makeNotification(
            "item/started", Json{{"threadId", "thread-event"}, {"turnId", "turn-event"}, {"item", missingIdItem}, {"startedAtMs", 18}});
        const Event missingIdEvent = decodeEvent(missingIdNotification);
        const ItemStarted* missingIdStarted = as<ItemStarted>(missingIdEvent);
        const UnknownItem* missingId = missingIdStarted ? std::get_if<UnknownItem>(&missingIdStarted->item) : nullptr;
        testResult.expectTrue(missingId && !missingId->metadata.id && missingId->metadata.threadId &&
                                  missingId->metadata.threadId->value == "thread-event" && missingId->metadata.turnId &&
                                  missingId->metadata.turnId->value == "turn-event" && missingId->raw == missingIdItem &&
                                  missingId->decodingError,
                              "item lifecycle events retain envelope location when the item has no stable ID");
    }

    void testUserMessageItemEvents(tests::support::TestResult& testResult) {
        const Json startedItem = userMessageItem("user-message-shared");
        const Notification startedNotification = makeNotification("item/started",
                                                                  Json{{"threadId", "thread-user-message"},
                                                                       {"turnId", "turn-user-message"},
                                                                       {"item", startedItem},
                                                                       {"startedAtMs", 1784637396096}});
        const Event startedEvent = decodeEvent(startedNotification);
        const ItemStarted* started = as<ItemStarted>(startedEvent);
        const UserMessageItem* startedUser = started ? std::get_if<UserMessageItem>(&started->item) : nullptr;
        const TextUserInput* startedText =
            startedUser && startedUser->content.size() == 2 ? std::get_if<TextUserInput>(&startedUser->content[0]) : nullptr;
        const UnknownUserInput* startedFuture =
            startedUser && startedUser->content.size() == 2 ? std::get_if<UnknownUserInput>(&startedUser->content[1]) : nullptr;
        testResult.expectTrue(started && started->startedAtMs == 1784637396096 && started->raw == startedNotification.raw,
                              "userMessage item/started preserves its lifecycle timestamp and complete event raw");
        testResult.expectTrue(startedUser && startedUser->metadata.id.value == "user-message-shared" && startedUser->metadata.threadId &&
                                  startedUser->metadata.threadId->value == "thread-user-message" && startedUser->metadata.turnId &&
                                  startedUser->metadata.turnId->value == "turn-user-message" && startedUser->clientId.isNull() &&
                                  startedText && startedText->text == "Answer just with OK!" &&
                                  startedText->raw == startedItem["content"][0] && startedFuture &&
                                  startedFuture->type == "futureInput" && startedFuture->raw == startedItem["content"][1] &&
                                  startedFuture->diagnostic &&
                                  startedFuture->diagnostic->kind ==
                                      ai::openai::codex::typed::DecodeIssueKind::UnknownDiscriminator &&
                                  startedFuture->diagnostic->severity ==
                                      ai::openai::codex::typed::DecodeIssueSeverity::ForwardCompatibility &&
                                  startedFuture->diagnostic->surface == "UserInput" &&
                                  startedFuture->diagnostic->fieldPath == "$.type" &&
                                  startedUser->diagnostics.size() == 1 &&
                                  startedUser->diagnostics.front().fieldPath == "$.content[1].type" &&
                                  startedUser->metadata.raw == startedItem,
                              "userMessage item/started preserves location, tri-state clientId, typed/unknown content, and item raw");

        Json completedItem = userMessageItem("user-message-shared", "client-user-message");
        completedItem["content"].push_back(Json{{"type", "localImage"}, {"path", "/tmp/input.png"}, {"detail", "original"}, {"future", 9}});
        const Notification completedNotification = makeNotification("item/completed",
                                                                    Json{{"threadId", "thread-user-message"},
                                                                         {"turnId", "turn-user-message"},
                                                                         {"item", completedItem},
                                                                         {"completedAtMs", 1784637396123}});
        const Event completedEvent = decodeEvent(completedNotification);
        const ItemCompleted* completed = as<ItemCompleted>(completedEvent);
        const UserMessageItem* completedUser = completed ? std::get_if<UserMessageItem>(&completed->item) : nullptr;
        const TextUserInput* completedText =
            completedUser && completedUser->content.size() == 3 ? std::get_if<TextUserInput>(&completedUser->content[0]) : nullptr;
        const UnknownUserInput* completedFuture =
            completedUser && completedUser->content.size() == 3 ? std::get_if<UnknownUserInput>(&completedUser->content[1]) : nullptr;
        const LocalImageUserInput* completedImage =
            completedUser && completedUser->content.size() == 3 ? std::get_if<LocalImageUserInput>(&completedUser->content[2]) : nullptr;
        testResult.expectTrue(completed && completed->completedAtMs == 1784637396123 && completed->raw == completedNotification.raw,
                              "userMessage item/completed preserves its lifecycle timestamp and complete event raw");
        testResult.expectTrue(completedUser && completedUser->metadata.id.value == "user-message-shared" &&
                                  completedUser->metadata.threadId && completedUser->metadata.threadId->value == "thread-user-message" &&
                                  completedUser->metadata.turnId && completedUser->metadata.turnId->value == "turn-user-message" &&
                                  completedUser->clientId.hasValue() &&
                                  completedUser->clientId->value == "client-user-message" &&
                                  completedText && completedText->raw == completedItem["content"][0] && completedFuture &&
                                  completedFuture->raw == completedItem["content"][1] && completedImage &&
                                  completedImage->path == "/tmp/input.png" && completedImage->detail.hasValue() &&
                                  completedImage->detail->value == "original" &&
                                  completedFuture->diagnostic &&
                                  completedFuture->diagnostic->kind ==
                                      ai::openai::codex::typed::DecodeIssueKind::UnknownDiscriminator &&
                                  completedFuture->diagnostic->severity ==
                                      ai::openai::codex::typed::DecodeIssueSeverity::ForwardCompatibility &&
                                  completedUser->diagnostics.size() == 1 &&
                                  completedUser->diagnostics.front().fieldPath == "$.content[1].type" &&
                                  completedImage->raw == completedItem["content"][2] && completedUser->metadata.raw == completedItem,
                              "userMessage completion keeps its identity and every typed/unknown content entry in order");
    }

    void testDeltaEvents(tests::support::TestResult& testResult) {
        const Json base = {
            {"threadId", "thread-delta"},
            {"turnId", "turn-delta"},
            {"itemId", "item-delta"},
            {"delta", "chunk"},
            {"futureDeltaField", true},
        };

        const Notification agentNotification = makeNotification("item/agentMessage/delta", base, "agent-envelope");
        const Event agentEvent = decodeEvent(agentNotification);
        const AgentMessageDelta* agent = as<AgentMessageDelta>(agentEvent);
        testResult.expectTrue(agent && agent->threadId.value == "thread-delta" && agent->turnId.value == "turn-delta" &&
                                  agent->itemId.value == "item-delta" && agent->text == "chunk" && agent->raw == agentNotification.raw,
                              "agent-message delta decodes IDs and text and preserves complete raw envelope");

        Json textParams = base;
        textParams["contentIndex"] = std::numeric_limits<std::int64_t>::min();
        const Notification textNotification = makeNotification("item/reasoning/textDelta", textParams);
        const Event textEvent = decodeEvent(textNotification);
        const ReasoningDelta* text = as<ReasoningDelta>(textEvent);
        testResult.expectTrue(text && text->kind == ReasoningDelta::Kind::Text && text->index == std::numeric_limits<std::int64_t>::min() &&
                                  text->text == "chunk",
                              "reasoning text delta preserves the full signed int64 content index range");

        Json summaryParams = base;
        summaryParams["summaryIndex"] = std::numeric_limits<std::int64_t>::max();
        const Notification summaryNotification = makeNotification("item/reasoning/summaryTextDelta", summaryParams);
        const Event summaryEvent = decodeEvent(summaryNotification);
        const ReasoningDelta* summary = as<ReasoningDelta>(summaryEvent);
        testResult.expectTrue(summary && summary->kind == ReasoningDelta::Kind::Summary &&
                                  summary->index == std::numeric_limits<std::int64_t>::max() && summary->raw == summaryNotification.raw,
                              "reasoning summary delta preserves maximum int64 summary index and raw envelope");

        const Notification commandNotification = makeNotification("item/commandExecution/outputDelta", base);
        const Event commandEvent = decodeEvent(commandNotification);
        const CommandOutputDelta* command = as<CommandOutputDelta>(commandEvent);
        testResult.expectTrue(command && command->output == "chunk" && command->itemId.value == "item-delta" &&
                                  command->raw == commandNotification.raw,
                              "command-output delta decodes output and preserves raw envelope");
    }

    void testAuxiliaryEvents(tests::support::TestResult& testResult) {
        const Json changes = Json::array({Json{{"path", "/tmp/a"}, {"kind", "future"}}});
        const Notification fileNotification =
            makeNotification("item/fileChange/patchUpdated",
                             Json{{"threadId", "thread-event"}, {"turnId", "turn-event"}, {"itemId", "file-event"}, {"changes", changes}});
        const Event fileEvent = decodeEvent(fileNotification);
        const FileChangeUpdated* file = as<FileChangeUpdated>(fileEvent);
        testResult.expectTrue(file && file->changes == changes && file->itemId.value == "file-event" && file->raw == fileNotification.raw,
                              "file-change patch update preserves typed IDs, opaque changes, and raw event");

        const Json usage = {
            {"last", Json{{"inputTokens", 1}}},
            {"total", Json{{"inputTokens", 2}}},
            {"futureUsage", true},
        };
        const Notification usageNotification = makeNotification(
            "thread/tokenUsage/updated", Json{{"threadId", "thread-event"}, {"turnId", "turn-event"}, {"tokenUsage", usage}});
        const Event usageEvent = decodeEvent(usageNotification);
        const TokenUsageUpdated* tokenUsage = as<TokenUsageUpdated>(usageEvent);
        testResult.expectTrue(tokenUsage && tokenUsage->usage == usage && tokenUsage->raw == usageNotification.raw,
                              "token-usage update preserves the complete current/future usage object and raw event");

        const Notification rerouteNotification = makeNotification("model/rerouted",
                                                                  Json{{"threadId", "thread-event"},
                                                                       {"turnId", "turn-event"},
                                                                       {"fromModel", "model-old"},
                                                                       {"toModel", "model-future"},
                                                                       {"reason", "futureReason"}});
        const Event rerouteEvent = decodeEvent(rerouteNotification);
        const ModelRerouted* rerouted = as<ModelRerouted>(rerouteEvent);
        testResult.expectTrue(rerouted && rerouted->from.value == "model-old" && rerouted->to.value == "model-future" &&
                                  rerouted->reason == "futureReason" && rerouted->raw == rerouteNotification.raw,
                              "model/rerouted preserves extensible model and reason strings plus raw event");

        const Json error = {{"message", "temporary failure"}, {"additionalDetails", nullptr}, {"future", 4}};
        const Notification errorNotification =
            makeNotification("error", Json{{"threadId", "thread-event"}, {"turnId", "turn-event"}, {"error", error}, {"willRetry", true}});
        const Event errorEvent = decodeEvent(errorNotification);
        const TurnErrorEvent* turnError = as<TurnErrorEvent>(errorEvent);
        testResult.expectTrue(turnError && turnError->error == error && turnError->willRetry && turnError->raw == errorNotification.raw,
                              "error notification decodes retry metadata and preserves current/future error and event raw");
    }

    void testUnknownAndMalformedEvents(tests::support::TestResult& testResult) {
        const Json unknownParams = Json::array({1, "two", false});
        const Notification unknownNotification = makeNotification("future/notification", unknownParams, Json{{"futureEnvelope", 5}});
        const Event unknownDecoded = decodeEvent(unknownNotification);
        const UnknownEvent* unknown = as<UnknownEvent>(unknownDecoded);
        testResult.expectTrue(unknown && unknown->method == "future/notification" && unknown->params == unknownParams &&
                                  unknown->raw == unknownNotification.raw && !unknown->decodingError,
                              "unknown notification becomes UnknownEvent with exact params/raw and no decoding error");

        const Json futureKnownParams = {
            {"threadId", "thread-event"},
            {"turnId", "turn-event"},
            {"itemId", "item-event"},
            {"futureReplacementForDelta", Json{{"text", "new shape"}}},
        };
        const Notification futureKnownNotification = makeNotification("item/agentMessage/delta", futureKnownParams, "future-shape");
        const Event futureKnownEvent = decodeEvent(futureKnownNotification);
        const UnknownEvent* futureKnown = as<UnknownEvent>(futureKnownEvent);
        testResult.expectTrue(futureKnown && futureKnown->decodingError && !futureKnown->decodingError->empty(),
                              "known method with an unrecognized future payload becomes UnknownEvent with a decode reason");
        testResult.expectTrue(futureKnown && futureKnown->params == futureKnownParams && futureKnown->raw == futureKnownNotification.raw,
                              "malformed/future known payload preserves exact params and complete notification envelope");

        const Json malformedItemParams = {
            {"threadId", "thread-event"},
            {"turnId", "turn-event"},
            {"item", Json{{"type", "agentMessage"}, {"id", "bad-item"}}},
            {"completedAtMs", 1},
        };
        const Notification malformedItemNotification = makeNotification("item/completed", malformedItemParams);
        const Event malformedItemEvent = decodeEvent(malformedItemNotification);
        const ItemCompleted* malformedItem = as<ItemCompleted>(malformedItemEvent);
        const UnknownItem* malformedTypedItem = malformedItem ? std::get_if<UnknownItem>(&malformedItem->item) : nullptr;
        testResult.expectTrue(malformedItem && malformedItem->completedAtMs == 1 && malformedItem->raw == malformedItemNotification.raw &&
                                  malformedTypedItem && malformedTypedItem->type == "agentMessage" && malformedTypedItem->metadata.id &&
                                  malformedTypedItem->metadata.id->value == "bad-item" && malformedTypedItem->metadata.threadId &&
                                  malformedTypedItem->metadata.threadId->value == "thread-event" && malformedTypedItem->metadata.turnId &&
                                  malformedTypedItem->metadata.turnId->value == "turn-event" &&
                                  malformedTypedItem->raw == malformedItemParams["item"] && malformedTypedItem->decodingError,
                              "known malformed item detail remains a located typed lifecycle event with an item-local diagnostic");

        const Json scalarItemParams = {
            {"threadId", "thread-event"},
            {"turnId", "turn-event"},
            {"item", Json::array({1, 2})},
            {"completedAtMs", 2},
        };
        const Notification scalarItemNotification = makeNotification("item/completed", scalarItemParams);
        const Event scalarItemEvent = decodeEvent(scalarItemNotification);
        const UnknownEvent* scalarItem = as<UnknownEvent>(scalarItemEvent);
        testResult.expectTrue(scalarItem && scalarItem->decodingError && scalarItem->params == scalarItemParams &&
                                  scalarItem->raw == scalarItemNotification.raw,
                              "structurally unusable non-object item remains a diagnosable UnknownEvent");

        const Notification scalarParamsNotification = makeNotification("turn/started", Json::array({1, 2}));
        const Event scalarParamsEvent = decodeEvent(scalarParamsNotification);
        const UnknownEvent* scalarParams = as<UnknownEvent>(scalarParamsEvent);
        testResult.expectTrue(scalarParams && scalarParams->decodingError && scalarParams->raw == scalarParamsNotification.raw,
                              "known event with non-object params becomes UnknownEvent without throwing");

        const Notification timestampOverflowNotification =
            makeNotification("item/started",
                             Json{{"threadId", "thread-event"},
                                  {"turnId", "turn-event"},
                                  {"item", agentItem("overflow-item")},
                                  {"startedAtMs", std::numeric_limits<std::uint64_t>::max()}});
        const Event timestampOverflowEvent = decodeEvent(timestampOverflowNotification);
        const UnknownEvent* timestampOverflow = as<UnknownEvent>(timestampOverflowEvent);
        testResult.expectTrue(timestampOverflow && timestampOverflow->decodingError &&
                                  timestampOverflow->raw == timestampOverflowNotification.raw,
                              "event decoder rejects an unsigned timestamp outside int64 without crashing or losing raw JSON");
    }
} // namespace

int main() {
    tests::support::TestResult testResult;

    testThreadEvents(testResult);
    testTurnEvents(testResult);
    testItemEvents(testResult);
    testUserMessageItemEvents(testResult);
    testDeltaEvents(testResult);
    testAuxiliaryEvents(testResult);
    testUnknownAndMalformedEvents(testResult);

    return testResult.processResult();
}
