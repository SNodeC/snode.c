/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

#ifndef UTILS_TIMEVAL_H
#define UTILS_TIMEVAL_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/system/time.h"

struct timeval;

#include <initializer_list>
#include <iostream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace utils {

    class Timeval {
    public:
        Timeval() noexcept;
        Timeval(const std::initializer_list<time_t>& initList) noexcept;
        Timeval(const Timeval& timeVal) noexcept = default;
        Timeval(double time) noexcept;            // cppcheck-suppress noExplicitConstructor
        Timeval(const timeval& timeVal) noexcept; // cppcheck-suppress noExplicitConstructor

        static Timeval currentTime();

        Timeval& operator=(const Timeval& timeVal);
        Timeval& operator=(const timeval& timeVal);

        Timeval operator+(const Timeval& timeVal) const;
        Timeval operator-(const Timeval& timeVal) const;
        Timeval operator*(double mul) const; // Corresponding commutated arguments operator is a friend function (see below)

        Timeval& operator+=(const Timeval& timeVal);
        Timeval& operator-=(const Timeval& timeVal);
        Timeval& operator*=(double mul);

        Timeval operator-() const;

        bool operator<(const Timeval& timeVal) const;
        bool operator>(const Timeval& timeVal) const;
        bool operator<=(const Timeval& timeVal) const;
        bool operator>=(const Timeval& timeVal) const;
        bool operator==(const Timeval& timeVal) const;
        bool operator!=(const Timeval& timeVal) const;

        timeval* operator&();
        const timeval* operator&() const;

        timespec getTimespec() const;

        int getMs() const;
        double getMsd() const;

    private:
        const Timeval& normalize();

        timeval timeVal = {0, 0};

    public:
        friend std::ostream& operator<<(std::ostream& ostream, const utils::Timeval& timeVal);
        friend Timeval operator*(double mul, const Timeval& timeVal);
    };

    //    Timeval operator*(double mul, const Timeval& timeVal);

} // namespace utils

#endif // UTILS_TIMEVAL_H
