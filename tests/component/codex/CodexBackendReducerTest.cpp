/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/backend/Reducer.h"
#include "ai/openai/codex/backend/Snapshot.h"
#include "support/TestResult.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace {
    namespace backend = ai::openai::codex::backend;
    namespace typed = ai::openai::codex::typed;

    using ai::openai::codex::Error;
    using ai::openai::codex::Json;
    using ai::openai::codex::ServerRequestId;
    using ai::openai::codex::ServerRequestToken;

    typed::ItemMetadata metadata(const std::string& threadId, const std::string& turnId, const std::string& itemId) {
        typed::ItemMetadata result;
        result.id = typed::ItemId{itemId};
        result.threadId = typed::ThreadId{threadId};
        result.turnId = typed::TurnId{turnId};
        result.raw = Json{{"id", itemId}, {"threadId", threadId}, {"turnId", turnId}};
        return result;
    }

    typed::Item agentItem(const std::string& threadId, const std::string& turnId, const std::string& itemId, std::string text = {}) {
        typed::AgentMessageItem item;
        item.metadata = metadata(threadId, turnId, itemId);
        item.text = std::move(text);
        return item;
    }

    typed::UserMessageItem userMessageItem(const std::string& threadId,
                                           const std::string& turnId,
                                           const std::string& itemId,
                                           Json content,
                                           Json raw,
                                           std::optional<std::string> clientId = std::nullopt) {
        typed::UserMessageItem item;
        item.metadata = metadata(threadId, turnId, itemId);
        item.metadata.raw = std::move(raw);
        item.clientId = std::move(clientId);
        item.content = std::move(content);
        return item;
    }

    typed::Item reasoningItem(const std::string& threadId,
                              const std::string& turnId,
                              const std::string& itemId,
                              std::string text = {},
                              std::string summary = {}) {
        typed::ReasoningItem item;
        item.metadata = metadata(threadId, turnId, itemId);
        if (!text.empty()) {
            item.content.push_back(std::move(text));
        }
        if (!summary.empty()) {
            item.summary.push_back(std::move(summary));
        }
        return item;
    }

    typed::Item commandItem(const std::string& threadId,
                            const std::string& turnId,
                            const std::string& itemId,
                            std::optional<std::string> output = std::nullopt) {
        typed::CommandExecutionItem item;
        item.metadata = metadata(threadId, turnId, itemId);
        item.command = "printf test";
        item.cwd = "/tmp/project";
        item.status = "inProgress";
        item.output = std::move(output);
        return item;
    }

    typed::Turn turn(const std::string& threadId,
                     const std::string& turnId,
                     typed::TurnStatus status = typed::TurnStatus::inProgress(),
                     std::vector<typed::Item> items = {}) {
        typed::Turn result;
        result.id = typed::TurnId{turnId};
        result.threadId = typed::ThreadId{threadId};
        result.status = std::move(status);
        result.itemsView = typed::TurnItemsView::full();
        result.items = std::move(items);
        result.raw = Json{{"id", turnId}, {"threadId", threadId}};
        return result;
    }

    typed::Thread thread(const std::string& threadId, std::vector<typed::Turn> turns = {}) {
        typed::Thread result;
        result.id = typed::ThreadId{threadId};
        result.title = "Thread " + threadId;
        result.cwd = "/tmp/project";
        result.model = typed::ModelId{"gpt-5"};
        result.modelProvider = "openai";
        result.preview = "preview " + threadId;
        result.status = typed::ThreadStatus{"idle", Json{{"type", "idle"}}};
        result.createdAt = 1;
        result.updatedAt = 2;
        result.turns = std::move(turns);
        result.raw = Json{{"id", threadId}, {"futureThreadField", true}};
        return result;
    }

    const backend::TurnState* findTurn(const backend::BackendState& state, const std::string& threadId, const std::string& turnId) {
        return backend::findTurn(state, typed::ThreadId{threadId}, typed::TurnId{turnId});
    }

    const backend::ItemState*
    findItem(const backend::BackendState& state, const std::string& threadId, const std::string& turnId, const std::string& itemId) {
        return backend::findItem(state, typed::ThreadId{threadId}, typed::TurnId{turnId}, typed::ItemId{itemId});
    }

    void testInitialStateAndLifecycle(tests::support::TestResult& result) {
        backend::BackendState state;
        backend::Reducer reducer;

        const backend::Snapshot first = backend::makeSnapshot(state);
        const backend::Snapshot second = backend::makeSnapshot(state);
        result.expectTrue(first == second, "unchanged empty state produces equal deterministic snapshots");
        result.expectTrue(first.lifecycle == backend::BackendLifecycle::Stopped && first.threads.empty() && first.pendingRequests.empty() &&
                              first.sessions.empty() && first.sequence.value() == 0,
                          "initial snapshot is stopped and contains no domain entities");

        const backend::Reduction starting =
            reducer.apply(state, backend::LifecycleChanged{backend::BackendLifecycle::Starting, std::nullopt, 1});
        result.expectTrue(starting.changed && !starting.flushImmediately && state.lifecycle == backend::BackendLifecycle::Starting,
                          "starting lifecycle transition changes canonical state");

        const Error error{Error::Category::Transport, 71, "connection failed"};
        const backend::Reduction failed = reducer.apply(state, backend::LifecycleChanged{backend::BackendLifecycle::Failed, error, 1});
        result.expectTrue(failed.changed && failed.flushImmediately && state.lastLifecycleError &&
                              state.lastLifecycleError->message == "connection failed",
                          "failure is retained and requests an immediate frontend flush");

        reducer.apply(state, backend::LifecycleChanged{backend::BackendLifecycle::Starting, std::nullopt, 2});
        result.expectTrue(state.lastLifecycleError.has_value(), "a prior lifecycle error remains visible while recovery is starting");
        reducer.apply(state, backend::LifecycleChanged{backend::BackendLifecycle::Ready, std::nullopt, 2});
        result.expectTrue(state.lifecycle == backend::BackendLifecycle::Ready && !state.lastLifecycleError,
                          "ready after restart clears the prior lifecycle error");

        backend::ReducerOptions options;
        options.retainedDiagnostics = 2;
        backend::Reducer boundedReducer(options);
        boundedReducer.apply(state, backend::DiagnosticReceived{"one"});
        boundedReducer.apply(state, backend::DiagnosticReceived{"two"});
        boundedReducer.apply(state, backend::DiagnosticReceived{"three"});
        result.expectTrue(state.diagnostics.received == 3 && state.diagnostics.recent == std::vector<std::string>({"two", "three"}),
                          "diagnostic total is monotonic while the retained detail list is bounded");
    }

    void testThreadAndTurnHydration(tests::support::TestResult& result) {
        backend::BackendState state;
        backend::Reducer reducer;

        typed::Thread started = thread("thread-start");
        reducer.apply(state, backend::ThreadUpserted{started, backend::EntityLoad::Summary});
        result.expectTrue(state.threads.size() == 1 && state.threadOrder.size() == 1 &&
                              state.threads.at("thread-start").thread.title == "Thread thread-start" &&
                              !state.threads.at("thread-start").fullyLoaded,
                          "thread/start result performs a summary ID upsert");

        typed::Thread resumed = started;
        resumed.title = "Resumed title";
        reducer.apply(state, backend::ThreadUpserted{resumed, backend::EntityLoad::Summary});
        result.expectTrue(state.threads.size() == 1 && state.threadOrder.size() == 1 &&
                              state.threads.at("thread-start").thread.title == "Resumed title",
                          "thread/resume result replaces the typed thread without duplicating its order entry");

        typed::ThreadPage firstPage;
        firstPage.data = {thread("thread-list-b"), thread("thread-list-a")};
        firstPage.nextCursor = "cursor-2";
        firstPage.backwardsCursor = "cursor-before";
        reducer.apply(state, backend::ThreadListUpdated{firstPage, std::nullopt, true});
        result.expectTrue(state.threadList.hasLoadedPage && !state.threadList.complete && state.threadList.pagesLoaded == 1 &&
                              state.threadList.nextCursor == "cursor-2" && state.threads.size() == 3,
                          "first thread/list page merges threads and retains pagination state");

        typed::ThreadPage secondPage;
        secondPage.data = {thread("thread-list-c"), thread("thread-list-b")};
        secondPage.backwardsCursor = "cursor-1";
        reducer.apply(state, backend::ThreadListUpdated{secondPage, std::string("cursor-2"), false});
        result.expectTrue(state.threadList.complete && state.threadList.pagesLoaded == 2 && !state.threadList.nextCursor &&
                              state.threads.size() == 4,
                          "subsequent thread/list page uses ID merge semantics and marks terminal pagination complete");

        const typed::Turn readTurnA = turn("thread-start", "turn-z");
        const typed::Turn readTurnB = turn("thread-start", "turn-a", typed::TurnStatus::completed());
        typed::Thread read = thread("thread-start", {readTurnA, readTurnB});
        read.title = "Fully read";
        reducer.apply(state, backend::ThreadUpserted{read, backend::EntityLoad::Full});
        const backend::ThreadState& hydrated = state.threads.at("thread-start");
        result.expectTrue(hydrated.fullyLoaded && hydrated.turns.size() == 2 && hydrated.turnOrder.size() == 2 &&
                              hydrated.turnOrder[0].value == "turn-z" && hydrated.turnOrder[1].value == "turn-a",
                          "thread/read hydrates turns fully in deterministic server order");

        typed::Turn turnUpdate = readTurnA;
        turnUpdate.status = typed::TurnStatus::completed();
        reducer.apply(state, backend::TurnUpserted{turnUpdate});
        result.expectTrue(hydrated.turns.at("turn-z").terminal && !hydrated.turns.at("turn-z").active && hydrated.turnOrder.size() == 2,
                          "turn/start or notification upserts status without duplicating turn order");

        const backend::Snapshot snapshot = backend::makeSnapshot(state);
        result.expectTrue(snapshot.threads.size() == 4 && snapshot.threads.front().id == "thread-start" &&
                              snapshot.threads.front().turns.size() == 2 && snapshot.threads.front().turns[0].id == "turn-z" &&
                              snapshot.threads.front().turns[1].id == "turn-a",
                          "snapshot preserves first-seen thread, turn, and item ordering instead of map key ordering");
    }

    void testItemsAndHighVolumeDeltas(tests::support::TestResult& result) {
        backend::BackendState state;
        backend::Reducer reducer;
        reducer.apply(state, backend::TurnUpserted{turn("thread-delta", "turn-delta")});

        const backend::Reduction agentStart = reducer.apply(state,
                                                            backend::ItemUpserted{typed::ThreadId{"thread-delta"},
                                                                                  typed::TurnId{"turn-delta"},
                                                                                  agentItem("thread-delta", "turn-delta", "item-agent"),
                                                                                  backend::ItemLifecycle::Started,
                                                                                  10});
        reducer.apply(state,
                      backend::ItemUpserted{typed::ThreadId{"thread-delta"},
                                            typed::TurnId{"turn-delta"},
                                            reasoningItem("thread-delta", "turn-delta", "item-reasoning"),
                                            backend::ItemLifecycle::Started,
                                            11});
        reducer.apply(state,
                      backend::ItemUpserted{typed::ThreadId{"thread-delta"},
                                            typed::TurnId{"turn-delta"},
                                            commandItem("thread-delta", "turn-delta", "item-command"),
                                            backend::ItemLifecycle::Started,
                                            12});
        result.expectTrue(agentStart.changed && !agentStart.flushImmediately,
                          "item start changes canonical state without forcing a terminal flush");

        std::string expectedAgent;
        std::string expectedReasoning;
        std::string expectedCommand;
        for (std::size_t index = 0; index < 1000; ++index) {
            const std::string agentDelta = "a" + std::to_string(index % 10);
            const std::string reasoningDelta = "r" + std::to_string(index % 10);
            const std::string commandDelta = "c" + std::to_string(index % 10);
            expectedAgent += agentDelta;
            expectedReasoning += reasoningDelta;
            expectedCommand += commandDelta;
            reducer.apply(state,
                          backend::ItemContentChanged{typed::ThreadId{"thread-delta"},
                                                      typed::TurnId{"turn-delta"},
                                                      typed::ItemId{"item-agent"},
                                                      backend::ItemContentChanged::Kind::AgentText,
                                                      agentDelta,
                                                      std::nullopt});
            reducer.apply(state,
                          backend::ItemContentChanged{typed::ThreadId{"thread-delta"},
                                                      typed::TurnId{"turn-delta"},
                                                      typed::ItemId{"item-reasoning"},
                                                      backend::ItemContentChanged::Kind::ReasoningText,
                                                      reasoningDelta,
                                                      static_cast<std::int64_t>(index)});
            reducer.apply(state,
                          backend::ItemContentChanged{typed::ThreadId{"thread-delta"},
                                                      typed::TurnId{"turn-delta"},
                                                      typed::ItemId{"item-command"},
                                                      backend::ItemContentChanged::Kind::CommandOutput,
                                                      commandDelta,
                                                      std::nullopt});
        }

        const backend::ItemState* accumulatedAgent = findItem(state, "thread-delta", "turn-delta", "item-agent");
        const backend::ItemState* accumulatedReasoning = findItem(state, "thread-delta", "turn-delta", "item-reasoning");
        const backend::ItemState* accumulatedCommand = findItem(state, "thread-delta", "turn-delta", "item-command");
        result.expectTrue(accumulatedAgent && accumulatedAgent->agentText == expectedAgent,
                          "1,000 agent-message deltas accumulate exact final text in canonical state");
        result.expectTrue(accumulatedReasoning && accumulatedReasoning->reasoningText == expectedReasoning,
                          "1,000 reasoning deltas accumulate exact final reasoning without crossing item boundaries");
        result.expectTrue(accumulatedCommand && accumulatedCommand->commandOutput == expectedCommand,
                          "1,000 command-output deltas accumulate exact final output without crossing item boundaries");

        reducer.apply(state,
                      backend::ItemContentChanged{typed::ThreadId{"thread-delta"},
                                                  typed::TurnId{"turn-delta"},
                                                  typed::ItemId{"item-reasoning"},
                                                  backend::ItemContentChanged::Kind::ReasoningSummary,
                                                  "summary",
                                                  0});
        result.expectTrue(findItem(state, "thread-delta", "turn-delta", "item-reasoning")->reasoningSummary == "summary",
                          "reasoning summaries are accumulated independently from reasoning text");

        const backend::Reduction agentComplete =
            reducer.apply(state,
                          backend::ItemUpserted{typed::ThreadId{"thread-delta"},
                                                typed::TurnId{"turn-delta"},
                                                agentItem("thread-delta", "turn-delta", "item-agent", expectedAgent),
                                                backend::ItemLifecycle::Completed,
                                                20});
        result.expectTrue(agentComplete.flushImmediately &&
                              findItem(state, "thread-delta", "turn-delta", "item-agent")->lifecycle == backend::ItemLifecycle::Completed &&
                              findItem(state, "thread-delta", "turn-delta", "item-agent")->completedAtMs == 20,
                          "item completion preserves final content and forces an immediate flush");

        const backend::TurnState* deltaTurn = findTurn(state, "thread-delta", "turn-delta");
        result.expectTrue(deltaTurn && deltaTurn->itemOrder.size() == 3 && deltaTurn->itemOrder[0].value == "item-agent" &&
                              deltaTurn->itemOrder[1].value == "item-reasoning" && deltaTurn->itemOrder[2].value == "item-command",
                          "independent items remain in deterministic first-seen order");

        const backend::Snapshot snapshot = backend::makeSnapshot(state);
        result.expectTrue(snapshot.threads.size() == 1 && snapshot.threads[0].turns.size() == 1 &&
                              snapshot.threads[0].turns[0].items.size() == 3 &&
                              snapshot.threads[0].turns[0].items[0].agentText == expectedAgent &&
                              snapshot.threads[0].turns[0].items[1].reasoningText == expectedReasoning &&
                              snapshot.threads[0].turns[0].items[2].commandOutput == expectedCommand,
                          "snapshot exposes exact accumulated content for each item without merging entities");
    }

    void testCompletionFailureAndAuxiliaryUpdates(tests::support::TestResult& result) {
        backend::BackendState state;
        backend::Reducer reducer;

        typed::Turn completed = turn("thread-terminal", "turn-completed", typed::TurnStatus::completed());
        const backend::Reduction completedReduction = reducer.apply(state, backend::TurnCompleted{completed});
        const backend::TurnState* completedState = findTurn(state, "thread-terminal", "turn-completed");
        result.expectTrue(completedReduction.flushImmediately && completedState && completedState->terminal && !completedState->active,
                          "turn completion marks terminal state and flushes immediately");

        typed::Turn failed = turn("thread-terminal", "turn-failed", typed::TurnStatus::failed());
        const Json failure = {{"message", "model failed"}, {"future", 9}};
        const backend::Reduction failedReduction = reducer.apply(state, backend::TurnFailed{failed, failure});
        const backend::TurnState* failedState = findTurn(state, "thread-terminal", "turn-failed");
        result.expectTrue(failedReduction.flushImmediately && failedState && failedState->terminal && failedState->failure == failure,
                          "turn failure retains structured failure information and flushes immediately");

        reducer.apply(state,
                      backend::TokenUsageUpdated{
                          typed::ThreadId{"thread-terminal"}, typed::TurnId{"turn-failed"}, Json{{"total", {{"inputTokens", 17}}}}});
        reducer.apply(state,
                      backend::ModelRerouted{typed::ThreadId{"thread-terminal"},
                                             typed::TurnId{"turn-failed"},
                                             typed::ModelId{"gpt-old"},
                                             typed::ModelId{"gpt-new"},
                                             "capacity"});
        failedState = findTurn(state, "thread-terminal", "turn-failed");
        result.expectTrue(failedState && failedState->tokenUsage && failedState->modelReroutes.size() == 1 &&
                              failedState->modelReroutes[0].to.value == "gpt-new",
                          "token usage and model rerouting are retained on the owning turn");

        typed::FileChangeItem file;
        file.metadata = metadata("thread-terminal", "turn-failed", "file-item");
        file.status = "inProgress";
        reducer.apply(state,
                      backend::ItemUpserted{typed::ThreadId{"thread-terminal"},
                                            typed::TurnId{"turn-failed"},
                                            typed::Item{file},
                                            backend::ItemLifecycle::Started,
                                            std::nullopt});
        const Json changes = Json::array({Json{{"path", "a.cpp"}, {"kind", "update"}}});
        reducer.apply(state,
                      backend::FileChangeUpdated{
                          typed::ThreadId{"thread-terminal"}, typed::TurnId{"turn-failed"}, typed::ItemId{"file-item"}, changes});
        const backend::ItemState* fileState = findItem(state, "thread-terminal", "turn-failed", "file-item");
        result.expectTrue(fileState && fileState->extensions.at("fileChanges") == changes,
                          "file-change updates remain associated with the correct item");
    }

    void testUserMessageLifecycle(tests::support::TestResult& result) {
        backend::BackendState state;
        backend::Reducer reducer;

        const Json startedContent = Json::array({Json{{"type", "text"}, {"text", "Answer just with OK!"}}});
        const Json startedRaw = {
            {"type", "userMessage"}, {"id", "user-item"}, {"clientId", nullptr}, {"content", startedContent}, {"future", 1}};
        const typed::UserMessageItem startedItem = userMessageItem("thread-user", "turn-user", "user-item", startedContent, startedRaw);
        const Json startedEnvelope =
            Json{{"method", "item/started"},
                 {"params", {{"threadId", "thread-user"}, {"turnId", "turn-user"}, {"item", startedRaw}, {"startedAtMs", 101}}}};
        const std::vector<backend::BackendEvent> startedEvents =
            reducer.translate(typed::Event{typed::ItemStarted{typed::Item{startedItem}, 101, startedEnvelope}});
        const auto* startedUpsert = startedEvents.size() == 1 ? std::get_if<backend::ItemUpserted>(&startedEvents.front()) : nullptr;
        result.expectTrue(startedUpsert && startedUpsert->threadId.value == "thread-user" && startedUpsert->turnId.value == "turn-user" &&
                              startedUpsert->lifecycle == backend::ItemLifecycle::Started && startedUpsert->occurredAtMs == 101,
                          "userMessage start translates to a canonical item upsert at its envelope location");
        if (startedUpsert) {
            reducer.apply(state, *startedUpsert);
        }

        const backend::ItemState* startedState = findItem(state, "thread-user", "turn-user", "user-item");
        const auto* canonicalStarted = startedState ? std::get_if<typed::UserMessageItem>(&startedState->item) : nullptr;
        result.expectTrue(canonicalStarted && canonicalStarted->content == startedContent && !canonicalStarted->clientId &&
                              canonicalStarted->metadata.raw == startedRaw && startedState->lifecycle == backend::ItemLifecycle::Started &&
                              startedState->startedAtMs == 101 && !startedState->completedAtMs && state.recentExtensions.empty(),
                          "userMessage start retains complete content, nullable client ID, raw item, timestamp, and no extension fallback");

        const Json completedContent = Json::array({Json{{"type", "text"}, {"text", "Answer just with OK!"}},
                                                   Json{{"type", "futureContent"}, {"payload", Json::array({1, 2, 3})}}});
        const Json completedRaw = {
            {"type", "userMessage"}, {"id", "user-item"}, {"clientId", nullptr}, {"content", completedContent}, {"future", 2}};
        const typed::UserMessageItem completedItem =
            userMessageItem("thread-user", "turn-user", "user-item", completedContent, completedRaw);
        const Json completedEnvelope =
            Json{{"method", "item/completed"},
                 {"params", {{"threadId", "thread-user"}, {"turnId", "turn-user"}, {"item", completedRaw}, {"completedAtMs", 202}}}};
        const std::vector<backend::BackendEvent> completedEvents =
            reducer.translate(typed::Event{typed::ItemCompleted{typed::Item{completedItem}, 202, completedEnvelope}});
        const auto* completedUpsert = completedEvents.size() == 1 ? std::get_if<backend::ItemUpserted>(&completedEvents.front()) : nullptr;
        result.expectTrue(completedUpsert && completedUpsert->threadId.value == "thread-user" &&
                              completedUpsert->turnId.value == "turn-user" &&
                              completedUpsert->lifecycle == backend::ItemLifecycle::Completed && completedUpsert->occurredAtMs == 202,
                          "userMessage completion translates to a canonical terminal item upsert");
        if (completedUpsert) {
            reducer.apply(state, *completedUpsert);
        }

        const backend::TurnState* userTurn = findTurn(state, "thread-user", "turn-user");
        const backend::ItemState* completedState = findItem(state, "thread-user", "turn-user", "user-item");
        const auto* canonicalCompleted = completedState ? std::get_if<typed::UserMessageItem>(&completedState->item) : nullptr;
        result.expectTrue(
            userTurn && userTurn->items.size() == 1 && userTurn->itemOrder.size() == 1 &&
                userTurn->itemOrder.front().value == "user-item" && canonicalCompleted && canonicalCompleted->content == completedContent &&
                canonicalCompleted->metadata.raw == completedRaw && completedState->lifecycle == backend::ItemLifecycle::Completed &&
                completedState->startedAtMs == 101 && completedState->completedAtMs == 202 && state.recentExtensions.empty(),
            "userMessage completion updates the same canonical item and retains both lifecycle timestamps without extensions");

        const backend::Snapshot itemSnapshot = backend::makeSnapshot(state);
        const Json& userMessageData = itemSnapshot.threads[0].turns[0].items[0].data;
        result.expectTrue(itemSnapshot.threads.size() == 1 && itemSnapshot.threads[0].turns.size() == 1 &&
                              itemSnapshot.threads[0].turns[0].items.size() == 1 &&
                              itemSnapshot.threads[0].turns[0].items[0].type == "user_message" &&
                              userMessageData.at("content").is_array() && userMessageData.at("content") == completedContent &&
                              userMessageData.at("clientId").is_null() && !userMessageData.at("contentTruncated").get<bool>() &&
                              userMessageData.at("originalContentBytes") == completedContent.dump().size() &&
                              userMessageData.at("retainedContentBytes") == completedContent.dump().size() &&
                              userMessageData.at("originalContentItems") == completedContent.size() &&
                              userMessageData.at("retainedContentItems") == completedContent.size(),
                          "small userMessage snapshots preserve array content and report equal original and retained bounds");

        typed::Turn terminalTurn =
            turn("thread-user", "turn-user", typed::TurnStatus::completed(), std::vector<typed::Item>{typed::Item{completedItem}});
        reducer.apply(state, backend::TurnCompleted{std::move(terminalTurn)});
        completedState = findItem(state, "thread-user", "turn-user", "user-item");
        result.expectTrue(completedState && completedState->lifecycle == backend::ItemLifecycle::Completed &&
                              completedState->startedAtMs == 101 && completedState->completedAtMs == 202,
                          "a later terminal turn snapshot does not erase item lifecycle timestamps");
    }

    void testUserMessageSnapshotBounding(tests::support::TestResult& result) {
        backend::Reducer reducer;

        backend::BackendState smallState;
        const Json smallContent =
            Json::array({Json{{"type", "text"}, {"text", "small"}}, Json{{"type", "future"}, {"nested", Json{{"kept", true}}}}});
        typed::UserMessageItem smallItem =
            userMessageItem("thread-small", "turn-small", "item-small", smallContent, Json{{"id", "item-small"}}, "client-small");
        reducer.apply(smallState,
                      backend::ItemUpserted{typed::ThreadId{"thread-small"},
                                            typed::TurnId{"turn-small"},
                                            typed::Item{smallItem},
                                            backend::ItemLifecycle::Completed,
                                            10});
        const backend::Snapshot smallStateSnapshot = backend::makeSnapshot(smallState);
        const backend::ItemSnapshot& smallSnapshot = smallStateSnapshot.threads[0].turns[0].items[0];
        const Json& smallData = smallSnapshot.data;
        result.expectTrue(
            smallData.at("clientId") == "client-small" && smallData.at("content").is_array() && smallData.at("content") == smallContent &&
                !smallData.at("contentTruncated").get<bool>() && smallData.at("originalContentBytes") == smallContent.dump().size() &&
                smallData.at("retainedContentBytes") == smallContent.dump().size() &&
                smallData.at("originalContentItems") == smallContent.size() && smallData.at("retainedContentItems") == smallContent.size(),
            "small userMessage content and a non-null clientId remain lossless with complete bound metadata");

        backend::BackendState largeState;
        Json largeContent = Json::array();
        largeContent.push_back(Json{{"type", "futureNested"}, {"payload", Json{{"unknown", Json::array({1, Json{{"deep", true}}})}}}});
        for (std::size_t index = 0; index < 8; ++index) {
            largeContent.push_back(Json{{"type", "futureChunk"},
                                        {"index", index},
                                        {"payload", std::string(12U * 1024U, static_cast<char>('a' + index))},
                                        {"opaque", Json{{"index", index}, {"enabled", index % 2 == 0}}}});
        }
        typed::UserMessageItem largeItem =
            userMessageItem("thread-large", "turn-large", "item-large", largeContent, Json{{"id", "item-large"}}, "client-large");
        reducer.apply(largeState,
                      backend::ItemUpserted{typed::ThreadId{"thread-large"},
                                            typed::TurnId{"turn-large"},
                                            typed::Item{largeItem},
                                            backend::ItemLifecycle::Completed,
                                            20});
        const backend::Snapshot largeStateSnapshot = backend::makeSnapshot(largeState);
        const backend::ItemSnapshot& largeSnapshot = largeStateSnapshot.threads[0].turns[0].items[0];
        const Json& largeData = largeSnapshot.data;
        const Json& retainedContent = largeData.at("content");
        const std::size_t retainedItems = retainedContent.size();
        bool retainedPrefixUnchanged = retainedItems > 0 && retainedItems < largeContent.size();
        for (std::size_t index = 0; index < retainedItems; ++index) {
            retainedPrefixUnchanged = retainedPrefixUnchanged && retainedContent[index] == largeContent[index] &&
                                      retainedContent[index].dump() == largeContent[index].dump();
        }
        const backend::ItemState* canonicalLarge = findItem(largeState, "thread-large", "turn-large", "item-large");
        const auto* canonicalLargeUser = canonicalLarge ? std::get_if<typed::UserMessageItem>(&canonicalLarge->item) : nullptr;
        result.expectTrue(
            largeContent.dump().size() > backend::MaxSerializedUserMessageDataBytes && retainedContent.is_array() &&
                largeData.at("contentTruncated").get<bool>() && retainedPrefixUnchanged &&
                largeData.at("originalContentItems") == largeContent.size() && largeData.at("retainedContentItems") == retainedItems &&
                largeData.at("originalContentBytes") == largeContent.dump().size() &&
                largeData.at("retainedContentBytes") == retainedContent.dump().size() &&
                largeData.dump().size() <= backend::MaxSerializedUserMessageDataBytes && canonicalLargeUser &&
                canonicalLargeUser->content == largeContent && !largeSnapshot.contentTruncated && largeSnapshot.droppedContentBytes == 0,
            "large userMessage snapshots retain an unchanged ordered prefix of complete opaque entries within 64 KiB");

        backend::BackendState oversizedFirstState;
        const Json oversizedFirstContent = Json::array({Json{{"type", "futureHuge"}, {"payload", std::string(70U * 1024U, 'z')}}});
        typed::UserMessageItem oversizedFirstItem =
            userMessageItem("thread-first", "turn-first", "item-first", oversizedFirstContent, Json{{"id", "item-first"}});
        reducer.apply(oversizedFirstState,
                      backend::ItemUpserted{typed::ThreadId{"thread-first"},
                                            typed::TurnId{"turn-first"},
                                            typed::Item{oversizedFirstItem},
                                            backend::ItemLifecycle::Completed,
                                            30});
        const backend::Snapshot oversizedFirstSnapshot = backend::makeSnapshot(oversizedFirstState);
        const Json& oversizedFirstData = oversizedFirstSnapshot.threads[0].turns[0].items[0].data;
        result.expectTrue(oversizedFirstData.at("content").is_array() && oversizedFirstData.at("content").empty() &&
                              oversizedFirstData.at("contentTruncated").get<bool>() && oversizedFirstData.at("originalContentItems") == 1 &&
                              oversizedFirstData.at("retainedContentItems") == 0 &&
                              oversizedFirstData.at("originalContentBytes") == oversizedFirstContent.dump().size() &&
                              oversizedFirstData.at("retainedContentBytes") == Json::array().dump().size() &&
                              oversizedFirstData.dump().size() <= backend::MaxSerializedUserMessageDataBytes,
                          "an oversized first userMessage entry yields an empty array without exposing partial JSON or text");
    }

    void testUnknownItemCommonMetadataFallbacks(tests::support::TestResult& result) {
        backend::Reducer reducer;
        backend::BackendState state;

        typed::UnknownItem located;
        located.type = "futureItem";
        located.raw = Json{{"type", "futureItem"}, {"id", "unknown-located"}, {"future", Json::array({1, 2, 3})}};
        located.decodingError = "future detailed field is unsupported";
        located.metadata.id = typed::ItemId{"unknown-located"};
        located.metadata.threadId = typed::ThreadId{"thread-unknown"};
        located.metadata.turnId = typed::TurnId{"turn-unknown"};
        const Json locatedEnvelope =
            Json{{"method", "item/started"},
                 {"params", {{"threadId", "thread-unknown"}, {"turnId", "turn-unknown"}, {"item", located.raw}, {"startedAtMs", 303}}}};
        const std::vector<backend::BackendEvent> locatedEvents =
            reducer.translate(typed::Event{typed::ItemStarted{typed::Item{located}, 303, locatedEnvelope}});
        const auto* locatedUpsert = locatedEvents.size() == 1 ? std::get_if<backend::ItemUpserted>(&locatedEvents.front()) : nullptr;
        result.expectTrue(locatedUpsert && locatedUpsert->threadId.value == "thread-unknown" &&
                              locatedUpsert->turnId.value == "turn-unknown",
                          "an unknown item with valid common metadata translates canonically instead of reporting a false location error");
        if (locatedUpsert) {
            reducer.apply(state, *locatedUpsert);
        }
        const backend::ItemState* locatedState = findItem(state, "thread-unknown", "turn-unknown", "unknown-located");
        const auto* canonicalUnknown = locatedState ? std::get_if<typed::UnknownItem>(&locatedState->item) : nullptr;
        result.expectTrue(canonicalUnknown && canonicalUnknown->metadata.id && canonicalUnknown->metadata.id->value == "unknown-located" &&
                              canonicalUnknown->metadata.threadId && canonicalUnknown->metadata.threadId->value == "thread-unknown" &&
                              canonicalUnknown->metadata.turnId && canonicalUnknown->metadata.turnId->value == "turn-unknown" &&
                              canonicalUnknown->raw == located.raw && canonicalUnknown->decodingError == located.decodingError &&
                              state.recentExtensions.empty(),
                          "canonical unknown items retain their ID, envelope location, raw JSON, and decoding error");

        backend::ReducerOptions boundedOptions;
        boundedOptions.retainedExtensions = 1;
        boundedOptions.maxExtensionBytes = 64;
        backend::Reducer boundedReducer(boundedOptions);
        backend::BackendState noIdState;
        typed::UnknownItem noId;
        noId.type = "futureWithoutId";
        noId.raw = Json{{"type", "futureWithoutId"}, {"payload", std::string(256, 'x')}};
        noId.metadata.threadId = typed::ThreadId{"thread-no-id"};
        noId.metadata.turnId = typed::TurnId{"turn-no-id"};
        const Json noIdEnvelope =
            Json{{"method", "item/started"},
                 {"params", {{"threadId", "thread-no-id"}, {"turnId", "turn-no-id"}, {"item", noId.raw}, {"startedAtMs", 404}}}};
        const std::vector<backend::BackendEvent> noIdEvents =
            boundedReducer.translate(typed::Event{typed::ItemStarted{typed::Item{noId}, 404, noIdEnvelope}});
        const auto* noIdUpsert = noIdEvents.size() == 1 ? std::get_if<backend::ItemUpserted>(&noIdEvents.front()) : nullptr;
        result.expectTrue(noIdUpsert && noIdUpsert->threadId.value == "thread-no-id" && noIdUpsert->turnId.value == "turn-no-id",
                          "an unknown item with a valid location but no stable ID reaches canonical ID validation");
        if (noIdUpsert) {
            boundedReducer.apply(noIdState, *noIdUpsert);
        }
        const backend::ExtensionRecord* noIdExtension =
            noIdState.recentExtensions.size() == 1 ? &noIdState.recentExtensions.front() : nullptr;
        result.expectTrue(noIdState.threads.empty() && noIdExtension && noIdExtension->method == "codex/item-without-id" &&
                              noIdExtension->decodingError == "typed item has no stable id" &&
                              noIdExtension->originalPayloadBytes.has_value() && noIdExtension->payload.value("truncated", false) &&
                              noIdExtension->payload.value("omitted", false),
                          "a truly ID-less item uses the bounded item-without-id extension fallback");

        backend::BackendState missingLocationState;
        typed::UnknownItem missingLocation;
        missingLocation.type = "futureMissingLocation";
        missingLocation.raw = Json{{"type", "futureMissingLocation"}, {"id", "unknown-missing-location"}};
        missingLocation.metadata.id = typed::ItemId{"unknown-missing-location"};
        missingLocation.metadata.threadId = typed::ThreadId{"thread-missing-location"};
        const Json missingLocationEnvelope =
            Json{{"method", "item/started"}, {"params", {{"threadId", "thread-missing-location"}, {"item", missingLocation.raw}}}};
        const std::vector<backend::BackendEvent> missingLocationEvents =
            reducer.translate(typed::Event{typed::ItemStarted{typed::Item{missingLocation}, 505, missingLocationEnvelope}});
        const auto* locationExtension =
            missingLocationEvents.size() == 1 ? std::get_if<backend::CodexExtensionReceived>(&missingLocationEvents.front()) : nullptr;
        result.expectTrue(locationExtension && locationExtension->method == "item/started" &&
                              locationExtension->payload == missingLocationEnvelope &&
                              locationExtension->decodingError == "item event omitted threadId or turnId",
                          "an item genuinely missing part of its envelope location retains the lifecycle extension fallback");
        if (locationExtension) {
            reducer.apply(missingLocationState, *locationExtension);
        }
        result.expectTrue(missingLocationState.threads.empty() && missingLocationState.recentExtensions.size() == 1,
                          "an unusable item location is not inserted into canonical thread state");
    }

    void testUnknownPreservationAndTranslation(tests::support::TestResult& result) {
        backend::ReducerOptions options;
        options.retainedExtensions = 2;
        backend::Reducer reducer(options);
        backend::BackendState state;

        typed::UnknownItem unknown;
        unknown.type = "futureItem";
        unknown.raw = Json{{"id", "unknown-item"}, {"future", Json::array({1, 2, 3})}};
        unknown.decodingError = "future item shape";
        reducer.apply(state,
                      backend::ItemUpserted{typed::ThreadId{"thread-extension"},
                                            typed::TurnId{"turn-extension"},
                                            typed::Item{unknown},
                                            backend::ItemLifecycle::Started,
                                            std::nullopt});
        const backend::ItemState* unknownState = findItem(state, "thread-extension", "turn-extension", "unknown-item");
        result.expectTrue(unknownState && backend::itemType(unknownState->item) == "futureItem" &&
                              std::get<typed::UnknownItem>(unknownState->item).raw == unknown.raw,
                          "unknown typed items with stable IDs remain visible with their extension payload");

        const typed::UnknownEvent unknownEvent{
            "future/event", Json{{"value", 7}}, Json{{"method", "future/event"}}, std::string("future decoder")};
        const std::vector<backend::BackendEvent> translated = reducer.translate(typed::Event{unknownEvent});
        const auto* extension = translated.size() == 1 ? std::get_if<backend::CodexExtensionReceived>(&translated[0]) : nullptr;
        result.expectTrue(extension && extension->method == "future/event" && extension->payload == unknownEvent.params &&
                              extension->decodingError == "future decoder",
                          "unknown typed Codex events translate to deliberately namespaced backend extensions");

        if (extension) {
            reducer.apply(state, *extension);
        }
        reducer.apply(state, backend::CodexExtensionReceived{"future/two", Json{{"value", 2}}, std::nullopt});
        reducer.apply(state, backend::CodexExtensionReceived{"future/three", Json{{"value", 3}}, std::nullopt});
        result.expectTrue(state.recentExtensions.size() == 2 && state.recentExtensions[0].method == "future/two" &&
                              state.recentExtensions[1].method == "future/three",
                          "unknown extension retention is bounded and evicts oldest records deterministically");

        backend::BackendState sensitiveState;
        const std::string accessToken = "extension-access-token-must-not-leak";
        const std::string secretAnswer = "extension-secret-answer-must-not-leak";
        const std::string longMethod(backend::MaxSnapshotExtensionMethodBytes + 17, 'm');
        const std::string longError(backend::MaxSnapshotExtensionDecodingErrorBytes + 19, 'e');
        reducer.apply(
            sensitiveState,
            backend::CodexExtensionReceived{longMethod,
                                            Json{{"safe", "visible"},
                                                 {"accessToken", accessToken},
                                                 {"Client_Secret", accessToken},
                                                 {"nested", {{"secret", true}, {"text", secretAnswer}, {"answer", secretAnswer}}}},
                                            longError});
        const backend::Snapshot sensitiveSnapshot = backend::makeSnapshot(sensitiveState);
        const backend::ExtensionSnapshot* safeExtension =
            sensitiveSnapshot.recentExtensions.size() == 1 ? &sensitiveSnapshot.recentExtensions.front() : nullptr;
        const std::string encodedSafeExtension = safeExtension ? safeExtension->payload.dump() : std::string();
        result.expectTrue(
            safeExtension && safeExtension->method.size() == backend::MaxSnapshotExtensionMethodBytes && safeExtension->methodTruncated &&
                safeExtension->decodingError && safeExtension->decodingError->size() == backend::MaxSnapshotExtensionDecodingErrorBytes &&
                safeExtension->decodingErrorTruncated && safeExtension->sensitiveFieldsRedacted &&
                safeExtension->payload.at("safe") == "visible" && encodedSafeExtension.find(accessToken) == std::string::npos &&
                encodedSafeExtension.find(secretAnswer) == std::string::npos,
            "public extension snapshots preserve safe fields while bounding text and recursively redacting credentials and secret answers");

        const std::string oversizedSecret(backend::MaxSnapshotExtensionPayloadBytes * 3, 's');
        reducer.apply(sensitiveState,
                      backend::CodexExtensionReceived{
                          "future/oversized", Json{{"accessToken", oversizedSecret}, {"safe", "would exceed the bound"}}, std::nullopt});
        const backend::Snapshot oversizedSnapshot = backend::makeSnapshot(sensitiveState);
        const backend::ExtensionSnapshot& oversizedExtension = oversizedSnapshot.recentExtensions.back();
        result.expectTrue(
            oversizedExtension.payloadTruncated && oversizedExtension.originalPayloadBytes.has_value() &&
                oversizedExtension.payload.dump().size() <= backend::MaxSnapshotExtensionPayloadBytes &&
                oversizedExtension.payload.value("omitted", false) &&
                oversizedExtension.payload.dump().find(oversizedSecret) == std::string::npos,
            "oversized extension payloads become explicit bounded omission records instead of leaking an unbatchable preview");

        backend::ReducerOptions manyOptions;
        manyOptions.retainedExtensions = backend::MaxSnapshotCodexExtensions + 2;
        backend::Reducer manyReducer(manyOptions);
        backend::BackendState manyState;
        for (std::size_t index = 0; index < backend::MaxSnapshotCodexExtensions + 2; ++index) {
            manyReducer.apply(manyState,
                              backend::CodexExtensionReceived{"future/" + std::to_string(index), Json{{"index", index}}, std::nullopt});
        }
        const backend::Snapshot manySnapshot = backend::makeSnapshot(manyState);
        result.expectTrue(
            manySnapshot.recentExtensions.size() == backend::MaxSnapshotCodexExtensions && manySnapshot.omittedRecentExtensions == 2 &&
                manySnapshot.recentExtensions.front().method == "future/2" && manySnapshot.recentExtensions.back().method == "future/65",
            "public snapshots retain a deterministic bounded newest suffix when a custom reducer keeps more extension records");

        const typed::AgentMessageDelta delta{
            typed::ThreadId{"thread-extension"}, typed::TurnId{"turn-extension"}, typed::ItemId{"unknown-item"}, "x", Json::object()};
        const std::vector<backend::BackendEvent> translatedDelta = reducer.translate(typed::Event{delta});
        const auto* content = translatedDelta.size() == 1 ? std::get_if<backend::ItemContentChanged>(&translatedDelta[0]) : nullptr;
        result.expectTrue(content && content->kind == backend::ItemContentChanged::Kind::AgentText && content->delta == "x",
                          "known typed deltas normalize to backend content changes instead of raw envelopes");
    }

    typed::CommandApprovalRequest approvalRequest() {
        return typed::CommandApprovalRequest{ServerRequestId{std::string("server-request")},
                                             ServerRequestToken{77},
                                             typed::ThreadId{"thread-request"},
                                             typed::TurnId{"turn-request"},
                                             typed::ItemId{"item-request"},
                                             13,
                                             std::string("make test"),
                                             std::string("/tmp/project"),
                                             std::string("needs approval"),
                                             Json{{"future", true}},
                                             Json{{"privateOccurrence", 77}}};
    }

    void testPendingRequestsAndSessions(tests::support::TestResult& result) {
        backend::BackendState state;
        backend::Reducer reducer;
        const backend::PendingRequestState pending{backend::PendingRequestId{9}, typed::TypedServerRequest{approvalRequest()}, 3};

        const backend::Reduction added = reducer.apply(state, backend::PendingRequestAdded{pending});
        result.expectTrue(added.flushImmediately && state.pendingRequests.size() == 1 &&
                              state.pendingRequests.at(backend::PendingRequestId{9}).connectionGeneration == 3,
                          "pending request insertion retains the exact typed occurrence until an authoritative removal");

        const backend::Snapshot pendingSnapshot = backend::makeSnapshot(state);
        result.expectTrue(pendingSnapshot.pendingRequests.size() == 1 && pendingSnapshot.pendingRequests[0].id.value() == 9 &&
                              pendingSnapshot.pendingRequests[0].type == "command_approval" &&
                              pendingSnapshot.pendingRequests[0].details.value("command", "") == "make test" &&
                              !pendingSnapshot.pendingRequests[0].details.contains("privateOccurrence"),
                          "pending request snapshot exposes safe frontend details without occurrence tokens or raw request data");

        const std::string unknownAccessToken = "unknown-request-access-token-must-not-leak";
        const std::string unknownSecretAnswer = "unknown-request-secret-answer-must-not-leak";
        const std::string occurrenceSentinel = "unknown-request-occurrence-token-must-not-leak";
        const std::string unknownDecodingError(backend::MaxSnapshotExtensionDecodingErrorBytes + 23, 'd');
        const typed::UnknownServerRequest unknownRequest{
            ServerRequestId{std::string("unknown-request")},
            ServerRequestToken{88},
            "future/request",
            Json{{"safe", "visible"}, {"accessToken", unknownAccessToken}, {"question", {{"secret", true}, {"text", unknownSecretAnswer}}}},
            Json{{"occurrenceToken", occurrenceSentinel}},
            unknownDecodingError};
        reducer.apply(state,
                      backend::PendingRequestAdded{
                          backend::PendingRequestState{backend::PendingRequestId{10}, typed::TypedServerRequest{unknownRequest}, 3}});
        const backend::Snapshot unknownPendingSnapshot = backend::makeSnapshot(state);
        const auto unknownPending = std::find_if(unknownPendingSnapshot.pendingRequests.begin(),
                                                 unknownPendingSnapshot.pendingRequests.end(),
                                                 [](const backend::PendingRequestSnapshot& value) {
                                                     return value.id == backend::PendingRequestId{10};
                                                 });
        const std::string compactUnknownPending =
            unknownPending != unknownPendingSnapshot.pendingRequests.end() ? unknownPending->details.dump() : std::string();
        result.expectTrue(unknownPending != unknownPendingSnapshot.pendingRequests.end() && unknownPending->type == "unknown" &&
                              unknownPending->details["params"].value("safe", "") == "visible" &&
                              unknownPending->details.value("sensitiveFieldsRedacted", false) &&
                              unknownPending->details.value("decodingError", "").size() ==
                                  backend::MaxSnapshotExtensionDecodingErrorBytes &&
                              unknownPending->details.value("decodingErrorTruncated", false) &&
                              compactUnknownPending.find(unknownAccessToken) == std::string::npos &&
                              compactUnknownPending.find(unknownSecretAnswer) == std::string::npos &&
                              compactUnknownPending.find(occurrenceSentinel) == std::string::npos,
                          "compact unknown-request snapshots recursively redact secret params and never expose the raw occurrence token");

        result.expectTrue(state.pendingRequests.size() == 2,
                          "a pending request remains present when no successful response-removal transition occurs");
        const backend::Reduction removed =
            reducer.apply(state, backend::PendingRequestRemoved{backend::PendingRequestId{9}, "response enqueued"});
        result.expectTrue(removed.changed && removed.flushImmediately && state.pendingRequests.size() == 1 &&
                              state.pendingRequests.contains(backend::PendingRequestId{10}),
                          "successful response removal erases only that pending request and flushes interactively");

        const backend::SessionId observer{1};
        const backend::SessionId controller{2};
        reducer.apply(state, backend::SessionChanged{observer, true, backend::SessionRole::Observer});
        reducer.apply(state, backend::SessionChanged{controller, true, backend::SessionRole::Observer});
        result.expectTrue(state.sessions.size() == 2 && !state.controller,
                          "newly connected sessions are retained as observers without implicit controller assignment");

        reducer.apply(state, backend::ControllerChanged{controller});
        result.expectTrue(state.controller == controller && state.sessions.at(controller).role == backend::SessionRole::Controller &&
                              state.sessions.at(observer).role == backend::SessionRole::Observer,
                          "controller acquisition changes exactly one session role");

        reducer.apply(state, backend::ControllerChanged{std::nullopt});
        result.expectTrue(!state.controller && state.sessions.at(controller).role == backend::SessionRole::Observer,
                          "controller release returns the former controller to observer role");

        reducer.apply(state, backend::ControllerChanged{controller});
        reducer.apply(state, backend::SessionChanged{controller, false, backend::SessionRole::Observer});
        result.expectTrue(!state.controller && state.sessions.size() == 1 && state.sessions.contains(observer),
                          "controller disconnect removes only that session and releases ownership");
    }

    void testSnapshotDeterminism(tests::support::TestResult& result) {
        backend::BackendState state;
        backend::Reducer reducer;
        reducer.apply(state, backend::ThreadUpserted{thread("thread-z"), backend::EntityLoad::Summary});
        reducer.apply(state, backend::ThreadUpserted{thread("thread-a"), backend::EntityLoad::Summary});
        reducer.apply(state, backend::TurnUpserted{turn("thread-z", "turn-z")});
        reducer.apply(state, backend::TurnUpserted{turn("thread-z", "turn-a")});
        reducer.apply(state,
                      backend::ItemUpserted{typed::ThreadId{"thread-z"},
                                            typed::TurnId{"turn-z"},
                                            agentItem("thread-z", "turn-z", "item-z", "z"),
                                            backend::ItemLifecycle::Started,
                                            std::nullopt});
        reducer.apply(state,
                      backend::ItemUpserted{typed::ThreadId{"thread-z"},
                                            typed::TurnId{"turn-z"},
                                            agentItem("thread-z", "turn-z", "item-a", "a"),
                                            backend::ItemLifecycle::Started,
                                            std::nullopt});

        state.sequence = backend::SequenceNumber{42};
        const backend::Snapshot first = backend::makeSnapshot(state);
        const backend::Snapshot second = backend::makeSnapshot(state);
        result.expectTrue(first == second && !(first != second), "two snapshots of unchanged state compare equal");
        result.expectTrue(first.sequence.value() == 42 && first.threads[0].id == "thread-z" && first.threads[1].id == "thread-a" &&
                              first.threads[0].turns[0].id == "turn-z" && first.threads[0].turns[1].id == "turn-a" &&
                              first.threads[0].turns[0].items[0].id == "item-z" && first.threads[0].turns[0].items[1].id == "item-a",
                          "snapshot includes sequence and deterministic first-seen entity ordering");

        reducer.apply(state, backend::DiagnosticReceived{"changed"});
        result.expectTrue(first != backend::makeSnapshot(state), "a visible state transition changes snapshot equality");
    }
} // namespace

int main() {
    tests::support::TestResult result;

    testInitialStateAndLifecycle(result);
    testThreadAndTurnHydration(result);
    testItemsAndHighVolumeDeltas(result);
    testCompletionFailureAndAuxiliaryUpdates(result);
    testUserMessageLifecycle(result);
    testUserMessageSnapshotBounding(result);
    testUnknownItemCommonMetadataFallbacks(result);
    testUnknownPreservationAndTranslation(result);
    testPendingRequestsAndSessions(result);
    testSnapshotDeterminism(result);

    return result.processResult();
}
