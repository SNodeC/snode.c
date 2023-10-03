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

#include "net/phy/stream/PhysicalClientSocket.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/socket.h"

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::phy::stream {

    template <typename PhysicalSocket>
    int PhysicalClientSocket<PhysicalSocket>::connect(SocketAddress& remoteAddress) {
        return core::system::connect(Super::getFd(), &remoteAddress.getSockAddr(), remoteAddress.getSockAddrLen());
    }

    template <typename PhysicalSocket>
    bool PhysicalClientSocket<PhysicalSocket>::connectInProgress(int cErrno) {
        return cErrno == EINPROGRESS;
    }

} // namespace net::phy::stream
