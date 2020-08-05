/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HTTPUTILS_H
#define HTTPUTILS_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <bits/types/struct_tm.h> // for tm
#include <string>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace httputils {

    std::string url_decode(const std::string& text);

    std::string& str_trimm(std::string& text);

    std::pair<std::string, std::string> str_split(const std::string& base, char c_middle);

    std::pair<std::string, std::string> str_split_last(const std::string& base, char c_middle);

    std::string str_substr_char(const std::string& string, char c, std::string::size_type* pos);

    std::string to_http_date(struct tm* tm = nullptr);

    struct tm from_http_date(const std::string& http_date);

    std::string file_mod_http_date(const std::string& filePath);

    std::string::iterator to_lower(std::string& string);

} // namespace httputils

#endif // HTTPUTILS_H
