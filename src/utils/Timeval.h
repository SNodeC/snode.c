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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/time.h" // IWYU pragma: export

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace ttime {

    class Timeval {
    public:
        Timeval(const Timeval& timeVal);
        Timeval(double time);
        Timeval(const struct timeval& timeVal);
        Timeval(const time_t timeVal);
        Timeval();

        Timeval operator=(const Timeval& timeVal);

        Timeval operator+(const Timeval& timeVal) const;
        Timeval operator-(const Timeval& timeVal) const;

        bool operator<(const Timeval& timeVal) const;
        bool operator>(const Timeval& timeVal) const;
        bool operator<=(const Timeval& timeVal) const;
        bool operator>=(const Timeval& timeVal) const;
        bool operator==(const Timeval& timeVal) const;

        operator struct timeval() const;
        operator struct timeval &();
        operator struct timeval &() const;
        operator struct timeval const*() const;
        operator struct timeval *();
        operator double() const;

        struct timeval& getTimeVal();

    private:
        const Timeval& normalize();

        struct timeval timeVal = {0, 0};
    };

} // namespace ttime

bool operator<(const struct timeval& tv1, const struct timeval& tv2);
bool operator>(const struct timeval& tv1, const struct timeval& tv2);
bool operator<=(const struct timeval& tv1, const struct timeval& tv2);
bool operator>=(const struct timeval& tv1, const struct timeval& tv2);
bool operator==(const struct timeval& tv1, const struct timeval& tv2);

struct timeval operator+(const struct timeval& tv1, const struct timeval& tv2);
struct timeval operator-(const struct timeval& tv1, const struct timeval& tv2);

#endif // UTILS_TIMEVAL_H
