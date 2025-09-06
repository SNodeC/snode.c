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

#include "core/State.h"
#include "core/socket/stream/SocketConnector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/PreserveErrno.h"

#include <iomanip>
#include <string>
#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    template <typename PhysicalSocketClient,
              typename Config,
              template <typename ConfigT, typename PhysicalSocketClientT> typename SocketConnection>
    SocketConnector<PhysicalSocketClient, Config, SocketConnection>::SocketConnector(
        const std::shared_ptr<SocketContextFactory>& socketContextFactory,
        const std::function<void(SocketConnection*)>& onConnect,
        const std::function<void(SocketConnection*)>& onConnected,
        const std::function<void(SocketConnection*)>& onDisconnect,
        const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
        const std::shared_ptr<Config>& config)
        : core::eventreceiver::ConnectEventReceiver(config->getInstanceName() + " SocketConnector", 0)
        , socketContextFactory(socketContextFactory)
        , onConnect(onConnect)
        , onConnected(onConnected)
        , onDisconnect(onDisconnect)
        , onStatus(onStatus)
        , config(config) {
        atNextTick([this]() {
            if (core::eventLoopState() == core::State::RUNNING) {
                init();
            } else {
                destruct();
            }
        });
    }

    template <typename PhysicalSocketServer,
              typename Config,
              template <typename ConfigT, typename PhysicalSocketServerT> typename SocketConnection>
    SocketConnector<PhysicalSocketServer, Config, SocketConnection>::SocketConnector(const SocketConnector& socketConnector)
        : core::eventreceiver::ConnectEventReceiver(socketConnector.config->getInstanceName() + " SocketConnector", 0)
        , socketContextFactory(socketConnector.socketContextFactory)
        , onConnect(socketConnector.onConnect)
        , onConnected(socketConnector.onConnected)
        , onDisconnect(socketConnector.onDisconnect)
        , onStatus(socketConnector.onStatus)
        , config(socketConnector.config) {
        atNextTick([this]() {
            if (core::eventLoopState() == core::State::RUNNING) {
                init();
            } else {
                destruct();
            }
        });
    }

    template <typename PhysicalSocketClient,
              typename Config,
              template <typename ConfigT, typename PhysicalSocketClientT> typename SocketConnection>
    SocketConnector<PhysicalSocketClient, Config, SocketConnection>::~SocketConnector() {
    }

    template <typename PhysicalSocketClient,
              typename Config,
              template <typename ConfigT, typename PhysicalSocketClientT> typename SocketConnection>
    void SocketConnector<PhysicalSocketClient, Config, SocketConnection>::init() {
        if (!config->getDisabled()) {
            try {
                LOG(TRACE) << config->getInstanceName() << " Starting";

                remoteAddress = config->Remote::getSocketAddress();
                SocketAddress localAddress = config->Local::getSocketAddress();

                try {
                    core::socket::State state = core::socket::STATE_OK;

                    if (physicalClientSocket.open(config->getSocketOptions(), PhysicalClientSocket::Flags::NONBLOCK) < 0) {
                        switch (errno) {
                            case EMFILE:
                            case ENFILE:
                            case ENOBUFS:
                            case ENOMEM:
                                PLOG(DEBUG) << config->getInstanceName() << " open: '" << localAddress.toString() << "'";

                                state = core::socket::STATE_ERROR;
                                break;
                            default:
                                PLOG(DEBUG) << config->getInstanceName() << " open: '" << localAddress.toString() << "'";

                                state = core::socket::STATE_FATAL;
                                break;
                        }

                        onStatus(remoteAddress, state);
                    } else if (physicalClientSocket.bind(localAddress) < 0) {
                        switch (errno) {
                            case EADDRINUSE:
                                PLOG(DEBUG) << config->getInstanceName() << " bind: '" << localAddress.toString() << "'";

                                state = core::socket::STATE_ERROR;
                                break;
                            default:
                                PLOG(DEBUG) << config->getInstanceName() << " bind: '" << localAddress.toString() << "'";

                                state = core::socket::STATE_FATAL;
                                break;
                        }

                        onStatus(remoteAddress, state);
                    } else if (physicalClientSocket.connect(remoteAddress) < 0 && !PhysicalClientSocket::connectInProgress(errno)) {
                        switch (errno) {
                            case EADDRINUSE:
                            case EADDRNOTAVAIL:
                            case ECONNREFUSED:
                            case ENETUNREACH:
                            case ENOENT:
                            case EHOSTDOWN:
                                PLOG(DEBUG) << config->getInstanceName() << " connect: '" << remoteAddress.toString() << "'";

                                state = core::socket::STATE_ERROR;
                                break;
                            default:
                                PLOG(DEBUG) << config->getInstanceName() << " connect: '" << remoteAddress.toString() << "'";

                                state = core::socket::STATE_FATAL;
                                break;
                        }

                        SocketAddress currentRemoteAddress = remoteAddress;
                        if (remoteAddress.useNext()) {
                            onStatus(currentRemoteAddress, state | core::socket::State::NO_RETRY);

                            LOG(DEBUG) << config->getInstanceName() << " using next SocketAddress: '"
                                       << config->Remote::getSocketAddress().toString() << "'";

                            useNextSocketAddress();
                        } else {
                            onStatus(currentRemoteAddress, state);
                        }
                    } else if (PhysicalClientSocket::connectInProgress(errno)) {
                        if (enable(physicalClientSocket.getFd())) {
                            LOG(DEBUG) << config->getInstanceName() << " connect in progress: '" << remoteAddress.toString() << "'";
                        } else {
                            LOG(DEBUG) << config->getInstanceName() << " not enabled: '" << remoteAddress.toString() << "'";

                            state = core::socket::STATE(core::socket::STATE_FATAL, ECANCELED, "SocketConnector not enabled");

                            onStatus(remoteAddress, state);
                        }
                    } else {
                        LOG(DEBUG) << config->getInstanceName() << " [" << physicalClientSocket.getFd() << "] connect success: '"
                                   << remoteAddress.toString() << "'";

                        onStatus(remoteAddress, core::socket::STATE_OK);

                        SocketConnection* socketConnection = new SocketConnection(config, std::move(physicalClientSocket), onDisconnect);

                        onConnect(socketConnection);
                        onConnected(socketConnection);
                    }
                } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                    LOG(DEBUG) << config->getInstanceName() << " " << badSocketAddress.what();

                    onStatus({}, core::socket::STATE(badSocketAddress.getState(), badSocketAddress.getErrnum(), badSocketAddress.what()));
                }
            } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                LOG(DEBUG) << config->getInstanceName() << " " << badSocketAddress.what();

                onStatus({}, core::socket::STATE(badSocketAddress.getState(), badSocketAddress.getErrnum(), badSocketAddress.what()));
            }
        } else {
            LOG(DEBUG) << config->getInstanceName() << " disabled";

            onStatus({}, core::socket::STATE_DISABLED);
        }

        if (isEnabled()) {
            core::eventreceiver::ConnectEventReceiver::setTimeout(config->getConnectTimeout());
        } else {
            destruct();
        }
    }

    template <typename PhysicalSocketClient,
              typename Config,
              template <typename ConfigT, typename PhysicalSocketClientT> typename SocketConnection>
    void SocketConnector<PhysicalSocketClient, Config, SocketConnection>::connectEvent() {
        int cErrno = 0;

        if (physicalClientSocket.getSockError(cErrno) == 0) { //  == 0->return valid : < 0->getsockopt failed
            const utils::PreserveErrno pe(cErrno);            // errno = cErrno

            if (errno == 0) {
                LOG(DEBUG) << config->getInstanceName() << " [" << physicalClientSocket.getFd() << "] connect success: '"
                           << remoteAddress.toString() << "'";

                onStatus(remoteAddress, core::socket::STATE_OK);

                SocketConnection* socketConnection = new SocketConnection(config, std::move(physicalClientSocket), onDisconnect);

                onConnect(socketConnection);
                onConnected(socketConnection);

                disable();
            } else if (PhysicalClientSocket::connectInProgress(errno)) {
                LOG(TRACE) << config->getInstanceName() << " connect still in progress: '" << remoteAddress.toString() << "'";
            } else {
                SocketAddress currentRemoteAddress = remoteAddress;
                if (remoteAddress.useNext()) {
                    core::socket::State state = core::socket::STATE_OK;

                    switch (errno) {
                        case EADDRINUSE:
                        case EADDRNOTAVAIL:
                        case ECONNREFUSED:
                        case ENETUNREACH:
                        case ENOENT:
                        case EHOSTDOWN:
                            PLOG(DEBUG) << config->getInstanceName() << " connect: '" << remoteAddress.toString() << "'";

                            state = core::socket::STATE_ERROR;
                            break;
                        default:
                            PLOG(DEBUG) << config->getInstanceName() << ": connect: '" << remoteAddress.toString() << "'";

                            state = core::socket::STATE_FATAL;
                            break;
                    }

                    onStatus(currentRemoteAddress, (state | core::socket::State::NO_RETRY));

                    LOG(DEBUG) << config->getInstanceName() << " using next SocketAddress: '"
                               << config->Remote::getSocketAddress().toString() << "'";

                    useNextSocketAddress();

                    disable();
                } else {
                    core::socket::State state = core::socket::STATE_OK;

                    switch (errno) {
                        case EADDRINUSE:
                        case EADDRNOTAVAIL:
                        case ECONNREFUSED:
                        case ENETUNREACH:
                        case ENOENT:
                        case EHOSTDOWN:
                            PLOG(DEBUG) << config->getInstanceName() << " connect: '" << remoteAddress.toString() << "'";

                            state = core::socket::STATE_ERROR;
                            break;
                        default:
                            PLOG(DEBUG) << config->getInstanceName() << " connect: '" << remoteAddress.toString() << "'";

                            state = core::socket::STATE_FATAL;
                            break;
                    }

                    onStatus(currentRemoteAddress, state);

                    disable();
                }
            }
        } else {
            PLOG(DEBUG) << config->getInstanceName() << " getsockopt syscall error: '" << remoteAddress.toString() << "'";

            onStatus(remoteAddress, core::socket::STATE_FATAL);
            disable();
        }
    }

    template <typename PhysicalSocketClient,
              typename Config,
              template <typename ConfigT, typename PhysicalSocketClientT> typename SocketConnection>
    void SocketConnector<PhysicalSocketClient, Config, SocketConnection>::unobservedEvent() {
        destruct();
    }

    template <typename PhysicalSocketClient,
              typename Config,
              template <typename ConfigT, typename PhysicalSocketClientT> typename SocketConnection>
    void SocketConnector<PhysicalSocketClient, Config, SocketConnection>::connectTimeout() {
        LOG(TRACE) << config->getInstanceName() << " connect timeout " << remoteAddress.toString();

        SocketAddress currentRemoteAddress = remoteAddress;
        if (remoteAddress.useNext()) {
            LOG(DEBUG) << config->getInstanceName() << " using next SocketAddress: '" << config->Remote::getSocketAddress().toString()
                       << "'";

            useNextSocketAddress();
        } else {
            LOG(DEBUG) << config->getInstanceName() << " connect timeout '" << remoteAddress.toString() << "'";
            errno = ETIMEDOUT;

            onStatus(currentRemoteAddress, core::socket::STATE_ERROR);
        }

        core::eventreceiver::ConnectEventReceiver::connectTimeout();
    }

    template <typename PhysicalSocketClient,
              typename Config,
              template <typename ConfigT, typename PhysicalSocketClientT> typename SocketConnection>
    void SocketConnector<PhysicalSocketClient, Config, SocketConnection>::destruct() {
        delete this;
    }

} // namespace core::socket::stream
