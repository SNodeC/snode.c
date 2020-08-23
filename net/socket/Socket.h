/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SOCKET_H
#define SOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h"
#include "socket/InetAddress.h"

namespace net::socket {

    class Socket : virtual public net::Descriptor {
    public:
        Socket() = default;

        Socket(const Socket&) = delete;

        virtual ~Socket();

        Socket& operator=(const Socket&) = delete;

        void open(int fd);
        void open(const std::function<void(int errnum)>& onError, int flags = 0);
        void bind(const InetAddress& localAddress, const std::function<void(int errnum)>& onError);

        InetAddress& getLocalAddress();
        void setLocalAddress(const InetAddress& localAddress);

    protected:
        InetAddress localAddress{};
    };

} // namespace net::socket

#endif // SOCKET_H
