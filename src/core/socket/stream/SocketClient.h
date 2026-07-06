/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef CORE_SOCKET_STREAM_SOCKETCLIENT_H
#define CORE_SOCKET_STREAM_SOCKETCLIENT_H

#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/socket/Socket.h"                      // IWYU pragma: export
#include "core/socket/State.h"                       // IWYU pragma: export
#include "core/socket/stream/ClientFlowController.h" // IWYU pragma: export
#include "core/timer/Timer.h"
#include "log/LogScopeOwner.h"
#include "log/SemanticLogger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/Random.h"

#include <algorithm>
#include <functional> // IWYU pragma: export
#include <optional>
#include <type_traits> // IWYU pragma: export

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    /** Sequence diagram showing how a connect to a peer is performed.
    @startuml
    !include core/socket/stream/pu/SocketClient.pu
    @enduml
    */
    template <typename SocketConnectorT, typename SocketContextFactoryT, typename... Args>
        requires std::is_base_of_v<core::eventreceiver::ConnectEventReceiver, SocketConnectorT> &&
                 std::is_base_of_v<core::socket::stream::SocketContextFactory, SocketContextFactoryT>
    class SocketClient : public core::socket::Socket<typename SocketConnectorT::Config> {
    private:
        using SocketConnector = SocketConnectorT;
        using SocketContextFactory = SocketContextFactoryT;

        using Super = core::socket::Socket<typename SocketConnector::Config>;

    public:
        using SocketConnection = typename SocketConnector::SocketConnection;
        using SocketAddress = typename SocketConnector::SocketAddress;
        using Config = typename SocketConnector::Config;

    private:
        struct Context {
            Context(const std::shared_ptr<Config>& config,
                    const std::shared_ptr<SocketContextFactory>& socketContextFactory,
                    const std::function<void(SocketConnection*)>& onConnect,
                    const std::function<void(SocketConnection*)>& onConnected,
                    const std::function<void(SocketConnection*)>& onDisconnect)
                : flowController(config->getInstanceName(),
                                 [config](const std::function<void()>& callback) {
                                     config->setOnDestroy([callback](auto*) {
                                         callback();
                                     });
                                 })
                , socketContextFactory(socketContextFactory)
                , onConnect(onConnect)
                , onConnected(onConnected)
                , onDisconnect(onDisconnect) {
            }

            ClientFlowController flowController;

            std::shared_ptr<SocketContextFactory> socketContextFactory;

            std::function<void(SocketConnection*)> onConnect;
            std::function<void(SocketConnection*)> onConnected;
            std::function<void(SocketConnection*)> onDisconnect;
        };

        SocketClient(const std::shared_ptr<Config>& config, const std::shared_ptr<Context>& sharedContext)
            : Super(config)
            , logScope(makeLogScope(config->getInstanceName()))
            , sharedContext(sharedContext) {
        }

    public:
        SocketClient(const std::string& name,
                     const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect,
                     Args&&... args)
            : Super(name)
            , logScope(makeLogScope(name))
            , sharedContext(std::make_shared<Context>(
                  this->config,
                  std::make_shared<SocketContextFactory>(std::forward<Args>(args)...),
                  [onConnect, log = this->log()](SocketConnection* socketConnection) { // onConnect
                      log.debug("{}: OnConnect", socketConnection->getConnectionName());

                      log.debug("Local: {}", socketConnection->getLocalAddress().toString());
                      log.debug("Peer: {}", socketConnection->getRemoteAddress().toString());

                      if (onConnect) {
                          onConnect(socketConnection);
                      }
                  },
                  [onConnected, log = this->log()](SocketConnection* socketConnection) { // onConnected
                      log.debug("{}: OnConnected", socketConnection->getConnectionName());

                      log.debug("Local: {}", socketConnection->getLocalAddress().toString());
                      log.debug("Peer: {}", socketConnection->getRemoteAddress().toString());

                      if (onConnected) {
                          onConnected(socketConnection);
                      }
                  },
                  [onDisconnect, log = this->log()](SocketConnection* socketConnection) { // onDisconnect
                      log.debug("{}: OnDisconnect", socketConnection->getConnectionName());

                      log.debug("Local: {}", socketConnection->getLocalAddress().toString());
                      log.debug("Peer: {}", socketConnection->getRemoteAddress().toString());

                      log.debug("Online Since: {}", socketConnection->getOnlineSince());
                      log.debug("Online Duration: {}", socketConnection->getOnlineDuration());

                      log.debug("Total Queued: {}", socketConnection->getTotalQueued());
                      log.debug("Total Sent: {}", socketConnection->getTotalSent());
                      log.debug("Write Delta: {}", socketConnection->getTotalQueued() - socketConnection->getTotalSent());
                      log.debug("Total Read: {}", socketConnection->getTotalRead());
                      log.debug("Total Processed: {}", socketConnection->getTotalProcessed());
                      log.debug("Read Delta: {}", socketConnection->getTotalRead() - socketConnection->getTotalProcessed());

                      if (onDisconnect) {
                          onDisconnect(socketConnection);
                      }
                  })) {
        }

        SocketClient(const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect,
                     Args&&... args)
            : SocketClient("", onConnect, onConnected, onDisconnect, std::forward<Args>(args)...) {
        }

        SocketClient(const std::string& name, Args&&... args)
            : SocketClient(name, {}, {}, {}, std::forward<Args>(args)...) {
        }

        explicit SocketClient(Args&&... args)
            : SocketClient("", std::forward<Args>(args)...) {
        }

    private:
        const SocketClient& realConnect(const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
                                        unsigned int tries,
                                        double retryTimeoutScale) const {
            sharedContext->flowController.startFlow(
                [config = this->config, sharedContext = this->sharedContext, log = this->log(), onStatus, tries, retryTimeoutScale] {
                    if (config->Instance::getParent() != nullptr || !config->Instance::getRequired()) {
                        log.debug("Initiating connect");

                        if (core::SNodeC::state() == core::State::RUNNING || core::SNodeC::state() == core::State::INITIALIZED) {
                            new SocketConnector(
                                sharedContext->socketContextFactory,
                                sharedContext->onConnect,
                                sharedContext->onConnected,
                                [config, sharedContext, log, onStatus](SocketConnection* socketConnection) {
                                    sharedContext->onDisconnect(socketConnection);

                                    if (config->getReconnect() && sharedContext->flowController.isReconnectEnabled() &&
                                        core::eventLoopState() == core::State::RUNNING) {
                                        double relativeReconnectTimeout = config->getReconnectTime();

                                        log.info("Reconnect in {} seconds", relativeReconnectTimeout);

                                        sharedContext->flowController.armReconnectTimer(
                                            relativeReconnectTimeout, [config, sharedContext, log, /*generation,*/ onStatus]() {
                                                if (!sharedContext->flowController.isReconnectEnabled()) {
                                                    return;
                                                }
                                                if (config->getReconnect()) {
                                                    sharedContext->flowController.reportFlowReconnect();
                                                    SocketClient(config, sharedContext).realConnect(onStatus, 0, config->getRetryBase());
                                                } else {
                                                    log.info("Reconnect disabled during wait");
                                                }
                                            });
                                    }
                                },
                                [sharedContext](core::eventreceiver::ConnectEventReceiver* connectEventReceiver) {
                                    sharedContext->flowController.observeConnectEventReceiver(connectEventReceiver);
                                },
                                [config, sharedContext, log, onStatus, tries, retryTimeoutScale](const SocketAddress& socketAddress,
                                                                                                 core::socket::State state) {
                                    const bool retryFlag = (state & core::socket::State::NO_RETRY) == 0;
                                    state &= ~core::socket::State::NO_RETRY;
                                    onStatus(socketAddress, state);

                                    if (retryFlag && config->getRetry() // Shall we potentially retry? In case are the ...
                                        && sharedContext->flowController.isRetryEnabled() &&
                                        (config->getRetryTries() == 0 ||
                                         tries < config->getRetryTries()) // ... limits not reached and has an ...
                                        && (state == core::socket::State::ERROR ||
                                            (state == core::socket::State::FATAL && config->getRetryOnFatal()))) { // error occurred?
                                        double relativeRetryTimeout =
                                            config->getRetryLimit() > 0
                                                ? std::min<double>(config->getRetryTimeout() * retryTimeoutScale, config->getRetryLimit())
                                                : config->getRetryTimeout() * retryTimeoutScale;
                                        relativeRetryTimeout -=
                                            utils::Random::getInRange(-config->getRetryJitter(), config->getRetryJitter()) *
                                            relativeRetryTimeout / 100.;

                                        log.info("Retry connect in {} seconds", relativeRetryTimeout);

                                        sharedContext->flowController.armRetryTimer(
                                            relativeRetryTimeout,
                                            [config,
                                             sharedContext,
                                             log,
                                             /*generation,*/ onStatus,
                                             tries,
                                             retryTimeoutScale]() {
                                                if (!sharedContext->flowController.isRetryEnabled()) {
                                                    return;
                                                }
                                                if (config->getRetry()) {
                                                    sharedContext->flowController.reportFlowRetry();
                                                    SocketClient(config, sharedContext)
                                                        .realConnect(onStatus, tries + 1, retryTimeoutScale * config->getRetryBase());
                                                } else {
                                                    log.info("Retry connect disabled during wait");
                                                }
                                            });
                                    }
                                },
                                config);
                        }
                    } else {
                        log.critical("{} required", config->getInstanceName());
                    }
                });

            return *this;
        }

    public:
        const SocketClient& connect(const std::function<void(const SocketAddress&, core::socket::State)>& onStatus) const {
            return realConnect(onStatus, 0, 1);
        }

        const SocketClient& connect(const SocketAddress& remoteAddress,
                                    const std::function<void(const SocketAddress&, core::socket::State)>& onStatus) const {
            Super::config->Remote::setSocketAddress(remoteAddress);

            return connect(onStatus);
        }

        const SocketClient& connect(const SocketAddress& remoteAddress,
                                    const SocketAddress& localAddress,
                                    const std::function<void(const SocketAddress&, core::socket::State)>& onStatus) const {
            Super::config->Local::setSocketAddress(localAddress);

            return connect(remoteAddress, onStatus);
        }

        logger::BoundaryLogger log() const {
            // Round 6 backend-backed default semantic logger.
            // The sink-taking overload remains available for tests and custom capture.
            // Production default logger uses the frozen startup semantic policy as its local threshold.
            return logScope.logger(logger::Logger::semanticSink());
        }

        logger::BoundaryLogger log(logger::BoundaryLogger::Sink sink,
                                   logger::LogLevel threshold = logger::LogLevel::Trace,
                                   logger::BoundaryLogger::Clock clock = {}) const {
            return logScope.logger(std::move(sink), threshold, std::move(clock));
        }

        std::function<void(SocketConnection*)>& getOnConnect() {
            return sharedContext->onConnect;
        }

        const SocketClient& setOnConnect(const std::function<void(SocketConnection*)>& onConnect, bool initialize = false) const {
            sharedContext->onConnect =
                initialize ? onConnect : [oldOnConnect = sharedContext->onConnect, onConnect](SocketConnection* socketConnection) {
                    oldOnConnect(socketConnection);
                    onConnect(socketConnection);
                };

            return *this;
        }

        std::function<void(SocketConnection*)>& getOnConnected() const {
            return sharedContext->onConnected;
        }

        const SocketClient& setOnConnected(const std::function<void(SocketConnection*)>& onConnected, bool initialize = false) const {
            sharedContext->onConnected =
                initialize ? onConnected : [oldOnConnected = sharedContext->onConnected, onConnected](SocketConnection* socketConnection) {
                    oldOnConnected(socketConnection);
                    onConnected(socketConnection);
                };

            return *this;
        }

        std::function<void(SocketConnection*)>& getOnDisconnect() const {
            return sharedContext->onDisconnect;
        }

        const SocketClient& setOnDisconnect(const std::function<void(SocketConnection*)>& onDisconnect, bool initialize = false) const {
            sharedContext->onDisconnect =
                initialize ? onDisconnect
                           : [oldOnDisconnect = sharedContext->onDisconnect, onDisconnect](SocketConnection* socketConnection) {
                                 oldOnDisconnect(socketConnection);
                                 onDisconnect(socketConnection);
                             };

            return *this;
        }

        ClientFlowController* getFlowController() const {
            return &sharedContext->flowController;
        }

        std::shared_ptr<SocketContextFactory> getSocketContextFactory() const {
            return sharedContext->socketContextFactory;
        }

    private:
        static logger::LogScopeOwner makeLogScope(const std::string& instanceName) {
            return logger::LogScopeOwner(logger::LogOrigin::Framework,
                                         logger::LogBoundary::Instance,
                                         "core.socket.stream",
                                         instanceName.empty() ? std::nullopt : std::optional<std::string>(instanceName),
                                         logger::LogRole::Client,
                                         std::nullopt);
        }

        logger::LogScopeOwner logScope;
        std::shared_ptr<Context> sharedContext;
    };

    template <typename SocketClient, typename... Args>
    SocketClient Client(const std::string& instanceName,
                        const std::function<void(typename SocketClient::Config*)>& configurator,
                        Args&&... socketContextFactoryArgs) {
        const SocketClient socketClient(instanceName, std::forward<Args>(socketContextFactoryArgs)...);

        configurator(socketClient.getConfig());

        return socketClient;
    }

    template <
        typename SocketClient,
        typename... Args,
        typename = std::enable_if_t<not std::is_invocable_v<std::tuple_element_t<0, std::tuple<Args...>>, typename SocketClient::Config*>>>
    SocketClient Client(const std::string& instanceName, Args&&... socketContextFactoryArgs) {
        return SocketClient(instanceName, std::forward<Args>(socketContextFactoryArgs)...);
    }

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCLIENT_H
