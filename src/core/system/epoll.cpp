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

#include "core/system/epoll.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::system {

    int epoll_create1(int flags) {
        errno = 0;
        return ::epoll_create1(flags);
    }

    int epoll_wait(int epfd, epoll_event* events, int maxevents, int timeout) {
        errno = 0;
        return ::epoll_wait(epfd, events, maxevents, timeout);
    }

    int epoll_ctl(int epfd, int op, int fd, epoll_event* event) {
        errno = 0;
        return ::epoll_ctl(epfd, op, fd, event);
    }

} // namespace core::system
