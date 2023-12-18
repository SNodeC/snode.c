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

#ifndef NET_L2_PHY_STREAM_PHYSICALSOCKETCLIENT_H
#define NET_L2_PHY_STREAM_PHYSICALSOCKETCLIENT_H

#include "net/l2/phy/stream/PhysicalSocket.h"    // IWYU pragma: export
#include "net/phy/stream/PhysicalSocketClient.h" // IWYU pragma: export

// IWYU pragma: no_include "net/l2/phy/stream/PhysicalSocket.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::l2::phy::stream {

    class PhysicalSocketClient : public net::l2::phy::stream::PhysicalSocket<net::phy::stream::PhysicalSocketClient> {
    private:
        using Super = net::l2::phy::stream::PhysicalSocket<net::phy::stream::PhysicalSocketClient>;

    public:
        using Super::Super;

        PhysicalSocketClient(PhysicalSocketClient&&) noexcept = default;

        ~PhysicalSocketClient() override;
    };

} // namespace net::l2::phy::stream

extern template class net::phy::stream::PhysicalSocketClient<net::l2::SocketAddress>;
extern template class net::l2::phy::stream::PhysicalSocket<net::phy::stream::PhysicalSocketClient>;
extern template class net::l2::phy::PhysicalSocket<net::phy::stream::PhysicalSocketClient>;

#endif // NET_L2_PHY_STREAM_PHYSICALSOCKETCLIENT_H
