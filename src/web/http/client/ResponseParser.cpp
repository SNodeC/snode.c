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

#include "web/http/client/ResponseParser.h"

#include "web/http/StatusCodes.h"
#include "web/http/http_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <regex>
#include <tuple>
#include <utility>

// IWYU pragma: no_include <bits/utility.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

// ************** TWO LINER ***************
// openssl s_server -CAfile snode.c_-_Root_CA.crt -cert snode.c_-_server.pem -CAfile snode.c_-_Root_CA.crt -key
// snode.c_-_server.key.encrypted.pem -www -port 8088 -msg -Verify 1

namespace web::http::client {

    ResponseParser::ResponseParser(
        core::socket::stream::SocketContext* socketContext,
        const std::function<void(void)>& onStart,
        const std::function<void(std::string&, std::string&, std::string&)>& onResponse,
        const std::function<void(std::map<std::string, std::string>&, std::map<std::string, CookieOptions>&)>& onHeader,
        const std::function<void(std::vector<uint8_t>&)>& onContent,
        const std::function<void(ResponseParser&)>& onParsed,
        const std::function<void(int, const std::string&)>& onError)
        : Parser(socketContext)
        , onStart(onStart)
        , onResponse(onResponse)
        , onHeader(onHeader)
        , onContent(onContent)
        , onParsed(onParsed)
        , onError(onError) {
    }

    void ResponseParser::reset() {
        Parser::reset();
        httpVersion.clear();
        statusCode.clear();
        reason.clear();
        cookies.clear();
    }

    void ResponseParser::begin() {
        onStart();
    }

    enum Parser::ParserState ResponseParser::parseStartLine(const std::string& line) {
        enum Parser::ParserState parserState = Parser::ParserState::HEADER;

        if (!line.empty()) {
            std::string remaining;

            std::tie(httpVersion, remaining) = httputils::str_split(line, ' ');

            std::smatch httpVersionMatch;
            if (!std::regex_match(httpVersion, httpVersionMatch, httpVersionRegex)) {
                parserState = parsingError(400, "Wrong protocol version");
            } else {
                httpMajor = std::stoi(httpVersionMatch.str(1));
                httpMinor = std::stoi(httpVersionMatch.str(2));

                std::tie(statusCode, reason) = httputils::str_split(remaining, ' ');
                if (StatusCode::contains(std::stoi(statusCode))) {
                    if (!reason.empty()) {
                        onResponse(httpVersion, statusCode, reason);
                    } else {
                        parserState = parsingError(400, "No reason phrase");
                    }
                } else {
                    parserState = parsingError(400, "Unknown status code");
                }
            }
        } else {
            parserState = parsingError(400, "Response line empty");
        }
        return parserState;
    }

    enum Parser::ParserState ResponseParser::parseHeader() {
        for (const auto& [headerFieldName, headerFieldValue] : Parser::headers) {
            if (headerFieldName != "set-cookie") {
                if (headerFieldName == "content-length") {
                    Parser::contentLength = static_cast<std::size_t>(std::stoi(headerFieldValue));
                }
            } else {
                std::string cookiesLine = headerFieldValue;

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
                    std::tie(cookieElement, inserted) = cookies.insert({cookieName, CookieOptions(cookieValue)});

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
            }
        }

        Parser::headers.erase("set-cookie");

        onHeader(Parser::headers, cookies);

        enum Parser::ParserState parserState = Parser::ParserState::BODY;
        if (contentLength == 0 && httpMinor == 1) {
            parsingFinished();
            parserState = ParserState::BEGIN;
        }

        return parserState;
    }

    Parser::ParserState ResponseParser::parseContent(std::vector<uint8_t>& content) {
        onContent(content);
        parsingFinished();

        return ParserState::BEGIN;
    }

    enum Parser::ParserState ResponseParser::parsingError(int code, const std::string& reason) {
        onError(code, reason);

        return ParserState::ERROR;
    }

    void ResponseParser::parsingFinished() {
        onParsed(*this);
    }

} // namespace web::http::client
