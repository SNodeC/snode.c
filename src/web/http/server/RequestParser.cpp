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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "web/http/http_utils.h"

#include <regex>
#include <tuple>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    RequestParser::RequestParser(core::socket::stream::SocketContext* socketContext,
                                 const std::function<void()>& onRequestStart,
                                 const std::function<void(Request&&)>& onRequestParsed,
                                 const std::function<void(int, const std::string&)>& onRequestParseError)
        : Parser(socketContext)
        , onRequestStart(onRequestStart)
        , onRequestParsed(onRequestParsed)
        , onRequestParseError(onRequestParseError) {
    }

    bool RequestParser::methodSupported(const std::string& method) const {
        return supportedMethods.contains(method);
    }

    void RequestParser::begin() {
        onRequestStart();
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
                parserState = parseError(400, "Bad request method");
            } else if (request.url.empty() || request.url.front() != '/') {
                parserState = parseError(400, "Malformed request");
            } else {
                std::smatch httpVersionMatch;
                if (!std::regex_match(request.httpVersion, httpVersionMatch, httpVersionRegex)) {
                    parserState = parseError(400, "Wrong protocol-version");
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
            parserState = parseError(400, "Request-line empty");
        }

        return parserState;
    }

    Parser::ParserState RequestParser::parseHeader() {
        if (headers.contains("Connection")) {
            const std::string& connection = headers["Connection"];
            if (web::http::ciContains(connection, "keep-alive")) {
                request.connectionState = ConnectionState::Keep;
            } else if (web::http::ciContains(connection, "close")) {
                request.connectionState = ConnectionState::Close;
            }
        }

        if (headers.contains("Cookie")) {
            std::string& cookiesLine = headers["Cookie"];

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

            headers.erase("Cookie");
        }

        request.headers = std::move(Parser::headers);

        httpMajor = request.httpMajor;
        httpMinor = request.httpMinor;

        ParserState parserState = Parser::ParserState::BODY;
        if (transferEncoding != TransferEncoding::Chunked && contentLength == 0) {
            parserState = parsingFinished();
        }

        return parserState;
    }

    Parser::ParserState RequestParser::parseContent(std::vector<char>& content) {
        request.body = std::move(content);

        return parsingFinished();
    }

    Parser::ParserState RequestParser::parsingFinished() {
        onRequestParsed(std::move(request));

        reset();

        return ParserState::BEGIN;
    }

    Parser::ParserState RequestParser::parseError(int code, const std::string& reason) {
        onRequestParseError(code, reason + "\r\n");

        reset();

        return ParserState::ERROR;
    }

} // namespace web::http::server
