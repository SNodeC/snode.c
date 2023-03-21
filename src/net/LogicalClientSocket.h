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

#ifndef NET_LOGICALCLIENTSOCKET_H
#define NET_LOGICALCLIENTSOCKET_H

#include "net/LogicalSocket.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace net {

    template <typename PhysicalClientSocketT, typename ConfigT>
    class LogicalClientSocket : public net::LogicalSocket<ConfigT> {
    protected:
        using Super = net::LogicalSocket<ConfigT>;
        using Super::Super;

    public:
        using Config = ConfigT;
        using PhysicalSocket = PhysicalClientSocketT;
        using SocketAddress = typename PhysicalSocket::SocketAddress;

        virtual void connect(const std::function<void(const SocketAddress&, int)>& onError) const = 0;

        virtual void connect(const SocketAddress& remoteAddress, const std::function<void(const SocketAddress&, int)>& onError) const = 0;

        virtual void connect(const SocketAddress& remoteAddress,
                             const SocketAddress& localAddress,
                             const std::function<void(const SocketAddress&, int)>& onError) const = 0;
    };

} // namespace net

#endif // NET_LOGICALCLIENTSOCKET_H
