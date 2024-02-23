/*
 * snode.c - a slim toolkit for network communication
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

#ifndef WEB_HTTP_SERVER_REQUESTPARSER_H
#define WEB_HTTP_SERVER_REQUESTPARSER_H

#include "web/http/Parser.h"
#include "web/http/server/Request.h" // IWYU pragma: export

namespace core::socket::stream {
    class SocketContext;
} // namespace core::socket::stream

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <set>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    class RequestParser : public web::http::Parser {
    public:
        RequestParser(core::socket::stream::SocketContext* socketContext,
                      const std::function<void(Request&&)>& onParsed,
                      const std::function<void(int, const std::string&)>& onError);

        RequestParser(const RequestParser&) = delete;
        RequestParser& operator=(const RequestParser&) = delete;

    private:
        // Supported web-methods
        std::set<std::string> supportedMethods{"GET", "PUT", "POST", "DELETE", "CONNECT", "OPTIONS", "TRACE", "PATCH", "HEAD"};

        // Check if request method is supported
        virtual bool methodSupported(const std::string& method) const;

        // Entrence
        void begin() override;

        // Parsers and Validators
        ParserState parseStartLine(const std::string& line) override;
        ParserState parseHeader() override;
        ParserState parseContent(std::vector<uint8_t>& content) override;

        // Exits
        ParserState parsingFinished();
        ParserState parsingError(int code, const std::string& reason) override;

        Request request;

        // Callbacks
        std::function<void(Request&&)> onParsed;
        std::function<void(int, const std::string&)> onError;
    };

} // namespace web::http::server

#endif // WEB_HTTP_SERVER_REQUESTPARSER_H
