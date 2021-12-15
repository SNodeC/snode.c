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

#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>
#include <sys/types.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace utils {

    Timeval::Timeval()
        : timeVal({0, 0}) {
    }

    Timeval::Timeval(const std::initializer_list<time_t>& initList)
        : timeVal({0, 0}) {
        if (initList.size() == 2) {
            timeVal.tv_sec = *initList.begin();
            timeVal.tv_usec = static_cast<useconds_t>(*(initList.begin() + 1));
        } else {
            LOG(WARNING) << "Timeval initialized with an list size != 2. Initializing Timeval with 0";
        }
    }

    Timeval::Timeval(const Timeval& timeVal) {
        this->timeVal = timeVal.timeVal;
    }

    Timeval::Timeval(double time) {
        timeVal.tv_sec = static_cast<time_t>(time);
        timeVal.tv_usec = static_cast<useconds_t>((time - (double) timeVal.tv_sec) * 1'000'000.0);

        normalize();
    }

    Timeval::Timeval(const timeval& timeVal)
        : timeVal(timeVal) {
    }

    Timeval Timeval::currentTime() {
        utils::Timeval currentTime;
        core::system::gettimeofday(&currentTime, NULL);

        return currentTime;
    }

    Timeval& Timeval::operator=(const Timeval& timeVal) {
        this->timeVal.tv_sec = timeVal.timeVal.tv_sec;
        this->timeVal.tv_usec = timeVal.timeVal.tv_usec;

        return *this;
    }

    Timeval& Timeval::operator=(const std::initializer_list<time_t>& initList) {
        if (initList.size() == 2) {
            timeVal.tv_sec = *initList.begin();
            timeVal.tv_usec = static_cast<useconds_t>(*(initList.begin() + 1));
        } else {
            this->timeVal.tv_sec = 0;
            this->timeVal.tv_usec = 0;
            LOG(WARNING) << "Timeval assigned with an list size != 2. Assigning Timeval with 0";
        }

        return *this;
    }

    Timeval& Timeval::operator=(const timeval& timeVal) {
        this->timeVal = timeVal;

        return *this;
    }

    Timeval Timeval::operator+(const Timeval& timeVal) const {
        utils::Timeval help;

        help.timeVal.tv_sec = this->timeVal.tv_sec + timeVal.timeVal.tv_sec;
        help.timeVal.tv_usec = this->timeVal.tv_usec + timeVal.timeVal.tv_usec;

        return help.normalize();
    }

    Timeval Timeval::operator-(const Timeval& timeVal) const {
        utils::Timeval help;

        help.timeVal.tv_sec = this->timeVal.tv_sec - timeVal.timeVal.tv_sec;
        help.timeVal.tv_usec = this->timeVal.tv_usec - timeVal.timeVal.tv_usec;

        return help.normalize();
    }

    Timeval& Timeval::operator+=(const Timeval& timeVal) {
        *this = (*this + timeVal).normalize();
        return *this;
    }

    Timeval& Timeval::operator-=(const Timeval& timeVal) {
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

    bool Timeval::operator!=(const Timeval& timeVal) const {
        return !(*this == timeVal);
    }

    timeval* Timeval::operator&() {
        return &timeVal;
    }

    const timeval* Timeval::operator&() const {
        return &timeVal;
    }

    const Timeval& Timeval::normalize() {
        if (timeVal.tv_usec > 999'999) {
            timeVal.tv_usec -= 1'000'000;
            timeVal.tv_sec++;
        } else if (timeVal.tv_usec < 0) {
            timeVal.tv_usec += 1'000'000;
            timeVal.tv_sec--;
        }

        if (timeVal.tv_sec < 0) {
            timeVal = {0, 0};
        }

        return *this;
    }

    std::ostream& operator<<(std::ostream& ostream, const utils::Timeval& timeVal) {
        return ostream << "{" + std::to_string(timeVal.timeVal.tv_sec) + ":" + std::to_string(timeVal.timeVal.tv_usec) + "}";
    }

} // namespace utils
