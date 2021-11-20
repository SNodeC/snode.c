/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#ifndef NET_SYSTEM_UNISTD_H
#define NET_SYSTEM_UNISTD_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

// IWYU pragma: begin_exports

#include <cstddef> // for size_t
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h> // for ssize_t
#include <unistd.h>

// IWYU pragma: end_exports

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::system {

    // #include <sys/types.h>, #include <sys/stat.h>, #include <fcntl.h>
    int open(const char* pathname, int flags);

    // #include <unistd.h>
    ssize_t read(int fd, void* buf, std::size_t count);
    ssize_t write(int fd, const void* buf, std::size_t count);
    int close(int fd);
    int pipe2(int pipefd[2], int flags);

} // namespace core::system

#endif // NET_SYSTEM_UNISTD_H
