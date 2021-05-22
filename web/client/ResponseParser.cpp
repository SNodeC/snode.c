/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#include "web/client/ResponseParser.h"

#include "web/http/StatusCodes.h"
#include "web/http_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <regex>   // for regex_match, match_results<>::_Base_type
#include <tuple>   // for tie, tuple
#include <utility> // for tuple_element<>::type, pair
#include <vector>  // for vector

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::client {

    ResponseParser::ResponseParser(
        const std::function<void(void)>& onStart,
        const std::function<void(const std::string&, const std::string&, const std::string&)>& onResponse,
        const std::function<void(const std::map<std::string, std::string>&, const std::map<std::string, CookieOptions>&)>& onHeader,
        const std::function<void(char*, std::size_t)>& onContent,
        const std::function<void(ResponseParser&)>& onParsed,
        const std::function<void(int status, const std::string& reason)>& onError)
        : onStart(onStart)
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
        for (auto& [field, value] : Parser::headers) {
            if (field != "set-cookie") {
                if (field == "content-length") {
                    Parser::contentLength = std::stoi(value);
                }
            } else {
                std::string cookiesLine = value;

                while (!cookiesLine.empty()) {
                    std::string cookieLine;
                    std::tie(cookieLine, cookiesLine) = httputils::str_split(cookiesLine, ',');

                    std::string cookieOptions;
                    std::string cookie;
                    std::tie(cookie, cookieOptions) = httputils::str_split(cookieLine, ';');

                    std::string name;
                    std::string value;
                    std::tie(name, value) = httputils::str_split(cookie, '=');
                    httputils::str_trimm(name);
                    httputils::str_trimm(value);

                    std::map<std::string, CookieOptions>::iterator cookieElement;
                    bool inserted;
                    std::tie(cookieElement, inserted) = cookies.insert({name, CookieOptions(value)});

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
        if (contentLength == 0) {
            parsingFinished();
            parserState = ParserState::BEGIN;
        }

        return parserState;
    }

    enum Parser::ParserState ResponseParser::parseContent(char* content, std::size_t size) {
        onContent(content, size);
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

} // namespace web::client
