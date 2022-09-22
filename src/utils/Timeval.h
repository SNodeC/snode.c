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

#ifndef UTILS_TIMEVAL_H
#define UTILS_TIMEVAL_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/time.h" // IWYU pragma: export

struct timeval;
#include <initializer_list>
#include <iostream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace utils {

    class Timeval {
    public:
        Timeval() noexcept;
        Timeval(const std::initializer_list<time_t>& initList) noexcept; // cppcheck-suppress noExplicitConstructor
        Timeval(const Timeval& timeVal) noexcept = default;
        Timeval(double time) noexcept;            // cppcheck-suppress noExplicitConstructor
        Timeval(const timeval& timeVal) noexcept; // cppcheck-suppress noExplicitConstructor

        static Timeval currentTime();

        Timeval& operator=(const Timeval& timeVal);
        Timeval& operator=(const timeval& timeVal);

        Timeval operator+(const Timeval& timeVal) const;
        Timeval operator-(const Timeval& timeVal) const;

        Timeval& operator+=(const Timeval& timeVal);
        Timeval& operator-=(const Timeval& timeVal);

        Timeval operator-() const;

        bool operator<(const Timeval& timeVal) const;
        bool operator>(const Timeval& timeVal) const;
        bool operator<=(const Timeval& timeVal) const;
        bool operator>=(const Timeval& timeVal) const;
        bool operator==(const Timeval& timeVal) const;
        bool operator!=(const Timeval& timeVal) const;

        timeval* operator&();
        const timeval* operator&() const;

        int ms() const;
        double msd() const;

    private:
        const Timeval& normalize();

        timeval timeVal = {0, 0};

    public:
        friend std::ostream& operator<<(std::ostream& ostream, const utils::Timeval& timeVal);
    };

} // namespace utils

#endif // UTILS_TIMEVAL_H
