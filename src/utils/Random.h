/*
 * SNode.C - a slim toolkit for network communication
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

#ifndef UTILS_RANDOM_H
#define UTILS_RANDOM_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace std {
    class random_device; // IWYU pragma: keep // NOLINT
} // namespace std

// IWYU pragma: no_include <random>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace utils {

    class Random {
    public:
        static double getInRange(double ll, double ul);

    private:
        static std::random_device rd;
    };

} // namespace utils

#endif // UTILS_RANDOM_H
