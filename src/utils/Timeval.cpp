/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h"

#include <climits>
#include <cstdint>
#include <string>
#include <sys/types.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace utils {

    Timeval::Timeval() noexcept
        : timeVal({}) {
    }

    Timeval::Timeval(const std::initializer_list<time_t>& initList) noexcept
        : timeVal({}) {
        timeVal.tv_sec = *initList.begin();
        timeVal.tv_usec = static_cast<suseconds_t>(*(initList.begin() + 1));
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
        utils::system::gettimeofday(&currentTime, nullptr);

        return currentTime;
    }

    Timeval& Timeval::operator=(const Timeval& timeVal) { // NOLINT
        if (&this->timeVal != &timeVal) {
            this->timeVal.tv_sec = timeVal.timeVal.tv_sec;
            this->timeVal.tv_usec = timeVal.timeVal.tv_usec;
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

    Timeval Timeval::operator*(double mul) const {
        utils::Timeval help;

        help.timeVal.tv_sec = static_cast<time_t>(static_cast<double>(this->timeVal.tv_sec) * mul);
        help.timeVal.tv_usec = static_cast<suseconds_t>(static_cast<double>(this->timeVal.tv_usec) * mul);

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

    Timeval& Timeval::operator*=(double mul) {
        *this = (*this * mul).normalize();
        return *this;
    }

    Timeval Timeval::operator-() const {
        return Timeval({-this->timeVal.tv_sec, -this->timeVal.tv_usec}).normalize();
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

    timespec Timeval::getTimespec() const {
        return timespec{timeVal.tv_sec, static_cast<int32_t>(timeVal.tv_usec * 1000)};
    }

    int Timeval::getMs() const {
        int ms = 0;

        if (timeVal.tv_sec > INT_MAX - 1) {
            ms = INT_MAX;
        } else {
            ms = static_cast<int>(static_cast<double>(timeVal.tv_sec) * 1'000. + static_cast<double>(timeVal.tv_usec) / 1'000. + 0.5);
        }

        return ms;
    }

    double Timeval::getMsd() const {
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
        while (timeVal.tv_usec > 999'999 || timeVal.tv_usec < 0) {
            if (timeVal.tv_usec > 999'999) {
                timeVal.tv_usec -= 1'000'000;
                timeVal.tv_sec++;
            } else if (timeVal.tv_usec < 0) {
                timeVal.tv_usec += 1'000'000;
                timeVal.tv_sec--;
            }
        }

        return *this;
    }

    std::ostream& operator<<(std::ostream& ostream, const utils::Timeval& timeVal) {
        return ostream << std::string("{") + std::to_string(timeVal.timeVal.tv_sec) + std::string(":") +
                              std::to_string(timeVal.timeVal.tv_usec) + std::string("}");
    }

    Timeval operator*(double mul, const utils::Timeval& timeVal) {
        utils::Timeval help;

        help.timeVal.tv_sec = static_cast<time_t>(static_cast<double>(timeVal.timeVal.tv_sec) * mul);
        help.timeVal.tv_usec = static_cast<suseconds_t>(static_cast<double>(timeVal.timeVal.tv_usec) * mul);

        return help.normalize();
    }

} // namespace utils
