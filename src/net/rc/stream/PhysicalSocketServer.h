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

#ifndef NET_RC_STREAM_PHYSICALSOCKETSERVER_H
#define NET_RC_STREAM_PHYSICALSOCKETSERVER_H

#include "net/rc/stream/PhysicalSocket.h"    // IWYU pragma: export
#include "net/phy/stream/PhysicalSocketServer.h" // IWYU pragma: export

// IWYU pragma: no_include "net/rc/stream/PhysicalSocket.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::rc::stream {

    class PhysicalSocketServer : public net::rc::stream::PhysicalSocket<net::phy::stream::PhysicalSocketServer> {
    private:
        using Super = net::rc::stream::PhysicalSocket<net::phy::stream::PhysicalSocketServer>;

    public:
        using Super::Super;

        PhysicalSocketServer(const PhysicalSocketServer&) = default;

        ~PhysicalSocketServer() override;

        using Super::operator=;
    };

} // namespace net::rc::stream

extern template class net::phy::stream::PhysicalSocketServer<net::rc::SocketAddress>;
extern template class net::rc::stream::PhysicalSocket<net::phy::stream::PhysicalSocketServer>;
extern template class net::rc::PhysicalSocket<net::phy::stream::PhysicalSocketServer>;

#endif // NET_STREAM_PHYSICALSOCKETSERVER_H
