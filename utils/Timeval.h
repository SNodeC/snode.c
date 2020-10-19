/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef TIMEVAL_H
#define TIMEVAL_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/time.h> // IWYU pragma: export

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

bool operator<(const struct timeval& tv1, const struct timeval& tv2);
bool operator>(const struct timeval& tv1, const struct timeval& tv2);
bool operator<=(const struct timeval& tv1, const struct timeval& tv2);
bool operator>=(const struct timeval& tv1, const struct timeval& tv2);
bool operator==(const struct timeval& tv1, const struct timeval& tv2);

struct timeval operator+(const struct timeval& tv1, const struct timeval& tv2);
struct timeval operator-(const struct timeval& tv1, const struct timeval& tv2);

#endif // TIMEVAL_H
