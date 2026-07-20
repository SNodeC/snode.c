/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef APPS_CODEX_BACKEND_CLIENT_COMMANDDRAINCONTROLLER_H
#define APPS_CODEX_BACKEND_CLIENT_COMMANDDRAINCONTROLLER_H

#include "ai/openai/codex/frontend/Messages.h"

#include <cstddef>
#include <deque>
#include <functional>
#include <list>
#include <string>

namespace apps::codex_backend_client {

    struct CommandDrainCallbacks {
        std::function<bool(const ai::openai::codex::frontend::ClientMessage&)> send;
        std::function<void()> requestExit;
        std::function<void(std::string)> reportFailure;
    };

    class CommandDrainController {
    public:
        enum class SessionState { Connecting, Synchronizing, Ready, Closing, Closed };
        enum class InputState { Reading, DrainOnEof, ImmediateQuit };
        enum class Outcome { Running, Success, Failure };

        explicit CommandDrainController(CommandDrainCallbacks callbacks = {});

        [[nodiscard]] bool enqueue(ai::openai::codex::frontend::ClientMessage message);

        void connected();
        void receive(const ai::openai::codex::frontend::ServerMessage& message);
        void inputEof();
        void inputFailed(std::string message);
        void connectionFailed(std::string message);
        void protocolFailed(std::string message);
        void disconnected();
        void quit();

        [[nodiscard]] SessionState sessionState() const noexcept;
        [[nodiscard]] InputState inputState() const noexcept;
        [[nodiscard]] Outcome outcome() const noexcept;
        [[nodiscard]] bool failed() const noexcept;
        [[nodiscard]] const std::string& failureReason() const noexcept;
        [[nodiscard]] std::size_t queuedCount() const noexcept;
        [[nodiscard]] std::size_t pendingResponseCount() const noexcept;
        [[nodiscard]] std::size_t pendingSyncCount() const noexcept;

    private:
        struct PendingCommand {
            std::string requestId;
            bool responsePending = true;
            bool waitForSyncOnSuccess = false;
            bool syncPending = false;
        };

        [[nodiscard]] bool sendNow(const ai::openai::codex::frontend::ClientMessage& message);
        [[nodiscard]] bool requestIdIsPending(const std::string& requestId) const noexcept;
        void flushQueued();
        void completeResponse(const ai::openai::codex::frontend::Response& response);
        void completeSync();
        void removeCompletedCommands();
        void maybeCompleteDrain();
        void finish(Outcome outcome);
        void fail(std::string message);

        CommandDrainCallbacks callbacks;
        SessionState currentSessionState = SessionState::Connecting;
        InputState currentInputState = InputState::Reading;
        Outcome currentOutcome = Outcome::Running;
        std::deque<ai::openai::codex::frontend::ClientMessage> queuedMessages;
        std::list<PendingCommand> pendingCommands;
        std::string currentFailureReason;
    };

} // namespace apps::codex_backend_client

#endif // APPS_CODEX_BACKEND_CLIENT_COMMANDDRAINCONTROLLER_H
