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

#include "core/system/poll.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

// IWYU pragma: no_include <time.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::system {

    int poll(pollfd* fds, nfds_t nfds, int timeout) {
        errno = 0;
        return ::poll(fds, nfds, timeout);
    }

    int ppoll(struct pollfd* fds, nfds_t nfds, const timespec* timeout, const sigset_t* sigMask) {
        errno = 0;
        return ::ppoll(fds, nfds, timeout, sigMask);
    }

} // namespace core::system
