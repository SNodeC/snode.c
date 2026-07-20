/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_BACKEND_FRONTENDSESSION_H
#define AI_OPENAI_CODEX_BACKEND_FRONTENDSESSION_H

#include "ai/openai/codex/backend/BackendCommand.h"
#include "ai/openai/codex/backend/BackendEvent.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace ai::openai::codex::backend {

    namespace detail {
        class BackendCoreRuntime;
    }

    struct FrontendSessionCallbacks {
        using Events = std::function<void(const std::vector<SequencedBackendEvent>&)>;
        using SnapshotReady = std::function<void(const Snapshot&)>;
        using CommandCompleted = std::function<void(const CommandCompletion&)>;
        using Closed = std::function<void(const std::string&)>;

        Events onEvents;
        SnapshotReady onSnapshot;
        CommandCompleted onCommandCompleted;
        Closed onClosed;
    };

    enum class SubmissionError { None, Closed, EmptyRequestId, DuplicateRequestId, QueueFull };

    struct CommandSubmission {
        bool accepted = false;
        SubmissionError error = SubmissionError::None;
        std::string message;

        explicit operator bool() const noexcept {
            return accepted;
        }
    };

    class FrontendSession {
    public:
        FrontendSession() noexcept;
        FrontendSession(const FrontendSession&) = delete;
        FrontendSession(FrontendSession&& other) noexcept;

        FrontendSession& operator=(const FrontendSession&) = delete;
        FrontendSession& operator=(FrontendSession&& other) noexcept;

        ~FrontendSession();

        SessionId id() const noexcept;
        SessionRole role() const noexcept;
        bool isOpen() const noexcept;

        CommandSubmission submit(std::string requestId, BackendCommand command);
        bool requestSnapshot();
        void close(std::string reason = "frontend session closed") noexcept;

    private:
        friend class detail::BackendCoreRuntime;
        struct Control;

        explicit FrontendSession(std::shared_ptr<Control> control) noexcept;

        std::shared_ptr<Control> control;
    };

    struct BackendObserverCallbacks {
        using Events = std::function<void(const std::vector<SequencedBackendEvent>&)>;
        using Resynchronize = std::function<void(const Snapshot&)>;

        Events onEvents;
        Resynchronize onResynchronize;
    };

    class BackendObserverSubscription {
    public:
        BackendObserverSubscription() noexcept;
        BackendObserverSubscription(const BackendObserverSubscription&) = delete;
        BackendObserverSubscription(BackendObserverSubscription&& other) noexcept;

        BackendObserverSubscription& operator=(const BackendObserverSubscription&) = delete;
        BackendObserverSubscription& operator=(BackendObserverSubscription&& other) noexcept;

        ~BackendObserverSubscription();

        bool isOpen() const noexcept;
        void close() noexcept;

    private:
        friend class detail::BackendCoreRuntime;
        struct Control;

        explicit BackendObserverSubscription(std::shared_ptr<Control> control) noexcept;

        std::shared_ptr<Control> control;
    };

} // namespace ai::openai::codex::backend

#endif // AI_OPENAI_CODEX_BACKEND_FRONTENDSESSION_H
