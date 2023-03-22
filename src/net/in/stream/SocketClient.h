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

#ifndef NET_IN_STREAM_SOCKETCLIENT_H
#define NET_IN_STREAM_SOCKETCLIENT_H

#include "net/LogicalSocketClient.h"            // IWYU pragma: export
#include "net/in/stream/PhysicalClientSocket.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in::stream {

    template <typename ConfigT>
    class SocketClient : public net::LogicalSocketClient<net::in::stream::PhysicalClientSocket, ConfigT> {
    private:
        using Super = net::LogicalSocketClient<net::in::stream::PhysicalClientSocket, ConfigT>;

    protected:
        using Super::Super;

    public:
        using Config = ConfigT;

        using Super::connect;

        void connect(const std::string& ipOrHostname, uint16_t port, const std::function<void(const SocketAddress&, int)>& onError) const;

        void connect(const std::string& ipOrHostname,
                     uint16_t port,
                     const std::string& bindIpOrHostname,
                     const std::function<void(const SocketAddress&, int)>& onError) const;

        void connect(const std::string& ipOrHostname,
                     uint16_t port,
                     uint16_t bindPort,
                     const std::function<void(const SocketAddress&, int)>& onError) const;

        void connect(const std::string& ipOrHostname,
                     uint16_t port,
                     const std::string& bindIpOrHostname,
                     uint16_t bindPort,
                     const std::function<void(const SocketAddress&, int)>& onError) const;
    };

} // namespace net::in::stream

#endif // NET_IN_STREAM_SOCKETCLIENT_H
