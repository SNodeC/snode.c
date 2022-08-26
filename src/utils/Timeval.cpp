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

#include "utils/Timeval.h"

#include "log/Logger.h"

#include <climits>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace utils {

    Timeval::Timeval() noexcept
        : timeVal({}) {
    }

    Timeval::Timeval(const std::initializer_list<time_t>& initList) noexcept
        : timeVal({}) {
        if (initList.size() == 2) {
            timeVal.tv_sec = *initList.begin();
            timeVal.tv_usec = static_cast<suseconds_t>(*(initList.begin() + 1));
        } else {
            LOG(WARNING) << "Timeval initialized with an list size != 2. Initializing Timeval with 0";
        }
    }

    Timeval::Timeval(const Timeval& timeVal) noexcept
        : timeVal(timeVal.timeVal) {
    }

    Timeval::Timeval(double time) noexcept {
        timeVal.tv_sec = static_cast<time_t>(time);
        timeVal.tv_usec = static_cast<suseconds_t>((time - static_cast<double>(timeVal.tv_sec)) * 1'000'000.0);

        normalize();
    }

    Timeval::Timeval(const timeval& timeVal) noexcept
        : timeVal(timeVal) {
    }

    Timeval Timeval::currentTime() {
        utils::Timeval currentTime;
        core::system::gettimeofday(&currentTime, nullptr);

        return currentTime;
    }

    Timeval& Timeval::operator=(const Timeval& timeVal) {
        this->timeVal.tv_sec = timeVal.timeVal.tv_sec;
        this->timeVal.tv_usec = timeVal.timeVal.tv_usec;

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

    Timeval Timeval::operator-() const {
        Timeval timeVal;

        timeVal.timeVal.tv_sec = -this->timeVal.tv_sec;
        timeVal.timeVal.tv_usec = -this->timeVal.tv_usec;

        return timeVal.normalize();
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

    int Timeval::ms() const {
        int ms = 0;

        if (timeVal.tv_sec > INT_MAX - 1) {
            ms = INT_MAX;
        } else {
            ms = static_cast<int>(static_cast<double>(timeVal.tv_sec) * 1'000. + static_cast<double>(timeVal.tv_usec) / 1'000. + 0.5);
        }

        return ms;
    }

    double Timeval::msd() const {
        double msd = 0;
        if (timeVal.tv_sec > INT_MAX - 1) {
            msd = INT_MAX;
        } else {
            msd = static_cast<double>(timeVal.tv_sec) * 1'000. + static_cast<double>(timeVal.tv_usec) / 1'000.;
        }

        return msd;
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

        return *this;
    }

    std::ostream& operator<<(std::ostream& ostream, const utils::Timeval& timeVal) {
        return ostream << std::string("{") + std::to_string(timeVal.timeVal.tv_sec) + std::string(":") +
                              std::to_string(timeVal.timeVal.tv_usec) + std::string("}");
    }

} // namespace utils
