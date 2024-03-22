/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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
#include "core/socket/State.h"

namespace core::socket::stream {
    class SocketContextFactory;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename PhysicalSocketClientT, typename ConfigT, template <typename PhysicalSocketClient> typename SocketConnectionT>
    class SocketConnector : protected core::eventreceiver::ConnectEventReceiver {
    private:
        using PhysicalClientSocket = PhysicalSocketClientT;

    protected:
        using Config = ConfigT;
        using SocketAddress = typename PhysicalClientSocket::SocketAddress;
        using SocketConnection = SocketConnectionT<PhysicalClientSocket>;

    public:
        SocketConnector(const std::shared_ptr<core::socket::stream::SocketContextFactory>& socketContextFactory,
                        const std::function<void(SocketConnection*)>& onConnect,
                        const std::function<void(SocketConnection*)>& onConnected,
                        const std::function<void(SocketConnection*)>& onDisconnect,
                        const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
                        const std::shared_ptr<Config>& config);

        SocketConnector(const SocketConnector& socketConnector);

        ~SocketConnector() override;

        virtual void useNextSocketAddress() = 0;

    protected:
        virtual void init();

    private:
        void connectEvent() override;

        void unobservedEvent() final;
        void connectTimeout() final;

    protected:
        void destruct() final;

    private:
        PhysicalClientSocket physicalClientSocket;
        SocketAddress remoteAddress;

    protected:
        std::shared_ptr<core::socket::stream::SocketContextFactory> socketContextFactory = nullptr;

        std::function<void(SocketConnection*)> onConnect;
        std::function<void(SocketConnection*)> onConnected;
        std::function<void(SocketConnection*)> onDisconnect;

        std::function<void(const SocketAddress&, core::socket::State)> onStatus;

        std::shared_ptr<Config> config = nullptr;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCONNECTOR_H
