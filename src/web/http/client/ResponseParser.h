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

#include "web/http/Parser.h"
#include "web/http/client/Response.h" // IWYU pragma: export

namespace core::socket::stream {
    class SocketContext;
} // namespace core::socket::stream

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    class ResponseParser : public web::http::Parser {
    public:
        ResponseParser(core::socket::stream::SocketContext* socketContext,
                       const std::function<void()>& onResponseStart,
                       const std::function<void(Response&&)>& onResponseParsed,
                       const std::function<void(int, const std::string&)>& onError);

        ResponseParser(const ResponseParser&) = delete;
        ResponseParser& operator=(const ResponseParser&) = delete;

    private:
        // Entrence
        void begin() override;

        // Parsers and Validators
        ParserState parseStartLine(const std::string& line) override;
        ParserState parseHeader() override;
        ParserState parseContent(std::vector<uint8_t>& content) override;

        // Exits
        ParserState parsingFinished();
        ParserState parsingError(int code, const std::string& reason) override;

        Response response;

        // Callbacks
        std::function<void()> onResponseStart;
        std::function<void(Response&&)> onResponseParsed;
        std::function<void(int, const std::string&)> onError;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_RESPONSEPARSER_H
