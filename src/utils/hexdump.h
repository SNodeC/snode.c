/*
 * snode.c - a slim toolkit for network communication
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

#ifndef UTILS_HEXDUMP_H
#define UTILS_HEXDUMP_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace utils {

    std::string hexDump(const std::vector<char>& bytes, int prefixLength = 0, bool prefixAtFirstLine = false);
    std::string hexDump(const std::string& string, int prefixLength = 0, bool prefixAtFirstLine = false);
    std::string hexDump(const char* bytes, uint64_t length, int prefixLength = 0, bool prefixAtFirstLine = false);

} // namespace utils

#endif // UTILS_HEXDUMP_H
