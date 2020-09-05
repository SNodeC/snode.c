/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TLS_HTTPSERVER_H
#define TLS_HTTPSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <netinet/in.h>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/tls/SocketConnection.h"

namespace net::socket::tls {
    class SocketServer;
}

namespace http {

    class Request;
    class Response;

    namespace tls {

        class HTTPServer {
        public:
            explicit HTTPServer(const std::function<void(net::socket::tls::SocketConnection*)>& onConnect,
                                const std::function<void(Request& req, Response& res)>& onRequestReady,
                                const std::function<void(net::socket::tls::SocketConnection*)>& onDisconnect, const std::string& cert,
                                const std::string& key, const std::string& password, const std::string& caFile = "",
                                const std::string& caDir = "", bool useDefaultCADir = false);

            HTTPServer& operator=(const HTTPServer& webApp) = delete;

        protected:
            net::socket::tls::SocketServer* socketServer() const;

        public:
            void listen(in_port_t port, const std::function<void(int err)>& onError = nullptr) const;
            void listen(const std::string& host, in_port_t port, const std::function<void(int err)>& onError = nullptr) const;

        protected:
            std::function<void(net::socket::tls::SocketConnection*)> onConnect;
            std::function<void(Request& req, Response& res)> onRequestReady;
            std::function<void(net::socket::tls::SocketConnection*)> onDisconnect;

        private:
            std::string cert;
            std::string key;
            std::string password;
            std::string caFile;
            std::string caDir;
            bool useDefaultCADir;
        };

    } // namespace tls

} // namespace http

#endif // TLS_HTTPSERVER_H
