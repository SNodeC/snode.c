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

#ifndef NET_UN_SOCKETADDRESS_H
#define NET_UN_SOCKETADDRESS_H

#include "net/SocketAddress.h" // IWYU pragma: export

// IWYU pragma: no_include "net/SocketAddress.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>
#include <sys/un.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un {

    class SocketAddress final : public net::SocketAddress<sockaddr_un> {
    private:
        using Super = net::SocketAddress<sockaddr_un>;

    public:
        using net::SocketAddress<sockaddr_un>::SocketAddress;

        SocketAddress();
        explicit SocketAddress(const std::string& sunPath);

        SocketAddress setSunPath(const std::string& sunPath);

        bool lock();
        bool unlock() const;

        std::string getAddress() const override;
        std::string toString() const override;

    private:
        mutable int lockFd = -1;
    };

} // namespace net::un

extern template class net::SocketAddress<sockaddr_un>;

#endif // NET_UN_SOCKETADDRESS_H
