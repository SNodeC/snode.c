/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#ifndef NET_L2_STREAM_CLIENTSOCKET_H
#define NET_L2_STREAM_CLIENTSOCKET_H

#include "core/socket/ClientSocket.h" // IWYU pragma: export
#include "net/l2/stream/Socket.h"     // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::l2::stream {

    class ClientSocket : public core::socket::ClientSocket<net::l2::stream::Socket> {
    public:
        using core::socket::ClientSocket<net::l2::stream::Socket>::connect;

        void connect(const std::string& address, uint16_t psm, const std::function<void(int)>& onError);

        void connect(const std::string& address, uint16_t psm, const std::string& bindAddress, const std::function<void(int)>& onError);

        void connect(const std::string& address,
                     uint16_t psm,
                     const std::string& bindAddress,
                     uint16_t bindPsm,
                     const std::function<void(int)>& onError);
    };

} // namespace net::l2::stream

#endif // NET_L2_STREAM_CLIENTSOCKET_H
