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

#include "regex_utils.h"

#include "express/Request.h" // for Request

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>            // for size_t
#include <ext/alloc_traits.h> // for __alloc_traits<>::value_type
#include <map>                // for map, map<>::mapped_type
#include <memory>             // for allocator_traits<>::value_type
#include <sstream>
#include <utility> // for move

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    const std::string path_concat(const std::vector<std::string>& stringvec) {
        std::string s;

        for (std::vector<std::string>::size_type i = 0; i < stringvec.size(); i++) {
            if (!stringvec[i].empty() && stringvec[i].front() != ' ') {
                s += "\\/" + stringvec[i];
            }
        }

        return s;
    }

    const std::vector<std::string> explode(const std::string& s, char delim) {
        std::vector<std::string> result;
        std::istringstream iss(s);

        for (std::string token; std::getline(iss, token, delim);) {
            result.push_back(std::move(token));
        }

        return result;
    }

    const std::regex& pathRegex() {
        static std::regex pathregex = std::regex(PATH_REGEX);

        return pathregex;
    }

    const std::smatch matchResult(const std::string& cpath) {
        std::smatch smatch;

        std::regex_search(cpath, smatch, pathRegex());

        return smatch;
    }

    bool hasResult(const std::string& cpath) {
        std::smatch smatch;

        return std::regex_search(cpath, smatch, pathRegex());
    }

    bool matchFunction(const std::string& cpath, const std::string& reqpath) {
        std::vector<std::string> explodedString = explode(cpath, '/');

        for (std::vector<std::string>::size_type i = 0; i < explodedString.size(); i++) {
            if (explodedString[i].front() == ':') {
                std::smatch smatch = matchResult(explodedString[i]);
                std::string regex = "(.*)";

                if (smatch.size() > 1) {
                    if (smatch[1] != "") {
                        regex = smatch[1];
                    }
                }

                explodedString[i] = regex;
            }
        }

        std::string regexPath = path_concat(explodedString);

        return std::regex_match(reqpath, std::regex(regexPath));
    }

    void setParams(const std::string& cpath, Request& req) {
        std::vector<std::string> explodedString = explode(cpath, '/');
        std::vector<std::string> explodedReqString = explode(req.url, '/');

        for (std::vector<std::string>::size_type i = 0; i < explodedString.size() && i < explodedReqString.size(); i++) {
            if (explodedString[i].front() == ':') {
                std::smatch smatch = matchResult(explodedString[i]);
                std::string regex = "(.*)";

                if (smatch.size() > 1) {
                    if (smatch[1] != "") {
                        regex = smatch[1];
                    }
                }

                if (std::regex_match(explodedReqString[i], std::regex(regex))) {
                    std::string attributeName = smatch[0];
                    attributeName.erase(0, 1);
                    attributeName.erase((attributeName.length() - static_cast<std::size_t>(smatch[1].length())),
                                        static_cast<std::size_t>(smatch[1].length()));

                    req.params[attributeName] = explodedReqString[i];
                }
            }
        }
    }

    std::string path_concat(const std::string& first, const std::string& second) {
        std::string result;

        if (first.back() == '/' && second.front() == '/') {
            result = first + second.substr(1);
        } else if (first.back() != '/' && second.front() != '/') {
            result = first + '/' + second;
        } else {
            result = first + second;
        }

        if (result.length() > 1 && result.back() == '/') {
            result.pop_back();
        }

        return result;
    }

    bool checkForUrlMatch(const std::string& cpath, const std::string& reqpath) {
        bool hasRegex = hasResult(cpath);
        return (!hasRegex && cpath == reqpath) || (hasRegex && matchFunction(cpath, reqpath));
    }

} // namespace express::dispatcher
