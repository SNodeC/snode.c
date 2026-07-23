/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_BACKEND_BACKENDEVENT_H
#define AI_OPENAI_CODEX_BACKEND_BACKENDEVENT_H

#include "ai/openai/codex/backend/BackendState.h"

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace ai::openai::codex::backend {

    enum class EntityLoad { Summary, Full };

    struct LifecycleChanged {
        BackendLifecycle lifecycle = BackendLifecycle::Stopped;
        std::optional<Error> error;
        std::uint64_t connectionGeneration = 0;
    };

    struct DiagnosticReceived {
        std::string message;
    };

    struct ThreadUpserted {
        typed::Thread thread;
        EntityLoad load = EntityLoad::Summary;
    };

    struct ThreadListUpdated {
        typed::ThreadPage page;
        std::optional<std::string> requestedCursor;
        bool initialRefresh = false;
    };

    struct ThreadStatusUpdated {
        typed::ThreadId threadId;
        typed::ThreadStatus status;
    };

    struct TurnUpserted {
        typed::Turn turn;
    };

    struct TurnCompleted {
        typed::Turn turn;
    };

    struct TurnFailed {
        typed::Turn turn;
        Json error = nullptr;
    };

    struct TurnErrorUpdated {
        typed::ThreadId threadId;
        typed::TurnId turnId;
        Json error = nullptr;
        bool willRetry = false;
    };

    struct ItemUpserted {
        typed::ThreadId threadId;
        typed::TurnId turnId;
        typed::Item item;
        ItemLifecycle lifecycle = ItemLifecycle::Unknown;
        std::optional<std::int64_t> occurredAtMs;
    };

    struct ItemContentChanged {
        enum class Kind { AgentText, ReasoningText, ReasoningSummary, CommandOutput };

        typed::ThreadId threadId;
        typed::TurnId turnId;
        typed::ItemId itemId;
        Kind kind = Kind::AgentText;
        std::string delta;
        std::optional<std::int64_t> index;
    };

    struct FileChangeUpdated {
        typed::ThreadId threadId;
        typed::TurnId turnId;
        typed::ItemId itemId;
        Json changes = Json::array();
    };

    struct TokenUsageUpdated {
        typed::ThreadId threadId;
        typed::TurnId turnId;
        Json usage = nullptr;
    };

    struct ModelRerouted {
        typed::ThreadId threadId;
        typed::TurnId turnId;
        typed::ModelId from;
        typed::ModelId to;
        std::string reason;
    };

    struct PendingRequestAdded {
        PendingRequestState pending;
    };

    struct PendingRequestRemoved {
        PendingRequestId id;
        std::string reason;
    };

    struct ControllerChanged {
        std::optional<SessionId> controller;
    };

    struct SessionChanged {
        SessionId id;
        bool connected = false;
        SessionRole role = SessionRole::Observer;
    };

    struct CodexExtensionReceived {
        std::string method;
        Json payload = nullptr;
        std::optional<std::string> decodingError;
        std::optional<typed::DecodeDiagnostic> diagnostic = std::nullopt;
    };

    using BackendEvent = std::variant<LifecycleChanged,
                                      DiagnosticReceived,
                                      ThreadUpserted,
                                      ThreadListUpdated,
                                      ThreadStatusUpdated,
                                      TurnUpserted,
                                      TurnCompleted,
                                      TurnFailed,
                                      TurnErrorUpdated,
                                      ItemUpserted,
                                      ItemContentChanged,
                                      FileChangeUpdated,
                                      TokenUsageUpdated,
                                      ModelRerouted,
                                      PendingRequestAdded,
                                      PendingRequestRemoved,
                                      ControllerChanged,
                                      SessionChanged,
                                      CodexExtensionReceived>;

    struct SequencedBackendEvent {
        SequenceNumber sequence;
        BackendEvent event;
    };

} // namespace ai::openai::codex::backend

#endif // AI_OPENAI_CODEX_BACKEND_BACKENDEVENT_H
