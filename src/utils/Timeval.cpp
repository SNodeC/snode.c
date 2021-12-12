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

#include "utils/Timeval.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

struct timeval;

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace ttime {

    Timeval::Timeval(double time) {
        timeVal.tv_sec = (time_t) time;
        timeVal.tv_usec = (suseconds_t) ((time - (double) timeVal.tv_sec) * 1000000);

        normalize();
    }

    Timeval::Timeval(const timeval& timeVal)
        : Timeval((double) timeVal.tv_sec + (double) timeVal.tv_usec / 100000.0) {
    }

    Timeval::Timeval(const time_t timeVal)
        : Timeval({timeVal, 0}) {
    }

    Timeval::Timeval()
        : Timeval((time_t) 0) {
    }

    Timeval::Timeval(const Timeval& timeVal)
        : Timeval(timeVal.timeVal) {
    }

    Timeval Timeval::operator=(const Timeval& timeVal) {
        return this->timeVal = timeVal.timeVal;
    }

    Timeval Timeval::operator+(const Timeval& timeVal) const {
        return Timeval(this->timeVal + timeVal.timeVal).normalize();
    }

    Timeval Timeval::operator-(const Timeval& timeVal) const {
        return Timeval(this->timeVal - timeVal.timeVal).normalize();
    }

    bool Timeval::operator<(const Timeval& timeVal) const {
        return (this->timeVal.tv_sec < timeVal.timeVal.tv_sec) ||
               ((this->timeVal.tv_sec == timeVal.timeVal.tv_sec) && (this->timeVal.tv_usec < timeVal.timeVal.tv_usec));
    }

    bool Timeval::operator>(const Timeval& timeVal) const {
        return timeVal < *this;
    }

    bool Timeval::operator<=(const Timeval& timeVal) const {
        return !(*this > timeVal);
    }

    bool Timeval::operator>=(const Timeval& timeVal) const {
        return !(*this < timeVal);
    }

    bool Timeval::operator==(const Timeval& timeVal) const {
        return !(*this < timeVal) && !(*this > timeVal);
    }

    Timeval::operator timeval() const {
        return timeVal;
    }

    Timeval::operator timeval const*() const {
        return &timeVal;
    }

    Timeval::operator timeval*() {
        return &timeVal;
    }

    Timeval::operator double() const {
        return (double) timeVal.tv_sec + (double) timeVal.tv_usec / 1000000.0;
    }

    timeval& Timeval::getTimeVal() {
        return timeVal;
    }

    const Timeval& Timeval::normalize() {
        if (timeVal.tv_usec > 999999) {
            timeVal.tv_usec -= 1000000;
            timeVal.tv_sec++;
        } else if (timeVal.tv_usec < 0) {
            timeVal.tv_usec += 1000000;
            timeVal.tv_sec--;
        }

        if (timeVal.tv_sec < 0) {
            timeVal = {0, 0};
        }

        return *this;
    }

} // namespace ttime

bool operator<(const struct timeval& tv1, const struct timeval& tv2) {
    return (tv1.tv_sec < tv2.tv_sec) || ((tv1.tv_sec == tv2.tv_sec) && (tv1.tv_usec < tv2.tv_usec));
}

bool operator>(const struct timeval& tv1, const struct timeval& tv2) {
    return (tv2.tv_sec < tv1.tv_sec) || ((tv2.tv_sec == tv1.tv_sec) && (tv2.tv_usec < tv1.tv_usec));
}

bool operator<=(const struct timeval& tv1, const struct timeval& tv2) {
    return !(tv1 > tv2);
}

bool operator>=(const struct timeval& tv1, const struct timeval& tv2) {
    return !(tv1 < tv2);
}

bool operator==(const struct timeval& tv1, const struct timeval& tv2) {
    return !(tv1 < tv2) && !(tv2 < tv1);
}

struct timeval operator+(const struct timeval& tv1, const struct timeval& tv2) {
    struct timeval help {};

    help.tv_sec = tv1.tv_sec + tv2.tv_sec;

    help.tv_usec = tv1.tv_usec + tv2.tv_usec;

    if (help.tv_usec > 999999) {
        help.tv_usec -= 1000000;
        help.tv_sec++;
    }

    return help;
}

struct timeval operator-(const struct timeval& tv1, const struct timeval& tv2) {
    struct timeval help {};

    help.tv_sec = tv1.tv_sec - tv2.tv_sec;
    help.tv_usec = tv1.tv_usec - tv2.tv_usec;

    if (help.tv_usec < 0) {
        help.tv_usec += 1000000;
        help.tv_sec--;
    }

    return help;
}
