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

#ifndef CORE_SOCKET_STREAM_PHYSICALCLIENTSOCKET_H
#define CORE_SOCKET_STREAM_PHYSICALCLIENTSOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace core::socket::stream {

    template <typename SocketAddressT>
    class PhysicalClientSocket {
    private:
        using SocketAddress = SocketAddressT;

    public:
        virtual ~PhysicalClientSocket() = default;

        virtual int connect(const SocketAddress& remoteAddress) = 0;

        virtual bool connectInProgress(int cErrno) = 0;
    };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_PHYSICALCLIENTSOCKET_H
