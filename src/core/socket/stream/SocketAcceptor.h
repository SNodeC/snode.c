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

    template <typename PhysicalSocketServerT, typename ConfigT, template <typename PhysicalSocketServer> typename SocketConnectionT>
    class SocketAcceptor
        : protected core::eventreceiver::InitAcceptEventReceiver
        , protected core::eventreceiver::AcceptEventReceiver {
    private:
        using PhysicalSocket = PhysicalSocketServerT;

    protected:
        using Config = ConfigT;
        using SocketAddress = typename PhysicalSocket::SocketAddress;
        using SocketConnection = SocketConnectionT<PhysicalSocket>;

    private:
        using SocketConnectionFactory = core::socket::stream::SocketConnectionFactory<PhysicalSocket, Config, SocketConnection>;

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
                       const std::function<void(const SocketAddress&, core::socket::State)>& onError,
                       const std::shared_ptr<Config>& config,
                       const std::shared_ptr<core::ProgressLog> progressLog = std::make_shared<core::ProgressLog>());

    public:
        ~SocketAcceptor() override;

    protected:
        void initAcceptEvent() override;

    private:
        void acceptEvent() override;

    protected:
        void destruct() final;

    private:
        void unobservedEvent() final;

        PhysicalSocket* physicalSocket = nullptr;
        SocketAddress localAddress;

    protected:
        std::shared_ptr<core::socket::stream::SocketContextFactory> socketContextFactory = nullptr;

        std::function<void(SocketConnection*)> onConnect;
        std::function<void(SocketConnection*)> onConnected;
        std::function<void(SocketConnection*)> onDisconnect;

        std::function<void(const SocketAddress&, core::socket::State)> onError = nullptr;

        std::shared_ptr<Config> config;
        std::shared_ptr<core::ProgressLog> progressLog;
#define PROGLOG(level) progressLog->addEntry((level))
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETACCEPTOR_H
