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

#ifndef WEB_HTTP_CLIENT_REQUEST_H
#define WEB_HTTP_CLIENT_REQUEST_H

#include "web/http/ConnectionState.h"

namespace web::http {
    class SocketContext;
} // namespace web::http

namespace web::http::client {
    class Response;
} // namespace web::http::client

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // IWYU pragma: export
#include <map>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    class Request {
    public:
        explicit Request(web::http::SocketContext* clientContext);

        explicit Request(Request&) = delete;
        explicit Request(Request&&) noexcept = default;

        Request& operator=(Request&) = delete;
        Request& operator=(Request&&) noexcept = default;

        virtual ~Request() = default;

        Request& setHost(const std::string& host);
        Request& append(const std::string& field, const std::string& value);
        Request& set(const std::string& field, const std::string& value, bool overwrite = false);
        Request& set(const std::map<std::string, std::string>& headers, bool overwrite = false);
        Request& cookie(const std::string& name, const std::string& value);
        Request& cookie(const std::map<std::string, std::string>& cookies);
        Request& type(const std::string& type);

        void sendFragment(const char* junk, std::size_t junkLen);
        void sendFragment(const std::string& data);

        void send(const char* junk, std::size_t junkLen);
        void send(const std::string& junk);

        void upgrade(const std::string& url, const std::string& protocols);
        void upgrade(std::shared_ptr<Response>& response);

        void sendHeader();

        void start();

        const std::string& header(const std::string& field);

    protected:
        virtual void reset();

        web::http::SocketContext* socketContext;

        ConnectionState connectionState = ConnectionState::Default;

    public:
        std::string method = "GET";
        std::string url;
        std::string host;
        int httpMajor = 1;
        int httpMinor = 1;

    protected:
        std::map<std::string, std::string> queries;
        std::map<std::string, std::string> headers;
        std::map<std::string, std::string> cookies;

    private:
        bool sendHeaderInProgress = false;
        bool headersSent = false;
        std::size_t contentSent = 0;
        std::size_t contentLength = 0;

        std::string nullstr;

        template <typename Request, typename Response>
        friend class SocketContext;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_REQUEST_H
