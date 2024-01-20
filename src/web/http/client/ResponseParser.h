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

#ifndef WEB_HTTP_CLIENT_RESPONSEPARSER_H
#define WEB_HTTP_CLIENT_RESPONSEPARSER_H

#include "web/http/CookieOptions.h"
#include "web/http/Parser.h"

namespace core::socket::stream {
    class SocketContext;
} // namespace core::socket::stream

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    class ResponseParser : public web::http::Parser {
    public:
        ResponseParser(
            core::socket::stream::SocketContext* socketContext,
            const std::function<void(void)>& onStart,
            const std::function<void(std::string&, std::string&, std::string&)>& onResponse,
            const std::function<void(std::map<std::string, std::string>&, std::map<std::string, web::http::CookieOptions>&)>& onHeader,
            const std::function<void(std::vector<uint8_t>&)>& onContent,
            const std::function<void(ResponseParser&)>& onParsed,
            const std::function<void(int, const std::string&)>& onError);

        void reset() override;

    private:
        // Entrence
        void begin() override;

        enum Parser::ParserState parseStartLine(const std::string& line) override;
        enum Parser::ParserState parseHeader() override;
        enum Parser::ParserState parseContent(std::vector<uint8_t>& content) override;
        enum Parser::ParserState parsingError(int code, const std::string& reason) override;

        void parsingFinished();

        std::string statusCode;
        std::string reason;
        std::map<std::string, CookieOptions> cookies;

        std::function<void(void)> onStart;
        std::function<void(std::string&, std::string&, std::string&)> onResponse;
        std::function<void(std::map<std::string, std::string>&, std::map<std::string, web::http::CookieOptions>&)> onHeader;
        std::function<void(std::vector<uint8_t>&)> onContent;
        std::function<void(ResponseParser&)> onParsed;
        std::function<void(int, const std::string&)> onError;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_RESPONSEPARSER_H
