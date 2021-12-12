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

#ifndef UTILS_TIMEVAL_H
#define UTILS_TIMEVAL_H

#include "core/system/time.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

struct timeval;
#include <iostream>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace utils {

    class Timeval {
    public:
        Timeval();
        Timeval(const Timeval& timeVal);
        Timeval(time_t tv_sec, suseconds_t tv_usec);
        Timeval(double time);
        Timeval(const time_t timeVal);

        Timeval operator=(const Timeval& timeVal);

        Timeval operator+(const Timeval& timeVal) const;
        Timeval operator+=(const Timeval& timeVal);

        Timeval operator-(const Timeval& timeVal) const;
        Timeval operator-=(const Timeval& timeVal);

        bool operator<(const Timeval& timeVal) const;
        bool operator>(const Timeval& timeVal) const;
        bool operator<=(const Timeval& timeVal) const;
        bool operator>=(const Timeval& timeVal) const;
        bool operator==(const Timeval& timeVal) const;

        operator timeval() const;

        operator timeval&();
        operator timeval const&() const;

        operator timeval*();
        operator timeval const*() const;

        operator double();
        operator double() const;

        operator std::string();
        operator std::string() const;

        timeval& getTimeVal();

    private:
        const Timeval& normalize();

        timeval timeVal = {0, 0};
    };

} // namespace utils

std::ostream& operator<<(std::ostream& ostream, const utils::Timeval& timeVal);

#endif // UTILS_TIMEVAL_H
