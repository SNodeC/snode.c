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

#ifndef HTTP_CLIENT_RESPONSEPARSER_H
#define HTTP_CLIENT_RESPONSEPARSER_H

#include "http/CookieOptions.h"
#include "http/Parser.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <functional>
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace http::client {

    class ResponseParser : public Parser {
    public:
        ResponseParser(
            const std::function<void(void)>& onStart,
            const std::function<void(const std::string&, const std::string&, const std::string&)>& onResponse,
            const std::function<void(const std::map<std::string, std::string>&, const std::map<std::string, CookieOptions>&)>& onHeader,
            const std::function<void(char*, std::size_t)>& onContent,
            const std::function<void(ResponseParser&)>& onParsed,
            const std::function<void(int status, const std::string& reason)>& onError);

        // Entrence
        void begin() override;

        enum Parser::ParserState parseStartLine(const std::string& line) override;
        enum Parser::ParserState parseHeader() override;
        enum Parser::ParserState parseContent(char* content, std::size_t size) override;
        enum Parser::ParserState parsingError(int code, const std::string& reason) override;

        void reset() override;

    protected:
        void parsingFinished();

        std::string httpVersion;
        std::string statusCode;
        std::string reason;
        std::map<std::string, CookieOptions> cookies;

        std::function<void(void)> onStart;
        std::function<void(const std::string&, const std::string&, const std::string&)> onResponse;
        std::function<void(const std::map<std::string, std::string>&, const std::map<std::string, CookieOptions>&)> onHeader;
        std::function<void(char*, std::size_t)> onContent;
        std::function<void(ResponseParser&)> onParsed;
        std::function<void(int status, const std::string& reason)> onError;
    };

} // namespace http::client

#endif // HTTP_CLIENT_RESPONSEPARSER_H
