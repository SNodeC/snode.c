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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/unistd.h"

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::system {

    int open(const char* pathname, int flags) {
        errno = 0;
        return ::open(pathname, flags);
    }

    ssize_t read(int fd, void* buf, std::size_t count) {
        errno = 0;
        return ::read(fd, buf, count);
    }

    ssize_t write(int fd, const void* buf, std::size_t count) {
        errno = 0;
        return ::write(fd, buf, count);
    }

    int close(int fd) {
        errno = 0;
        return ::close(fd);
    }

    int pipe2(int pipefd[2], int flags) {
        errno = 0;
        return ::pipe2(pipefd, flags);
    }

} // namespace core::system
