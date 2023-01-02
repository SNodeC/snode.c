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

#ifndef NET_SYSTEM_SELECT_H
#define NET_SYSTEM_SELECT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// IWYU pragma: begin_exports

#include <sys/select.h>

// IWYU pragma: end_exports

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::system {

    // #include <sys/socket.h>
    int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout);

} // namespace core::system

#endif // NET_SYSTEM_SELECT_H
