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

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace utils {

    Timeval::Timeval()
        : timeVal({0, 0}) {
    }

    Timeval::Timeval(const Timeval& timeVal) {
        this->timeVal = timeVal.timeVal;
    }

    Timeval::Timeval(time_t tv_sec, suseconds_t tv_usec) {
        this->timeVal.tv_sec = tv_sec;
        this->timeVal.tv_usec = tv_usec;

        normalize();
    }

    Timeval::Timeval(double time) {
        timeVal.tv_sec = (time_t) time;
        timeVal.tv_usec = (suseconds_t) ((time - (double) timeVal.tv_sec) * 1000000.0);
        normalize();
    }

    Timeval::Timeval(const time_t timeVal)
        : Timeval({timeVal, 0}) {
    }

    Timeval Timeval::operator=(const Timeval& timeVal) {
        this->timeVal.tv_sec = timeVal.timeVal.tv_sec;
        this->timeVal.tv_usec = timeVal.timeVal.tv_usec;

        return *this;
    }

    Timeval Timeval::operator+(const Timeval& timeVal) const {
        utils::Timeval help;

        help.timeVal.tv_sec = this->timeVal.tv_sec + timeVal.timeVal.tv_sec;
        help.timeVal.tv_usec = this->timeVal.tv_usec + timeVal.timeVal.tv_usec;

        return help.normalize();
    }

    Timeval Timeval::operator+=(const Timeval& timeVal) {
        *this = (*this + timeVal).normalize();
        return *this;
    }

    Timeval Timeval::operator-(const Timeval& timeVal) const {
        utils::Timeval help;

        help.timeVal.tv_sec = this->timeVal.tv_sec - timeVal.timeVal.tv_sec;
        help.timeVal.tv_usec = this->timeVal.tv_usec - timeVal.timeVal.tv_usec;

        return help.normalize();
    }

    Timeval Timeval::operator-=(const Timeval& timeVal) {
        *this = (*this - timeVal).normalize();
        return *this;
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

    Timeval::operator timeval&() {
        return timeVal;
    }

    Timeval::operator timeval const&() const {
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

    utils::Timeval::operator std::string() {
        std::string string = "{" + std::to_string(timeVal.tv_sec) + ":" + std::to_string(timeVal.tv_usec) + "}";
        return string;
    }

    utils::Timeval::operator std::string() const {
        std::string string = "{" + std::to_string(timeVal.tv_sec) + ":" + std::to_string(timeVal.tv_usec) + "}";
        return string;
    }

    Timeval::operator double() {
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

} // namespace utils

std::ostream& operator<<(std::ostream& ostream, const utils::Timeval& timeVal) {
    ostream << "TimeVal: " << (std::string) timeVal;
    return ostream;
}
