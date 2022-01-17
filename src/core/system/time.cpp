/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "core/system/time.h"

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::system {

    time_t time(time_t* tloc) {
        errno = 0;
        return ::time(tloc);
    }

    int gettimeofday(struct timeval* tv, struct timezone* tz) {
        errno = 0;
        return ::gettimeofday(tv, tz);
    }

    struct tm* gmtime(const time_t* timep) {
        errno = 0;
        return ::gmtime(timep);
    }

    time_t mktime(struct tm* tm) {
        errno = 0;
        return ::mktime(tm);
    }

} // namespace core::system
