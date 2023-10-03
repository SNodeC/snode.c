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

#include "net/rc/stream/PhysicalSocketClient.h"

#include "net/phy/stream/PhysicalSocketClient.hpp" // IWYU pragma: keep
#include "net/rc/stream/PhysicalSocket.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::rc::stream {

    PhysicalSocketClient::~PhysicalSocketClient() {
    }

} // namespace net::rc::stream

template class net::phy::stream::PhysicalSocketClient<net::rc::SocketAddress>;
template class net::rc::stream::PhysicalSocket<net::phy::stream::PhysicalSocketClient>;
template class net::rc::PhysicalSocket<net::phy::stream::PhysicalSocketClient>;
