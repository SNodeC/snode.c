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

#include "web/http/client/ResponseParser.h"

#include "web/http/StatusCodes.h"
#include "web/http/http_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <regex>
#include <tuple>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

// ************** TWO LINER ***************
// openssl s_server -CAfile snode.c_-_Root_CA.crt -cert snode.c_-_server.pem -CAfile snode.c_-_Root_CA.crt -key
// snode.c_-_server.key.encrypted.pem -www -port 8088 -msg -Verify 1

namespace web::http::client {

    ResponseParser::ResponseParser(core::socket::stream::SocketContext* socketContext,
                                   const std::function<void()>& onResponseStart,
                                   const std::function<void(Response&&)>& onResponseParsed,
                                   const std::function<void(int, const std::string&)>& onError)
        : Parser(socketContext)
        , onResponseStart(onResponseStart)
        , onResponseParsed(onResponseParsed)
        , onError(onError) {
    }

    void ResponseParser::begin() {
        reset();

        onResponseStart();
    }

    Parser::ParserState ResponseParser::parseStartLine(const std::string& line) {
        Parser::ParserState parserState = Parser::ParserState::HEADER;

        if (!line.empty()) {
            std::string remaining;

            std::tie(response.httpVersion, remaining) = httputils::str_split(line, ' ');

            std::smatch httpVersionMatch;
            if (!std::regex_match(response.httpVersion, httpVersionMatch, httpVersionRegex)) {
                parserState = parsingError(400, "Wrong protocol version");
            } else {
                response.httpMajor = std::stoi(httpVersionMatch.str(1));
                response.httpMinor = std::stoi(httpVersionMatch.str(2));

                std::tie(response.statusCode, response.reason) = httputils::str_split(remaining, ' ');
                if (StatusCode::contains(std::stoi(response.statusCode))) {
                    if (response.reason.empty()) {
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

    Parser::ParserState ResponseParser::parseHeader() {
        for (const auto& [headerFieldName, headerFieldValue] : Parser::headers) {
            if (headerFieldName != "set-cookie") {
                if (headerFieldName == "content-length") {
                    Parser::contentLength = static_cast<std::size_t>(std::stoi(headerFieldValue));
                } else if (headerFieldName == "connection" && httputils::ci_contains(headerFieldValue, "close")) {
                    response.connectionState = ConnectionState::Close;
                } else if (headerFieldName == "connection" && httputils::ci_contains(headerFieldValue, "keep-alive")) {
                    response.connectionState = ConnectionState::Keep;
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
            }
        }

        headers.erase("set-cookie");

        response.headers = std::move(Parser::headers);

        ParserState parserState = Parser::ParserState::BODY;
        if (contentLength == 0 && response.httpMinor == 1) {
            parserState = parsingFinished();
        } else {
            httpMajor = response.httpMajor;
            httpMinor = response.httpMinor;
        }

        return parserState;
    }

    Parser::ParserState ResponseParser::parseContent(std::vector<uint8_t>& content) {
        response.body = std::move(content);

        return parsingFinished();
    }

    Parser::ParserState ResponseParser::parsingFinished() {
        onResponseParsed(std::move(response));

        return ParserState::BEGIN;
    }

    Parser::ParserState ResponseParser::parsingError(int code, const std::string& reason) {
        onError(code, reason);

        return ParserState::ERROR;
    }

} // namespace web::http::client
