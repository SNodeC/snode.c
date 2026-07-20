/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_BACKEND_BACKENDCORE_H
#define AI_OPENAI_CODEX_BACKEND_BACKENDCORE_H

#include "ai/openai/codex/backend/BackendState.h"
#include "ai/openai/codex/backend/FrontendSession.h"
#include "ai/openai/codex/backend/Reducer.h"
#include "ai/openai/codex/backend/Snapshot.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>

namespace ai::openai::codex {
    class AppServerClient;
}

namespace ai::openai::codex::backend {

    using Scheduler = std::function<void(std::function<void()>)>;

    struct BackendCoreOptions {
        std::uint32_t initialThreadListLimit = 50;
        std::size_t maxSessionQueueEntries = 4096;
        std::size_t maxSessionQueueBytes = 8U * 1024U * 1024U;
        std::size_t maxObserverQueueEntries = 4096;
        std::size_t maxObserverQueueBytes = 8U * 1024U * 1024U;
        std::size_t maxEventsPerCallback = 512;
        ReducerOptions reducer;
        Scheduler scheduler;
    };

    class BackendCore {
    public:
        explicit BackendCore(std::unique_ptr<AppServerClient> client, BackendCoreOptions options = {});
        BackendCore(const BackendCore&) = delete;
        BackendCore(BackendCore&&) = delete;

        BackendCore& operator=(const BackendCore&) = delete;
        BackendCore& operator=(BackendCore&&) = delete;

        ~BackendCore();

        void start();
        void stop();

        BackendState state() const;
        Snapshot snapshot() const;
        bool isReady() const noexcept;

        FrontendSession openSession(FrontendSessionCallbacks callbacks);
        BackendObserverSubscription subscribe(BackendObserverCallbacks callbacks);

    private:
        class Impl;
        std::shared_ptr<Impl> impl;
    };

} // namespace ai::openai::codex::backend

#endif // AI_OPENAI_CODEX_BACKEND_BACKENDCORE_H
