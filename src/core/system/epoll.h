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

#ifndef NET_SYSTEM_EPOLL_H
#define NET_SYSTEM_EPOLL_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// IWYU pragma: begin_exports

#include <sys/epoll.h>

// IWYU pragma: end_exports

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::system {

    int epoll_create1(int flags);
    int epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout);
    int epoll_ctl(int epfd, int op, int fd, struct epoll_event* event);

} // namespace core::system

#endif // NET_SYSTEM_EPOLL_H
