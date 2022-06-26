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

#include "net/Socket.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    template <typename SocketAddress>
    void Socket<SocketAddress>::open(const std::function<void(int)>& onError, int flags) {
        attachFd(create(flags));

        if (getFd() >= 0) {
            onError(0);
        } else {
            onError(errno);
        }
    }

    template <typename SocketAddress>
    void Socket<SocketAddress>::bind(const SocketAddress& localAddress, const std::function<void(int)>& onError) {
        int ret = core::system::bind(getFd(), &localAddress.getSockAddr(), sizeof(typename SocketAddress::SockAddr));

        if (ret < 0) {
            onError(errno);
        } else {
            this->bindAddress = localAddress;
            onError(0);
        }
    }

    template <typename SocketAddress>
    void Socket<SocketAddress>::shutdown(enum shutdown how) {
        core::system::shutdown(getFd(), how);
    }

    template <typename SocketAddress>
    const SocketAddress& Socket<SocketAddress>::getBindAddress() const {
        return bindAddress;
    }

} // namespace net
