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

#include "net/un/PhysicalSocket.h"

#include "net/PhysicalSocket.hpp" // IWYU pragma: keep

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cerrno>
#include <cstdio>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un {

    PhysicalSocket::PhysicalSocket(int type, int protocol)
        : Super(PF_UNIX, type, protocol) {
    }

    PhysicalSocket::~PhysicalSocket() {
        if (!doNotRemove && !getBindAddress().address().empty() && std::remove(getBindAddress().address().data()) != 0) {
            PLOG(ERROR) << "remove: sunPath: " << getBindAddress().address();
        }
    }

    int PhysicalSocket::bind(const SocketAddress& bindAddress) {
        int ret = Super::bind(bindAddress);

        doNotRemove = ret != 0 && errno == EADDRINUSE;

        return ret;
    }

} // namespace net::un

template class net::PhysicalSocket<net::un::SocketAddress>;
