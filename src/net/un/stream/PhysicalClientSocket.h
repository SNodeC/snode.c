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

#ifndef NET_UN_STREAM_PHYSICALCLIENTSOCKET_H
#define NET_UN_STREAM_PHYSICALCLIENTSOCKET_H

#include "net/un/stream/PhysicalSocket.h" // IWYU pragma: export
//
#include "net/stream/PhysicalClientSocket.h" // IWYU pragma: export

// IWYU pragma: no_include "net/stream/PhysicalClientSocket.hpp"
// IWYU pragma: no_include "net/un/stream/PhysicalSocket.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un::stream {

    class PhysicalClientSocket : public net::un::stream::PhysicalSocket<net::stream::PhysicalClientSocket> {
    private:
        using Super = net::un::stream::PhysicalSocket<net::stream::PhysicalClientSocket>;

    public:
        using Super::Super;

        ~PhysicalClientSocket() override;

        using Super::operator=;

        bool connectInProgress(int cErrno) override;
    };

} // namespace net::un::stream

extern template class net::stream::PhysicalClientSocket<net::un::SocketAddress>;
extern template class net::un::stream::PhysicalSocket<net::stream::PhysicalClientSocket>;
extern template class net::un::PhysicalSocket<net::stream::PhysicalClientSocket>;

#endif // NET_UN_STREAM_PHYSICALCLIENTSOCKET_H
