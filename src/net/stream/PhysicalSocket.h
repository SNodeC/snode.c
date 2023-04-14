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

#ifndef NET_STREAM_PHYSICALSOCKET_H
#define NET_STREAM_PHYSICALSOCKET_H

#include "net/PhysicalSocket.h" // IWYU pragma: export

// IWYU pragma: no_include "net/PhysicalSocket.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::stream {

    template <typename SocketAddressT>
    class PhysicalSocket : public net::PhysicalSocket<SocketAddressT> {
    private:
        using Super = net::PhysicalSocket<SocketAddressT>;

    public:
        using SocketAddress = SocketAddressT;

        using Super::Super;
        using Super::operator=;

    protected:
        enum SHUT { WR = SHUT_WR, RD = SHUT_RD, RDWR = SHUT_RDWR };

        void shutdown(SHUT how);
    };

} // namespace net::stream

#endif // NET_STREAM_PHYSICALSOCKET_H
