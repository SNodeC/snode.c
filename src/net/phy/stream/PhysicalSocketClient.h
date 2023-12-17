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

#ifndef NET_PHY_STREAM_PHYSICALSOCKETCLIENT_H
#define NET_PHY_STREAM_PHYSICALSOCKETCLIENT_H

#include "net/phy/stream/PhysicalSocket.h" // IWYU pragma: export

// IWYU pragma: no_include "net/phy/stream/PhysicalSocket.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::phy::stream {

    template <typename SocketAddressT>
    class PhysicalSocketClient : public net::phy::stream::PhysicalSocket<SocketAddressT> {
    private:
        using Super = net::phy::stream::PhysicalSocket<SocketAddressT>;

    public:
        using SocketAddress = SocketAddressT;

        using Super::Super;

        int connect(SocketAddress& remoteAddress);

        static bool connectInProgress(int cErrno);
    };

} // namespace net::phy::stream

#endif // NET_PHY_STREAM_PHYSICALSOCKETCLIENT_H
