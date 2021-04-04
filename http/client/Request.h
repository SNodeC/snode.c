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

#ifndef HTTP_CLIENT_REQUEST_H
#define HTTP_CLIENT_REQUEST_H

#include "http/ConnectionState.h"
#include "net/stream/Sink.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // for size_t
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace http::client {

    class ClientContextBase;

    class Request : public net::stream::Sink {
    protected:
        explicit Request(ClientContextBase* clientContext);

    public:
        std::string method = "GET";
        std::string url;
        std::string host;
        int httpMajor = 1;
        int httpMinor = 1;

        ConnectionState connectionState = ConnectionState::Default;

        Request& setHost(const std::string& host);
        Request& append(const std::string& field, const std::string& value);
        Request& set(const std::string& field, const std::string& value, bool overwrite = false);
        Request& set(const std::map<std::string, std::string>& headers, bool overwrite = false);
        Request& cookie(const std::string& name, const std::string& value);
        Request& cookie(const std::map<std::string, std::string>& cookies);
        Request& type(const std::string& type);

        void send(const char* junk, std::size_t junkLen);
        void send(const std::string& junk);

        void sendHeader();

        void end();

    protected:
        void enqueue(const char* junk, std::size_t junkLen);
        void enqueue(const std::string& data);

        bool sendHeaderInProgress = false;
        bool headersSent = false;
        std::size_t contentSent = 0;
        std::size_t contentLength = 0;

        ClientContextBase* clientContext;

        virtual void reset();

        std::map<std::string, std::string> queries;
        std::map<std::string, std::string> headers;
        std::map<std::string, std::string> cookies;

        std::string nullstr = "";

        void receive(const char* junk, std::size_t junkLen) override;
        void eof() override;
        void error(int errnum) override;

        template <typename Request, typename Response>
        friend class ClientContext;
    };

} // namespace http::client

#endif // HTTP_CLIENT_REQUEST_H
