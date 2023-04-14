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

#ifndef NET_STREAM_PHYSICALSERVERSOCKET_H
#define NET_STREAM_PHYSICALSERVERSOCKET_H

#include "net/stream/PhysicalSocket.h" // IWYU pragma: export

// IWYU pragma: no_include "net/stream/PhysicalSocket.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::stream {

    template <typename SocketAddressT>
    class PhysicalServerSocket : public net::stream::PhysicalSocket<SocketAddressT> {
    private:
        using Super = net::stream::PhysicalSocket<SocketAddressT>;

    public:
        using SocketAddress = SocketAddressT;

        using Super::Super;
        using Super::operator=;

        int listen(int backlog);

        int accept(SocketAddress& remoteAddress);
        int accept4(SocketAddress& remoteAddress, int flags);
    };

} // namespace net::stream

#endif // NET_STREAM_PHYSICALSERVERSOCKET_H
