/*
 * SNode.C - a slim toolkit for network communication
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

#ifndef CORE_SOCKET_STREAM_LEGACY_SOCKETACCEPTOR_H
#define CORE_SOCKET_STREAM_LEGACY_SOCKETACCEPTOR_H

#include "core/socket/stream/SocketAcceptor.h"

namespace core::socket::stream::legacy {
    template <typename PhysicalSocketT>
    class SocketConnection;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::legacy {

    template <typename PhysicalServerSocketT, typename ConfigT>
    class SocketAcceptor
        : private core::socket::stream::SocketAcceptor<PhysicalServerSocketT, ConfigT, core::socket::stream::legacy::SocketConnection> {
    private:
        using Super = core::socket::stream::SocketAcceptor<PhysicalServerSocketT, ConfigT, core::socket::stream::legacy::SocketConnection>;

    public:
        using SocketAddress = typename Super::SocketAddress;
        using Config = typename Super::Config;
        using SocketConnection = typename Super::SocketConnection;

        SocketAcceptor(const std::shared_ptr<core::socket::stream::SocketContextFactory>& socketContextFactory,
                       const std::function<void(SocketConnection*)>& onConnect,
                       const std::function<void(SocketConnection*)>& onConnected,
                       const std::function<void(SocketConnection*)>& onDisconnect,
                       const std::function<void(const SocketAddress&, core::socket::State)>& onStatus,
                       const std::shared_ptr<Config>& config);

        SocketAcceptor(const SocketAcceptor& socketAcceptor);

    private:
        void useNextSocketAddress() override;
    };

} // namespace core::socket::stream::legacy

#endif // CORE_SOCKET_STREAM_LEGACY_SOCKETACCEPTOR_H
