/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#ifndef CORE_SOCKET_STREAM_TLS_SOCKETCLIENT_H
#define CORE_SOCKET_STREAM_TLS_SOCKETCLIENT_H

#include "core/socket/stream/SocketClient.h"        // IWYU pragma: export
#include "core/socket/stream/tls/SocketConnector.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::tls {

    template <typename SocketClientT, typename SocketContextFactoryT>
    class SocketClient
        : public core::socket::stream::SocketClient<SocketClientT, core::socket::stream::tls::SocketConnector, SocketContextFactoryT> {
    private:
        using Super = core::socket::stream::SocketClient<SocketClientT, core::socket::stream::tls::SocketConnector, SocketContextFactoryT>;

        using Super::Super;

    public:
        using SocketConnection = typename Super::SocketConnection;
        using SocketAddress = typename Super::SocketAddress;
    };

} // namespace core::socket::stream::tls

#endif // CORE_SOCKET_STREAM_TLS_SOCKETCLIENT_H
