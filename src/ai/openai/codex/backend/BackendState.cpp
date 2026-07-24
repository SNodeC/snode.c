/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/backend/BackendState.h"

#include <optional>
#include <string>
#include <type_traits>

namespace ai::openai::codex::backend {

    BackendLifecycle toBackendLifecycle(State state) noexcept {
        switch (state) {
            case State::Stopped:
                return BackendLifecycle::Stopped;
            case State::Starting:
                return BackendLifecycle::Starting;
            case State::Initializing:
                return BackendLifecycle::Initializing;
            case State::Ready:
                return BackendLifecycle::Ready;
            case State::Stopping:
                return BackendLifecycle::Stopping;
            case State::Failed:
                return BackendLifecycle::Failed;
        }
        return BackendLifecycle::Failed;
    }

    bool isTerminal(const typed::TurnStatus& status) noexcept {
        return status.value == "completed" || status.value == "interrupted" || status.value == "failed" || status.value == "cancelled";
    }

    std::optional<typed::ItemId> itemId(const typed::Item& item) {
        return std::visit(
            [](const auto& value) -> std::optional<typed::ItemId> {
                using Value = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<Value, typed::UnknownItem>) {
                    if (value.metadata.id && !value.metadata.id->value.empty()) {
                        return value.metadata.id;
                    }
                    try {
                        if (value.raw.is_object()) {
                            const auto iterator = value.raw.find("id");
                            if (iterator != value.raw.end() && iterator->is_string() &&
                                !iterator->template get_ref<const std::string&>().empty()) {
                                return typed::ItemId{iterator->template get<std::string>()};
                            }
                        }
                    } catch (...) {
                    }
                    return std::nullopt;
                } else {
                    return value.metadata.id.value.empty() ? std::nullopt : std::optional<typed::ItemId>{value.metadata.id};
                }
            },
            item);
    }

    std::string itemType(const typed::Item& item) {
        return std::visit(
            [](const auto& value) -> std::string {
                using Value = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<Value, typed::AgentMessageItem>) {
                    return "agent_message";
                } else if constexpr (std::is_same_v<Value, typed::UserMessageItem>) {
                    return "user_message";
                } else if constexpr (std::is_same_v<Value, typed::ReasoningItem>) {
                    return "reasoning";
                } else if constexpr (std::is_same_v<Value, typed::CommandExecutionItem>) {
                    return "command_execution";
                } else if constexpr (std::is_same_v<Value, typed::FileChangeItem>) {
                    return "file_change";
                } else if constexpr (std::is_same_v<Value, typed::DynamicToolCallThreadItem>) {
                    return "tool_call";
                } else if constexpr (std::is_same_v<Value, typed::ToolCallItem>) {
                    return "tool_call";
                } else if constexpr (std::is_same_v<Value, typed::WebSearchItem>) {
                    return "web_search";
                } else if constexpr (std::is_same_v<Value, typed::CollabAgentToolCallThreadItem>) {
                    return "collabAgentToolCall";
                } else if constexpr (std::is_same_v<Value, typed::ContextCompactionThreadItem>) {
                    return "contextCompaction";
                } else if constexpr (std::is_same_v<Value, typed::EnteredReviewModeThreadItem>) {
                    return "enteredReviewMode";
                } else if constexpr (std::is_same_v<Value, typed::ExitedReviewModeThreadItem>) {
                    return "exitedReviewMode";
                } else if constexpr (std::is_same_v<Value, typed::HookPromptThreadItem>) {
                    return "hookPrompt";
                } else if constexpr (std::is_same_v<Value, typed::ImageGenerationThreadItem>) {
                    return "imageGeneration";
                } else if constexpr (std::is_same_v<Value, typed::ImageViewThreadItem>) {
                    return "imageView";
                } else if constexpr (std::is_same_v<Value, typed::PlanThreadItem>) {
                    return "plan";
                } else if constexpr (std::is_same_v<Value, typed::SleepThreadItem>) {
                    return "sleep";
                } else if constexpr (std::is_same_v<Value, typed::SubAgentActivityThreadItem>) {
                    return "subAgentActivity";
                } else {
                    return value.type && !value.type->empty() ? *value.type : "unknown";
                }
            },
            item);
    }

    ThreadState* findThread(BackendState& state, const typed::ThreadId& threadId) noexcept {
        const auto iterator = state.threads.find(threadId.value);
        return iterator == state.threads.end() ? nullptr : &iterator->second;
    }

    const ThreadState* findThread(const BackendState& state, const typed::ThreadId& threadId) noexcept {
        const auto iterator = state.threads.find(threadId.value);
        return iterator == state.threads.end() ? nullptr : &iterator->second;
    }

    TurnState* findTurn(BackendState& state, const typed::ThreadId& threadId, const typed::TurnId& turnId) noexcept {
        ThreadState* thread = findThread(state, threadId);
        if (!thread) {
            return nullptr;
        }
        const auto iterator = thread->turns.find(turnId.value);
        return iterator == thread->turns.end() ? nullptr : &iterator->second;
    }

    const TurnState* findTurn(const BackendState& state, const typed::ThreadId& threadId, const typed::TurnId& turnId) noexcept {
        const ThreadState* thread = findThread(state, threadId);
        if (!thread) {
            return nullptr;
        }
        const auto iterator = thread->turns.find(turnId.value);
        return iterator == thread->turns.end() ? nullptr : &iterator->second;
    }

    ItemState*
    findItem(BackendState& state, const typed::ThreadId& threadId, const typed::TurnId& turnId, const typed::ItemId& itemIdValue) noexcept {
        TurnState* turn = findTurn(state, threadId, turnId);
        if (!turn) {
            return nullptr;
        }
        const auto iterator = turn->items.find(itemIdValue.value);
        return iterator == turn->items.end() ? nullptr : &iterator->second;
    }

    const ItemState* findItem(const BackendState& state,
                              const typed::ThreadId& threadId,
                              const typed::TurnId& turnId,
                              const typed::ItemId& itemIdValue) noexcept {
        const TurnState* turn = findTurn(state, threadId, turnId);
        if (!turn) {
            return nullptr;
        }
        const auto iterator = turn->items.find(itemIdValue.value);
        return iterator == turn->items.end() ? nullptr : &iterator->second;
    }

} // namespace ai::openai::codex::backend
