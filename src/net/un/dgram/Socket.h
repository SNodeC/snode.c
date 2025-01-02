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

#ifndef NET_UN_DGRAM_SOCKET_H
#define NET_UN_DGRAM_SOCKET_H

#include "net/phy/dgram/PeerSocket.h"  // IWYU pragma: export
#include "net/un/phy/PhysicalSocket.h" // IWYU pragma: export

// IWYU pragma: no_include "net/un/phy/PhysicalSocket.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un::dgram {

    class Socket : public net::un::phy::PhysicalSocket<net::phy::dgram::PeerSocket> {
    private:
        using Super = net::un::phy::PhysicalSocket<net::phy::dgram::PeerSocket>;

    public:
        using Super::Super;
        using Super::operator=;

        Socket();

        ~Socket() override;

        ssize_t sendFd(SocketAddress&& destAddress, int sendfd);
        ssize_t sendFd(SocketAddress& destAddress, int sendfd);
        ssize_t recvFd(int* recvfd);
    };

} // namespace net::un::dgram

extern template class net::phy::dgram::PeerSocket<net::un::SocketAddress>;
extern template class net::un::phy::PhysicalSocket<net::phy::dgram::PeerSocket>;

#endif // NET_UN_DGRAM_SOCKET_H
