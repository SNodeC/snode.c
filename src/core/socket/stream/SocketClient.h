/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include "core/SNodeC.h"
#include "core/eventreceiver/ConnectEventReceiver.h"
#include "core/socket/Socket.h" // IWYU pragma: export
#include "core/socket/State.h"  // IWYU pragma: export
#include "core/socket/stream/SocketContextFactory.h"
#include "core/timer/Timer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/Random.h"

#include <algorithm>
#include <functional>  // IWYU pragma: export
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
        SocketClient(const std::shared_ptr<Config>& config,
                     const std::shared_ptr<SocketContextFactory>& socketContextFactory,
                     const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect)
            : Super(config)
            , socketContextFactory(socketContextFactory)
            , onConnect(onConnect)
            , onConnected(onConnected)
            , onDisconnect(onDisconnect) {
        }

    public:
        SocketClient(const std::string& name,
                     const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect,
                     Args&&... args)
            : Super(name)
            , socketContextFactory(std::make_shared<SocketContextFactory>(std::forward<Args>(args)...))
            , onConnect([onConnect](SocketConnection* socketConnection) { // onConnect
                LOG(DEBUG) << socketConnection->getConnectionName() << ": OnConnect";

                LOG(DEBUG) << "  Local: " << socketConnection->getLocalAddress().toString();
                LOG(DEBUG) << "   Peer: " << socketConnection->getRemoteAddress().toString();

                if (onConnect) {
                    onConnect(socketConnection);
                }
            })
            , onConnected([onConnected](SocketConnection* socketConnection) { // onConnected
                LOG(DEBUG) << socketConnection->getConnectionName() << ": OnConnected";

                LOG(DEBUG) << "  Local: " << socketConnection->getLocalAddress().toString();
                LOG(DEBUG) << "   Peer: " << socketConnection->getRemoteAddress().toString();

                if (onConnected) {
                    onConnected(socketConnection);
                }
            })
            , onDisconnect([onDisconnect](SocketConnection* socketConnection) { // onDisconnect
                LOG(DEBUG) << socketConnection->getConnectionName() << ": OnDisconnect";

                LOG(DEBUG) << "            Local: " << socketConnection->getLocalAddress().toString();
                LOG(DEBUG) << "             Peer: " << socketConnection->getRemoteAddress().toString();

                LOG(DEBUG) << "     Online Since: " << socketConnection->getOnlineSince();
                LOG(DEBUG) << "  Online Duration: " << socketConnection->getOnlineDuration();

                LOG(DEBUG) << "     Total Queued: " << socketConnection->getTotalQueued();
                LOG(DEBUG) << "       Total Sent: " << socketConnection->getTotalSent();
                LOG(DEBUG) << "      Write Delta: " << socketConnection->getTotalQueued() - socketConnection->getTotalSent();
                LOG(DEBUG) << "       Total Read: " << socketConnection->getTotalRead();
                LOG(DEBUG) << "  Total Processed: " << socketConnection->getTotalProcessed();
                LOG(DEBUG) << "       Read Delta: " << socketConnection->getTotalRead() - socketConnection->getTotalProcessed();

                if (onDisconnect) {
                    onDisconnect(socketConnection);
                }
            }) {
        }

        SocketClient(const std::function<void(SocketConnection*)>& onConnect,
                     const std::function<void(SocketConnection*)>& onConnected,
                     const std::function<void(SocketConnection*)>& onDisconnect,
                     Args&&... args)
            : SocketClient("", onConnect, onConnected, onDisconnect, std::forward<Args>(args)...) {
        }

        // VLOG() is used hire as this are log messages for the application
        SocketClient(const std::string& name, Args&&... args)
            : SocketClient(name, {}, {}, {}, std::forward<Args>(args)...) {
        }

        explicit SocketClient(Args&&... args)
            : SocketClient("", std::forward<Args>(args)...) {
        }

    private:
        void realConnect(const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
                         unsigned int tries,
                         double retryTimeoutScale) const {
            if (core::SNodeC::state() == core::State::RUNNING || core::SNodeC::state() == core::State::INITIALIZED) {
                new SocketConnector(
                    socketContextFactory,
                    onConnect,
                    onConnected,
                    [config = this->config,
                     onConnect = this->onConnect,
                     onConnected = this->onConnected,
                     onDisconnect = this->onDisconnect,
                     socketContextFactory = this->socketContextFactory,
                     onStatus](SocketConnection* socketConnection) {
                        onDisconnect(socketConnection);

                        if (config->getReconnect() && core::eventLoopState() == core::State::RUNNING) {
                            double relativeReconnectTimeout = config->getReconnectTime();

                            LOG(INFO) << config->getInstanceName() << ": OnStatus";
                            LOG(INFO) << "  reconnecting in " << relativeReconnectTimeout << " seconds";

                            core::timer::Timer::singleshotTimer(
                                [config, onConnect, onConnected, onDisconnect, onStatus, socketContextFactory]() mutable {
                                    if (config->getReconnect()) {
                                        SocketClient(config, socketContextFactory, onConnect, onConnected, onDisconnect)
                                            .realConnect(onStatus, 0, config->getRetryBase());
                                    } else {
                                        LOG(INFO) << config->getInstanceName() << ": Reconnect disabled during wait";
                                    }
                                },
                                relativeReconnectTimeout);
                        }
                    },
                    [config = this->config,
                     onConnect = this->onConnect,
                     onConnected = this->onConnected,
                     onDisconnect = this->onDisconnect,
                     socketContextFactory = this->socketContextFactory,
                     onStatus,
                     tries,
                     retryTimeoutScale](const SocketAddress& socketAddress, core::socket::State state) mutable {
                        const bool retryFlag = (state & core::socket::State::NO_RETRY) == 0;
                        state &= ~core::socket::State::NO_RETRY;
                        onStatus(socketAddress, state);

                        if (retryFlag && config->getRetry() // Shall we potentially retry? In case are the ...
                            && (config->getRetryTries() == 0 || tries < config->getRetryTries()) // ... limits not reached and has an ...
                            && (state == core::socket::State::ERROR ||
                                (state == core::socket::State::FATAL && config->getRetryOnFatal()))) { // error occurred?
                            double relativeRetryTimeout =
                                config->getRetryLimit() > 0
                                    ? std::min<double>(config->getRetryTimeout() * retryTimeoutScale, config->getRetryLimit())
                                    : config->getRetryTimeout() * retryTimeoutScale;
                            relativeRetryTimeout -= utils::Random::getInRange(-config->getRetryJitter(), config->getRetryJitter()) *
                                                    relativeRetryTimeout / 100.;

                            LOG(INFO) << config->getInstanceName() << ": OnStatus";
                            LOG(INFO) << "  retrying in " << relativeRetryTimeout << " seconds";

                            core::timer::Timer::singleshotTimer(
                                [config,
                                 onConnect,
                                 onConnected,
                                 onDisconnect,
                                 onStatus,
                                 tries,
                                 retryTimeoutScale,
                                 socketContextFactory]() mutable {
                                    if (config->getRetry()) {
                                        SocketClient(config, socketContextFactory, onConnect, onConnected, onDisconnect)
                                            .realConnect(onStatus, tries + 1, retryTimeoutScale * config->getRetryBase());
                                    } else {
                                        LOG(INFO) << config->getInstanceName() << ": Retry disabled during wait";
                                    }
                                },
                                relativeRetryTimeout);
                        }
                    },
                    Super::config);
            }
        }

    public:
        void connect(const std::function<void(const SocketAddress&, core::socket::State)>& onStatus) const {
            realConnect(onStatus, 0, 1);
        }

        void connect(const SocketAddress& remoteAddress,
                     const std::function<void(const SocketAddress&, core::socket::State)>& onStatus) const {
            Super::config->Remote::setSocketAddress(remoteAddress);

            connect(onStatus);
        }

        void connect(const SocketAddress& remoteAddress,
                     const SocketAddress& localAddress,
                     const std::function<void(const SocketAddress&, core::socket::State)>& onStatus) const {
            Super::config->Local::setSocketAddress(localAddress);

            connect(remoteAddress, onStatus);
        }

        std::function<void(SocketConnection*)>& getOnConnect() {
            return onConnect;
        }

        std::function<void(SocketConnection*)> setOnConnect(const std::function<void(SocketConnection*)>& onConnect,
                                                            bool initialize = false) {
            std::function<void(SocketConnection*)> oldOnConnect = this->onConnect;

            this->onConnect = initialize ? onConnect : [oldOnConnect, onConnect](SocketConnection* socketConnection) {
                oldOnConnect(socketConnection);
                onConnect(socketConnection);
            };

            return oldOnConnect;
        }

        std::function<void(SocketConnection*)>& getOnConnected() {
            return onConnected();
        }

        std::function<void(SocketConnection*)> setOnConnected(const std::function<void(SocketConnection*)>& onConnected,
                                                              bool initialize = false) {
            std::function<void(SocketConnection*)> oldOnConnected = this->onConnected;

            this->onConnected = initialize ? onConnected : [oldOnConnected, onConnected](SocketConnection* socketConnection) {
                oldOnConnected(socketConnection);
                onConnected(socketConnection);
            };

            return oldOnConnected;
        }

        std::function<void(SocketConnection*)>& getOnDisconnect() {
            return onDisconnect;
        }

        std::function<void(SocketConnection*)> setOnDisconnect(const std::function<void(SocketConnection*)>& onDisconnect,
                                                               bool initialize = false) {
            std::function<void(SocketConnection*)> oldOnDisconnect = this->onDisconnect;

            this->onDisconnect = initialize ? onDisconnect : [oldOnDisconnect, onDisconnect](SocketConnection* socketConnection) {
                oldOnDisconnect(socketConnection);
                onDisconnect(socketConnection);
            };

            return oldOnDisconnect;
        }

        std::shared_ptr<SocketContextFactory> getSocketContextFactory() const {
            return socketContextFactory;
        }

    private:
        const std::shared_ptr<SocketContextFactory> socketContextFactory;

        std::function<void(SocketConnection*)> onConnect;
        std::function<void(SocketConnection*)> onConnected;
        std::function<void(SocketConnection*)> onDisconnect;
    };

    template <typename SocketClient, typename... Args>
    SocketClient Client(const std::string& instanceName,
                        const std::function<void(typename SocketClient::Config&)>& configurator,
                        Args&&... socketContextFactoryArgs) {
        const SocketClient socketClient(instanceName, std::forward<Args>(socketContextFactoryArgs)...);

        configurator(socketClient.getConfig());

        return socketClient;
    }

    template <typename SocketClient, typename... Args>
    SocketClient Client(const std::string& instanceName, Args&&... socketContextFactoryArgs) {
        return SocketClient(instanceName, std::forward<Args>(socketContextFactoryArgs)...);
    }

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCLIENT_H
