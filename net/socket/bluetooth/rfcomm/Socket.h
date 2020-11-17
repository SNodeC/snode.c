/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef NET_SOCKET_BLUETOOTH_RFCOMM_SOCKET_H
#define NET_SOCKET_BLUETOOTH_RFCOMM_SOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h" // for Descriptor
#include "socket/Socket.h"
#include "socket/bluetooth/address/RfCommAddress.h" // IWYU pragma: keep

namespace net::socket::bluetooth::rfcomm {

    class Socket : public socket::Socket<address::RfCommAddress> {
    public:
        Socket() = default;

        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;

        using net::Descriptor::open;
        void open(const std::function<void(int errnum)>& onError, int flags = 0) override;
    };

} // namespace net::socket::bluetooth::rfcomm

#endif // NET_SOCKET_BLUETOOTH_RFCOMM_SOCKET_H
