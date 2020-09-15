/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#include <easylogging++.h>
#include <regex>
#include <tuple> // for tie, tuple
#include <utility>
#include <vector> // for vector

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "HTTPRequestParser.h"
#include "http_utils.h"

namespace http {

    HTTPRequestParser::HTTPRequestParser(
        const std::function<void(const std::string&, const std::string&, const std::string&, const std::map<std::string, std::string>&)>&
            onRequest,
        const std::function<void(const std::map<std::string, std::string>&, const std::map<std::string, std::string>&)>& onHeader,
        const std::function<void(char*, size_t)>& onContent, const std::function<void(HTTPRequestParser&)>& onParsed,
        const std::function<void(int status, const std::string& reason)>& onError)
        : onRequest(onRequest)
        , onHeader(onHeader)
        , onContent(onContent)
        , onParsed(onParsed)
        , onError(onError) {
    }

    void HTTPRequestParser::reset() {
        HTTPParser::reset();
        method.clear();
        url.clear();
        httpVersion.clear();
        queries.clear();
        cookies.clear();
        httpMajor = 0;
        httpMinor = 0;
    }

    enum HTTPParser::PAS HTTPRequestParser::parseStartLine(std::string& line) {
        enum HTTPParser::PAS PAS = HTTPParser::PAS::HEADER;

        if (!line.empty()) {
            std::string remaining;

            std::tie(method, remaining) = httputils::str_split(line, ' ');
            std::tie(url, httpVersion) = httputils::str_split(remaining, ' ');

            std::string queriesLine;
            std::tie(std::ignore, queriesLine) = httputils::str_split(url, '?');

            if (!methodSupported(method)) {
                PAS = parsingError(400, "Bad request method");
            } else if (url.empty() || url.front() != '/') {
                PAS = parsingError(400, "Malformed request");
            } else {
                std::smatch httpVersionMatch;
                if (!std::regex_match(httpVersion, httpVersionMatch, httpVersionRegex)) {
                    PAS = parsingError(400, "Wrong protocol-version");
                } else {
                    httpMajor = std::stoi(httpVersionMatch.str(1));
                    httpMinor = std::stoi(httpVersionMatch.str(2));

                    while (!queriesLine.empty()) {
                        std::string query;
                        std::tie(query, queriesLine) = httputils::str_split(queriesLine, '&');

                        std::string key;
                        std::string value;
                        std::tie(key, value) = httputils::str_split(query, '=');

                        queries.insert({key, value});
                    }

                    onRequest(method, url, httpVersion, queries);
                }
            }
        } else {
            PAS = parsingError(400, "Request-line empty");
        }

        return PAS;
    }

    enum HTTPParser::PAS HTTPRequestParser::parseHeader() {
        for (auto& [field, value] : HTTPParser::headers) {
            VLOG(2) << "++ Parse header field: " << field << " = " << value;
            if (field != "cookie") {
                if (field == "content-length") {
                    HTTPParser::contentLength = std::stoi(value);
                }
            } else {
                std::string cookiesLine = value;

                while (!cookiesLine.empty()) {
                    std::string cookieLine;
                    std::tie(cookieLine, cookiesLine) = httputils::str_split(cookiesLine, ',');

                    while (!cookieLine.empty()) {
                        std::string cookie;
                        std::tie(cookie, cookieLine) = httputils::str_split(cookieLine, ';');

                        std::string name;
                        std::string value;
                        std::tie(name, value) = httputils::str_split(cookie, '=');
                        httputils::str_trimm(name);
                        httputils::str_trimm(value);

                        VLOG(2) << "++ Cookie: " << name << " = " << value;

                        cookies.insert({name, value});
                    }
                }
            }
        }

        HTTPParser::headers.erase("cookie");

        onHeader(HTTPParser::headers, cookies);

        enum HTTPParser::PAS PAS = HTTPParser::PAS::BODY;
        if (contentLength == 0) {
            parsingFinished();
            PAS = PAS::FIRSTLINE;
        }

        return PAS;
    }

    enum HTTPParser::PAS HTTPRequestParser::parseContent(char* content, size_t size) {
        onContent(content, size);
        parsingFinished();

        return PAS::FIRSTLINE;
    }

    void HTTPRequestParser::parsingFinished() {
        onParsed(*this);
    }

    enum HTTPParser::PAS HTTPRequestParser::parsingError(int code, const std::string& reason) {
        onError(code, reason);
        reset();

        return PAS::ERROR;
    }

} // namespace http
