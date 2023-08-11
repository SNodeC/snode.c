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

#ifndef CORE_SOCKET_STREAM_SOCKETCONNECTOR_H
#define CORE_SOCKET_STREAM_SOCKETCONNECTOR_H

#include "core/eventreceiver/ConnectEventReceiver.h"
#include "core/socket/stream/SocketConnectionFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename PhysicalClientSocketT, typename ConfigT, template <typename PhysicalClientSocket> typename SocketConnectionT>
    class SocketConnector
        : protected core::eventreceiver::InitConnectEventReceiver
        , protected core::eventreceiver::ConnectEventReceiver {
    public:
        using Config = ConfigT;

    private:
        using PhysicalSocket = PhysicalClientSocketT;

    protected:
        using SocketAddress = typename PhysicalSocket::SocketAddress;
        using SocketConnection = SocketConnectionT<PhysicalSocket>;

    private:
        using SocketConnectionFactory = core::socket::stream::SocketConnectionFactory<PhysicalSocket, Config, SocketConnection>;

    public:
        SocketConnector() = delete;
        SocketConnector(const SocketConnector&) = delete;
        SocketConnector(SocketConnector&&) = delete;

        SocketConnector& operator=(const SocketConnector&) = delete;
        SocketConnector& operator=(SocketConnector&&) = delete;

        SocketConnector(const std::shared_ptr<core::socket::stream::SocketContextFactory>& socketContextFactory,
                        const std::function<void(SocketConnection*)>& onConnect,
                        const std::function<void(SocketConnection*)>& onConnected,
                        const std::function<void(SocketConnection*)>& onDisconnect,
                        const std::function<void(const SocketAddress&, int)>& onError,
                        const std::shared_ptr<Config>& config);

        ~SocketConnector() override;

    protected:
        void initConnectEvent() override;

    private:
        void connectEvent() override;

    protected:
        void destruct() final;

    private:
        void unobservedEvent() final;

        PhysicalSocket* physicalSocket = nullptr;

        SocketConnectionFactory socketConnectionFactory;

        SocketAddress localAddress;
        SocketAddress remoteAddress;

    protected:
        std::function<void(const SocketAddress& socketAddress, int err)> onError;

        std::shared_ptr<Config> config = nullptr;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCONNECTOR_H
