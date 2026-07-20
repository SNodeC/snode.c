/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef TESTS_COMPONENT_CODEX_CODEXBACKENDTESTSUPPORT_H
#define TESTS_COMPONENT_CODEX_CODEXBACKENDTESTSUPPORT_H

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/detail/Transport.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace tests::codex {

    using ai::openai::codex::AppServerClient;
    using ai::openai::codex::Json;
    using ai::openai::codex::detail::ProcessExit;
    using ai::openai::codex::detail::Transport;
    using ai::openai::codex::detail::TransportCallbacks;

    struct FakeTransportState {
        TransportCallbacks callbacks;
        std::vector<TransportCallbacks> callbackGenerations;
        std::vector<Json> outgoing;
        std::function<void(const Json&, const TransportCallbacks&)> sendHook;
        bool rejectNextSend = false;
        bool running = false;
        std::size_t startCount = 0;
        std::size_t stopCount = 0;

        void inject(const Json& message) const {
            if (callbacks.onMessage) {
                callbacks.onMessage(message.dump());
            }
        }
    };

    inline bool hasCallbacks(const TransportCallbacks& callbacks) {
        return callbacks.onStarted || callbacks.onMessage || callbacks.onDiagnostic || callbacks.onError || callbacks.onExited;
    }

    class FakeTransport final : public Transport {
    public:
        explicit FakeTransport(std::shared_ptr<FakeTransportState> state)
            : state(std::move(state)) {
        }

        void setCallbacks(TransportCallbacks callbacks) override {
            state->callbacks = std::move(callbacks);
            if (hasCallbacks(state->callbacks)) {
                state->callbackGenerations.push_back(state->callbacks);
            }
        }

        void start() override {
            ++state->startCount;
            state->running = true;
            if (state->callbacks.onStarted) {
                state->callbacks.onStarted();
            }
        }

        bool send(std::string message) override {
            state->outgoing.push_back(Json::parse(message));
            if (std::exchange(state->rejectNextSend, false)) {
                return false;
            }
            if (state->sendHook) {
                const TransportCallbacks callbacks = state->callbacks;
                state->sendHook(state->outgoing.back(), callbacks);
            }
            return true;
        }

        void stop() override {
            ++state->stopCount;
            if (!std::exchange(state->running, false)) {
                return;
            }
            if (state->callbacks.onExited) {
                state->callbacks.onExited(ProcessExit{true, 0, false, 0});
            }
        }

    private:
        std::shared_ptr<FakeTransportState> state;
    };

    class FakeAppServerClient final : public AppServerClient {
    public:
        explicit FakeAppServerClient(const std::shared_ptr<FakeTransportState>& state, FakeAppServerClient** instance = nullptr)
            : AppServerClient(std::make_unique<FakeTransport>(state), {"codex_backend_test", "Codex Backend Test", "1"})
            , instance(instance) {
            if (this->instance != nullptr) {
                *this->instance = this;
            }
        }

        ~FakeAppServerClient() override {
            if (instance != nullptr) {
                *instance = nullptr;
            }
        }

    private:
        FakeAppServerClient** instance;
    };

    inline void inject(const TransportCallbacks& callbacks, const Json& message) {
        if (callbacks.onMessage) {
            callbacks.onMessage(message.dump());
        }
    }

    inline Json initializeResult() {
        return {
            {"codexHome", "/tmp/codex-backend-test"},
            {"platformFamily", "unix"},
            {"platformOs", "linux"},
            {"userAgent", "codex-backend-test/1"},
        };
    }

    inline Json threadValue(const std::string& id, Json turns = Json::array()) {
        return {
            {"id", id},
            {"name", "Backend test thread"},
            {"cwd", "/tmp/project"},
            {"modelProvider", "openai"},
            {"preview", "backend test"},
            {"cliVersion", "0.144.6"},
            {"sessionId", "session-" + id},
            {"createdAt", 1},
            {"updatedAt", 2},
            {"ephemeral", false},
            {"source", "appServer"},
            {"status", {{"type", "idle"}}},
            {"turns", std::move(turns)},
        };
    }

    inline Json threadOperationResult(const std::string& id, Json turns = Json::array()) {
        return {
            {"approvalPolicy", "on-request"},
            {"approvalsReviewer", "user"},
            {"cwd", "/tmp/project"},
            {"model", "gpt-5"},
            {"modelProvider", "openai"},
            {"sandbox", {{"type", "workspaceWrite"}}},
            {"thread", threadValue(id, std::move(turns))},
        };
    }

    inline Json
    turnValue(const std::string& threadId, const std::string& turnId, std::string status = "inProgress", Json items = Json::array()) {
        return {
            {"id", turnId},
            {"threadId", threadId},
            {"status", std::move(status)},
            {"items", std::move(items)},
        };
    }

    inline Json turnOperationResult(const std::string& threadId, const std::string& turnId) {
        return {
            {"turn", turnValue(threadId, turnId)},
        };
    }

    inline void installInitializingFake(const std::shared_ptr<FakeTransportState>& state,
                                        std::function<void(const Json&, const TransportCallbacks&)> afterInitialize = {}) {
        state->sendHook = [afterInitialize = std::move(afterInitialize)](const Json& message, const TransportCallbacks& callbacks) {
            const auto method = message.find("method");
            if (method == message.end() || !method->is_string()) {
                return;
            }
            const auto id = message.find("id");
            if (*method == "initialize" && id != message.end()) {
                inject(callbacks, Json{{"id", *id}, {"result", initializeResult()}});
                return;
            }
            if (afterInitialize) {
                afterInitialize(message, callbacks);
            }
        };
    }

} // namespace tests::codex

#endif // TESTS_COMPONENT_CODEX_CODEXBACKENDTESTSUPPORT_H
