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

#ifndef NET_IN6_PHY_STREAM_PHYSICALSOCKETCLIENT_H
#define NET_IN6_PHY_STREAM_PHYSICALSOCKETCLIENT_H

#include "net/in6/phy/stream/PhysicalSocket.h" // IWYU pragma: export
#include "net/phy/stream/PhysicalSocketClient.h"

// IWYU pragma: no_include "net/in6/phy/stream/PhysicalSocket.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in6::phy::stream {

    class PhysicalSocketClient : public net::in6::phy::stream::PhysicalSocket<net::phy::stream::PhysicalSocketClient> {
    public:
        using Super = net::in6::phy::stream::PhysicalSocket<net::phy::stream::PhysicalSocketClient>;
        using Super::Super;

        PhysicalSocketClient(PhysicalSocketClient&&) noexcept = default;

        ~PhysicalSocketClient() override;
    };

} // namespace net::in6::phy::stream

extern template class net::phy::stream::PhysicalSocketClient<net::in6::SocketAddress>;
extern template class net::in6::phy::stream::PhysicalSocket<net::phy::stream::PhysicalSocketClient>;
extern template class net::in6::phy::PhysicalSocket<net::phy::stream::PhysicalSocketClient>;

#endif // NET_IN6_PHY_STREAM_PHYSICALSOCKETCLIENT_H
