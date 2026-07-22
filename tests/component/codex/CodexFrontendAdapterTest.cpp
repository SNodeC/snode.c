/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "CodexBackendTestSupport.h"
#include "ai/openai/codex/backend/BackendCore.h"
#include "ai/openai/codex/backend/Snapshot.h"
#include "ai/openai/codex/frontend/BackendAdapter.h"
#include "ai/openai/codex/frontend/EventCoalescer.h"
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace {
    namespace backend = ai::openai::codex::backend;
    namespace frontend = ai::openai::codex::frontend;

    using FakeBackendCore = backend::BackendCore<tests::codex::FakeAppServerClient>;

    using ai::openai::codex::Json;
    using ai::openai::codex::detail::TransportCallbacks;

    Json agentItemValue(const std::string& id, const std::string& text = {}) {
        return {{"type", "agentMessage"}, {"id", id}, {"text", text}};
    }

    class ManualScheduler {
    public:
        void schedule(std::function<void()> callback) {
            callbacks.push_back(std::move(callback));
            ++scheduled;
        }

        bool runOne() {
            if (callbacks.empty()) {
                return false;
            }
            std::function<void()> callback = std::move(callbacks.front());
            callbacks.pop_front();
            callback();
            ++executed;
            return true;
        }

        void drain(std::size_t limit = 100'000) {
            std::size_t count = 0;
            while (runOne()) {
                if (++count >= limit) {
                    throw std::runtime_error("manual scheduler drain limit exceeded");
                }
            }
        }

        std::size_t pending() const noexcept {
            return callbacks.size();
        }

        std::size_t scheduled = 0;
        std::size_t executed = 0;

    private:
        std::deque<std::function<void()>> callbacks;
    };

    struct Observations {
        std::vector<frontend::ServerMessage> messages;
        std::vector<std::string> compactJson;
        std::vector<std::size_t> serializedBytes;
        std::vector<std::string> closeReasons;
    };

    frontend::FrontendConnectionCallbacks callbacksFor(Observations& observations) {
        return {[&observations](const frontend::OutboundMessage& message) {
                    observations.messages.push_back(message.message);
                    observations.compactJson.push_back(message.compactJson);
                    observations.serializedBytes.push_back(message.serializedBytes);
                    return true;
                },
                [&observations](const std::string& reason) {
                    observations.closeReasons.push_back(reason);
                }};
    }

    frontend::ClientMessage hello(std::optional<frontend::SequenceNumber> resumeAfter = std::nullopt) {
        return frontend::Hello{resumeAfter, frontend::Json::object()};
    }

    frontend::ClientMessage command(std::string requestId, frontend::CommandParameters parameters) {
        return frontend::Command{std::move(requestId), std::move(parameters), frontend::Json::object(), frontend::Json::object()};
    }

    const frontend::Welcome* welcome(const Observations& observations) {
        for (const frontend::ServerMessage& message : observations.messages) {
            if (const auto* value = std::get_if<frontend::Welcome>(&message)) {
                return value;
            }
        }
        return nullptr;
    }

    const frontend::Response* response(const Observations& observations, const std::string& requestId) {
        for (const frontend::ServerMessage& message : observations.messages) {
            if (const auto* value = std::get_if<frontend::Response>(&message); value && value->requestId == requestId) {
                return value;
            }
        }
        return nullptr;
    }

    bool responseHasError(const Observations& observations, const std::string& requestId, frontend::ErrorCode code) {
        const frontend::Response* value = response(observations, requestId);
        return value && !value->ok && value->error && value->error->code == code;
    }

    bool hasSuccessfulResponse(const Observations& observations, const std::string& requestId) {
        for (const frontend::ServerMessage& message : observations.messages) {
            if (const auto* value = std::get_if<frontend::Response>(&message); value && value->requestId == requestId && value->ok) {
                return true;
            }
        }
        return false;
    }

    std::size_t countSnapshots(const Observations& observations) {
        std::size_t result = 0;
        for (const frontend::ServerMessage& message : observations.messages) {
            result += std::holds_alternative<frontend::Snapshot>(message) ? 1U : 0U;
        }
        return result;
    }

    const frontend::Snapshot* latestSnapshot(const Observations& observations) {
        for (auto iterator = observations.messages.rbegin(); iterator != observations.messages.rend(); ++iterator) {
            if (const auto* snapshot = std::get_if<frontend::Snapshot>(&*iterator)) {
                return snapshot;
            }
        }
        return nullptr;
    }

    std::vector<frontend::FrontendEvent> events(const Observations& observations) {
        std::vector<frontend::FrontendEvent> result;
        for (const frontend::ServerMessage& message : observations.messages) {
            if (const auto* batch = std::get_if<frontend::EventBatch>(&message)) {
                result.insert(result.end(), batch->events.begin(), batch->events.end());
            }
        }
        return result;
    }

    void testCoalescingAndBounds(tests::support::TestResult& result) {
        frontend::EventCoalescer coalescer({32});
        const frontend::CoalescingKey agentKey = frontend::CoalescingKey::itemContent("thread-1", "turn-1", "item-1", "agentText");

        std::string accumulated;
        std::size_t schedulingRequests = 0;
        for (std::size_t index = 0; index < 1000; ++index) {
            accumulated += static_cast<char>('a' + static_cast<int>(index % 26));
            const frontend::CoalescerMarkResult marked =
                coalescer.mark({agentKey,
                                "item.content.updated",
                                frontend::Json{{"itemId", "item-1"}, {"channel", "agentText"}, {"content", accumulated}},
                                frontend::FlushUrgency::Deferred});
            schedulingRequests += marked.scheduleRequired ? 1U : 0U;
        }
        result.expectTrue(coalescer.dirtyCount() == 1 && schedulingRequests == 1 && coalescer.flushScheduled(),
                          "1,000 raw deltas dirty one entity and request exactly one next-tick flush");

        const frontend::CoalescerDrainResult drained = coalescer.drain();
        result.expectTrue(drained.updates.size() == 1 && drained.updates.front().data["content"] == accumulated,
                          "coalescing preserves the exact final accumulated agent text");

        frontend::EventJournal journal({64, 64U * 1024U, frontend::SequenceNumber{0}});
        const auto appended = journal.append(drained.updates.front().type, drained.updates.front().data);
        frontend::UpdateBatchBuilder batches({16, 16U * 1024U});
        const auto built = batches.build({*appended.event});
        result.expectTrue(built.success() && built.batches.size() == 1 && built.batches.front().batch.events.size() == 1 &&
                              built.batches.front().batch.events.size() < 1000,
                          "1,000 token deltas become one normalized frontend message, substantially below raw granularity");

        const auto reasoning = coalescer.mark({frontend::CoalescingKey::itemContent("thread-1", "turn-1", "item-1", "reasoningText"),
                                               "item.content.updated",
                                               frontend::Json{{"content", "reasoning-final"}},
                                               frontend::FlushUrgency::Deferred});
        const auto commandOutput = coalescer.mark({frontend::CoalescingKey::itemContent("thread-1", "turn-1", "item-1", "commandOutput"),
                                                   "item.content.updated",
                                                   frontend::Json{{"content", "command-final"}},
                                                   frontend::FlushUrgency::Deferred});
        const auto otherTurn = coalescer.mark({frontend::CoalescingKey::itemContent("thread-1", "turn-2", "item-1", "reasoningText"),
                                               "item.content.updated",
                                               frontend::Json{{"content", "other-turn"}},
                                               frontend::FlushUrgency::Deferred});
        result.expectTrue(reasoning.accepted() && commandOutput.accepted() && otherTurn.accepted() && coalescer.dirtyCount() == 3,
                          "reasoning, command output, and a same-named item in another turn never coalesce together");

        const frontend::CoalescerMarkResult terminal = coalescer.mark({frontend::CoalescingKey::item("thread-1", "turn-1", "item-1"),
                                                                       "item.updated",
                                                                       frontend::Json{{"status", "completed"}},
                                                                       frontend::FlushUrgency::Immediate});
        result.expectTrue(terminal.immediateFlush, "item completion upgrades a pending flush to immediate");
        const frontend::CoalescerDrainResult terminalDrain = coalescer.drain();
        result.expectTrue(terminalDrain.updates.size() == 4 && terminalDrain.updates.back().type == "item.updated",
                          "terminal flush preserves independent dirty-entity insertion order");

        frontend::EventCoalescer terminalOrdering({8});
        const frontend::CoalescingKey turnKey = frontend::CoalescingKey::turn("thread-order", "turn-order");
        result.expectTrue(terminalOrdering
                              .mark({turnKey,
                                     "turn.updated",
                                     frontend::Json{{"turn", {{"id", "turn-order"}, {"terminal", false}}}},
                                     frontend::FlushUrgency::Deferred})
                              .accepted(),
                          "turn start dirties the turn key before content arrives");
        result.expectTrue(terminalOrdering
                              .mark({frontend::CoalescingKey::itemContent("thread-order", "turn-order", "item-order", "agentText"),
                                     "item.content.updated",
                                     frontend::Json{{"itemId", "item-order"}, {"channel", "agentText"}, {"content", "final"}},
                                     frontend::FlushUrgency::Deferred})
                              .accepted(),
                          "final item content remains independently dirty from its turn");
        result.expectTrue(terminalOrdering
                              .mark({turnKey,
                                     "turn.updated",
                                     frontend::Json{{"turn", {{"id", "turn-order"}, {"terminal", true}}}},
                                     frontend::FlushUrgency::Immediate})
                              .accepted(),
                          "turn completion replaces its earlier dirty turn state");
        const frontend::CoalescerDrainResult orderedTerminalDrain = terminalOrdering.drain();
        result.expectTrue(orderedTerminalDrain.updates.size() == 2 && orderedTerminalDrain.updates[0].type == "item.content.updated" &&
                              orderedTerminalDrain.updates[1].type == "turn.updated" &&
                              orderedTerminalDrain.updates[1].data["turn"]["terminal"] == true,
                          "final item content precedes terminal turn state even when turn.started dirtied the key first");

        frontend::EventCoalescer bounded({1});
        result.expectTrue(
            bounded
                .mark(
                    {frontend::CoalescingKey::thread("one"), "thread.updated", frontend::Json::object(), frontend::FlushUrgency::Deferred})
                .accepted(),
            "bounded coalescer accepts its first dirty entity");
        const auto overflow = bounded.mark(
            {frontend::CoalescingKey::thread("two"), "thread.updated", frontend::Json::object(), frontend::FlushUrgency::Deferred});
        result.expectTrue(overflow.status == frontend::CoalescerMarkStatus::SnapshotRequired && bounded.dirtyCount() == 1,
                          "dirty-entity capacity is bounded and degrades to a snapshot instead of growing");
    }

    void testAdapterHandshakeRolesReplayAndIsolation(tests::support::TestResult& result) {
        const auto transport = std::make_shared<tests::codex::FakeTransportState>();
        ManualScheduler scheduler;
        backend::BackendCoreOptions backendOptions;
        backendOptions.scheduler = [&scheduler](std::function<void()> callback) {
            scheduler.schedule(std::move(callback));
        };
        backendOptions.maxEventsPerCallback = 128;
        FakeBackendCore core(backendOptions, transport);

        frontend::BackendAdapterOptions adapterOptions;
        adapterOptions.scheduler = [&scheduler](std::function<void()> callback) {
            scheduler.schedule(std::move(callback));
        };
        adapterOptions.journal = {8, 128U * 1024U, frontend::SequenceNumber{0}};
        adapterOptions.batches = {8, 32U * 1024U};
        adapterOptions.maxOutboundMessagesPerConnection = 64;
        adapterOptions.maxOutboundBytesPerConnection = 512U * 1024U;
        frontend::BackendAdapter adapter(core, adapterOptions);

        Observations observerA;
        Observations observerB;
        frontend::FrontendConnection connectionA = adapter.openConnection(callbacksFor(observerA));
        frontend::FrontendConnection connectionB = adapter.openConnection(callbacksFor(observerB));
        result.expectTrue(connectionA.receive(hello()).accepted() && connectionB.receive(hello()).accepted() &&
                              observerA.messages.empty() && observerB.messages.empty(),
                          "hello output is asynchronous for every transport-neutral connection");
        scheduler.drain();

        result.expectTrue(welcome(observerA) && welcome(observerA)->role == frontend::SessionRole::Observer &&
                              welcome(observerA)->syncMode == frontend::SyncMode::Snapshot && countSnapshots(observerA) == 1 &&
                              welcome(observerB) && welcome(observerB)->role == frontend::SessionRole::Observer,
                          "hello creates backend sessions as observers and completes initial snapshot synchronization");
        result.expectTrue(connectionA.helloComplete() && connectionB.helloComplete() && connectionA.sessionId() != connectionB.sessionId(),
                          "successful hello exposes stable distinct backend session IDs");

        const std::size_t messagesBeforeCommands = observerA.messages.size();
        result.expectTrue(connectionA.receive(command("acquire-a", frontend::ControllerAcquire{})).accepted(),
                          "observer A submits explicit controller acquisition");
        result.expectTrue(!connectionA.receive(command("acquire-a", frontend::ControllerAcquire{})).accepted(),
                          "duplicate still-pending requestId is rejected locally");
        result.expectTrue(connectionB.receive(command("observer-mutate", frontend::ThreadStart{})).accepted(),
                          "observer mutation is accepted for one correlated permission response");
        scheduler.drain();
        result.expectTrue(hasSuccessfulResponse(observerA, "acquire-a"),
                          "controller acquisition completes successfully without duplicate backend completion");
        std::size_t duplicateResponses = 0;
        for (const frontend::ServerMessage& message : observerA.messages) {
            if (const auto* value = std::get_if<frontend::Response>(&message);
                value && value->requestId == "acquire-a" && !value->ok && value->error &&
                value->error->code == frontend::ErrorCode::DuplicateRequestId) {
                ++duplicateResponses;
            }
        }
        result.expectTrue(duplicateResponses == 1 && responseHasError(observerB, "observer-mutate", frontend::ErrorCode::PermissionDenied),
                          "duplicate correlation and observer permission errors use stable isolated responses");
        result.expectTrue(observerA.messages.size() > messagesBeforeCommands,
                          "controller transition produces frontend-visible normalized output");

        const std::string secretSentinel = "frontend-auth-secret-must-not-leak";
        result.expectTrue(
            connectionA.receive(command("auth-secret", frontend::AuthenticationRespond{"999", secretSentinel, "account", "plus"}))
                .accepted(),
            "controller submits an authentication response without exposing its secret in the server contract");
        scheduler.drain();
        const bool secretLeaked =
            std::any_of(observerA.compactJson.begin(), observerA.compactJson.end(), [&secretSentinel](const auto& json) {
                return json.find(secretSentinel) != std::string::npos;
            });
        result.expectTrue(responseHasError(observerA, "auth-secret", frontend::ErrorCode::NotFound) && !secretLeaked,
                          "server output never serializes an authentication access token from a frontend command");

        const std::vector<frontend::FrontendEvent> eventsA = events(observerA);
        const std::vector<frontend::FrontendEvent> eventsB = events(observerB);
        bool ordered = true;
        for (std::size_t index = 1; index < eventsA.size(); ++index) {
            ordered = ordered && eventsA[index - 1].sequence < eventsA[index].sequence;
        }
        result.expectTrue(!eventsA.empty() && !eventsB.empty() && ordered,
                          "multiple observers receive normalized frontend batches in strict sequence order");

        const frontend::SequenceNumber resumePosition = adapter.currentSequence();
        connectionA.close("controller A disconnected");
        scheduler.drain();
        result.expectTrue(connectionB.isOpen(), "controller disconnect leaves another observer and BackendCore running");

        Observations replayed;
        frontend::FrontendConnection replayConnection = adapter.openConnection(callbacksFor(replayed));
        result.expectTrue(replayConnection.receive(hello(resumePosition)).accepted(),
                          "a reconnect may request replay after its last sequence");
        scheduler.drain();
        result.expectTrue(welcome(replayed) && welcome(replayed)->syncMode == frontend::SyncMode::Replay && countSnapshots(replayed) == 0 &&
                              !events(replayed).empty(),
                          "retained normalized frontend events replay without a redundant snapshot");

        // Generate more separately flushed controller transitions than the
        // configured journal can retain, then reconnect from sequence zero.
        result.expectTrue(connectionB.receive(command("acquire-b", frontend::ControllerAcquire{})).accepted(),
                          "observer B acquires controller after A disconnects");
        scheduler.drain();
        for (std::size_t index = 0; index < 6; ++index) {
            result.expectTrue(connectionB.receive(command("release-" + std::to_string(index), frontend::ControllerRelease{})).accepted(),
                              "controller release command is accepted during eviction setup");
            scheduler.drain();
            result.expectTrue(connectionB.receive(command("acquire-" + std::to_string(index), frontend::ControllerAcquire{})).accepted(),
                              "controller reacquisition command is accepted during eviction setup");
            scheduler.drain();
        }

        Observations snapshotFallback;
        frontend::FrontendConnection oldReconnect = adapter.openConnection(callbacksFor(snapshotFallback));
        result.expectTrue(oldReconnect.receive(hello(frontend::SequenceNumber{0})).accepted(),
                          "an old reconnect position is accepted for synchronization planning");
        scheduler.drain();
        result.expectTrue(welcome(snapshotFallback) && welcome(snapshotFallback)->syncMode == frontend::SyncMode::Snapshot &&
                              countSnapshots(snapshotFallback) == 1,
                          "journal eviction deterministically falls back to one complete snapshot");

        Observations badClient;
        frontend::FrontendConnection beforeHello = adapter.openConnection(callbacksFor(badClient));
        result.expectTrue(beforeHello.receive(command("too-early", frontend::SnapshotGet{})).status ==
                              frontend::ConnectionReceiveStatus::Closing,
                          "a command before hello is rejected and closes only that frontend after its error");
        result.expectTrue(beforeHello.receive(hello()).status == frontend::ConnectionReceiveStatus::Closing,
                          "a connection pending protocol-error close rejects later coalesced input before opening a backend session");
        scheduler.drain();
        result.expectTrue(!beforeHello.isOpen() && connectionB.isOpen(), "pre-hello protocol failure is isolated from healthy clients");

        Observations slow;
        frontend::FrontendConnection slowObserver = adapter.openConnection({[&slow](const frontend::OutboundMessage& message) {
                                                                                slow.messages.push_back(message.message);
                                                                                return false;
                                                                            },
                                                                            [&slow](const std::string& reason) {
                                                                                slow.closeReasons.push_back(reason);
                                                                            }});
        result.expectTrue(slowObserver.receive(hello()).accepted(), "slow observer starts handshake independently");
        scheduler.drain();
        result.expectTrue(!slowObserver.isOpen() && connectionB.isOpen() && connectionB.queuedMessages() == 0 &&
                              slowObserver.queuedMessages() == 0,
                          "a backpressured observer is disconnected and releases all queued data without growing the controller queue");

        Observations throwing;
        frontend::FrontendConnection throwingObserver =
            adapter.openConnection({[&throwing](const frontend::OutboundMessage&) -> bool {
                                        throwing.messages.emplace_back(frontend::ProtocolErrorMessage{});
                                        throw std::runtime_error("intentional frontend transport failure");
                                    },
                                    [&throwing](const std::string& reason) {
                                        throwing.closeReasons.push_back(reason);
                                    }});
        result.expectTrue(throwingObserver.receive(hello()).accepted(), "throwing observer starts handshake independently");
        scheduler.drain();
        result.expectTrue(!throwingObserver.isOpen() && connectionB.isOpen(),
                          "a throwing outbound callback is exception-bounded and does not suppress another observer");

        Observations selfClosing;
        std::optional<frontend::FrontendConnection> selfClosingConnection;
        selfClosingConnection.emplace(adapter.openConnection({[&selfClosingConnection](const frontend::OutboundMessage&) {
                                                                  selfClosingConnection->close("closed during delivery");
                                                                  return true;
                                                              },
                                                              [&selfClosing](const std::string& reason) {
                                                                  selfClosing.closeReasons.push_back(reason);
                                                              }}));
        result.expectTrue(selfClosingConnection->receive(hello()).accepted(), "frontend may request close during outbound delivery");
        scheduler.drain();
        result.expectTrue(!selfClosingConnection->isOpen() && connectionB.isOpen(),
                          "close during callback delivery is generation/lifetime safe and peer-isolated");

        adapter.close("adapter test complete");
        scheduler.drain();
        result.expectTrue(!adapter.isOpen() && !connectionB.isOpen() && !replayConnection.isOpen() && !oldReconnect.isOpen(),
                          "adapter shutdown detaches every frontend without destroying BackendCore");
    }

    void testSnapshotReplayBarrier(tests::support::TestResult& result) {
        const auto transport = std::make_shared<tests::codex::FakeTransportState>();
        ManualScheduler scheduler;
        backend::BackendCoreOptions backendOptions;
        backendOptions.scheduler = [&scheduler](std::function<void()> callback) {
            scheduler.schedule(std::move(callback));
        };
        FakeBackendCore core(backendOptions, transport);

        frontend::BackendAdapterOptions adapterOptions;
        adapterOptions.scheduler = [&scheduler](std::function<void()> callback) {
            scheduler.schedule(std::move(callback));
        };
        adapterOptions.coalescer = {1};
        frontend::BackendAdapter adapter(core, adapterOptions);

        Observations initial;
        frontend::FrontendConnection initialConnection = adapter.openConnection(callbacksFor(initial));
        result.expectTrue(initialConnection.receive(hello()).accepted(), "replay-barrier client completes an initial hello");
        scheduler.drain();

        const frontend::SequenceNumber unchangedSequence = adapter.currentSequence();
        result.expectTrue(initialConnection.receive(command("unchanged-snapshot", frontend::SnapshotGet{})).accepted(),
                          "an explicit unchanged-state snapshot request is accepted");
        scheduler.drain();
        result.expectTrue(adapter.currentSequence() == unchangedSequence && hasSuccessfulResponse(initial, "unchanged-snapshot"),
                          "an explicit snapshot does not advance or invalidate replay continuity");

        Observations unchangedReplay;
        frontend::FrontendConnection unchangedReplayConnection = adapter.openConnection(callbacksFor(unchangedReplay));
        result.expectTrue(unchangedReplayConnection.receive(hello(unchangedSequence)).accepted(),
                          "an unchanged-state reconnect requests replay at the current sequence");
        scheduler.drain();
        result.expectTrue(welcome(unchangedReplay) && welcome(unchangedReplay)->syncMode == frontend::SyncMode::Replay &&
                              countSnapshots(unchangedReplay) == 0,
                          "unchanged state remains replayable without a redundant snapshot");

        const frontend::SequenceNumber beforeFallback = adapter.currentSequence();
        backend::FrontendSession dirtySessionA = core.openSession({});
        backend::FrontendSession dirtySessionB = core.openSession({});
        scheduler.drain();
        result.expectTrue(adapter.currentSequence() > beforeFallback,
                          "dirty-entity overflow advances the frontend synchronization barrier monotonically");

        Observations staleReconnect;
        frontend::FrontendConnection staleConnection = adapter.openConnection(callbacksFor(staleReconnect));
        result.expectTrue(staleConnection.receive(hello(beforeFallback)).accepted(),
                          "a stale client may reconnect from the sequence immediately before snapshot fallback");
        scheduler.drain();
        result.expectTrue(welcome(staleReconnect) && welcome(staleReconnect)->syncMode == frontend::SyncMode::Snapshot &&
                              countSnapshots(staleReconnect) == 1,
                          "snapshot fallback establishes a replay gap even when no normalized event was journaled");

        staleConnection.close();
        unchangedReplayConnection.close();
        initialConnection.close();
        dirtySessionB.close();
        dirtySessionA.close();
        adapter.close("replay barrier test complete");
        scheduler.drain();
    }

    class FrontendBurstRunner {
    public:
        explicit FrontendBurstRunner(tests::support::TestResult& result)
            : result(result) {
        }

        void start() {
            transport = std::make_shared<tests::codex::FakeTransportState>();
            tests::codex::installInitializingFake(transport, [this](const Json& message, const TransportCallbacks& callbacks) {
                handleOutgoing(message, callbacks);
            });

            backend::BackendCoreOptions backendOptions;
            backendOptions.initialThreadListLimit = 1;
            backendOptions.maxObserverQueueEntries = 2'048;
            backendOptions.maxObserverQueueBytes = 32U * 1024U * 1024U;
            backendOptions.maxEventsPerCallback = 2'048;
            backendCore = std::make_unique<FakeBackendCore>(std::move(backendOptions), transport);

            frontend::BackendAdapterOptions adapterOptions;
            adapterOptions.journal = {128, 2U * 1024U * 1024U, frontend::SequenceNumber{0}};
            adapterOptions.batches = {8, 128U * 1024U};
            adapterOptions.coalescer = {64};
            adapterOptions.maxOutboundMessagesPerConnection = maxOutboundMessages;
            adapterOptions.maxOutboundBytesPerConnection = maxOutboundBytes;
            adapterOptions.maxMessagesPerDelivery = 4;
            adapter = std::make_unique<frontend::BackendAdapter>(*backendCore, std::move(adapterOptions));

            connectionA.emplace(adapter->openConnection(callbacksFor(observerA)));
            connectionB.emplace(adapter->openConnection(callbacksFor(observerB)));
            expect(connectionA->receive(hello()).accepted() && connectionB->receive(hello()).accepted(),
                   "both protocol sessions accept hello before the high-volume stream");

            backendCore->start();
            waitUntil(
                "frontend burst backend reaches Ready and completes bounded hydration",
                [this]() {
                    const backend::Snapshot snapshot = backendCore->snapshot();
                    return backendCore->isReady() && snapshot.threadList.pagesLoaded == 1 && connectionA->helloComplete() &&
                           connectionB->helloComplete();
                },
                [this]() {
                    afterTicks(8, [this]() {
                        emitExtensions();
                    });
                });
        }

        bool isFinished() const noexcept {
            return finished;
        }

    private:
        static void defer(std::function<void()> callback) {
            core::EventReceiver::atNextTick(std::move(callback));
        }

        void expect(bool condition, const std::string& message) {
            result.expectTrue(condition, message);
        }

        void
        waitUntil(std::string description, std::function<bool()> predicate, std::function<void()> next, std::size_t remaining = 8'000) {
            defer([this,
                   description = std::move(description),
                   predicate = std::move(predicate),
                   next = std::move(next),
                   remaining]() mutable {
                if (finished) {
                    return;
                }
                if (predicate()) {
                    next();
                    return;
                }
                if (remaining == 0) {
                    expect(false, description);
                    finish();
                    return;
                }
                waitUntil(std::move(description), std::move(predicate), std::move(next), remaining - 1);
            });
        }

        void afterTicks(std::size_t count, std::function<void()> next) {
            if (count == 0) {
                next();
                return;
            }
            defer([this, count, next = std::move(next)]() mutable {
                afterTicks(count - 1, std::move(next));
            });
        }

        void handleOutgoing(const Json& message, const TransportCallbacks& callbacks) {
            const auto method = message.find("method");
            const auto id = message.find("id");
            if (method == message.end() || !method->is_string() || id == message.end()) {
                return;
            }
            if (*method == "thread/list") {
                tests::codex::inject(
                    callbacks,
                    Json{{"id", *id}, {"result", {{"data", Json::array()}, {"nextCursor", nullptr}, {"backwardsCursor", nullptr}}}});
            }
        }

        void emitBurst() {
            baselineMessagesA = observerA.messages.size();
            baselineMessagesB = observerB.messages.size();
            baselineEventsA = events(observerA).size();
            baselineEventsB = events(observerB).size();
            baselineSequence = adapter->currentSequence();

            transport->inject(
                {{"method", "turn/started"}, {"params", {{"threadId", threadId}, {"turn", tests::codex::turnValue(threadId, turnId)}}}});
            transport->inject(
                {{"method", "item/started"},
                 {"params", {{"threadId", threadId}, {"turnId", turnId}, {"item", agentItemValue(itemId)}, {"startedAtMs", 10}}}});

            expectedText.clear();
            expectedText.reserve(deltaCount);
            for (std::size_t index = 0; index < deltaCount; ++index) {
                const std::string delta(1, static_cast<char>('a' + static_cast<int>(index % 26)));
                expectedText += delta;
                transport->inject({{"method", "item/agentMessage/delta"},
                                   {"params", {{"threadId", threadId}, {"turnId", turnId}, {"itemId", itemId}, {"delta", delta}}}});
            }
            transport->inject(
                {{"method", "item/completed"},
                 {"params",
                  {{"threadId", threadId}, {"turnId", turnId}, {"item", agentItemValue(itemId, expectedText)}, {"completedAtMs", 20}}}});
            transport->inject(
                {{"method", "item/started"},
                 {"params",
                  {{"threadId", threadId},
                   {"turnId", turnId},
                   {"item", {{"id", userMessageItemId}, {"type", "userMessage"}, {"clientId", nullptr}, {"content", userMessageContent}}},
                   {"startedAtMs", userMessageStartedAtMs}}}});
            transport->inject(
                {{"method", "item/completed"},
                 {"params",
                  {{"threadId", threadId},
                   {"turnId", turnId},
                   {"item", {{"id", userMessageItemId}, {"type", "userMessage"}, {"clientId", nullptr}, {"content", userMessageContent}}},
                   {"completedAtMs", userMessageCompletedAtMs}}}});
            transport->inject({{"method", "item/started"},
                               {"params",
                                {{"threadId", threadId},
                                 {"turnId", turnId},
                                 {"item", {{"id", unknownItemId}, {"type", unknownItemType}, {"futureField", Json{{"value", 7}}}}},
                                 {"startedAtMs", unknownItemStartedAtMs}}}});
            transport->inject({{"method", "item/completed"},
                               {"params",
                                {{"threadId", threadId},
                                 {"turnId", turnId},
                                 {"item", {{"id", unknownItemId}, {"type", unknownItemType}, {"futureField", Json{{"value", 7}}}}},
                                 {"completedAtMs", unknownItemCompletedAtMs}}}});
            transport->inject({{"method", "turn/completed"},
                               {"params", {{"threadId", threadId}, {"turn", tests::codex::turnValue(threadId, turnId, "completed")}}}});

            waitUntil(
                "1,000 typed deltas reach exact terminal frontend state",
                [this]() {
                    return terminalBackendText() == expectedText && latestFrontendText(observerA) == expectedText &&
                           latestFrontendText(observerB) == expectedText &&
                           hasCompletedItemUpdate(
                               observerA, userMessageItemId, "user_message", userMessageStartedAtMs, userMessageCompletedAtMs) &&
                           hasCompletedItemUpdate(
                               observerA, unknownItemId, unknownItemType, unknownItemStartedAtMs, unknownItemCompletedAtMs) &&
                           hasCompletedItemUpdate(
                               observerB, userMessageItemId, "user_message", userMessageStartedAtMs, userMessageCompletedAtMs) &&
                           hasCompletedItemUpdate(
                               observerB, unknownItemId, unknownItemType, unknownItemStartedAtMs, unknownItemCompletedAtMs);
                },
                [this]() {
                    verifyBurst();
                });
        }

        void emitExtensions() {
            transport->inject(
                {{"method", "future/safe-extension"},
                 {"params",
                  {{"safe", "visible"},
                   {"accessToken", extensionAccessToken},
                   {"Client_Secret", extensionAccessToken},
                   {"nested", {{"secret", true}, {"text", extensionSecretAnswer}, {"answers", Json::array({extensionSecretAnswer})}}}}}});
            oversizedExtensionSecret.assign(backend::MaxSnapshotExtensionPayloadBytes * 3, 'x');
            transport->inject({{"method", "future/oversized-extension"},
                               {"params", {{"authorization", oversizedExtensionSecret}, {"padding", oversizedExtensionSecret}}}});

            waitUntil(
                "both sanitized extension events reach the frontend",
                [this]() {
                    const std::vector<frontend::FrontendEvent> received = events(observerA);
                    const bool hasSafe = std::any_of(received.begin(), received.end(), [](const frontend::FrontendEvent& event) {
                        return event.type == "codex.extension" && event.data.value("method", "") == "future/safe-extension";
                    });
                    const bool hasOversized = std::any_of(received.begin(), received.end(), [](const frontend::FrontendEvent& event) {
                        return event.type == "codex.extension" && event.data.value("method", "") == "future/oversized-extension";
                    });
                    return hasSafe && hasOversized && backendCore->snapshot().recentExtensions.size() >= 2;
                },
                [this]() {
                    expect(connectionA->receive(command("extension-snapshot", frontend::SnapshotGet{})).accepted(),
                           "frontend requests a fresh snapshot after unknown extensions");
                    waitUntil(
                        "sanitized extension history reaches an explicit frontend snapshot",
                        [this]() {
                            const frontend::Snapshot* snapshot = latestSnapshot(observerA);
                            return snapshot && snapshot->state.contains("codexExtensions") &&
                                   snapshot->state["codexExtensions"].is_array() && snapshot->state["codexExtensions"].size() >= 2 &&
                                   hasSuccessfulResponse(observerA, "extension-snapshot");
                        },
                        [this]() {
                            verifyExtensions();
                            afterTicks(8, [this]() {
                                emitBurst();
                            });
                        });
                });
        }

        void verifyExtensions() {
            const std::vector<frontend::FrontendEvent> received = events(observerA);
            const auto safeEvent = std::find_if(received.rbegin(), received.rend(), [](const frontend::FrontendEvent& event) {
                return event.type == "codex.extension" && event.data.value("method", "") == "future/safe-extension";
            });
            const auto oversizedEvent = std::find_if(received.rbegin(), received.rend(), [](const frontend::FrontendEvent& event) {
                return event.type == "codex.extension" && event.data.value("method", "") == "future/oversized-extension";
            });
            expect(safeEvent != received.rend() && safeEvent->data.at("params").at("safe") == "visible" &&
                       safeEvent->data.value("sensitiveFieldsRedacted", false),
                   "codex.extension preserves safe unknown fields and explicitly reports recursive redaction");
            expect(oversizedEvent != received.rend() && oversizedEvent->data.at("params").value("omitted", false) &&
                       oversizedEvent->data.contains("truncation"),
                   "an oversized unknown event remains observable as one explicit bounded codex.extension record");

            const frontend::Snapshot* snapshot = latestSnapshot(observerA);
            expect(snapshot && snapshot->state["codexExtensions"].back().value("method", "") == "future/oversized-extension" &&
                       snapshot->state["codexExtensions"].back()["params"].value("omitted", false),
                   "snapshot fallback includes the reducer-retained sanitized extension history");

            const bool secretLeaked =
                std::any_of(observerA.compactJson.begin(), observerA.compactJson.end(), [this](const std::string& compact) {
                    return compact.find(extensionAccessToken) != std::string::npos ||
                           compact.find(extensionSecretAnswer) != std::string::npos ||
                           compact.find(oversizedExtensionSecret) != std::string::npos;
                });
            const bool extensionMessagesBounded =
                std::all_of(observerA.messages.begin(), observerA.messages.end(), [](const frontend::ServerMessage& message) {
                    if (const auto* batch = std::get_if<frontend::EventBatch>(&message)) {
                        const bool hasExtension =
                            std::any_of(batch->events.begin(), batch->events.end(), [](const frontend::FrontendEvent& event) {
                                return event.type == "codex.extension";
                            });
                        if (hasExtension) {
                            const auto encoded = frontend::Codec::serializeServer(message);
                            return encoded && encoded.value().size() <= backend::MaxSerializedCodexExtensionEventBytes;
                        }
                    }
                    return true;
                });
            expect(!secretLeaked && extensionMessagesBounded,
                   "compact extension snapshots/events contain no access tokens or secret answers and stay within the 64 KiB event bound");
        }

        std::string terminalBackendText() const {
            const backend::Snapshot snapshot = backendCore->snapshot();
            for (const backend::ThreadSnapshot& thread : snapshot.threads) {
                if (thread.id != threadId) {
                    continue;
                }
                for (const backend::TurnSnapshot& turn : thread.turns) {
                    if (turn.id != turnId || !turn.terminal) {
                        continue;
                    }
                    for (const backend::ItemSnapshot& item : turn.items) {
                        if (item.id == itemId) {
                            return item.agentText;
                        }
                    }
                }
            }
            return {};
        }

        std::string latestFrontendText(const Observations& observations) const {
            std::string latest;
            const std::vector<frontend::FrontendEvent> received = events(observations);
            for (const frontend::FrontendEvent& event : received) {
                if (event.type == "item.content.updated" && event.data.value("itemId", "") == itemId &&
                    event.data.value("channel", "") == "agentText") {
                    latest = event.data.value("content", "");
                }
            }
            return latest;
        }

        bool isCompletedItemUpdate(const frontend::FrontendEvent& event,
                                   const std::string& expectedItemId,
                                   const std::string& expectedType,
                                   std::int64_t expectedStartedAtMs,
                                   std::int64_t expectedCompletedAtMs) const {
            if (event.type != "item.updated" || event.data.value("threadId", "") != threadId || event.data.value("turnId", "") != turnId) {
                return false;
            }
            const auto item = event.data.find("item");
            return item != event.data.end() && item->is_object() && item->value("id", "") == expectedItemId &&
                   item->value("type", "") == expectedType && item->value("status", "") == "completed" &&
                   item->value("startedAtMs", std::int64_t{-1}) == expectedStartedAtMs &&
                   item->value("completedAtMs", std::int64_t{-1}) == expectedCompletedAtMs;
        }

        bool hasCompletedItemUpdate(const Observations& observations,
                                    const std::string& expectedItemId,
                                    const std::string& expectedType,
                                    std::int64_t expectedStartedAtMs,
                                    std::int64_t expectedCompletedAtMs) const {
            const std::vector<frontend::FrontendEvent> received = events(observations);
            return std::any_of(received.begin(), received.end(), [&](const frontend::FrontendEvent& event) {
                return isCompletedItemUpdate(event, expectedItemId, expectedType, expectedStartedAtMs, expectedCompletedAtMs);
            });
        }

        std::vector<frontend::FrontendEvent> burstEvents(const Observations& observations, std::size_t baseline) const {
            std::vector<frontend::FrontendEvent> received = events(observations);
            if (baseline >= received.size()) {
                return {};
            }
            return {received.begin() + static_cast<std::ptrdiff_t>(baseline), received.end()};
        }

        void verifyBurst() {
            const std::vector<frontend::FrontendEvent> receivedA = burstEvents(observerA, baselineEventsA);
            const std::vector<frontend::FrontendEvent> receivedB = burstEvents(observerB, baselineEventsB);
            const std::size_t frontendMessagesA = observerA.messages.size() - baselineMessagesA;
            const std::size_t frontendMessagesB = observerB.messages.size() - baselineMessagesB;

            expect(connectionA->isOpen() && connectionB->isOpen(), "both protocol sessions remain open after 1,000 fine-grained deltas");
            expect(connectionA->queuedMessages() <= maxOutboundMessages && connectionB->queuedMessages() <= maxOutboundMessages &&
                       connectionA->queuedBytes() <= maxOutboundBytes && connectionB->queuedBytes() <= maxOutboundBytes,
                   "per-connection queues remain within explicit message and byte bounds during the delta burst");
            expect(frontendMessagesA < deltaCount / 10 && frontendMessagesB < deltaCount / 10 && receivedA.size() < deltaCount / 10 &&
                       receivedB.size() < deltaCount / 10,
                   "1,000 App Server deltas produce substantially fewer than 1,000 frontend messages and updates");
            expect(receivedA == receivedB && !receivedA.empty(),
                   "controller-independent protocol sessions receive identical normalized burst updates");
            expect(std::is_sorted(receivedA.begin(),
                                  receivedA.end(),
                                  [](const frontend::FrontendEvent& left, const frontend::FrontendEvent& right) {
                                      return left.sequence < right.sequence;
                                  }) &&
                       adapter->currentSequence() > baselineSequence,
                   "coalesced burst updates retain strict frontend sequence order");
            std::optional<std::size_t> finalContentIndex;
            std::optional<std::size_t> terminalTurnIndex;
            for (std::size_t index = 0; index < receivedA.size(); ++index) {
                const frontend::FrontendEvent& event = receivedA[index];
                if (event.type == "item.content.updated" && event.data.value("itemId", "") == itemId &&
                    event.data.value("content", "") == expectedText) {
                    finalContentIndex = index;
                }
                const auto turn = event.data.find("turn");
                if (event.type == "turn.updated" && turn != event.data.end() && turn->is_object() && turn->value("id", "") == turnId &&
                    turn->value("terminal", false)) {
                    terminalTurnIndex = index;
                }
            }
            expect(finalContentIndex.has_value() && terminalTurnIndex.has_value() && *finalContentIndex < *terminalTurnIndex,
                   "adapter emits final item.content.updated before terminal turn.updated when turn.started was already dirty");
            expect(latestFrontendText(observerA) == expectedText && latestFrontendText(observerB) == expectedText,
                   "coalescing preserves exact final text for every protocol session");

            const auto countCompletedItemUpdates = [&](const std::string& expectedItemId,
                                                       const std::string& expectedType,
                                                       std::int64_t expectedStartedAtMs,
                                                       std::int64_t expectedCompletedAtMs) {
                return std::count_if(receivedA.begin(), receivedA.end(), [&](const frontend::FrontendEvent& event) {
                    return isCompletedItemUpdate(event, expectedItemId, expectedType, expectedStartedAtMs, expectedCompletedAtMs);
                });
            };
            expect(countCompletedItemUpdates(userMessageItemId, "user_message", userMessageStartedAtMs, userMessageCompletedAtMs) == 1,
                   "userMessage start/completion coalesce into one canonical completed frontend item update");
            expect(countCompletedItemUpdates(unknownItemId, unknownItemType, unknownItemStartedAtMs, unknownItemCompletedAtMs) == 1,
                   "an unknown item with common metadata coalesces into one canonical completed frontend item update");
            const auto userMessageUpdate = std::find_if(receivedA.begin(), receivedA.end(), [&](const frontend::FrontendEvent& event) {
                return isCompletedItemUpdate(event, userMessageItemId, "user_message", userMessageStartedAtMs, userMessageCompletedAtMs);
            });
            const Json userMessageItem =
                userMessageUpdate == receivedA.end() ? Json::object() : userMessageUpdate->data.value("item", Json::object());
            const Json userMessageData = userMessageItem.is_object() ? userMessageItem.value("data", Json::object()) : Json::object();
            const Json retainedUserMessageContent = userMessageData.value("content", Json::object());
            bool userMessagePrefixPreserved = retainedUserMessageContent.is_array() && !retainedUserMessageContent.empty() &&
                                              retainedUserMessageContent.size() < userMessageContent.size();
            if (retainedUserMessageContent.is_array()) {
                for (std::size_t index = 0; index < retainedUserMessageContent.size(); ++index) {
                    userMessagePrefixPreserved = userMessagePrefixPreserved &&
                                                 retainedUserMessageContent[index] == userMessageContent[index] &&
                                                 retainedUserMessageContent[index].dump() == userMessageContent[index].dump();
                }
            }
            const bool userMessageDataPreserved =
                userMessageData.is_object() && userMessageData.contains("clientId") && userMessageData.at("clientId").is_null() &&
                userMessageContent.dump().size() > backend::MaxSerializedUserMessageDataBytes && userMessagePrefixPreserved &&
                userMessageData.value("contentTruncated", false) &&
                userMessageData.value("originalContentBytes", std::uint64_t{0}) == userMessageContent.dump().size() &&
                userMessageData.value("retainedContentBytes", std::uint64_t{0}) == retainedUserMessageContent.dump().size() &&
                userMessageData.value("originalContentItems", std::uint64_t{0}) == userMessageContent.size() &&
                userMessageData.value("retainedContentItems", std::uint64_t{0}) == retainedUserMessageContent.size() &&
                userMessageData.dump().size() <= backend::MaxSerializedUserMessageDataBytes &&
                !userMessageItem.value("contentTruncated", true) && userMessageItem.value("droppedContentBytes", std::uint64_t{1}) == 0;
            expect(userMessageDataPreserved,
                   "Decoder through BackendAdapter preserves a bounded array prefix and independent user-message truncation metadata");
            const auto unknownUpdate = std::find_if(receivedA.begin(), receivedA.end(), [&](const frontend::FrontendEvent& event) {
                return isCompletedItemUpdate(event, unknownItemId, unknownItemType, unknownItemStartedAtMs, unknownItemCompletedAtMs);
            });
            const Json unknownData =
                unknownUpdate == receivedA.end() ? Json::object() : unknownUpdate->data.at("item").value("data", Json::object());
            expect(unknownData.is_object() && unknownData.value("codexType", "") == unknownItemType,
                   "the canonical frontend unknown item preserves its future discriminator");

            const bool hasItemLifecycleExtension =
                std::any_of(receivedA.begin(), receivedA.end(), [](const frontend::FrontendEvent& event) {
                    if (event.type != "codex.extension") {
                        return false;
                    }
                    const std::string method = event.data.value("method", "");
                    const std::string decodingError = event.data.value("decodingError", "");
                    return method == "item/started" || method == "item/completed" ||
                           decodingError.find("item event omitted threadId or turnId") != std::string::npos;
                });
            expect(!hasItemLifecycleExtension,
                   "valid userMessage and unknown item lifecycle events never become location-error codex.extension events");

            adapter->close("burst test complete");
            backendCore->stop();
            waitUntil(
                "burst BackendCore reaches Stopped after adapter isolation checks",
                [this]() {
                    return backendCore->snapshot().lifecycle == backend::BackendLifecycle::Stopped;
                },
                [this]() {
                    finish();
                });
        }

        void finish() {
            if (finished) {
                return;
            }
            finished = true;
            if (adapter) {
                adapter->close("frontend burst runner finished");
            }
            if (backendCore) {
                backendCore->stop();
            }
            connectionA.reset();
            connectionB.reset();
            adapter.reset();
            backendCore.reset();
            core::SNodeC::stop();
        }

        static constexpr std::size_t deltaCount = 1'000;
        static constexpr std::size_t maxOutboundMessages = 32;
        static constexpr std::size_t maxOutboundBytes = 1024U * 1024U;
        const std::string threadId = "thread-burst";
        const std::string turnId = "turn-burst";
        const std::string itemId = "item-burst";
        const std::string userMessageItemId = "user-message-burst";
        const Json userMessageContent = []() {
            Json content =
                Json::array({Json{{"type", "futureContent"},
                                  {"payload", Json{{"preserved", true}, {"nested", Json::array({1, Json{{"future", "value"}}})}}}}});
            for (std::size_t index = 0; index < 8; ++index) {
                content.push_back(Json{{"type", "futureLargeContent"},
                                       {"index", index},
                                       {"payload", std::string(12U * 1024U, static_cast<char>('a' + index))},
                                       {"future", Json{{"nested", index}, {"preserved", true}}}});
            }
            return content;
        }();
        static constexpr std::int64_t userMessageStartedAtMs = 30;
        static constexpr std::int64_t userMessageCompletedAtMs = 40;
        const std::string unknownItemId = "unknown-item-burst";
        const std::string unknownItemType = "futureAdapterItem";
        static constexpr std::int64_t unknownItemStartedAtMs = 50;
        static constexpr std::int64_t unknownItemCompletedAtMs = 60;

        tests::support::TestResult& result;
        std::shared_ptr<tests::codex::FakeTransportState> transport;
        std::unique_ptr<FakeBackendCore> backendCore;
        std::unique_ptr<frontend::BackendAdapter> adapter;
        std::optional<frontend::FrontendConnection> connectionA;
        std::optional<frontend::FrontendConnection> connectionB;
        Observations observerA;
        Observations observerB;
        std::string expectedText;
        const std::string extensionAccessToken = "wire-extension-access-token-must-not-leak";
        const std::string extensionSecretAnswer = "wire-extension-secret-answer-must-not-leak";
        std::string oversizedExtensionSecret;
        frontend::SequenceNumber baselineSequence;
        std::size_t baselineMessagesA = 0;
        std::size_t baselineMessagesB = 0;
        std::size_t baselineEventsA = 0;
        std::size_t baselineEventsB = 0;
        bool finished = false;
    };

} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult result;
    int returnCode = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("CodexFrontendAdapterTest");
    } else {
        core::SNodeC::init(argc, argv);
        testCoalescingAndBounds(result);
        testAdapterHandshakeRolesReplayAndIsolation(result);
        testSnapshotReplayBarrier(result);
        bool timedOut = false;
        FrontendBurstRunner runner(result);
        [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
            [&timedOut]() {
                timedOut = true;
                core::SNodeC::stop();
            },
            utils::Timeval({10, 0}));
        runner.start();
        const int eventLoopResult = core::SNodeC::start(utils::Timeval({12, 0}));
        result.expectTrue(!timedOut, "frontend 1,000-delta scenario completes before the watchdog");
        result.expectTrue(runner.isFinished(), "frontend 1,000-delta scenario reaches a clean terminal state");
        result.expectEqual(0, eventLoopResult, "frontend adapter event loop exits cleanly");

        core::SNodeC::free();
        returnCode = result.processResult();
    }
    return returnCode;
}
