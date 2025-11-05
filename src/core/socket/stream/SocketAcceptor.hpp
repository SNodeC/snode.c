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
#include "core/socket/stream/SocketAcceptor.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/PreserveErrno.h"

#include <iomanip>
#include <string>
#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    template <typename PhysicalSocketServer,
              typename Config,
              template <typename ConfigT, typename PhysicalSocketServerT> typename SocketConnection>
    SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::SocketAcceptor(
        const std::shared_ptr<core::socket::stream::SocketContextFactory>& socketContextFactory,
        const std::function<void(SocketConnection*)>& onConnect,
        const std::function<void(SocketConnection*)>& onConnected,
        const std::function<void(SocketConnection*)>& onDisconnect,
        const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
        const std::shared_ptr<Config>& config)
        : core::eventreceiver::AcceptEventReceiver(config->getInstanceName() + " SocketAcceptor", 0)
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
    SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::SocketAcceptor(const SocketAcceptor& socketAcceptor)
        : core::eventreceiver::AcceptEventReceiver(socketAcceptor.config->getInstanceName() + " SocketAcceptor", 0)
        , socketContextFactory(socketAcceptor.socketContextFactory)
        , onConnect(socketAcceptor.onConnect)
        , onConnected(socketAcceptor.onConnected)
        , onDisconnect(socketAcceptor.onDisconnect)
        , onStatus(socketAcceptor.onStatus)
        , config(socketAcceptor.config) {
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
    SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::~SocketAcceptor() {
    }

    template <typename PhysicalSocketServer,
              typename Config,
              template <typename ConfigT, typename PhysicalSocketServerT> typename SocketConnection>
    void SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::init() {
        if (!config->getDisabled()) {
            try {
                core::socket::State state = core::socket::STATE_OK;

                LOG(TRACE) << config->getInstanceName() << ": Starting";

                bindAddress = config->Local::getSocketAddress();

                if (physicalServerSocket.open(config->getSocketOptions(), PhysicalServerSocket::Flags::NONBLOCK) < 0) {
                    PLOG(ERROR) << config->getInstanceName() << " open " << bindAddress.toString();

                    switch (errno) {
                        case EMFILE:
                        case ENFILE:
                        case ENOBUFS:
                        case ENOMEM:
                            state = core::socket::STATE_ERROR;
                            break;
                        default:
                            state = core::socket::STATE_FATAL;
                            break;
                    }
                } else {
                    LOG(DEBUG) << config->getInstanceName() << " open " << bindAddress.toString() << ": success";

                    if (physicalServerSocket.bind(bindAddress) < 0) {
                        PLOG(ERROR) << config->getInstanceName() << " bind " << bindAddress.toString();

                        switch (errno) {
                            case EADDRINUSE:

                                state = core::socket::STATE_ERROR;
                                break;
                            default:

                                state = core::socket::STATE_FATAL;
                                break;
                        }
                    } else {
                        LOG(DEBUG) << config->getInstanceName() << " bind " << bindAddress.toString() << ": success";

                        if (physicalServerSocket.listen(config->getBacklog()) < 0) {
                            PLOG(ERROR) << config->getInstanceName() << " listen " << bindAddress.toString();

                            switch (errno) {
                                case EADDRINUSE:
                                    state = core::socket::STATE_ERROR;
                                    break;
                                default:
                                    state = core::socket::STATE_FATAL;
                                    break;
                            }
                        } else {
                            LOG(DEBUG) << config->getInstanceName() << " listen " << bindAddress.toString() << ": success";

                            if (enable(physicalServerSocket.getFd())) {
                                LOG(DEBUG) << config->getInstanceName() << " enable " << bindAddress.toString() << ": success";
                            } else {
                                LOG(ERROR) << config->getInstanceName() << " enable " << bindAddress.toString()
                                           << ": failed. No valid descriptor created";

                                state = core::socket::STATE(core::socket::STATE_FATAL, ECANCELED, "SocketAcceptor not enabled");
                            }
                        }
                    }
                }

                SocketAddress currentLocalAddress = bindAddress;
                if (bindAddress.useNext()) {
                    onStatus(currentLocalAddress, (state | core::socket::State::NO_RETRY));

                    LOG(INFO) << config->getInstanceName()
                              << ": Using next SocketAddress: " << config->Local::getSocketAddress().toString();

                    useNextSocketAddress();
                } else {
                    onStatus(currentLocalAddress, state);
                }
            } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                core::socket::State state =
                    core::socket::STATE(badSocketAddress.getState(), badSocketAddress.getErrnum(), badSocketAddress.what());

                LOG(ERROR) << state.what();

                onStatus({}, state);
            }
        } else {
            LOG(DEBUG) << config->getInstanceName() << ": disabled";

            onStatus({}, core::socket::STATE_DISABLED);
        }

        if (isEnabled()) {
            core::eventreceiver::AcceptEventReceiver::setTimeout(config->getAcceptTimeout());
        } else {
            destruct();
        }
    }

    template <typename PhysicalSocketServer,
              typename Config,
              template <typename ConfigT, typename PhysicalSocketServerT> typename SocketConnection>
    void SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::acceptEvent() {
        int acceptsPerTick = config->getAcceptsPerTick();

        do {
            PhysicalServerSocket connectedPhysicalSocket(physicalServerSocket.accept4(PhysicalServerSocket::Flags::NONBLOCK), bindAddress);

            if (connectedPhysicalSocket.isValid()) {
                SocketConnection* socketConnection = new SocketConnection(std::move(connectedPhysicalSocket), onDisconnect, config);

                LOG(DEBUG) << config->getInstanceName() << " accept " << bindAddress.toString() << ": success";
                LOG(DEBUG) << "  " << socketConnection->getRemoteAddress().toString() << " -> "
                           << socketConnection->getLocalAddress().toString();

                onConnect(socketConnection);
                onConnected(socketConnection);
            } else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                PLOG(WARNING) << config->getInstanceName() << " accept " << bindAddress.toString();
            }
        } while (--acceptsPerTick > 0);
    }

    template <typename PhysicalSocketServer,
              typename Config,
              template <typename ConfigT, typename PhysicalSocketServerT> typename SocketConnection>
    void SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::unobservedEvent() {
        destruct();
    }

    template <typename PhysicalSocketServer,
              typename Config,
              template <typename ConfigT, typename PhysicalSocketServerT> typename SocketConnection>
    void SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::destruct() {
        delete this;
    }

} // namespace core::socket::stream
