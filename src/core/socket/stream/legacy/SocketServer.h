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

#ifndef CORE_SOCKET_STREAM_LEGACY_SOCKETSERVER_H
#define CORE_SOCKET_STREAM_LEGACY_SOCKETSERVER_H

#include "core/socket/stream/SocketServer.h"          // IWYU pragma: export
#include "core/socket/stream/legacy/SocketAcceptor.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream::legacy {

    template <typename ServerConfigT, typename ServerSocketT, typename SocketContextFactoryT>
    class SocketServer
        : public core::socket::stream::SocketServer<
              ServerConfigT,
              ServerSocketT,
              core::socket::stream::legacy::SocketAcceptor<ServerConfigT, typename ServerSocketT::Socket>,
              SocketContextFactoryT> {
    private:
        using Super =
            core::socket::stream::SocketServer<ServerConfigT,
                                               ServerSocketT,
                                               core::socket::stream::legacy::SocketAcceptor<ServerConfigT, typename ServerSocketT::Socket>,
                                               SocketContextFactoryT>;
        using Super::Super;
    };

} // namespace core::socket::stream::legacy

#endif // CORE_SOCKET_STREAM_LEGACY_SOCKETSERVER_H
