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

#include "SemanticLog.h"
#include "core/State.h"
#include "core/socket/stream/SocketAcceptor.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/PreserveErrno.h"

#include <cstdint>
#include <iomanip>
#include <string>
#include <utility>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream {

    template <typename PhysicalSocketServer,
              typename Config,
              template <typename ConfigT, typename PhysicalSocketServerT> typename SocketConnection>
    SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::SocketAcceptor(
        const std::function<void(SocketConnection*)>& onConnect,
        const std::function<void(SocketConnection*)>& onConnected,
        const std::function<void(SocketConnection*)>& onDisconnect,
        const std::function<void(core::eventreceiver::AcceptEventReceiver*)>& onInitState,
        const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
        const std::function<std::uint64_t()>& allocateConnectionId,
        const std::shared_ptr<Config>& config)
        : SocketAcceptor(onConnect, onConnected, onDisconnect, onInitState, onStatus, allocateConnectionId, config, {}) {
    }

    template <typename PhysicalSocketServer,
              typename Config,
              template <typename ConfigT, typename PhysicalSocketServerT> typename SocketConnection>
    SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::SocketAcceptor(
        const std::function<void(SocketConnection*)>& onConnect,
        const std::function<void(SocketConnection*)>& onConnected,
        const std::function<void(SocketConnection*)>& onDisconnect,
        const std::function<void(core::eventreceiver::AcceptEventReceiver*)>& onInitState,
        const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
        const std::function<std::uint64_t()>& allocateConnectionId,
        const std::shared_ptr<Config>& config,
        const std::function<void()>& shutdownCallback)
        : core::eventreceiver::AcceptEventReceiver(config->getInstanceName() + " SocketAcceptor", 0)
        , onConnect(onConnect)
        , onConnected(onConnected)
        , onDisconnect(onDisconnect)
        , onInitState(onInitState)
        , onStatus(onStatus)
        , allocateConnectionId(allocateConnectionId)
        , shutdownCallback(shutdownCallback)
        , logScope(makeLogScope(config->getInstanceName()))
        , config(config) {
    }

    template <typename PhysicalSocketServer,
              typename Config,
              template <typename ConfigT, typename PhysicalSocketServerT> typename SocketConnection>
    SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::SocketAcceptor(const SocketAcceptor& socketAcceptor)
        : core::eventreceiver::AcceptEventReceiver(socketAcceptor.config->getInstanceName() + " SocketAcceptor", 0)
        , onConnect(socketAcceptor.onConnect)
        , onConnected(socketAcceptor.onConnected)
        , onDisconnect(socketAcceptor.onDisconnect)
        , onInitState(socketAcceptor.onInitState)
        , onStatus(socketAcceptor.onStatus)
        , allocateConnectionId(socketAcceptor.allocateConnectionId)
        , shutdownCallback(socketAcceptor.shutdownCallback)
        , logScope(socketAcceptor.logScope)
        , config(socketAcceptor.config) {
    }

    template <typename PhysicalSocketServer,
              typename Config,
              template <typename ConfigT, typename PhysicalSocketServerT> typename SocketConnection>
    SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::~SocketAcceptor() {
    }

    template <typename PhysicalSocketServer,
              typename Config,
              template <typename ConfigT, typename PhysicalSocketServerT> typename SocketConnection>
    void SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::onShutdown() {
        if (shutdownCallback) {
            shutdownCallback();
        }
    }

    template <typename PhysicalSocketServer,
              typename Config,
              template <typename ConfigT, typename PhysicalSocketServerT> typename SocketConnection>
    void SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::init() {
        if (!config->getDisabled()) {
            try {
                core::socket::State state = core::socket::STATE_OK;
                bool bindSucceeded = false;

                snode::semantic::coreSocketLog().debug() << config->getInstanceName() << " Listen: starting";

                configuredAddress = config->Local::getSocketAddress();

                if (physicalServerSocket.open(config->getSocketOptions(), PhysicalServerSocket::Flags::NONBLOCK) < 0) {
                    const int errnum = errno;
                    snode::semantic::sysError(snode::semantic::coreSocketLog(), logger::LogLevel::Error, errnum)
                        << config->getInstanceName() << " open " << configuredAddress.toString();

                    switch (errnum) {
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
                    snode::semantic::coreSocketLog().debug()
                        << config->getInstanceName() << " open " << configuredAddress.toString() << ": success";

                    if (physicalServerSocket.bind(configuredAddress) < 0) {
                        const int errnum = errno;
                        snode::semantic::sysError(snode::semantic::coreSocketLog(), logger::LogLevel::Error, errnum)
                            << config->getInstanceName() << " bind " << configuredAddress.toString();

                        switch (errnum) {
                            case EADDRINUSE:

                                state = core::socket::STATE_ERROR;
                                break;
                            default:

                                state = core::socket::STATE_FATAL;
                                break;
                        }
                    } else {
                        bindSucceeded = true;

                        const std::string configuredAddressString = configuredAddress.toString();
                        const std::string effectiveBindAddressString = physicalServerSocket.getBindAddress().toString();

                        snode::semantic::coreSocketLog().debug()
                            << config->getInstanceName() << " bind " << configuredAddressString
                            << (configuredAddressString == effectiveBindAddressString ? ""
                                                                                      : " (effective: " + effectiveBindAddressString + ")")
                            << ": success";

                        if (physicalServerSocket.listen(config->getBacklog()) < 0) {
                            const int errnum = errno;
                            snode::semantic::sysError(snode::semantic::coreSocketLog(), logger::LogLevel::Error, errnum)
                                << config->getInstanceName() << " listen " << physicalServerSocket.getBindAddress().toString();

                            switch (errnum) {
                                case EADDRINUSE:
                                    state = core::socket::STATE_ERROR;
                                    break;
                                default:
                                    state = core::socket::STATE_FATAL;
                                    break;
                            }
                        } else {
                            snode::semantic::coreSocketLog().debug() << config->getInstanceName() << " listen "
                                                                     << physicalServerSocket.getBindAddress().toString() << ": success";

                            if (enable(physicalServerSocket.getFd())) {
                                snode::semantic::coreSocketLog().debug() << config->getInstanceName() << " enable "
                                                                         << physicalServerSocket.getBindAddress().toString() << ": success";
                            } else {
                                snode::semantic::coreSocketLog().error()
                                    << config->getInstanceName() << " enable " << physicalServerSocket.getBindAddress().toString()
                                    << ": failed. No valid descriptor created";

                                state = core::socket::STATE(core::socket::STATE_FATAL, ECANCELED, "SocketAcceptor not enabled");
                            }
                        }
                    }
                }

                SocketAddress currentLocalAddress = bindSucceeded ? physicalServerSocket.getBindAddress() : configuredAddress;
                if (configuredAddress.useNext()) {
                    onStatus(currentLocalAddress, (state | core::socket::State::NO_RETRY));

                    snode::semantic::coreSocketLog().info()
                        << config->getInstanceName() << ": Using next SocketAddress: " << config->Local::getSocketAddress().toString();

                    useNextSocketAddress();
                } else {
                    onStatus(currentLocalAddress, state);
                }
            } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                core::socket::State state =
                    core::socket::STATE(badSocketAddress.getState(), badSocketAddress.getErrnum(), badSocketAddress.what());

                snode::semantic::coreSocketLog().error() << state.what();

                onStatus({}, state);
            }
        } else {
            snode::semantic::coreSocketLog().debug() << config->getInstanceName() << ": disabled";

            onStatus({}, core::socket::STATE_DISABLED);
        }

        if (isEnabled()) {
            onInitState(this);
            core::eventreceiver::AcceptEventReceiver::setTimeout(config->getAcceptTimeout());
        } else {
            destruct();
        }
    }

    template <typename PhysicalSocketServer,
              typename Config,
              template <typename ConfigT, typename PhysicalSocketServerT> typename SocketConnection>
    void SocketAcceptor<PhysicalSocketServer, Config, SocketConnection>::acceptEvent() {
        if (isEnabled()) {
            int acceptsPerTick = config->getAcceptsPerTick();

            do {
                PhysicalServerSocket connectedPhysicalSocket(physicalServerSocket.accept4(PhysicalServerSocket::Flags::NONBLOCK),
                                                             physicalServerSocket.getBindAddress());
                const int errnum = errno;

                if (connectedPhysicalSocket.isValid()) {
                    const std::uint64_t connectionId = allocateConnectionId();
                    SocketConnection* socketConnection =
                        new SocketConnection(std::move(connectedPhysicalSocket), onDisconnect, connectionId, config);

                    snode::semantic::coreSocketLog().debug()
                        << config->getInstanceName() << " accept " << physicalServerSocket.getBindAddress().toString() << ": success";
                    snode::semantic::coreSocketLog().debug() << "  " << socketConnection->getRemoteAddress().toString() << " -> "
                                                             << socketConnection->getLocalAddress().toString();

                    onConnect(socketConnection);
                    onConnected(socketConnection);
                } else if (errnum != EINTR && errnum != EAGAIN && errnum != EWOULDBLOCK) {
                    snode::semantic::sysError(snode::semantic::coreSocketLog(), logger::LogLevel::Warn, errnum)
                        << config->getInstanceName() << " accept " << physicalServerSocket.getBindAddress().toString();
                }
            } while (--acceptsPerTick > 0);
        }
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
        if (!config->getDisabled()) {
            onInitState(this);
        }

        delete this;
    }

} // namespace core::socket::stream
