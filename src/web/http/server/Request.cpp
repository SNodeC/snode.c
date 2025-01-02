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

#include "web/http/server/Request.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <iterator>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    const std::string& Request::get(const std::string& key, int i) const {
        if (headers.find(key) != headers.end()) {
            std::pair<std::multimap<std::string, std::string>::const_iterator, std::multimap<std::string, std::string>::const_iterator>
                range = headers.equal_range(key);

            if (std::distance(range.first, range.second) >= i) {
                std::advance(range.first, i);
                return (*(range.first)).second;
            }

            return nullstr;
        }

        return nullstr;
    }

    const std::string& Request::cookie(const std::string& key) const {
        const std::map<std::string, std::string>::const_iterator it = cookies.find(key);

        if (it != cookies.end()) {
            return it->second;
        }

        return nullstr;
    }

    const std::string& Request::query(const std::string& key) const {
        const std::map<std::string, std::string>::const_iterator it = queries.find(key);

        if (it != queries.end()) {
            return it->second;
        }

        return nullstr;
    }

} // namespace web::http::server
