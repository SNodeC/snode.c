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

#ifndef EXPRESS_DISPATCHER_REGEX_UTILS_H
#define EXPRESS_DISPATCHER_REGEX_UTILS_H

namespace express {

    class Request;

}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <regex>
#include <string>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    const std::string path_concat(const std::vector<std::string>& stringvec);

    const std::vector<std::string> explode(const std::string& s, char delim);

    const std::regex& pathRegex();

    const std::smatch matchResult(const std::string& cpath);

    bool hasResult(const std::string& cpath);

    bool matchFunction(const std::string& cpath, const std::string& reqpath);

    void setParams(const std::string& cpath, Request& req);

    bool checkForUrlMatch(const std::string& cpath, const std::string& reqpath);

    std::string path_concat(const std::string& first, const std::string& second);

} // namespace express::dispatcher

#endif // EXPRESS_DISPATCHER_REGEX_UTILS_H
