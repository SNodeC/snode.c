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

#include "web/http/server/RequestParser.h"

#include "web/http/http_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <regex>
#include <tuple>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    RequestParser::RequestParser(core::socket::stream::SocketContext* socketContext,
                                 const std::function<void(Request&)>& onParsed,
                                 const std::function<void(int, const std::string&)>& onError)
        : Parser(socketContext)
        , onParsed(onParsed)
        , onError(onError) {
    }

    bool RequestParser::methodSupported(const std::string& method) const {
        return supportedMethods.contains(method);
    }

    void RequestParser::begin() {
    }

    Parser::ParserState RequestParser::parseStartLine(const std::string& line) {
        Parser::ParserState parserState = Parser::ParserState::HEADER;

        if (!line.empty()) {
            std::string remaining;

            std::tie(request.method, remaining) = httputils::str_split(line, ' ');
            std::tie(request.url, request.httpVersion) = httputils::str_split(remaining, ' ');

            std::string queriesLine;
            std::tie(std::ignore, queriesLine) = httputils::str_split(request.url, '?');

            if (!methodSupported(request.method)) {
                parserState = parsingError(400, "Bad request method");
            } else if (request.url.empty() || request.url.front() != '/') {
                parserState = parsingError(400, "Malformed request");
            } else {
                std::smatch httpVersionMatch;
                if (!std::regex_match(request.httpVersion, httpVersionMatch, httpVersionRegex)) {
                    parserState = parsingError(400, "Wrong protocol-version");
                } else {
                    request.httpMajor = std::stoi(httpVersionMatch.str(1));
                    request.httpMinor = std::stoi(httpVersionMatch.str(2));

                    while (!queriesLine.empty()) {
                        std::string query;

                        std::tie(query, queriesLine) = httputils::str_split(queriesLine, '&');
                        request.queries.insert(httputils::str_split(query, '='));
                    }
                }
            }
        } else {
            parserState = parsingError(400, "Request-line empty");
        }

        return parserState;
    }

    Parser::ParserState RequestParser::parseHeader() {
        for (auto& [headerFieldName, headerFieldValue] : Parser::headers) {
            if (headerFieldName != "cookie") {
                if (headerFieldName == "content-length") {
                    Parser::contentLength = std::stoul(headerFieldValue);
                }
            } else {
                std::string cookiesLine = headerFieldValue;

                while (!cookiesLine.empty()) {
                    std::string cookieLine;
                    std::tie(cookieLine, cookiesLine) = httputils::str_split(cookiesLine, ',');

                    while (!cookieLine.empty()) {
                        std::string cookie;
                        std::tie(cookie, cookieLine) = httputils::str_split(cookieLine, ';');

                        std::string cookieName;
                        std::string cookieValue;
                        std::tie(cookieName, cookieValue) = httputils::str_split(cookie, '=');

                        httputils::str_trimm(cookieName);
                        httputils::str_trimm(cookieValue);

                        request.cookies.insert({cookieName, cookieValue});
                    }
                }
            }
        }

        Parser::headers.erase("cookie");

        request.headers = std::move(Parser::headers);

        enum Parser::ParserState parserState = Parser::ParserState::BODY;
        if (contentLength == 0 && request.httpMinor == 1) {
            parserState = parsingFinished();
        } else {
            httpMajor = request.httpMajor;
            httpMinor = request.httpMinor;
        }

        return parserState;
    }

    Parser::ParserState RequestParser::parseContent(std::vector<uint8_t>& content) {
        request.body = std::move(content);

        return parsingFinished();
    }

    Parser::ParserState RequestParser::parsingFinished() {
        onParsed(request);

        reset();

        return ParserState::BEGIN;
    }

    Parser::ParserState RequestParser::parsingError(int code, const std::string& reason) {
        onError(code, reason + "\r\n");

        reset();

        return ParserState::ERROR;
    }

} // namespace web::http::server
