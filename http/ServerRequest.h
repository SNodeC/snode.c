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

#ifndef SERVERREQUEST_H
#define SERVERREQUEST_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // for size_t
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ConnectionState.h"
#include "stream/Sink.h"

namespace http {

    class ClientContext;

    class ServerRequest : public net::stream::Sink {
    protected:
        explicit ServerRequest(ClientContext* clientContext);

    public:
        std::string method = "GET";
        std::string url;
        std::string host;
        int httpMajor = 1;
        int httpMinor = 1;

        ConnectionState connectionState = ConnectionState::Default;

        ServerRequest& setHost(const std::string& host);
        ServerRequest& append(const std::string& field, const std::string& value);
        ServerRequest& set(const std::string& field, const std::string& value, bool overwrite = false);
        ServerRequest& set(const std::map<std::string, std::string>& headers, bool overwrite = false);
        ServerRequest& cookie(const std::string& name, const std::string& value);
        ServerRequest& cookie(const std::map<std::string, std::string>& cookies);
        ServerRequest& type(const std::string& type);

        void send(const char* buffer, std::size_t size);
        void send(const std::string& text);

        void sendHeader();

        void end();

    protected:
        void enqueue(const char* buf, std::size_t len);
        void enqueue(const std::string& data);

        bool sendHeaderInProgress = false;
        bool headersSent = false;
        std::size_t contentSent = 0;
        std::size_t contentLength = 0;

        ClientContext* clientContext;

        virtual void reset();

        std::map<std::string, std::string> queries;
        std::map<std::string, std::string> headers;
        std::map<std::string, std::string> cookies;

        std::string nullstr = "";

        void receive(const char* junk, std::size_t junkLen) override;
        void eof() override;
        void error(int errnum) override;

        friend class ClientContext;
    };

} // namespace http

#endif // SERVERREQUEST_H
