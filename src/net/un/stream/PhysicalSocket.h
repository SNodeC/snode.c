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

#ifndef NET_UN_STREAM_PHYSICALSOCKET_H
#define NET_UN_STREAM_PHYSICALSOCKET_H

#include "net/stream/PhysicalSocket.h" // IWYU pragma: export
#include "net/un/PhysicalSocket.h"     // IWYU pragma: export

// IWYU pragma: no_include "net/un/PhysicalSocket.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un::stream {

    template <template <typename SocketAddressT> typename PhysicalPeerSocketT>
    class PhysicalSocket : public net::un::PhysicalSocket<PhysicalPeerSocketT> {
    private:
        using Super = net::un::PhysicalSocket<PhysicalPeerSocketT>;

    public:
        using Super::Super;

        PhysicalSocket();
        PhysicalSocket(const PhysicalSocket&) = default;

        using Super::operator=;

        ~PhysicalSocket() override;
    };

} // namespace net::un::stream

extern template class net::stream::PhysicalSocket<net::un::SocketAddress>;

#endif // NET_UN_STREAM_PHYSICALSOCKET_H
