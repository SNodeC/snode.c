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

#ifndef CORE_SOCKET_STREAM_LOGICALSOCKETSERVER_H
#define CORE_SOCKET_STREAM_LOGICALSOCKETSERVER_H

#include "core/socket/LogicalSocket.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace core::socket::stream {

    template <typename PhysicalServerSocketT, typename ConfigT>
    class LogicalSocketServer : public core::socket::LogicalSocket<ConfigT> {
    protected:
        using Super = core::socket::LogicalSocket<ConfigT>;
        using Super::Super;

    public:
        using Config = ConfigT;
        using PhysicalSocket = PhysicalServerSocketT;
        using SocketAddress = typename PhysicalSocket::SocketAddress;

        virtual void listen(const std::function<void(const SocketAddress&, int)>& onError) const = 0;
        void listen(const SocketAddress& localAddress, const std::function<void(const SocketAddress&, int)>& onError) const;
        void listen(const SocketAddress& localAddress, int backlog, const std::function<void(const SocketAddress&, int)>& onError) const;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_LOGICALSOCKETSERVER_H
