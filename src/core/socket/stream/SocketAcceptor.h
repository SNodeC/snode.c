/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#ifndef CORE_SOCKET_STREAM_SOCKETACCEPTOR_H
#define CORE_SOCKET_STREAM_SOCKETACCEPTOR_H

#include "core/eventreceiver/AcceptEventReceiver.h"
#include "core/socket/stream/SocketConnectionFactory.h"
#include "net/un/dgram/Socket.h"

namespace core::socket::stream {

    template <typename PhysicalSocket, typename Config, typename SocketConnection>
    class SocketConnectionFactory;

    class SocketContextFactory;

} // namespace core::socket::stream

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename PhysicalServerSocketT, typename ConfigT, template <typename PhysicalServerSocket> typename SocketConnectionT>
    class SocketAcceptor
        : protected core::eventreceiver::InitAcceptEventReceiver
        , protected core::eventreceiver::AcceptEventReceiver {
    public:
        using Config = ConfigT;

    private:
        using PrimaryPhysicalSocket = PhysicalServerSocketT;
        using SecondarySocket = net::un::dgram::Socket;

    protected:
        using SocketAddress = typename PrimaryPhysicalSocket::SocketAddress;
        using SocketConnection = SocketConnectionT<PrimaryPhysicalSocket>;

    private:
        using SocketConnectionFactory = core::socket::stream::SocketConnectionFactory<PrimaryPhysicalSocket, Config, SocketConnection>;

    public:
        SocketAcceptor() = delete;
        SocketAcceptor(const SocketAcceptor&) = delete;
        SocketAcceptor(SocketAcceptor&&) = delete;

        SocketAcceptor& operator=(const SocketAcceptor&) = delete;
        SocketAcceptor& operator=(SocketAcceptor&&) = delete;

        SocketAcceptor(const std::shared_ptr<core::socket::stream::SocketContextFactory>& socketContextFactory,
                       const std::function<void(SocketConnection*)>& onConnect,
                       const std::function<void(SocketConnection*)>& onConnected,
                       const std::function<void(SocketConnection*)>& onDisconnect,
                       const std::function<void(const SocketAddress&, int)>& onError,
                       const std::shared_ptr<Config>& config);

        ~SocketAcceptor() override {
            if (secondaryPhysicalSocket != nullptr) {
                delete secondaryPhysicalSocket;
            }
            if (primaryPhysicalSocket != nullptr) {
                delete primaryPhysicalSocket;
            }
        }

    protected:
        void initAcceptEvent() override;

    private:
        void acceptEvent() override;

    protected:
        void destruct() override;

    private:
        void unobservedEvent() override;

        PrimaryPhysicalSocket* primaryPhysicalSocket = nullptr;
        SecondarySocket* secondaryPhysicalSocket = nullptr;

        SocketConnectionFactory socketConnectionFactory;

    protected:
        std::function<void(const SocketAddress&, int)> onError = nullptr;

        std::shared_ptr<Config> config;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETACCEPTOR_H
