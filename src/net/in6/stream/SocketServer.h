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

#ifndef NET_IN6_STREAM_SOCKETSERVER_H
#define NET_IN6_STREAM_SOCKETSERVER_H

#include "core/socket/stream/SocketServer.h"     // IWYU pragma: export
#include "net/in6/stream/PhysicalServerSocket.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in6::stream {

    template <template <typename PhysicalServerSocket, typename ConfigSocketServer> typename SocketAcceptorT,
              typename ConfigSocketServerT,
              typename SocketContextFactoryT>
    class SocketServer
        : public core::socket::stream::SocketServer<SocketAcceptorT<net::in6::stream::PhysicalServerSocket, ConfigSocketServerT>,
                                                    SocketContextFactoryT> {
    private:
        using Super = core::socket::stream::SocketServer<SocketAcceptorT<net::in6::stream::PhysicalServerSocket, ConfigSocketServerT>,
                                                         SocketContextFactoryT>;

    public:
        using Super::Super;

        using Super::listen;

        void listen(uint16_t port, const std::function<void(const SocketAddress&, int)>& onError) const {
            Super::getConfig().Local::setPort(port);

            listen(onError);
        }

        void listen(uint16_t port, int backlog, const std::function<void(const SocketAddress&, int)>& onError) const {
            Super::getConfig().Local::setPort(port).setBacklog(backlog);

            listen(onError);
        }

        void listen(const std::string& ipOrHostname, uint16_t port, const std::function<void(const SocketAddress&, int)>& onError) const {
            Super::getConfig().Local::setIpOrHostname(ipOrHostname).setPort(port);

            listen(onError);
        }

        void listen(const std::string& ipOrHostname,
                    uint16_t port,
                    int backlog,
                    const std::function<void(const SocketAddress&, int)>& onError) const {
            Super::getConfig().Local::setIpOrHostname(ipOrHostname).setPort(port).setBacklog(backlog);

            listen(onError);
        }
    };

} // namespace net::in6::stream

#endif // NET_IN6_STREAM_SOCKETSERVER_H
