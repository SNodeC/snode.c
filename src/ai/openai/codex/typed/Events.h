/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_TYPED_EVENTS_H
#define AI_OPENAI_CODEX_TYPED_EVENTS_H

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/typed/CodexErrorInfo.h"
#include "ai/openai/codex/typed/Threads.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <variant>

namespace ai::openai::codex::typed {

    struct ThreadStarted {
        Thread thread;
        Json raw;
    };

    struct ThreadStatusChanged {
        ThreadId threadId;
        ThreadStatus status;
        Json raw;
    };

    struct TurnStarted {
        Turn turn;
        Json raw;
    };

    struct TurnCompleted {
        Turn turn;
        Json raw;
    };

    struct TurnFailed {
        Turn turn;
        Json error = nullptr;
        Json raw;
    };

    struct ItemStarted {
        Item item;
        std::int64_t startedAtMs = 0;
        Json raw;
    };

    struct ItemCompleted {
        Item item;
        std::int64_t completedAtMs = 0;
        Json raw;
    };

    struct AgentMessageDelta {
        ThreadId threadId;
        TurnId turnId;
        ItemId itemId;
        std::string text;
        Json raw;
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
    };

    struct CommandOutputDelta {
        ThreadId threadId;
        TurnId turnId;
        ItemId itemId;
        std::string output;
        Json raw;
    };

    struct FileChangeUpdated {
        ThreadId threadId;
        TurnId turnId;
        ItemId itemId;
        Json changes = Json::array();
        Json raw;
    };

    struct TokenUsageUpdated {
        ThreadId threadId;
        TurnId turnId;
        Json usage;
        Json raw;
    };

    struct ModelRerouted {
        ThreadId threadId;
        TurnId turnId;
        ModelId from;
        ModelId to;
        std::string reason;
        Json raw;
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
                               ModelRerouted,
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
