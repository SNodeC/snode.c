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

#include "net/l2/stream/Socket.h"

#include "core/socket/Socket.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <bluetooth/bluetooth.h> // for BTPROTO_L2CAP

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::l2::stream {

    int Socket::create(int flags) {
        return core::system::socket(PF_BLUETOOTH, SOCK_SEQPACKET | flags, BTPROTO_L2CAP);
    }

} // namespace net::l2::stream

namespace core::socket {
    template class Socket<net::l2::SocketAddress>;
}
