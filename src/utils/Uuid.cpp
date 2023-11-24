/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "Uuid.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <random>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace utils {

    std::string Uuid::getUuid() {
        static std::random_device dev;
        static std::mt19937 rng(dev());

        std::uniform_int_distribution<int> dist(0, 15);

        const char* v = "0123456789abcdef";
        const bool dash[] = {false, false, false, false, true, false, true, false, true, false, true, false, false, false, false, false};

        std::string res;
        for (const bool isDash : dash) {
            if (isDash) {
                res += "-";
            }
            res += v[dist(rng)];
            res += v[dist(rng)];
        }
        return res;
    }

} // namespace utils
