/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#ifndef NET_RC_PHY_STREAM_PHYSICALSOCKETSERVER_H
#define NET_RC_PHY_STREAM_PHYSICALSOCKETSERVER_H

#include "net/phy/stream/PhysicalSocketServer.h"
#include "net/rc/phy/stream/PhysicalSocket.h" // IWYU pragma: export

// IWYU pragma: no_include "net/rc/phy/stream/PhysicalSocket.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::rc::phy::stream {

    class PhysicalSocketServer : public net::rc::phy::stream::PhysicalSocket<net::phy::stream::PhysicalSocketServer> {
    public:
        using Super = net::rc::phy::stream::PhysicalSocket<net::phy::stream::PhysicalSocketServer>;
        using Super::Super;

        PhysicalSocketServer(PhysicalSocketServer&&) noexcept = default;

        ~PhysicalSocketServer() override;
    };

} // namespace net::rc::phy::stream

extern template class net::phy::stream::PhysicalSocketServer<net::rc::SocketAddress>;
extern template class net::rc::phy::stream::PhysicalSocket<net::phy::stream::PhysicalSocketServer>;
extern template class net::rc::phy::PhysicalSocket<net::phy::stream::PhysicalSocketServer>;

#endif // NET_RC_PHY_STREAM_PHYSICALSOCKETSERVER_H
