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

#include "net/phy/stream/PhysicalServerSocket.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/socket.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::stream {

    template <typename PhysicalSocket>
    int PhysicalServerSocket<PhysicalSocket>::listen(int backlog) {
        return core::system::listen(Super::getFd(), backlog);
    }

    template <typename PhysicalSocket>
    int PhysicalServerSocket<PhysicalSocket>::accept() {
        return core::system::accept(Super::getFd(), nullptr, nullptr);
    }

    template <typename PhysicalSocket>
    int PhysicalServerSocket<PhysicalSocket>::accept4(int flags) {
        return core::system::accept4(Super::getFd(), nullptr, nullptr, flags);
    }

} // namespace net::stream
