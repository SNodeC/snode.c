/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/backend/Reducer.h"

#include "ai/openai/codex/backend/detail/PreserveUnmodeledTypedEvent.h"
#include "ai/openai/codex/detail/ConversationCodec.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "log/LogScopeOwner.h"
#include "log/Logger.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace ai::openai::codex::backend {

    namespace {
        template <typename... Visitors>
        struct Overloaded : Visitors... {
            using Visitors::operator()...;
        };

        template <typename... Visitors>
        Overloaded(Visitors...) -> Overloaded<Visitors...>;

        template <typename Unsigned>
        std::uint64_t saturatingUint64(Unsigned value) noexcept {
            static_assert(std::is_unsigned_v<Unsigned>);
            if constexpr (std::numeric_limits<Unsigned>::digits > std::numeric_limits<std::uint64_t>::digits) {
                if (value > static_cast<Unsigned>(std::numeric_limits<std::uint64_t>::max())) {
                    return std::numeric_limits<std::uint64_t>::max();
                }
            }
            return static_cast<std::uint64_t>(value);
        }

        using ServerNotificationTarget = ::ai::openai::codex::detail::ServerNotificationTarget;

        template <typename Notification>
        std::vector<BackendEvent> preserveTypedNotification(const Notification& value, ServerNotificationTarget target) {
            const ::ai::openai::codex::detail::ProtocolSurfaceEntry& registryEntry = ::ai::openai::codex::detail::entryFor(target);
            const std::optional<typed::DecodeDiagnostic> diagnostic =
                value.diagnostics.empty() ? std::nullopt : std::optional<typed::DecodeDiagnostic>{value.diagnostics.front()};
            return {detail::preserveUnmodeledTypedEvent(
                {std::string(registryEntry.key.name), value.raw.at("params"), std::nullopt, diagnostic})};
        }

        logger::BoundaryLogger lifecycleLog() {
            static const logger::LogScopeOwner scope(
                logger::LogOrigin::Framework, logger::LogBoundary::Connection, "ai.openai.codex.backend");
            return scope.logger(logger::Logger::semanticSink());
        }

        typed::Thread placeholderThread(const typed::ThreadId& id) {
            typed::Thread thread;
            thread.id = id;
            thread.raw = Json::object({{"backendPlaceholder", true}});
            return thread;
        }

        typed::Turn placeholderTurn(const typed::ThreadId& threadId, const typed::TurnId& turnId) {
            typed::Turn turn;
            turn.id = turnId;
            turn.threadId = threadId;
            turn.status = typed::TurnStatus::inProgress();
            turn.itemsView = typed::TurnItemsView::notLoaded();
            turn.raw = Json::object({{"backendPlaceholder", true}});
            return turn;
        }

        ThreadState& ensureThread(BackendState& state, const typed::ThreadId& id) {
            ThreadState initial;
            initial.thread = placeholderThread(id);
            auto [iterator, inserted] = state.threads.try_emplace(id.value, std::move(initial));
            if (inserted) {
                state.threadOrder.push_back(id);
            }
            return iterator->second;
        }

        TurnState& ensureTurn(BackendState& state, const typed::ThreadId& threadId, const typed::TurnId& turnId) {
            ThreadState& thread = ensureThread(state, threadId);
            TurnState initial;
            initial.turn = placeholderTurn(threadId, turnId);
            auto [iterator, inserted] = thread.turns.try_emplace(turnId.value, std::move(initial));
            if (inserted) {
                thread.turnOrder.push_back(turnId);
            }
            return iterator->second;
        }

        void assignBounded(std::string& target, const std::string& value, std::size_t limit, std::uint64_t& dropped) {
            if (limit == 0) {
                dropped += value.size();
                target.clear();
            } else if (value.size() > limit) {
                dropped += value.size() - limit;
                target.assign(value.end() - static_cast<std::ptrdiff_t>(limit), value.end());
            } else {
                target = value;
            }
        }

        void appendBounded(std::string& target, const std::string& value, std::size_t limit, std::uint64_t& dropped) {
            if (value.empty()) {
                return;
            }
            if (limit == 0) {
                dropped += target.size() + value.size();
                target.clear();
                return;
            }
            if (value.size() >= limit) {
                dropped += target.size() + value.size() - limit;
                target.assign(value.end() - static_cast<std::ptrdiff_t>(limit), value.end());
                return;
            }
            if (target.size() > limit - value.size()) {
                const std::size_t remove = target.size() - (limit - value.size());
                dropped += remove;
                target.erase(0, remove);
            }
            target += value;
        }

        std::optional<std::vector<typed::FileUpdateChange>> decodeFileUpdateChanges(const Json& changes) {
            if (!changes.is_array()) {
                return std::nullopt;
            }

            std::vector<typed::FileUpdateChange> decoded;
            decoded.reserve(changes.size());
            for (const Json& change : changes) {
                if (!change.is_object()) {
                    return std::nullopt;
                }
                const auto diff = change.find("diff");
                const auto kind = change.find("kind");
                const auto path = change.find("path");
                if (diff == change.end() || !diff->is_string() || kind == change.end() || path == change.end() || !path->is_string()) {
                    return std::nullopt;
                }
                ai::openai::codex::detail::ConversationDecodeResult<typed::PatchChangeKind> decodedKind =
                    ai::openai::codex::detail::decodePatchChangeKind(*kind);
                if (!decodedKind.value) {
                    return std::nullopt;
                }
                decoded.push_back({diff->get<std::string>(), std::move(*decodedKind.value), path->get<std::string>()});
            }
            return decoded;
        }

        void initializeVisibleContent(ItemState& state, bool authoritative, std::size_t limit) {
            std::visit(Overloaded{[&state, authoritative, limit](const typed::AgentMessageItem& item) {
                                      if ((!item.text.empty() && authoritative) || state.agentText.empty()) {
                                          assignBounded(state.agentText, item.text, limit, state.droppedContentBytes);
                                      }
                                  },
                                  [&state, authoritative, limit](const typed::ReasoningItem& item) {
                                      const std::vector<std::string>& content = item.contentOrDefault();
                                      const bool hasContent = std::any_of(content.begin(), content.end(), [](const std::string& value) {
                                          return !value.empty();
                                      });
                                      if ((authoritative && hasContent) || state.reasoningText.empty()) {
                                          state.reasoningText.clear();
                                          for (const std::string& value : content) {
                                              appendBounded(state.reasoningText, value, limit, state.droppedContentBytes);
                                          }
                                      }
                                      const std::vector<std::string>& summary = item.summaryOrDefault();
                                      const bool hasSummary = std::any_of(summary.begin(), summary.end(), [](const std::string& value) {
                                          return !value.empty();
                                      });
                                      if ((authoritative && hasSummary) || state.reasoningSummary.empty()) {
                                          state.reasoningSummary.clear();
                                          for (const std::string& value : summary) {
                                              appendBounded(state.reasoningSummary, value, limit, state.droppedContentBytes);
                                          }
                                      }
                                  },
                                  [&state, authoritative, limit](const typed::CommandExecutionItem& item) {
                                      if (item.aggregatedOutput &&
                                          ((!item.aggregatedOutput->empty() && authoritative) || state.commandOutput.empty())) {
                                          assignBounded(state.commandOutput, *item.aggregatedOutput, limit, state.droppedContentBytes);
                                      }
                                  },
                                  [](const auto&) {
                                  }},
                       state.item);
        }

        std::optional<std::pair<typed::ThreadId, typed::TurnId>> itemLocation(const typed::Item& item) {
            return std::visit(
                [](const auto& value) -> std::optional<std::pair<typed::ThreadId, typed::TurnId>> {
                    using Value = std::decay_t<decltype(value)>;
                    if constexpr (std::is_same_v<Value, typed::UnknownItem>) {
                        if (value.metadata.threadId && !value.metadata.threadId->value.empty() && value.metadata.turnId &&
                            !value.metadata.turnId->value.empty()) {
                            return std::pair{*value.metadata.threadId, *value.metadata.turnId};
                        }
                    } else {
                        if (value.metadata.threadId && !value.metadata.threadId->value.empty() && value.metadata.turnId &&
                            !value.metadata.turnId->value.empty()) {
                            return std::pair{*value.metadata.threadId, *value.metadata.turnId};
                        }
                    }
                    return std::nullopt;
                },
                item);
        }

        bool upsertItem(BackendState& state,
                        const typed::ThreadId& threadId,
                        const typed::TurnId& turnId,
                        const typed::Item& item,
                        ItemLifecycle lifecycle,
                        std::optional<std::int64_t> occurredAtMs,
                        std::size_t contentLimit) {
            const std::optional<typed::ItemId> id = itemId(item);
            if (!id) {
                return false;
            }

            TurnState& turn = ensureTurn(state, threadId, turnId);
            auto iterator = turn.items.find(id->value);
            if (iterator == turn.items.end()) {
                ItemState itemState;
                itemState.item = item;
                itemState.lifecycle = lifecycle;
                if (lifecycle == ItemLifecycle::Started) {
                    itemState.startedAtMs = occurredAtMs;
                } else if (lifecycle == ItemLifecycle::Completed) {
                    itemState.completedAtMs = occurredAtMs;
                }
                initializeVisibleContent(itemState, true, contentLimit);
                turn.items.emplace(id->value, std::move(itemState));
                turn.itemOrder.push_back(*id);
            } else {
                ItemState& itemState = iterator->second;
                itemState.item = item;
                if (lifecycle != ItemLifecycle::Unknown || itemState.lifecycle == ItemLifecycle::Unknown) {
                    itemState.lifecycle = lifecycle;
                }
                if (lifecycle == ItemLifecycle::Started && occurredAtMs) {
                    itemState.startedAtMs = occurredAtMs;
                } else if (lifecycle == ItemLifecycle::Completed && occurredAtMs) {
                    itemState.completedAtMs = occurredAtMs;
                }
                initializeVisibleContent(itemState, lifecycle == ItemLifecycle::Completed, contentLimit);
            }
            return true;
        }

        bool upsertTurn(BackendState& state, const typed::Turn& value, std::size_t contentLimit) {
            ThreadState& thread = ensureThread(state, value.threadId);
            auto iterator = thread.turns.find(value.id.value);
            if (iterator == thread.turns.end()) {
                TurnState turn;
                turn.turn = value;
                turn.active = !isTerminal(value.status);
                turn.terminal = isTerminal(value.status);
                iterator = thread.turns.emplace(value.id.value, std::move(turn)).first;
                thread.turnOrder.push_back(value.id);
            } else {
                iterator->second.turn = value;
                iterator->second.active = !isTerminal(value.status);
                iterator->second.terminal = isTerminal(value.status);
                if (value.error.hasValue()) {
                    iterator->second.failure = value.error->raw;
                }
            }

            for (const typed::Item& item : value.items) {
                upsertItem(state,
                           value.threadId,
                           value.id,
                           item,
                           iterator->second.terminal ? ItemLifecycle::Completed : ItemLifecycle::Unknown,
                           std::nullopt,
                           contentLimit);
            }
            return true;
        }

        bool upsertThread(BackendState& state, const typed::Thread& value, EntityLoad load, std::size_t contentLimit) {
            auto iterator = state.threads.find(value.id.value);
            if (iterator == state.threads.end()) {
                ThreadState thread;
                thread.thread = value;
                thread.fullyLoaded = load == EntityLoad::Full;
                iterator = state.threads.emplace(value.id.value, std::move(thread)).first;
                state.threadOrder.push_back(value.id);
            } else {
                iterator->second.thread = value;
                iterator->second.fullyLoaded = iterator->second.fullyLoaded || load == EntityLoad::Full;
            }

            for (const typed::Turn& turn : value.turns) {
                upsertTurn(state, turn, contentLimit);
            }
            return true;
        }

        void retainExtension(BackendState& state,
                             ExtensionRecord extension,
                             std::size_t limit,
                             std::size_t methodByteLimit,
                             std::size_t payloadByteLimit,
                             std::size_t decodingErrorByteLimit) {
            if (limit == 0) {
                return;
            }
            if (extension.method.size() > methodByteLimit) {
                extension.originalMethodBytes = static_cast<std::uint64_t>(extension.method.size());
                extension.method.resize(methodByteLimit);
            }
            if (extension.decodingError && extension.decodingError->size() > decodingErrorByteLimit) {
                extension.originalDecodingErrorBytes = static_cast<std::uint64_t>(extension.decodingError->size());
                extension.decodingError->resize(decodingErrorByteLimit);
            }
            if (extension.diagnostic) {
                std::uint64_t originalDiagnosticBytes = 0;
                const auto account = [&originalDiagnosticBytes](std::size_t bytes) {
                    const std::uint64_t value = saturatingUint64(bytes);
                    originalDiagnosticBytes = value > std::numeric_limits<std::uint64_t>::max() - originalDiagnosticBytes
                                                  ? std::numeric_limits<std::uint64_t>::max()
                                                  : originalDiagnosticBytes + value;
                };
                account(extension.diagnostic->surface.size());
                account(extension.diagnostic->fieldPath.size());
                account(extension.diagnostic->message.size());
                bool diagnosticTruncated = false;
                if (extension.diagnostic->surface.size() > methodByteLimit) {
                    extension.diagnostic->surface.resize(methodByteLimit);
                    diagnosticTruncated = true;
                }
                if (extension.diagnostic->fieldPath.size() > decodingErrorByteLimit) {
                    extension.diagnostic->fieldPath.resize(decodingErrorByteLimit);
                    diagnosticTruncated = true;
                }
                if (extension.diagnostic->message.size() > decodingErrorByteLimit) {
                    extension.diagnostic->message.resize(decodingErrorByteLimit);
                    diagnosticTruncated = true;
                }
                if (diagnosticTruncated) {
                    extension.originalDiagnosticBytes = originalDiagnosticBytes;
                }
            }
            try {
                const std::string encoded = extension.payload.dump();
                if (encoded.size() > payloadByteLimit) {
                    extension.originalPayloadBytes = static_cast<std::uint64_t>(encoded.size());
                    extension.payload = Json::object({{"truncated", true},
                                                      {"originalBytes", encoded.size()},
                                                      {"omitted", true},
                                                      {"reason", "extension payload exceeds canonical reducer bound"}});
                }
            } catch (...) {
                extension.payload = Json::object({{"omitted", true}, {"reason", "extension serialization failed"}});
            }
            state.recentExtensions.push_back(std::move(extension));
            if (state.recentExtensions.size() > limit) {
                state.recentExtensions.erase(state.recentExtensions.begin(),
                                             state.recentExtensions.begin() +
                                                 static_cast<std::ptrdiff_t>(state.recentExtensions.size() - limit));
            }
        }
    } // namespace

    CodexExtensionReceived detail::preserveUnmodeledTypedEvent(UnmodeledTypedEvent event) {
        return {.method = std::move(event.surface),
                .payload = std::move(event.rawPayload),
                .decodingError = std::move(event.decodingError),
                .diagnostic = std::move(event.diagnostic)};
    }

    Reducer::Reducer(ReducerOptions options)
        : options(std::move(options)) {
    }

    Reduction Reducer::apply(BackendState& state, const BackendEvent& event) const {
        return std::visit(
            Overloaded{
                [&state](const LifecycleChanged& value) {
                    const bool changed =
                        state.lifecycle != value.lifecycle || state.lastLifecycleError.has_value() != value.error.has_value() ||
                        (value.error && (!state.lastLifecycleError || state.lastLifecycleError->category != value.error->category ||
                                         state.lastLifecycleError->code != value.error->code ||
                                         state.lastLifecycleError->message != value.error->message));
                    state.lifecycle = value.lifecycle;
                    if (value.error) {
                        state.lastLifecycleError = value.error;
                    } else if (value.lifecycle == BackendLifecycle::Ready) {
                        state.lastLifecycleError.reset();
                    }
                    return Reduction{changed, value.lifecycle == BackendLifecycle::Failed || value.lifecycle == BackendLifecycle::Stopping};
                },
                [this, &state](const DiagnosticReceived& value) {
                    ++state.diagnostics.received;
                    if (options.retainedDiagnostics != 0) {
                        std::string message = value.message;
                        if (message.size() > options.maxDiagnosticBytes) {
                            message.erase(0, message.size() - options.maxDiagnosticBytes);
                        }
                        state.diagnostics.recent.push_back(std::move(message));
                        if (state.diagnostics.recent.size() > options.retainedDiagnostics) {
                            state.diagnostics.recent.erase(
                                state.diagnostics.recent.begin(),
                                state.diagnostics.recent.begin() +
                                    static_cast<std::ptrdiff_t>(state.diagnostics.recent.size() - options.retainedDiagnostics));
                        }
                    }
                    return Reduction{true, false};
                },
                [this, &state](const ThreadUpserted& value) {
                    return Reduction{upsertThread(state, value.thread, value.load, options.maxAccumulatedItemBytes), false};
                },
                [this, &state](const ThreadListUpdated& value) {
                    for (const typed::Thread& thread : value.page.data) {
                        upsertThread(state, thread, EntityLoad::Summary, options.maxAccumulatedItemBytes);
                    }
                    state.threadList.hasLoadedPage = true;
                    ++state.threadList.pagesLoaded;
                    state.threadList.nextCursor = value.page.nextCursor.hasValue() ? value.page.nextCursor.value : std::nullopt;
                    state.threadList.backwardsCursor =
                        value.page.backwardsCursor.hasValue() ? value.page.backwardsCursor.value : std::nullopt;
                    state.threadList.complete = !value.page.nextCursor.hasValue();
                    return Reduction{true, false};
                },
                [&state](const ThreadStatusUpdated& value) {
                    ThreadState& thread = ensureThread(state, value.threadId);
                    thread.thread.status = value.status;
                    if (thread.thread.raw.is_object() && thread.thread.raw.size() == 1 &&
                        thread.thread.raw.value("backendPlaceholder", false)) {
                        thread.thread.raw["backendPlaceholderStatusKnown"] = true;
                    }
                    return Reduction{true, false};
                },
                [this, &state](const TurnUpserted& value) {
                    return Reduction{upsertTurn(state, value.turn, options.maxAccumulatedItemBytes), false};
                },
                [this, &state](const TurnCompleted& value) {
                    const TurnState* prior = findTurn(state, value.turn.threadId, value.turn.id);
                    const bool emitTerminal = prior != nullptr && !prior->terminal;
                    upsertTurn(state, value.turn, options.maxAccumulatedItemBytes);
                    TurnState& turn = ensureTurn(state, value.turn.threadId, value.turn.id);
                    turn.active = false;
                    turn.terminal = true;
                    if (emitTerminal) {
                        const bool cancelled = value.turn.status.value == "cancelled" || value.turn.status.value == "interrupted";
                        const char* outcome = value.turn.status.value == "failed" ? "failed" : (cancelled ? "cancelled" : "completed");
                        lifecycleLog().debug("turn {}: thread={} turn={}", outcome, value.turn.threadId.value, value.turn.id.value);
                    }
                    return Reduction{true, true};
                },
                [this, &state](const TurnFailed& value) {
                    const TurnState* prior = findTurn(state, value.turn.threadId, value.turn.id);
                    const bool emitTerminal = prior != nullptr && !prior->terminal;
                    upsertTurn(state, value.turn, options.maxAccumulatedItemBytes);
                    TurnState& turn = ensureTurn(state, value.turn.threadId, value.turn.id);
                    turn.active = false;
                    turn.terminal = true;
                    turn.failure = value.error;
                    if (emitTerminal) {
                        lifecycleLog().debug("turn failed: thread={} turn={}", value.turn.threadId.value, value.turn.id.value);
                    }
                    return Reduction{true, true};
                },
                [&state](const TurnErrorUpdated& value) {
                    TurnState& turn = ensureTurn(state, value.threadId, value.turnId);
                    turn.failure = value.error;
                    if (!value.willRetry) {
                        turn.active = false;
                    }
                    return Reduction{true, !value.willRetry};
                },
                [this, &state](const ItemUpserted& value) {
                    if (upsertItem(state,
                                   value.threadId,
                                   value.turnId,
                                   value.item,
                                   value.lifecycle,
                                   value.occurredAtMs,
                                   options.maxAccumulatedItemBytes)) {
                        return Reduction{true, value.lifecycle == ItemLifecycle::Completed || value.lifecycle == ItemLifecycle::Failed};
                    }
                    retainExtension(state,
                                    {.method = "codex/item-without-id",
                                     .payload = std::visit(
                                         [](const auto& item) {
                                             using Item = std::decay_t<decltype(item)>;
                                             if constexpr (std::is_same_v<Item, typed::UnknownItem>) {
                                                 return item.raw;
                                             } else {
                                                 return item.metadata.raw;
                                             }
                                         },
                                         value.item),
                                     .decodingError = "typed item has no stable id",
                                     .originalMethodBytes = std::nullopt,
                                     .originalPayloadBytes = std::nullopt,
                                     .originalDecodingErrorBytes = std::nullopt,
                                     .diagnostic = std::nullopt,
                                     .originalDiagnosticBytes = std::nullopt},
                                    options.retainedExtensions,
                                    options.maxExtensionMethodBytes,
                                    options.maxExtensionBytes,
                                    options.maxExtensionDecodingErrorBytes);
                    return Reduction{true, value.lifecycle == ItemLifecycle::Completed};
                },
                [this, &state](const ItemContentChanged& value) {
                    TurnState& turn = ensureTurn(state, value.threadId, value.turnId);
                    auto iterator = turn.items.find(value.itemId.value);
                    if (iterator == turn.items.end()) {
                        typed::UnknownItem placeholder;
                        placeholder.type = "backend.delta-placeholder";
                        placeholder.raw = Json::object({{"id", value.itemId.value}, {"backendPlaceholder", true}});
                        placeholder.metadata = {value.itemId, value.threadId, value.turnId};
                        upsertItem(state,
                                   value.threadId,
                                   value.turnId,
                                   typed::Item{std::move(placeholder)},
                                   ItemLifecycle::Started,
                                   std::nullopt,
                                   options.maxAccumulatedItemBytes);
                        iterator = turn.items.find(value.itemId.value);
                    }
                    ItemState& item = iterator->second;
                    switch (value.kind) {
                        case ItemContentChanged::Kind::AgentText:
                            appendBounded(item.agentText, value.delta, options.maxAccumulatedItemBytes, item.droppedContentBytes);
                            break;
                        case ItemContentChanged::Kind::ReasoningText:
                            appendBounded(item.reasoningText, value.delta, options.maxAccumulatedItemBytes, item.droppedContentBytes);
                            break;
                        case ItemContentChanged::Kind::ReasoningSummary:
                            appendBounded(item.reasoningSummary, value.delta, options.maxAccumulatedItemBytes, item.droppedContentBytes);
                            break;
                        case ItemContentChanged::Kind::CommandOutput:
                            appendBounded(item.commandOutput, value.delta, options.maxAccumulatedItemBytes, item.droppedContentBytes);
                            break;
                    }
                    return Reduction{true, false};
                },
                [this, &state](const FileChangeUpdated& value) {
                    ItemState* item = findItem(state, value.threadId, value.turnId, value.itemId);
                    if (!item) {
                        typed::UnknownItem placeholder;
                        placeholder.type = "fileChange";
                        placeholder.raw = Json::object({{"id", value.itemId.value}, {"changes", value.changes}});
                        placeholder.metadata = {value.itemId, value.threadId, value.turnId};
                        upsertItem(state,
                                   value.threadId,
                                   value.turnId,
                                   typed::Item{std::move(placeholder)},
                                   ItemLifecycle::Started,
                                   std::nullopt,
                                   options.maxAccumulatedItemBytes);
                        item = findItem(state, value.threadId, value.turnId, value.itemId);
                    }
                    if (item) {
                        if (auto* fileChange = std::get_if<typed::FileChangeItem>(&item->item)) {
                            fileChange->metadata.raw["changes"] = value.changes;
                            if (std::optional<std::vector<typed::FileUpdateChange>> changes = decodeFileUpdateChanges(value.changes)) {
                                fileChange->changes = std::move(*changes);
                            }
                        }
                        item->extensions["fileChanges"] = value.changes;
                    }
                    return Reduction{true, false};
                },
                [&state](const TokenUsageUpdated& value) {
                    ensureTurn(state, value.threadId, value.turnId).tokenUsage = value.usage;
                    return Reduction{true, false};
                },
                [this, &state](const ModelRerouted& value) {
                    TurnState& turn = ensureTurn(state, value.threadId, value.turnId);
                    if (options.maxModelReroutesPerTurn != 0) {
                        turn.modelReroutes.push_back({value.from, value.to, value.reason});
                        if (turn.modelReroutes.size() > options.maxModelReroutesPerTurn) {
                            turn.modelReroutes.erase(
                                turn.modelReroutes.begin(),
                                turn.modelReroutes.begin() +
                                    static_cast<std::ptrdiff_t>(turn.modelReroutes.size() - options.maxModelReroutesPerTurn));
                        }
                    }
                    return Reduction{true, false};
                },
                [&state](const PendingRequestAdded& value) {
                    state.pendingRequests.insert_or_assign(value.pending.id, value.pending);
                    return Reduction{true, true};
                },
                [&state](const PendingRequestRemoved& value) {
                    return Reduction{state.pendingRequests.erase(value.id) != 0, true};
                },
                [&state](const ControllerChanged& value) {
                    const bool changed = state.controller != value.controller;
                    if (state.controller) {
                        const auto previous = state.sessions.find(*state.controller);
                        if (previous != state.sessions.end()) {
                            previous->second.role = SessionRole::Observer;
                        }
                    }
                    state.controller = value.controller;
                    if (state.controller) {
                        const auto current = state.sessions.find(*state.controller);
                        if (current != state.sessions.end()) {
                            current->second.role = SessionRole::Controller;
                        }
                    }
                    return Reduction{changed, true};
                },
                [&state](const SessionChanged& value) {
                    if (value.connected) {
                        state.sessions.insert_or_assign(value.id, ConnectedSessionState{value.id, value.role});
                    } else {
                        state.sessions.erase(value.id);
                        if (state.controller == value.id) {
                            state.controller.reset();
                        }
                    }
                    return Reduction{true, true};
                },
                [this, &state](const CodexExtensionReceived& value) {
                    retainExtension(state,
                                    {.method = value.method,
                                     .payload = value.payload,
                                     .decodingError = value.decodingError,
                                     .originalMethodBytes = std::nullopt,
                                     .originalPayloadBytes = std::nullopt,
                                     .originalDecodingErrorBytes = std::nullopt,
                                     .diagnostic = value.diagnostic,
                                     .originalDiagnosticBytes = std::nullopt},
                                    options.retainedExtensions,
                                    options.maxExtensionMethodBytes,
                                    options.maxExtensionBytes,
                                    options.maxExtensionDecodingErrorBytes);
                    return Reduction{true, false};
                }},
            event);
    }

    std::vector<BackendEvent> Reducer::translate(const typed::Event& event) const {
        return std::visit(
            Overloaded{
                [](const typed::ThreadStarted& value) -> std::vector<BackendEvent> {
                    lifecycleLog().info("thread created: thread={}", value.thread.id.value);
                    return {ThreadUpserted{value.thread, EntityLoad::Summary}};
                },
                [](const typed::ThreadStatusChanged& value) -> std::vector<BackendEvent> {
                    return {ThreadStatusUpdated{value.threadId, value.status}};
                },
                [](const typed::TurnStarted& value) -> std::vector<BackendEvent> {
                    lifecycleLog().debug("turn started: thread={} turn={}", value.turn.threadId.value, value.turn.id.value);
                    return {TurnUpserted{value.turn}};
                },
                [](const typed::TurnCompleted& value) -> std::vector<BackendEvent> {
                    return {TurnCompleted{value.turn}};
                },
                [](const typed::TurnFailed& value) -> std::vector<BackendEvent> {
                    return {TurnFailed{value.turn, value.error}};
                },
                [](const typed::ItemStarted& value) -> std::vector<BackendEvent> {
                    const auto location = itemLocation(value.item);
                    if (!location) {
                        return {CodexExtensionReceived{"item/started", value.raw, "item event omitted threadId or turnId", std::nullopt}};
                    }
                    return {ItemUpserted{location->first, location->second, value.item, ItemLifecycle::Started, value.startedAtMs}};
                },
                [](const typed::ItemCompleted& value) -> std::vector<BackendEvent> {
                    const auto location = itemLocation(value.item);
                    if (!location) {
                        return {CodexExtensionReceived{"item/completed", value.raw, "item event omitted threadId or turnId", std::nullopt}};
                    }
                    return {ItemUpserted{location->first, location->second, value.item, ItemLifecycle::Completed, value.completedAtMs}};
                },
                [](const typed::AgentMessageDelta& value) -> std::vector<BackendEvent> {
                    return {ItemContentChanged{
                        value.threadId, value.turnId, value.itemId, ItemContentChanged::Kind::AgentText, value.text, std::nullopt}};
                },
                [](const typed::ReasoningDelta& value) -> std::vector<BackendEvent> {
                    return {ItemContentChanged{value.threadId,
                                               value.turnId,
                                               value.itemId,
                                               value.kind == typed::ReasoningDelta::Kind::Summary
                                                   ? ItemContentChanged::Kind::ReasoningSummary
                                                   : ItemContentChanged::Kind::ReasoningText,
                                               value.text,
                                               value.index}};
                },
                [](const typed::CommandOutputDelta& value) -> std::vector<BackendEvent> {
                    return {ItemContentChanged{
                        value.threadId, value.turnId, value.itemId, ItemContentChanged::Kind::CommandOutput, value.output, std::nullopt}};
                },
                [](const typed::FileChangeUpdated& value) -> std::vector<BackendEvent> {
                    return {FileChangeUpdated{value.threadId, value.turnId, value.itemId, value.changes}};
                },
                [](const typed::TokenUsageUpdated& value) -> std::vector<BackendEvent> {
                    return {TokenUsageUpdated{value.threadId, value.turnId, value.usage}};
                },
                [](const typed::TerminalInteractionNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::TerminalInteraction);
                },
                [](const typed::FileChangeOutputDeltaNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::FileChangeOutputDelta);
                },
                [](const typed::McpToolCallProgressNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::McpToolCallProgress);
                },
                [](const typed::PlanDeltaNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::PlanDelta);
                },
                [](const typed::ReasoningSummaryPartAddedNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::ReasoningSummaryPartAdded);
                },
                [](const typed::ThreadArchivedNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::ThreadArchived);
                },
                [](const typed::ThreadClosedNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::ThreadClosed);
                },
                [](const typed::ContextCompactedNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::ContextCompacted);
                },
                [](const typed::ThreadDeletedNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::ThreadDeleted);
                },
                [](const typed::ThreadGoalClearedNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::ThreadGoalCleared);
                },
                [](const typed::ThreadGoalUpdatedNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::ThreadGoalUpdated);
                },
                [](const typed::ThreadNameUpdatedNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::ThreadNameUpdated);
                },
                [](const typed::ThreadRealtimeClosedNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::ThreadRealtimeClosed);
                },
                [](const typed::ThreadRealtimeErrorNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::ThreadRealtimeError);
                },
                [](const typed::ThreadRealtimeItemAddedNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::ThreadRealtimeItemAdded);
                },
                [](const typed::ThreadRealtimeOutputAudioDeltaNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::ThreadRealtimeOutputAudioDelta);
                },
                [](const typed::ThreadRealtimeSdpNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::ThreadRealtimeSdp);
                },
                [](const typed::ThreadRealtimeStartedNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::ThreadRealtimeStarted);
                },
                [](const typed::ThreadRealtimeTranscriptDeltaNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::ThreadRealtimeTranscriptDelta);
                },
                [](const typed::ThreadRealtimeTranscriptDoneNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::ThreadRealtimeTranscriptDone);
                },
                [](const typed::ThreadSettingsUpdatedNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::ThreadSettingsUpdated);
                },
                [](const typed::ThreadUnarchivedNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::ThreadUnarchived);
                },
                [](const typed::TurnDiffUpdatedNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::TurnDiffUpdated);
                },
                [](const typed::TurnModerationMetadataNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::TurnModerationMetadata);
                },
                [](const typed::TurnPlanUpdatedNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::TurnPlanUpdated);
                },
                [](const typed::AccountLoginCompletedNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::AccountLoginCompleted);
                },
                [](const typed::AccountRateLimitsUpdatedNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::AccountRateLimitsUpdated);
                },
                [](const typed::AccountUpdatedNotification& value) -> std::vector<BackendEvent> {
                    return preserveTypedNotification(value, ServerNotificationTarget::AccountUpdated);
                },
                [](const typed::ModelRerouted& value) -> std::vector<BackendEvent> {
                    return {ModelRerouted{value.threadId, value.turnId, value.from, value.to, value.reason}};
                },
                [](const typed::TurnErrorEvent& value) -> std::vector<BackendEvent> {
                    return {TurnErrorUpdated{value.threadId, value.turnId, value.error, value.willRetry}};
                },
                [](const typed::UnknownEvent& value) -> std::vector<BackendEvent> {
                    return {detail::preserveUnmodeledTypedEvent({value.method, value.params, value.decodingError, value.diagnostic})};
                }},
            event);
    }

} // namespace ai::openai::codex::backend
