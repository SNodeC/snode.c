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
#include "ai/openai/codex/typed/Client.h"
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "log/LogScopeOwner.h"
#include "log/Logger.h"
#include "log/SemanticLogger.h"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <exception>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>

namespace ai::openai::codex {

    namespace {
        constexpr std::size_t MAX_PENDING_CLIENT_REQUESTS = 4096;
        constexpr std::size_t MAX_PENDING_SERVER_REQUESTS = 1024;
        constexpr std::size_t MAX_PRE_READY_MESSAGES = 1024;
        constexpr std::size_t MAX_DEFERRED_INCOMING_MESSAGES = 4096;

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

        void invokePublicCallback(const std::function<void()>& callback, const logger::BoundaryLogger& callbackLogger) noexcept {
            try {
                callback();
            } catch (const std::exception& exception) {
                try {
                    callbackLogger.error("Exception escaped Codex app-server public callback: {}", exception.what());
                } catch (...) {
                }
            } catch (...) {
                try {
                    callbackLogger.error("Unknown exception escaped Codex app-server public callback");
                } catch (...) {
                }
            }
        }

        bool isReservedInitializationMethod(std::string_view method) {
            return method == "initialize" || method == "initialized";
        }

        std::string serverRequestId(const ServerRequestId& id) {
            return std::visit(
                [](const auto& value) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::string>) {
                        return value;
                    } else {
                        return std::to_string(value);
                    }
                },
                id.value());
        }
    } // namespace

    class AppServerClient::Impl {
    public:
        Impl(std::unique_ptr<detail::Transport> transport, ClientInfo clientInfo)
            : transport(std::move(transport))
            , clientInfo(std::move(clientInfo))
            , lifetime(std::make_shared<Lifetime>())
            , rawProtocol(*this)
            , logScope(logger::LogOrigin::Framework, logger::LogBoundary::Connection, "ai.openai.codex") {
        }

        ~Impl() {
            lifetime.reset();
            transport->setCallbacks({});
            transport->stop();
        }

        void start() {
            if (state != State::Stopped || core::SNodeC::state() == core::State::STOPPING) {
                return;
            }

            const std::optional<std::uint64_t> allocatedGeneration = nextConnectionGeneration();
            if (!allocatedGeneration) {
                transition(State::Failed, Error{Error::Category::Capacity, EOVERFLOW, "app-server connection generation space exhausted"});
                return;
            }
            const std::uint64_t generation = *allocatedGeneration;
            initializeResult.reset();
            protocolSessionStarted = false;
            lifetime->callbackGeneration = generation;
            lifetime->protocolActive = true;
            installTransportCallbacks(generation);

            transition(State::Starting);
            schedule([this, generation]() {
                if (state == State::Starting && generation == connectionGeneration) {
                    if (core::SNodeC::state() == core::State::STOPPING) {
                        invalidateConnection("app-server start cancelled by event-loop shutdown");
                        transportActive = false;
                        transition(State::Stopped);
                        return;
                    }
                    transportActive = true;
                    transport->start();
                }
            });
        }

        void stop() {
            if (state == State::Stopped || state == State::Stopping) {
                return;
            }

            invalidateConnection("app-server connection stopped");
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

        RawProtocol& raw() noexcept {
            return rawProtocol;
        }

        const RawProtocol& raw() const noexcept {
            return rawProtocol;
        }

        void installTypedFacades(std::unique_ptr<typed::Threads> threads,
                                 std::unique_ptr<typed::Turns> turns,
                                 std::unique_ptr<typed::Events> events,
                                 std::unique_ptr<typed::Requests> requests) {
            typedThreads = std::move(threads);
            typedTurns = std::move(turns);
            typedEvents = std::move(events);
            typedRequests = std::move(requests);
        }

        typed::Threads& threads() noexcept {
            return *typedThreads;
        }

        const typed::Threads& threads() const noexcept {
            return *typedThreads;
        }

        typed::Turns& turns() noexcept {
            return *typedTurns;
        }

        const typed::Turns& turns() const noexcept {
            return *typedTurns;
        }

        typed::Events& events() noexcept {
            return *typedEvents;
        }

        const typed::Events& events() const noexcept {
            return *typedEvents;
        }

        typed::Requests& requests() noexcept {
            return *typedRequests;
        }

        const typed::Requests& requests() const noexcept {
            return *typedRequests;
        }

        std::optional<InitializeResult> getInitializeResult() const {
            return initializeResult;
        }

        void setOnStateChanged(Callbacks::StateChanged callback) {
            callbacks.onStateChanged = std::move(callback);
        }

        void setOnDiagnostic(Callbacks::DiagnosticReceived callback) {
            callbacks.onDiagnostic = std::move(callback);
        }

        RawProtocol::Submission request(std::string method, Json params, RawProtocol::ResponseHandler handler) {
            if (state != State::Ready || !lifetime->protocolActive) {
                return submissionFailure(Error::Category::InvalidState, EINVAL, "raw request requires a ready app-server connection");
            }
            if (method.empty()) {
                return submissionFailure(Error::Category::Protocol, EINVAL, "raw request method must not be empty");
            }
            if (isReservedInitializationMethod(method)) {
                return submissionFailure(Error::Category::InvalidState, EPERM, "raw callers cannot send initialization operations");
            }
            if (!handler) {
                return submissionFailure(Error::Category::Protocol, EINVAL, "raw request requires a response handler");
            }
            if (pendingRequests.size() >= MAX_PENDING_CLIENT_REQUESTS) {
                return submissionFailure(Error::Category::Capacity, ENOBUFS, "pending app-server request capacity reached");
            }

            const std::optional<std::int64_t> allocatedId = allocateRequestId();
            if (!allocatedId) {
                return submissionFailure(Error::Category::Capacity, EOVERFLOW, "app-server client request ID space exhausted");
            }

            const std::int64_t id = *allocatedId;
            const std::uint64_t generation = connectionGeneration;
            PendingRequest pending{std::move(method), std::move(handler), generation, nextSubmissionSequence++, false};
            const auto [iterator, inserted] = pendingRequests.emplace(id, std::move(pending));
            if (!inserted) {
                return submissionFailure(Error::Category::Protocol, EEXIST, "allocated app-server request ID is already pending");
            }

            std::string encodeError;
            std::optional<std::string> wireMessage = detail::ProtocolCodec::encodeRequest(id, iterator->second.method, params, encodeError);
            if (!wireMessage) {
                pendingRequests.erase(iterator);
                return submissionFailure(Error::Category::Enqueue, EINVAL, std::move(encodeError));
            }

            std::string enqueueError;
            const bool accepted = enqueue(std::move(*wireMessage), enqueueError);
            bool submissionAccepted = false;
            auto pendingIterator = pendingRequests.find(id);
            if (accepted && pendingIterator != pendingRequests.end() && state == State::Ready && lifetime->protocolActive &&
                generation == connectionGeneration) {
                pendingIterator->second.accepted = true;
                submissionAccepted = true;
                logScope.logger(logger::Logger::semanticSink())
                    .debug("request started: id={} method={}", id, pendingIterator->second.method);
            } else if (pendingIterator != pendingRequests.end()) {
                pendingRequests.erase(pendingIterator);
            }

            flushDeferredIncoming();

            if (!submissionAccepted) {
                if (enqueueError.empty()) {
                    enqueueError = accepted ? "app-server connection stopped while the request was being enqueued"
                                            : "app-server transport rejected the request";
                }
                return submissionFailure(accepted ? Error::Category::Cancelled : Error::Category::Enqueue,
                                         accepted ? ECANCELED : ENOBUFS,
                                         std::move(enqueueError));
            }

            return {{ClientRequestId(id)}, std::nullopt};
        }

        RawProtocol::SendResult notify(std::string method, Json params) {
            if (state != State::Ready || !lifetime->protocolActive) {
                return sendFailure(Error::Category::InvalidState, EINVAL, "raw notification requires a ready app-server connection");
            }
            if (method.empty()) {
                return sendFailure(Error::Category::Protocol, EINVAL, "raw notification method must not be empty");
            }
            if (isReservedInitializationMethod(method)) {
                return sendFailure(Error::Category::InvalidState, EPERM, "raw callers cannot send initialization operations");
            }

            std::string encodeError;
            std::optional<std::string> wireMessage = detail::ProtocolCodec::encodeNotification(method, params, encodeError);
            if (!wireMessage) {
                return sendFailure(Error::Category::Enqueue, EINVAL, std::move(encodeError));
            }

            std::string enqueueError;
            const std::uint64_t generation = connectionGeneration;
            const bool accepted = enqueue(std::move(*wireMessage), enqueueError);
            flushDeferredIncoming();
            if (!accepted) {
                if (enqueueError.empty()) {
                    enqueueError = "app-server transport rejected the notification";
                }
                return sendFailure(Error::Category::Enqueue, ENOBUFS, std::move(enqueueError));
            }
            if (state != State::Ready || !lifetime->protocolActive || generation != connectionGeneration) {
                return sendFailure(
                    Error::Category::Cancelled, ECANCELED, "app-server connection stopped while the notification was being enqueued");
            }
            return {true, std::nullopt};
        }

        RawProtocol::SendResult respond(const ServerRequestId& id, Json result) {
            return answerServerRequest(id, std::nullopt, std::move(result), std::nullopt);
        }

        RawProtocol::SendResult reject(const ServerRequestId& id, ProtocolError error) {
            return answerServerRequest(id, std::nullopt, nullptr, std::move(error));
        }

        RawProtocol::SendResult respondOwned(const ServerRequestId& id, ServerRequestToken token, Json result) {
            return answerServerRequest(id, token, std::move(result), std::nullopt);
        }

        RawProtocol::SendResult rejectOwned(const ServerRequestId& id, ServerRequestToken token, ProtocolError error) {
            return answerServerRequest(id, token, nullptr, std::move(error));
        }

        void setOnNotification(RawProtocol::NotificationHandler handler) {
            onNotification = std::move(handler);
        }

        void setOnServerRequest(RawProtocol::ServerRequestHandler handler) {
            onServerRequest = std::move(handler);
        }

        void setOnUnknownMessage(RawProtocol::UnknownMessageHandler handler) {
            onUnknownMessage = std::move(handler);
        }

        void setTypedNotificationDispatcher(RawProtocol::NotificationHandler handler) {
            typedNotificationDispatcher = std::move(handler);
        }

        void setTypedServerRequestDispatcher(RawProtocol::ServerRequestHandler handler) {
            typedServerRequestDispatcher = std::move(handler);
        }

    private:
        struct Lifetime {
            std::uint64_t callbackGeneration = 0;
            bool protocolActive = false;
        };

        struct PendingRequest {
            std::string method;
            RawProtocol::ResponseHandler handler;
            std::uint64_t connectionGeneration = 0;
            std::uint64_t submissionSequence = 0;
            bool accepted = false;
        };

        void installTransportCallbacks(std::uint64_t generation) {
            const std::weak_ptr<Lifetime> weakLifetime = lifetime;
            transport->setCallbacks({[this, weakLifetime, generation]() {
                                         const std::shared_ptr<Lifetime> locked = weakLifetime.lock();
                                         if (locked && locked->callbackGeneration == generation) {
                                             transportStarted(generation);
                                         }
                                     },
                                     [this, weakLifetime, generation](std::string message) {
                                         const std::shared_ptr<Lifetime> locked = weakLifetime.lock();
                                         if (locked && locked->callbackGeneration == generation) {
                                             messageReceived(generation, std::move(message));
                                         }
                                     },
                                     [this, weakLifetime, generation](Diagnostic diagnostic) {
                                         const std::shared_ptr<Lifetime> locked = weakLifetime.lock();
                                         if (locked && locked->callbackGeneration == generation) {
                                             diagnosticReceived(generation, std::move(diagnostic));
                                         }
                                     },
                                     [this, weakLifetime, generation](Error error) {
                                         const std::shared_ptr<Lifetime> locked = weakLifetime.lock();
                                         if (locked && locked->callbackGeneration == generation) {
                                             transportFailed(std::move(error));
                                         }
                                     },
                                     [this, weakLifetime, generation](detail::ProcessExit status) {
                                         const std::shared_ptr<Lifetime> locked = weakLifetime.lock();
                                         if (locked && locked->callbackGeneration == generation) {
                                             processExited(status);
                                         }
                                     }});
        }

        std::optional<std::uint64_t> nextConnectionGeneration() {
            if (connectionGeneration == std::numeric_limits<std::uint64_t>::max()) {
                return std::nullopt;
            }
            return ++connectionGeneration;
        }

        std::optional<std::int64_t> allocateRequestId() {
            if (requestIdsExhausted) {
                return std::nullopt;
            }

            const std::int64_t id = nextRequestId;
            if (nextRequestId == std::numeric_limits<std::int64_t>::max()) {
                requestIdsExhausted = true;
            } else {
                ++nextRequestId;
            }
            return id;
        }

        void schedule(std::function<void()> callback) {
            const std::weak_ptr<Lifetime> weakLifetime = lifetime;
            core::EventReceiver::atNextTick([weakLifetime, callback = std::move(callback)]() {
                if (weakLifetime.lock()) {
                    callback();
                }
            });
        }

        void schedulePublic(std::function<void()> callback) {
            const std::weak_ptr<Lifetime> weakLifetime = lifetime;
            const logger::BoundaryLogger callbackLogger = logScope.logger(logger::Logger::semanticSink());
            core::EventReceiver::atNextTick([weakLifetime, callback = std::move(callback), callbackLogger]() {
                if (weakLifetime.lock()) {
                    invokePublicCallback(callback, callbackLogger);
                }
            });
        }

        void scheduleProtocol(std::uint64_t generation, std::function<void()> callback) {
            const std::weak_ptr<Lifetime> weakLifetime = lifetime;
            const logger::BoundaryLogger callbackLogger = logScope.logger(logger::Logger::semanticSink());
            core::EventReceiver::atNextTick([weakLifetime, generation, callback = std::move(callback), callbackLogger]() {
                const std::shared_ptr<Lifetime> locked = weakLifetime.lock();
                if (locked && locked->protocolActive && locked->callbackGeneration == generation) {
                    invokePublicCallback(callback, callbackLogger);
                }
            });
        }

        void scheduleForConnection(std::uint64_t generation, std::function<void()> callback) {
            const std::weak_ptr<Lifetime> weakLifetime = lifetime;
            const logger::BoundaryLogger callbackLogger = logScope.logger(logger::Logger::semanticSink());
            core::EventReceiver::atNextTick([weakLifetime, generation, callback = std::move(callback), callbackLogger]() {
                const std::shared_ptr<Lifetime> locked = weakLifetime.lock();
                if (locked && locked->callbackGeneration == generation) {
                    invokePublicCallback(callback, callbackLogger);
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
                const logger::BoundaryLogger callbackLogger = logScope.logger(logger::Logger::semanticSink());
                invokePublicCallback(
                    [callback, stateChange = std::move(stateChange)]() {
                        callback(stateChange);
                    },
                    callbackLogger);
            }
        }

        bool enqueue(std::string wireMessage, std::string& errorMessage) {
            ++transportSendDepth;
            bool accepted = false;
            try {
                accepted = transport->send(std::move(wireMessage));
            } catch (const std::exception& exception) {
                errorMessage = std::string("app-server transport enqueue threw an exception: ") + exception.what();
            } catch (...) {
                errorMessage = "app-server transport enqueue threw an unknown exception";
            }
            --transportSendDepth;
            return accepted;
        }

        void flushDeferredIncoming() {
            if (transportSendDepth != 0 || flushingDeferredIncoming) {
                return;
            }

            flushingDeferredIncoming = true;
            while (!deferredIncoming.empty()) {
                DeferredIncoming incoming = std::move(deferredIncoming.front());
                deferredIncoming.pop_front();
                if (incoming.generation == connectionGeneration && lifetime->protocolActive) {
                    routeDecodedMessage(incoming.generation, std::move(incoming.message));
                }
            }
            flushingDeferredIncoming = false;
        }

        void transportStarted(std::uint64_t generation) {
            if (state != State::Starting || generation != connectionGeneration || !lifetime->protocolActive) {
                transport->stop();
                return;
            }

            transportActive = true;
            transition(State::Initializing);

            const std::optional<std::int64_t> requestId = allocateRequestId();
            if (!requestId) {
                fail({Error::Category::Capacity, EOVERFLOW, "app-server initialize request ID space exhausted"});
                return;
            }

            pendingInitializeId = *requestId;
            std::string encodeError;
            std::optional<std::string> wireMessage =
                detail::ProtocolCodec::encodeRequest(*requestId, "initialize", initializeParams(), encodeError);
            if (!wireMessage) {
                pendingInitializeId.reset();
                fail({Error::Category::Initialization, EINVAL, std::move(encodeError)});
                return;
            }

            std::string enqueueError;
            const bool accepted = enqueue(std::move(*wireMessage), enqueueError);
            if (!accepted) {
                pendingInitializeId.reset();
                fail({Error::Category::Transport,
                      ENOBUFS,
                      enqueueError.empty() ? "initialize request exceeds the bounded transport output queue" : std::move(enqueueError)});
                flushDeferredIncoming();
                return;
            }
            flushDeferredIncoming();
        }

        Json initializeParams() const {
            return {{"clientInfo", {{"name", clientInfo.name}, {"title", clientInfo.title}, {"version", clientInfo.version}}}};
        }

        void messageReceived(std::uint64_t generation, std::string wireMessage) {
            if (!lifetime->protocolActive || generation != connectionGeneration ||
                (state != State::Initializing && state != State::Ready)) {
                return;
            }

            std::string decodeError;
            std::optional<detail::ProtocolMessage> message = detail::ProtocolCodec::decode(wireMessage, decodeError);
            if (!message) {
                fail({Error::Category::Protocol, 0, std::move(decodeError)});
                return;
            }

            if (transportSendDepth != 0 || flushingDeferredIncoming) {
                if (deferredIncoming.size() >= MAX_DEFERRED_INCOMING_MESSAGES) {
                    fail({Error::Category::Protocol, ENOBUFS, "deferred app-server input queue capacity exceeded"});
                    return;
                }
                if (state == State::Initializing && !isInitializeResponse(*message)) {
                    std::size_t queuedPreReadyMessages = preReadyMessages.size();
                    for (const DeferredIncoming& incoming : deferredIncoming) {
                        if (!isInitializeResponse(incoming.message)) {
                            ++queuedPreReadyMessages;
                        }
                    }
                    if (queuedPreReadyMessages >= MAX_PRE_READY_MESSAGES) {
                        fail({Error::Category::Protocol, ENOBUFS, "pre-ready app-server message queue capacity exceeded"});
                        return;
                    }
                }
                deferredIncoming.push_back({generation, std::move(*message)});
                return;
            }
            routeDecodedMessage(generation, std::move(*message));
        }

        void routeDecodedMessage(std::uint64_t generation, detail::ProtocolMessage message) {
            if (!lifetime->protocolActive || generation != connectionGeneration) {
                return;
            }
            if (state == State::Initializing) {
                if (isInitializeResponse(message)) {
                    handleInitializeResponse(generation, std::move(message));
                } else if (preReadyMessages.size() >= MAX_PRE_READY_MESSAGES) {
                    fail({Error::Category::Protocol, ENOBUFS, "pre-ready app-server message queue capacity exceeded"});
                } else {
                    preReadyMessages.push_back(std::move(message));
                }
                return;
            }
            if (state == State::Ready) {
                dispatchMessage(generation, std::move(message));
            }
        }

        bool isInitializeResponse(const detail::ProtocolMessage& message) const {
            if (message.kind != detail::ProtocolMessage::Kind::Response || !pendingInitializeId || !message.id) {
                return false;
            }
            const auto integerId = std::get_if<std::int64_t>(&*message.id);
            return integerId && *integerId == *pendingInitializeId;
        }

        void handleInitializeResponse(std::uint64_t generation, detail::ProtocolMessage message) {
            pendingInitializeId.reset();
            if (message.error) {
                Error initializationError{Error::Category::Initialization, 0, message.error->message};
                if (message.error->code >= std::numeric_limits<int>::min() && message.error->code <= std::numeric_limits<int>::max()) {
                    initializationError.code = static_cast<int>(message.error->code);
                } else {
                    initializationError.message += " (remote error code " + std::to_string(message.error->code) + ")";
                }
                fail(std::move(initializationError));
                return;
            }

            std::string resultError;
            std::optional<InitializeResult> decodedInitializeResult = detail::decodeInitializeResult(message.result, resultError);
            if (!decodedInitializeResult) {
                fail({Error::Category::Initialization, 0, std::move(resultError)});
                return;
            }

            std::string encodeError;
            std::optional<std::string> initialized = detail::ProtocolCodec::encodeNotification("initialized", Json::object(), encodeError);
            if (!initialized) {
                fail({Error::Category::Initialization, EINVAL, std::move(encodeError)});
                return;
            }

            std::string enqueueError;
            const bool accepted = enqueue(std::move(*initialized), enqueueError);
            if (!accepted) {
                fail({Error::Category::Transport,
                      ENOBUFS,
                      enqueueError.empty() ? "initialized notification exceeds the bounded transport output queue"
                                           : std::move(enqueueError)});
                flushDeferredIncoming();
                return;
            }
            if (state != State::Initializing || generation != connectionGeneration || !lifetime->protocolActive) {
                flushDeferredIncoming();
                return;
            }

            initializeResult = std::move(decodedInitializeResult);
            transition(State::Ready);
            protocolSessionStarted = true;
            logScope.logger(logger::Logger::semanticSink()).info("app-server session started");

            std::deque<detail::ProtocolMessage> queuedMessages = std::move(preReadyMessages);
            preReadyMessages.clear();
            for (detail::ProtocolMessage& queuedMessage : queuedMessages) {
                if (state != State::Ready || generation != connectionGeneration || !lifetime->protocolActive) {
                    break;
                }
                dispatchMessage(generation, std::move(queuedMessage));
            }
            flushDeferredIncoming();
        }

        void dispatchMessage(std::uint64_t generation, detail::ProtocolMessage message) {
            switch (message.kind) {
                case detail::ProtocolMessage::Kind::Response:
                    dispatchResponse(generation, std::move(message));
                    break;
                case detail::ProtocolMessage::Kind::Request:
                    dispatchServerRequest(generation, std::move(message));
                    break;
                case detail::ProtocolMessage::Kind::Notification:
                    dispatchNotification(generation, std::move(message));
                    break;
            }
        }

        void dispatchResponse(std::uint64_t generation, detail::ProtocolMessage message) {
            if (!message.id) {
                fail({Error::Category::Protocol, EINVAL, "decoded app-server response has no ID"});
                return;
            }

            const auto integerId = std::get_if<std::int64_t>(&*message.id);
            if (!integerId) {
                dispatchUnknown(generation, std::move(message.raw), "string response ID cannot match an SNode.C client request");
                return;
            }

            auto pending = pendingRequests.find(*integerId);
            if (pending == pendingRequests.end() || !pending->second.accepted || pending->second.connectionGeneration != generation) {
                dispatchUnknown(generation, std::move(message.raw), "response ID does not match a pending SNode.C client request");
                return;
            }

            PendingRequest request = std::move(pending->second);
            pendingRequests.erase(pending);

            logScope.logger(logger::Logger::semanticSink())
                .debug("request {}: id={} method={}", message.error ? "failed" : "completed", *integerId, request.method);

            Response response{
                ClientRequestId(*integerId), std::move(request.method), Response::Kind::Result, nullptr, std::nullopt, std::nullopt};
            if (message.error) {
                response.kind = Response::Kind::RemoteError;
                response.remoteError = std::move(message.error);
            } else {
                response.kind = Response::Kind::Result;
                response.result = std::move(message.result);
            }

            // Normal remote completions belong to the generation that received them.
            scheduleProtocol(generation, [handler = std::move(request.handler), response = std::move(response)]() {
                handler(response);
            });
        }

        void dispatchNotification(std::uint64_t generation, detail::ProtocolMessage message) {
            const RawProtocol::NotificationHandler typedHandler = typedNotificationDispatcher;
            const RawProtocol::NotificationHandler rawHandler = onNotification;
            if (!typedHandler && !rawHandler) {
                return;
            }

            const auto notification = std::make_shared<const Notification>(
                Notification{std::move(message.method), std::move(message.params), std::move(message.raw)});
            if (typedHandler) {
                scheduleProtocol(generation, [typedHandler, notification]() {
                    typedHandler(*notification);
                });
            }
            if (rawHandler) {
                scheduleProtocol(generation, [rawHandler, notification]() {
                    rawHandler(*notification);
                });
            }
        }

        void dispatchServerRequest(std::uint64_t generation, detail::ProtocolMessage message) {
            if (!message.id) {
                fail({Error::Category::Protocol, EINVAL, "decoded app-server request has no ID"});
                return;
            }

            ServerRequestId id(*message.id);
            if (pendingServerRequests.contains(id)) {
                fail({Error::Category::Protocol, EEXIST, "duplicate pending app-server request ID"});
                return;
            }
            if (pendingServerRequests.size() >= MAX_PENDING_SERVER_REQUESTS) {
                fail({Error::Category::Protocol, ENOBUFS, "pending app-server server-request capacity exceeded"});
                return;
            }
            if (serverRequestTokensExhausted) {
                fail({Error::Category::Capacity, EOVERFLOW, "app-server server-request token space exhausted"});
                return;
            }

            const ServerRequestToken token(nextServerRequestToken);
            if (nextServerRequestToken == std::numeric_limits<std::uint64_t>::max()) {
                serverRequestTokensExhausted = true;
            } else {
                ++nextServerRequestToken;
            }

            ServerRequest request{id, std::move(message.method), std::move(message.params), std::move(message.raw), token};
            const auto [iterator, inserted] = pendingServerRequests.emplace(id, std::move(request));
            if (!inserted) {
                fail({Error::Category::Protocol, EEXIST, "duplicate pending app-server request ID"});
                return;
            }

            logScope.logger(logger::Logger::semanticSink())
                .debug("request started: id={} method={}", serverRequestId(id), iterator->second.method);

            const RawProtocol::ServerRequestHandler typedHandler = typedServerRequestDispatcher;
            const RawProtocol::ServerRequestHandler rawHandler = onServerRequest;
            if (typedHandler || rawHandler) {
                const auto callbackRequest = std::make_shared<const ServerRequest>(iterator->second);
                if (typedHandler) {
                    scheduleProtocol(generation, [typedHandler, callbackRequest]() {
                        typedHandler(*callbackRequest);
                    });
                }
                if (rawHandler) {
                    scheduleProtocol(generation, [rawHandler, callbackRequest]() {
                        rawHandler(*callbackRequest);
                    });
                }
            }
        }

        void dispatchUnknown(std::uint64_t generation, Json raw, std::string reason) {
            const RawProtocol::UnknownMessageHandler handler = onUnknownMessage;
            if (!handler) {
                return;
            }

            UnknownMessage message{std::move(raw), std::move(reason)};
            scheduleProtocol(generation, [handler, message = std::move(message)]() {
                handler(message);
            });
        }

        RawProtocol::SendResult answerServerRequest(const ServerRequestId& id,
                                                    std::optional<ServerRequestToken> token,
                                                    Json result,
                                                    std::optional<ProtocolError> protocolError) {
            if (state != State::Ready || !lifetime->protocolActive) {
                return sendFailure(Error::Category::InvalidState, EINVAL, "answering a server request requires a ready connection");
            }

            const auto pending = pendingServerRequests.find(id);
            if (pending == pendingServerRequests.end()) {
                return sendFailure(Error::Category::InvalidState, ENOENT, "server request ID is not currently pending");
            }
            if (token.has_value() && pending->second.token != *token) {
                return sendFailure(
                    Error::Category::InvalidState, ESTALE, "server request ownership token does not match the currently pending request");
            }

            std::string encodeError;
            std::optional<std::string> wireMessage =
                protocolError ? detail::ProtocolCodec::encodeErrorResponse(id.value(), *protocolError, encodeError)
                              : detail::ProtocolCodec::encodeSuccessResponse(id.value(), result, encodeError);
            if (!wireMessage) {
                return sendFailure(Error::Category::Enqueue, EINVAL, std::move(encodeError));
            }

            std::string enqueueError;
            const std::uint64_t generation = connectionGeneration;
            const bool accepted = enqueue(std::move(*wireMessage), enqueueError);

            if (!accepted) {
                flushDeferredIncoming();
                if (enqueueError.empty()) {
                    enqueueError = "app-server transport rejected the server-request response";
                }
                return sendFailure(Error::Category::Enqueue, ENOBUFS, std::move(enqueueError));
            }
            if (state != State::Ready || !lifetime->protocolActive || generation != connectionGeneration) {
                flushDeferredIncoming();
                return sendFailure(Error::Category::Cancelled,
                                   ECANCELED,
                                   "app-server connection stopped while the server-request response was being enqueued");
            }
            const std::string method = pending->second.method;
            pendingServerRequests.erase(id);
            logScope.logger(logger::Logger::semanticSink())
                .debug("request {}: id={} method={}", protocolError ? "failed" : "completed", serverRequestId(id), method);
            flushDeferredIncoming();
            return {true, std::nullopt};
        }

        static RawProtocol::Submission submissionFailure(Error::Category category, int code, std::string message) {
            return {std::nullopt, Error{category, code, std::move(message)}};
        }

        static RawProtocol::SendResult sendFailure(Error::Category category, int code, std::string message) {
            return {false, Error{category, code, std::move(message)}};
        }

        void diagnosticReceived(std::uint64_t generation, Diagnostic diagnostic) {
            const Callbacks::DiagnosticReceived callback = callbacks.onDiagnostic;
            if (callback) {
                scheduleForConnection(generation, [callback, diagnostic = std::move(diagnostic)]() {
                    callback(diagnostic);
                });
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

            if (state == State::Stopping || core::SNodeC::state() == core::State::STOPPING) {
                invalidateConnection("app-server process exited");
                if (state != State::Stopping) {
                    transition(State::Stopping);
                }
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

            logScope.logger(logger::Logger::semanticSink()).error("Codex app-server client failed: {}", error.message);
            if (!protocolSessionStarted && (state == State::Starting || state == State::Initializing)) {
                logScope.logger(logger::Logger::semanticSink()).debug("app-server session start failed");
            }
            invalidateConnection(error.message, "failed");
            transition(State::Failed, error);
            transport->stop();
        }

        void invalidateConnection(const std::string& reason, const char* serverRequestOutcome = "cancelled") {
            if (lifetime->protocolActive) {
                lifetime->protocolActive = false;
            }
            pendingInitializeId.reset();
            preReadyMessages.clear();
            deferredIncoming.clear();
            for (const auto& [id, pending] : pendingServerRequests) {
                logScope.logger(logger::Logger::semanticSink())
                    .debug("request {}: id={} method={}", serverRequestOutcome, serverRequestId(id), pending.method);
            }
            pendingServerRequests.clear();

            std::vector<PendingCancellation> cancellations;
            cancellations.reserve(pendingRequests.size());
            for (auto& [id, pending] : pendingRequests) {
                if (pending.accepted) {
                    logScope.logger(logger::Logger::semanticSink()).debug("request cancelled: id={} method={}", id, pending.method);
                    cancellations.push_back({id, std::move(pending.method), std::move(pending.handler), pending.submissionSequence});
                }
            }
            pendingRequests.clear();
            std::sort(cancellations.begin(), cancellations.end(), [](const PendingCancellation& left, const PendingCancellation& right) {
                return left.submissionSequence < right.submissionSequence;
            });

            for (PendingCancellation& cancellation : cancellations) {
                Response response{ClientRequestId(cancellation.id),
                                  std::move(cancellation.method),
                                  Response::Kind::Result,
                                  nullptr,
                                  std::nullopt,
                                  std::nullopt};
                response.kind = Response::Kind::Cancelled;
                response.localError = Error{Error::Category::Cancelled,
                                            ECANCELED,
                                            reason.empty() ? "app-server request cancelled" : "app-server request cancelled: " + reason};
                // Invalidation-created cancellations remain lifetime-bound so an explicit restart cannot suppress them.
                schedulePublic([handler = std::move(cancellation.handler), response = std::move(response)]() {
                    handler(response);
                });
            }

            stopProtocolSession();
        }

        void stopProtocolSession() {
            if (protocolSessionStarted) {
                logScope.logger(logger::Logger::semanticSink()).info("app-server session stopped");
                protocolSessionStarted = false;
            }
        }

        struct DeferredIncoming {
            std::uint64_t generation = 0;
            detail::ProtocolMessage message;
        };

        struct PendingCancellation {
            std::int64_t id = 0;
            std::string method;
            RawProtocol::ResponseHandler handler;
            std::uint64_t submissionSequence = 0;
        };

        std::unique_ptr<detail::Transport> transport;
        ClientInfo clientInfo;
        Callbacks callbacks;

        State state = State::Stopped;
        bool transportActive = false;
        bool protocolSessionStarted = false;
        std::int64_t nextRequestId = 0;
        bool requestIdsExhausted = false;
        std::optional<std::int64_t> pendingInitializeId;
        std::uint64_t connectionGeneration = 0;
        std::uint64_t nextSubmissionSequence = 0;
        std::uint64_t nextServerRequestToken = 1;
        bool serverRequestTokensExhausted = false;

        std::map<std::int64_t, PendingRequest> pendingRequests;
        std::map<ServerRequestId, ServerRequest> pendingServerRequests;
        std::deque<detail::ProtocolMessage> preReadyMessages;
        std::deque<DeferredIncoming> deferredIncoming;
        std::size_t transportSendDepth = 0;
        bool flushingDeferredIncoming = false;

        std::optional<InitializeResult> initializeResult;
        RawProtocol::NotificationHandler onNotification;
        RawProtocol::ServerRequestHandler onServerRequest;
        RawProtocol::UnknownMessageHandler onUnknownMessage;
        RawProtocol::NotificationHandler typedNotificationDispatcher;
        RawProtocol::ServerRequestHandler typedServerRequestDispatcher;

        std::deque<StateChange> stateChanges;
        bool stateDispatchScheduled = false;
        std::shared_ptr<Lifetime> lifetime;
        RawProtocol rawProtocol;
        std::unique_ptr<typed::Threads> typedThreads;
        std::unique_ptr<typed::Turns> typedTurns;
        std::unique_ptr<typed::Events> typedEvents;
        std::unique_ptr<typed::Requests> typedRequests;

        logger::LogScopeOwner logScope;
    };

    AppServerClient::RawProtocol::RawProtocol(Impl& impl) noexcept
        : impl(&impl) {
    }

    AppServerClient::RawProtocol::Submission::operator bool() const noexcept {
        return id.has_value() && !error.has_value();
    }

    AppServerClient::RawProtocol::SendResult::operator bool() const noexcept {
        return accepted && !error.has_value();
    }

    AppServerClient::RawProtocol::Submission
    AppServerClient::RawProtocol::request(std::string method, Json params, ResponseHandler handler) {
        return impl->request(std::move(method), std::move(params), std::move(handler));
    }

    AppServerClient::RawProtocol::SendResult AppServerClient::RawProtocol::notify(std::string method, Json params) {
        return impl->notify(std::move(method), std::move(params));
    }

    AppServerClient::RawProtocol::SendResult AppServerClient::RawProtocol::respond(const ServerRequestId& id, Json result) {
        return impl->respond(id, std::move(result));
    }

    AppServerClient::RawProtocol::SendResult AppServerClient::RawProtocol::reject(const ServerRequestId& id, ProtocolError error) {
        return impl->reject(id, std::move(error));
    }

    void AppServerClient::RawProtocol::setOnNotification(NotificationHandler handler) {
        impl->setOnNotification(std::move(handler));
    }

    void AppServerClient::RawProtocol::setOnServerRequest(ServerRequestHandler handler) {
        impl->setOnServerRequest(std::move(handler));
    }

    void AppServerClient::RawProtocol::setOnUnknownMessage(UnknownMessageHandler handler) {
        impl->setOnUnknownMessage(std::move(handler));
    }

    void AppServerClient::RawProtocol::setTypedNotificationDispatcher(NotificationHandler handler) {
        impl->setTypedNotificationDispatcher(std::move(handler));
    }

    void AppServerClient::RawProtocol::setTypedServerRequestDispatcher(ServerRequestHandler handler) {
        impl->setTypedServerRequestDispatcher(std::move(handler));
    }

    AppServerClient::RawProtocol::SendResult
    AppServerClient::RawProtocol::respondOwned(const ServerRequestId& id, ServerRequestToken token, Json result) {
        return impl->respondOwned(id, token, std::move(result));
    }

    AppServerClient::RawProtocol::SendResult
    AppServerClient::RawProtocol::rejectOwned(const ServerRequestId& id, ServerRequestToken token, ProtocolError error) {
        return impl->rejectOwned(id, token, std::move(error));
    }

    AppServerClient::AppServerClient(std::unique_ptr<detail::Transport> transport, ClientInfo clientInfo)
        : impl(std::make_unique<Impl>(std::move(transport), std::move(clientInfo))) {
        impl->installTypedFacades(std::unique_ptr<typed::Threads>(new typed::Threads(impl->raw())),
                                  std::unique_ptr<typed::Turns>(new typed::Turns(impl->raw())),
                                  std::unique_ptr<typed::Events>(new typed::Events(impl->raw())),
                                  std::unique_ptr<typed::Requests>(new typed::Requests(impl->raw())));
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

    AppServerClient::RawProtocol& AppServerClient::raw() noexcept {
        return impl->raw();
    }

    const AppServerClient::RawProtocol& AppServerClient::raw() const noexcept {
        return impl->raw();
    }

    typed::Threads& AppServerClient::threads() noexcept {
        return impl->threads();
    }

    const typed::Threads& AppServerClient::threads() const noexcept {
        return impl->threads();
    }

    typed::Turns& AppServerClient::turns() noexcept {
        return impl->turns();
    }

    const typed::Turns& AppServerClient::turns() const noexcept {
        return impl->turns();
    }

    typed::Events& AppServerClient::events() noexcept {
        return impl->events();
    }

    const typed::Events& AppServerClient::events() const noexcept {
        return impl->events();
    }

    typed::Requests& AppServerClient::requests() noexcept {
        return impl->requests();
    }

    const typed::Requests& AppServerClient::requests() const noexcept {
        return impl->requests();
    }

    std::optional<InitializeResult> AppServerClient::getInitializeResult() const {
        return impl->getInitializeResult();
    }

    void AppServerClient::setOnStateChanged(Callbacks::StateChanged callback) {
        impl->setOnStateChanged(std::move(callback));
    }

    void AppServerClient::setOnDiagnostic(Callbacks::DiagnosticReceived callback) {
        impl->setOnDiagnostic(std::move(callback));
    }

} // namespace ai::openai::codex
