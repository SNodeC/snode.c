/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "web/http/client/ResponseParser.h"

#include "web/http/StatusCodes.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "web/http/http_utils.h"

#include <regex>
#include <tuple>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    ResponseParser::ResponseParser(core::socket::stream::SocketContext* socketContext,
                                   const std::function<void()>& onResponseStart,
                                   const std::function<void(Response&)>& onResponseParsed,
                                   const std::function<void(int, const std::string&)>& onResponseParseError)
        : Parser(socketContext)
        , onResponseStart(onResponseStart)
        , onResponseParsed(onResponseParsed)
        , onResponseParseError(onResponseParseError) {
    }

    void ResponseParser::begin() {
        onResponseStart();
    }

    void ResponseParser::parseStartLine(const std::string& line) {
        parserState = Parser::ParserState::HEADER;

        if (!line.empty()) {
            std::string remaining;

            std::tie(response.httpVersion, remaining) = httputils::str_split(line, ' ');

            std::smatch httpVersionMatch;
            if (!std::regex_match(response.httpVersion, httpVersionMatch, httpVersionRegex)) {
                parseError(400, "Wrong protocol version: " + response.httpVersion);
            } else {
                httpMajor = std::stoi(httpVersionMatch.str(1));
                httpMinor = std::stoi(httpVersionMatch.str(2));

                if (httpMinor == 0) {
                    transferEncoding = TransferEncoding::HTTP10;
                } else if (httpMinor == 1) {
                    transferEncoding = TransferEncoding::Identity;
                }

                std::tie(response.statusCode, response.reason) = httputils::str_split(remaining, ' ');
                if (StatusCode::contains(std::stoi(response.statusCode))) {
                    if (response.reason.empty()) {
                        parseError(400, "No reason phrase");
                    }
                } else {
                    parseError(400, "Unknown status code");
                }
            }
        } else {
            parseError(400, "Response line empty");
        }
    }

    void ResponseParser::analyzeHeader() {
        Parser::analyzeHeader();

        if (headers.contains("Connection")) {
            const std::string& connection = headers["Connection"];
            if (web::http::ciContains(connection, "keep-alive")) {
                response.connectionState = ConnectionState::Keep;
            } else if (web::http::ciContains(connection, "close")) {
                response.connectionState = ConnectionState::Close;
            }
        }
        if (headers.contains("Set-Cookie")) {
            std::string cookiesLine = headers["Set-Cookie"];

            while (!cookiesLine.empty()) {
                std::string cookieLine;
                std::tie(cookieLine, cookiesLine) = httputils::str_split(cookiesLine, ',');

                std::string cookieOptions;
                std::string cookie;
                std::tie(cookie, cookieOptions) = httputils::str_split(cookieLine, ';');

                std::string cookieName;
                std::string cookieValue;
                std::tie(cookieName, cookieValue) = httputils::str_split(cookie, '=');

                httputils::str_trimm(cookieName);
                httputils::str_trimm(cookieValue);

                std::map<std::string, CookieOptions>::iterator cookieElement;
                bool inserted = false;
                std::tie(cookieElement, inserted) = response.cookies.insert({cookieName, CookieOptions(cookieValue)});

                while (!cookieOptions.empty()) {
                    std::string option;
                    std::tie(option, cookieOptions) = httputils::str_split(cookieOptions, ';');

                    std::string optionName;
                    std::string optionValue;
                    std::tie(optionName, optionValue) = httputils::str_split(option, '=');

                    httputils::str_trimm(optionName);
                    httputils::str_trimm(optionValue);

                    cookieElement->second.setOption(optionName, optionValue);
                }
            }

            Parser::headers.erase("Set-Cookie");
        }

        if (transferEncoding == TransferEncoding::Identity && contentLength == 0) {
            parsingFinished();
        } else {
            parserState = Parser::ParserState::BODY;
        }
    }

    void ResponseParser::parsingFinished() {
        response.httpMajor = httpMajor;
        response.httpMinor = httpMinor;
        response.headers = std::move(headers);
        response.body = std::move(content);

        onResponseParsed(response);

        reset();

        parserState = ParserState::BEGIN;
    }

    void ResponseParser::parseError(int code, const std::string& reason) {
        onResponseParseError(code, reason);

        reset();

        parserState = ParserState::ERROR;
    }

} // namespace web::http::client
