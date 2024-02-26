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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "CiStringMap.h"

#include <algorithm>
#include <cctype>
#include <strings.h>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http {

    bool ciContains(const std::string& str1, const std::string& str2) {
        auto it = std::search(str1.begin(), str1.end(), str2.begin(), str2.end(), [](char ch1, char ch2) {
            return std::tolower(ch1) == std::tolower(ch2);
        });

        return (it != str1.end());
    }

    bool ciEquals(const std::string& str1, const std::string& str2) {
        return std::equal(str1.begin(), str1.end(), str2.begin(), str2.end(), [](char ch1, char ch2) {
            return std::tolower(ch1) == std::tolower(ch2);
        });
    }

    bool ciLess::operator()(const std::string& a, const std::string& b) const {
        return ::strcasecmp(a.c_str(), b.c_str()) < 0;
    }

} // namespace web::http
