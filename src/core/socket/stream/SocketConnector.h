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
#include "core/socket/State.h"
#include "core/socket/stream/SocketConnectionFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <memory>

namespace core {
    class ProgressLog;
}

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename PhysicalSocketClientT, typename ConfigT, template <typename PhysicalSocketClient> typename SocketConnectionT>
    class SocketConnector
        : protected core::eventreceiver::InitConnectEventReceiver
        , protected core::eventreceiver::ConnectEventReceiver {
    private:
        using PhysicalSocket = PhysicalSocketClientT;

    protected:
        using Config = ConfigT;
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
                        const std::shared_ptr<Config>& config,
                        const std::shared_ptr<core::ProgressLog> progressLog = std::make_shared<core::ProgressLog>());

        ~SocketConnector() override;

    protected:
        void initConnectEvent() override;

    private:
        void connectEvent() override;

    protected:
        void destruct() final;

    private:
        void unobservedEvent() final;
        void connectTimeout() final;

        PhysicalSocket* physicalSocket = nullptr;
        SocketAddress remoteAddress;

    protected:
        std::shared_ptr<core::socket::stream::SocketContextFactory> socketContextFactory = nullptr;

        std::function<void(SocketConnection*)> onConnect;
        std::function<void(SocketConnection*)> onConnected;
        std::function<void(SocketConnection*)> onDisconnect;

        std::function<void(const SocketAddress&, int)> onError;

        std::shared_ptr<Config> config = nullptr;
        std::shared_ptr<core::ProgressLog> progressLog;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETCONNECTOR_H
