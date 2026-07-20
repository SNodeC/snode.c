/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_BACKEND_BACKENDCORE_H
#define AI_OPENAI_CODEX_BACKEND_BACKENDCORE_H

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/backend/BackendState.h"
#include "ai/openai/codex/backend/FrontendSession.h"
#include "ai/openai/codex/backend/Reducer.h"
#include "ai/openai/codex/backend/Snapshot.h"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace ai::openai::codex::frontend {
    class BackendAdapter;
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

    namespace detail {

        // Non-template runtime retained in BackendCore.cpp. It borrows the
        // concrete client owned by BackendCore<ClientT> and never outlives it.
        class BackendCoreRuntime {
        public:
            explicit BackendCoreRuntime(AppServerClient& client, BackendCoreOptions options = {});
            BackendCoreRuntime(const BackendCoreRuntime&) = delete;
            BackendCoreRuntime(BackendCoreRuntime&&) = delete;

            BackendCoreRuntime& operator=(const BackendCoreRuntime&) = delete;
            BackendCoreRuntime& operator=(BackendCoreRuntime&&) = delete;

            ~BackendCoreRuntime();

            void start();
            void stop();

            [[nodiscard]] BackendState state() const;
            [[nodiscard]] Snapshot snapshot() const;
            [[nodiscard]] bool isReady() const noexcept;

            [[nodiscard]] FrontendSession openSession(FrontendSessionCallbacks callbacks);
            [[nodiscard]] BackendObserverSubscription subscribe(BackendObserverCallbacks callbacks);

        private:
            class Impl;
            std::shared_ptr<Impl> impl;
        };

    } // namespace detail

    template <typename ClientT>
        requires std::derived_from<ClientT, AppServerClient>
    class BackendCore {
    public:
        BackendCore()
            requires std::default_initializable<ClientT>
            : ownedClient()
            , runtime(ownedClient) {
        }

        explicit BackendCore(BackendCoreOptions options)
            requires std::default_initializable<ClientT>
            : ownedClient()
            , runtime(ownedClient, std::move(options)) {
        }

        template <typename FirstClientArg, typename... ClientArgs>
            requires(!std::same_as<std::remove_cvref_t<FirstClientArg>, BackendCoreOptions> &&
                     std::constructible_from<ClientT, FirstClientArg, ClientArgs...>)
        explicit BackendCore(FirstClientArg&& firstClientArg, ClientArgs&&... clientArgs)
            : ownedClient(std::forward<FirstClientArg>(firstClientArg), std::forward<ClientArgs>(clientArgs)...)
            , runtime(ownedClient) {
        }

        template <typename... ClientArgs>
            requires std::constructible_from<ClientT, ClientArgs...>
        BackendCore(BackendCoreOptions options, ClientArgs&&... clientArgs)
            : ownedClient(std::forward<ClientArgs>(clientArgs)...)
            , runtime(ownedClient, std::move(options)) {
        }

        BackendCore(const BackendCore&) = delete;
        BackendCore(BackendCore&&) = delete;

        BackendCore& operator=(const BackendCore&) = delete;
        BackendCore& operator=(BackendCore&&) = delete;

        ~BackendCore() = default;

        void start() {
            runtime.start();
        }

        void stop() {
            runtime.stop();
        }

        [[nodiscard]] BackendState state() const {
            return runtime.state();
        }

        [[nodiscard]] Snapshot snapshot() const {
            return runtime.snapshot();
        }

        [[nodiscard]] bool isReady() const noexcept {
            return runtime.isReady();
        }

        [[nodiscard]] FrontendSession openSession(FrontendSessionCallbacks callbacks) {
            return runtime.openSession(std::move(callbacks));
        }

        [[nodiscard]] BackendObserverSubscription subscribe(BackendObserverCallbacks callbacks) {
            return runtime.subscribe(std::move(callbacks));
        }

    private:
        friend class ai::openai::codex::frontend::BackendAdapter;

        [[nodiscard]] detail::BackendCoreRuntime& implementation() noexcept {
            return runtime;
        }

        // Members are destroyed in reverse declaration order: runtime first,
        // then the concrete client it borrows.
        ClientT ownedClient;
        detail::BackendCoreRuntime runtime;
    };

} // namespace ai::openai::codex::backend

#endif // AI_OPENAI_CODEX_BACKEND_BACKENDCORE_H
