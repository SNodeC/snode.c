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

#include "web/http/Parser.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // for std::size_t
#include <functional>
#include <map>
#include <set>
#include <string> // for string, basic_string, operator<

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    class RequestParser : public Parser {
    public:
        RequestParser(
            net::socket::stream::SocketConnectionBase* socketConnection,
            const std::function<void(void)>& onStart,
            const std::function<
                void(const std::string&, const std::string&, const std::string&, int, int, const std::map<std::string, std::string>&)>&
                onRequest,
            const std::function<void(const std::map<std::string, std::string>&, const std::map<std::string, std::string>&)>& onHeader,
            const std::function<void(char*, std::size_t)>& onContent,
            const std::function<void()>& onParsed,
            const std::function<void(int status, const std::string& reason)>& onError);

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
        enum Parser::ParserState parseContent(char* content, std::size_t size) override;

        // Exits
        void parsingFinished();
        enum Parser::ParserState parsingError(int code, const std::string& reason) override;

        // Supported web-methods
        std::set<std::string> supportedMethods{"GET", "PUT", "POST", "DELETE", "CONNECT", "OPTIONS", "TRACE", "PATCH", "HEAD"};

        // Data specific to HTTP request messages
        std::string method;
        std::string url;
        std::string httpVersion;
        std::map<std::string, std::string> cookies;
        std::map<std::string, std::string> queries;
        int httpMajor = 0;
        int httpMinor = 0;

        // Callbacks
        std::function<void(void)> onStart;
        std::function<void(const std::string&, const std::string&, const std::string&, int, int, const std::map<std::string, std::string>&)>
            onRequest;
        std::function<void(const std::map<std::string, std::string>&, const std::map<std::string, std::string>&)> onHeader;
        std::function<void(char*, std::size_t)> onContent;
        std::function<void()> onParsed;
        std::function<void(int status, const std::string& reason)> onError;
    };

} // namespace web::http::server

#endif // WEB_HTTP_SERVER_REQUESTPARSER_H
