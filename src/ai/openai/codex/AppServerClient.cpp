/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/AppServerClient.h"

#include "ai/openai/codex/detail/ProtocolCodec.h"
#include "ai/openai/codex/detail/Transport.h"
#include "core/EventReceiver.h"
#include "log/LogScopeOwner.h"
#include "log/Logger.h"
#include "log/SemanticLogger.h"

#include <cerrno>
#include <cstdint>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

namespace ai::openai::codex {

    namespace {
        const char* stateName(State state) {
            switch (state) {
                case State::Stopped:
                    return "Stopped";
                case State::Starting:
                    return "Starting";
                case State::Initializing:
                    return "Initializing";
                case State::Ready:
                    return "Ready";
                case State::Stopping:
                    return "Stopping";
                case State::Failed:
                    return "Failed";
            }

            return "Unknown";
        }

        std::string processExitMessage(const detail::ProcessExit& status) {
            if (status.exited) {
                return "codex app-server exited unexpectedly with status " + std::to_string(status.exitCode);
            }
            if (status.signaled) {
                return "codex app-server terminated unexpectedly from signal " + std::to_string(status.signalNumber);
            }
            return "codex app-server exit status could not be determined";
        }
    } // namespace

    class AppServerClient::Impl {
    public:
        Impl(std::unique_ptr<detail::Transport> transport, ClientInfo clientInfo)
            : transport(std::move(transport))
            , clientInfo(std::move(clientInfo))
            , lifetime(std::make_shared<Lifetime>())
            , logScope(logger::LogOrigin::Framework, logger::LogBoundary::Connection, "ai.openai.codex") {
            this->transport->setCallbacks({[this]() {
                                               transportStarted();
                                           },
                                           [this](std::string message) {
                                               messageReceived(std::move(message));
                                           },
                                           [this](Diagnostic diagnostic) {
                                               diagnosticReceived(std::move(diagnostic));
                                           },
                                           [this](Error error) {
                                               transportFailed(std::move(error));
                                           },
                                           [this](detail::ProcessExit status) {
                                               processExited(status);
                                           }});
        }

        ~Impl() {
            lifetime.reset();
            transport->setCallbacks({});
            transport->stop();
        }

        void start() {
            if (state != State::Stopped) {
                return;
            }

            const std::uint64_t generation = ++operationGeneration;
            transition(State::Starting);
            schedule([this, generation]() {
                if (state == State::Starting && generation == operationGeneration) {
                    transportActive = true;
                    transport->start();
                }
            });
        }

        void stop() {
            if (state == State::Stopped || state == State::Stopping) {
                return;
            }

            ++operationGeneration;
            pendingInitializeId.reset();

            transition(State::Stopping);
            if (transportActive) {
                transport->stop();
            } else {
                schedule([this]() {
                    if (state == State::Stopping && !transportActive) {
                        transition(State::Stopped);
                    }
                });
            }
        }

        State getState() const noexcept {
            return state;
        }

        void setOnStateChanged(Callbacks::StateChanged callback) {
            callbacks.onStateChanged = std::move(callback);
        }

        void setOnDiagnostic(Callbacks::DiagnosticReceived callback) {
            callbacks.onDiagnostic = std::move(callback);
        }

    private:
        struct Lifetime {};

        void schedule(std::function<void()> callback) {
            const std::weak_ptr<Lifetime> weakLifetime = lifetime;
            core::EventReceiver::atNextTick([weakLifetime, callback = std::move(callback)]() {
                if (weakLifetime.lock()) {
                    callback();
                }
            });
        }

        void transition(State next, std::optional<Error> error = std::nullopt) {
            if (next == state) {
                return;
            }

            const State previous = state;
            state = next;
            stateChanges.push_back({previous, next, std::move(error)});
            logScope.logger(logger::Logger::semanticSink())
                .trace("Codex app-server client: {} -> {}", stateName(previous), stateName(next));
            scheduleStateDispatch();
        }

        void scheduleStateDispatch() {
            if (stateDispatchScheduled || stateChanges.empty()) {
                return;
            }

            stateDispatchScheduled = true;
            schedule([this]() {
                dispatchOneStateChange();
            });
        }

        void dispatchOneStateChange() {
            stateDispatchScheduled = false;
            if (stateChanges.empty()) {
                return;
            }

            StateChange stateChange = std::move(stateChanges.front());
            stateChanges.pop_front();
            scheduleStateDispatch();

            const Callbacks::StateChanged callback = callbacks.onStateChanged;
            if (callback) {
                callback(stateChange);
            }
        }

        void transportStarted() {
            if (state != State::Starting) {
                transport->stop();
                return;
            }

            transportActive = true;
            transition(State::Initializing);

            const std::int64_t requestId = nextRequestId++;
            pendingInitializeId = requestId;
            if (!transport->send(detail::ProtocolCodec::initializeRequest(requestId, clientInfo))) {
                fail({Error::Category::Transport, ENOBUFS, "initialize request exceeds the bounded transport output queue"});
            }
        }

        void messageReceived(std::string wireMessage) {
            if (state != State::Initializing) {
                return;
            }

            std::string decodeError;
            std::optional<detail::ProtocolMessage> message = detail::ProtocolCodec::decode(wireMessage, decodeError);
            if (!message) {
                fail({Error::Category::Protocol, 0, std::move(decodeError)});
                return;
            }

            if (message->kind == detail::ProtocolMessage::Kind::Request) {
                diagnosticReceived({"Unhandled app-server request during initialization: " + message->method});
                return;
            }
            if (message->kind == detail::ProtocolMessage::Kind::Notification) {
                return;
            }
            if (!pendingInitializeId || message->id != pendingInitializeId) {
                return;
            }

            pendingInitializeId.reset();
            if (message->error) {
                fail({Error::Category::Initialization, message->error->code, message->error->message});
                return;
            }
            if (!message->hasResult || !message->initializeResult) {
                const std::string reason = message->resultError.empty() ? "initialize response has no result" : message->resultError;
                fail({Error::Category::Initialization, 0, reason});
                return;
            }
            if (!transport->send(detail::ProtocolCodec::initializedNotification())) {
                fail({Error::Category::Transport, ENOBUFS, "initialized notification exceeds the bounded transport output queue"});
                return;
            }

            transition(State::Ready);
        }

        void diagnosticReceived(Diagnostic diagnostic) {
            const Callbacks::DiagnosticReceived callback = callbacks.onDiagnostic;
            if (callback) {
                callback(diagnostic);
            }
        }

        void transportFailed(Error error) {
            if (state == State::Starting && error.category == Error::Category::Launch) {
                transportActive = false;
            }
            if (state != State::Stopping) {
                fail(std::move(error));
            }
        }

        void processExited(const detail::ProcessExit& status) {
            transportActive = false;
            pendingInitializeId.reset();

            if (state == State::Stopping) {
                transition(State::Stopped);
            } else if (state != State::Failed && state != State::Stopped) {
                const int code = status.exited ? status.exitCode : status.signalNumber;
                fail({Error::Category::Process, code, processExitMessage(status)});
            }
        }

        void fail(Error error) {
            if (state == State::Failed || state == State::Stopped || state == State::Stopping) {
                return;
            }

            ++operationGeneration;
            pendingInitializeId.reset();
            logScope.logger(logger::Logger::semanticSink()).error("Codex app-server client failed: {}", error.message);
            transition(State::Failed, error);
            transport->stop();
        }

        std::unique_ptr<detail::Transport> transport;
        ClientInfo clientInfo;
        Callbacks callbacks;

        State state = State::Stopped;
        bool transportActive = false;
        std::int64_t nextRequestId = 0;
        std::optional<std::int64_t> pendingInitializeId;
        std::uint64_t operationGeneration = 0;

        std::deque<StateChange> stateChanges;
        bool stateDispatchScheduled = false;
        std::shared_ptr<Lifetime> lifetime;

        logger::LogScopeOwner logScope;
    };

    AppServerClient::AppServerClient(std::unique_ptr<detail::Transport> transport, ClientInfo clientInfo)
        : impl(std::make_unique<Impl>(std::move(transport), std::move(clientInfo))) {
    }

    AppServerClient::~AppServerClient() = default;

    void AppServerClient::start() {
        impl->start();
    }

    void AppServerClient::stop() {
        impl->stop();
    }

    State AppServerClient::getState() const noexcept {
        return impl->getState();
    }

    bool AppServerClient::isReady() const noexcept {
        return getState() == State::Ready;
    }

    void AppServerClient::setOnStateChanged(Callbacks::StateChanged callback) {
        impl->setOnStateChanged(std::move(callback));
    }

    void AppServerClient::setOnDiagnostic(Callbacks::DiagnosticReceived callback) {
        impl->setOnDiagnostic(std::move(callback));
    }

} // namespace ai::openai::codex
