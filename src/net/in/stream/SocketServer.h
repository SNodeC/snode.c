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

#ifndef NET_IN_STREAM_SOCKETSERVER_H
#define NET_IN_STREAM_SOCKETSERVER_H

#include "net/in/stream/PhysicalServerSocket.h" // IWYU pragma: export
#include "net/stream/SocketServer.h"            // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in::stream {

    template <typename ConfigT>
    class SocketServer : public net::stream::SocketServer<net::in::stream::PhysicalServerSocket, ConfigT> {
    private:
        using Super = net::stream::SocketServer<net::in::stream::PhysicalServerSocket, ConfigT>;

    protected:
        explicit SocketServer(const std::string& name);

    public:
        using Config = ConfigT;

        using Super::listen;

        void listen(uint16_t port, int backlog, const std::function<void(const SocketAddress&, int)>& onError) const;

        void listen(const std::string& ipOrHostname, int backlog, const std::function<void(const SocketAddress&, int)>& onError) const;

        void listen(const std::string& ipOrHostname,
                    uint16_t port,
                    int backlog,
                    const std::function<void(const SocketAddress&, int)>& onError) const;
    };

} // namespace net::in::stream

#endif // NET_IN_STREAM_SOCKETSERVER_H
