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

#ifndef WEB_HTTP_SERVER_REQUESTPARSER_H
#define WEB_HTTP_SERVER_REQUESTPARSER_H

#include "web/http/Parser.h"         // IWYU pragma: export
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
                      const std::function<void(int, std::string&&)>& onError);

        RequestParser(const RequestParser&) = delete;
        RequestParser& operator=(const RequestParser&) = delete;

        void reset() override;

    private:
        // Check if request method is supported
        virtual bool methodSupported(const std::string& method) const;

        // Entrence
        void begin() override;

        // Parsers and Validators
        enum Parser::ParserState parseStartLine(const std::string& line) override;
        enum Parser::ParserState parseHeader() override;
        enum Parser::ParserState parseContent(std::vector<uint8_t>& content) override;

        // Exits
        void parsingFinished();
        enum Parser::ParserState parsingError(int code, std::string&& reason) override;

        // Supported web-methods
        std::set<std::string> supportedMethods{"GET", "PUT", "POST", "DELETE", "CONNECT", "OPTIONS", "TRACE", "PATCH", "HEAD"};

        Request request;

        // Callbacks
        std::function<void(Request&&)> onParsed;
        std::function<void(int, std::string&&)> onError;
    };

} // namespace web::http::server

#endif // WEB_HTTP_SERVER_REQUESTPARSER_H
