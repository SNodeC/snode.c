/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#include "express/dispatcher/regex_utils.h"

#include "express/Request.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <ext/alloc_traits.h>
#include <map>
#include <memory>
#include <sstream>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::dispatcher {

    std::string path_concat(const std::vector<std::string>& stringvec) {
        std::string s;

        for (std::vector<std::string>::size_type i = 0; i < stringvec.size(); i++) {
            if (!stringvec[i].empty() && stringvec[i].front() != ' ') {
                s += "\\/" + stringvec[i];
            }
        }

        return s;
    }

    std::vector<std::string> explode(const std::string& s, char delim) {
        std::vector<std::string> result;
        std::istringstream iss(s);

        for (std::string token; std::getline(iss, token, delim);) {
            result.push_back(std::move(token));
        }

        return result;
    }

#define PATH_REGEX ":[a-zA-Z0-9]+(\\(.+\\))?"
    const std::regex& pathRegex() {
        static const std::regex pathregex(PATH_REGEX);

        return pathregex;
    }

    std::smatch matchResult(const std::string& cpath) {
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
                const std::smatch smatch = matchResult(explodedString[i]);
                std::string regex = "(.*)";

                if (smatch.size() > 1) {
                    if (smatch[1] != "") {
                        regex = smatch[1];
                    }
                }

                explodedString[i] = regex;
            }
        }

        const std::string regexPath = path_concat(explodedString);

        return std::regex_match(reqpath, std::regex(regexPath));
    }

    void setParams(const std::string& cpath, Request& req) {
        std::vector<std::string> explodedString = explode(cpath, '/');
        std::vector<std::string> explodedReqString = explode(req.url, '/');

        for (std::vector<std::string>::size_type i = 0; i < explodedString.size() && i < explodedReqString.size(); i++) {
            if (explodedString[i].front() == ':') {
                const std::smatch smatch = matchResult(explodedString[i]);
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
        return hasResult(cpath) && matchFunction(cpath, reqpath);
    }

} // namespace express::dispatcher
