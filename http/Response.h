/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef RESPONSE_H
#define RESPONSE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "CookieOptions.h"

class FileReader;

namespace http {

    class ServerContext;

    class Response {
    protected:
        explicit Response(ServerContext* httpContext);
        virtual ~Response() = default;

    public:
        void send(const char* buffer, size_t size);
        void send(const std::string& text);

        void end();

        Response& status(int status);
        Response& append(const std::string& field, const std::string& value);
        Response& set(const std::string& field, const std::string& value, bool overwrite = false);
        Response& set(const std::map<std::string, std::string>& headers, bool overwrite = false);
        Response& cookie(const std::string& name, const std::string& value, const std::map<std::string, std::string>& options = {});
        Response& clearCookie(const std::string& name, const std::map<std::string, std::string>& options = {});
        Response& type(const std::string& type);

        bool headersSent = false;
        bool keepAlive = true;

    protected:
        mutable size_t contentLength = 0;

        void enqueue(const char* buf, size_t len);
        void enqueue(const std::string& str);
        void sendHeader();

        virtual void disable();
        virtual void reset();

    private: /*
         class ResponseCookie {
         public:
             ResponseCookie(const std::string& value, const std::map<std::string, std::string>& options)
                 : value(value)
                 , options(options) {
             }

         protected:
             std::string value;
             std::map<std::string, std::string> options;

             friend class Response;
         };
 */
    protected:
        ServerContext* httpServerContext;

        size_t contentSent = 0;
        bool sendHeaderInProgress = false;

        int responseStatus = 0;
        std::map<std::string, std::string> headers;
        std::map<std::string, CookieOptions> cookies;

        friend class ServerContext;
    };

} // namespace http

#endif // RESPONSE_H
