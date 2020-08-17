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

#ifndef TLSHTTPSERVER_H
#define TLSHTTPSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <netinet/in.h>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

class Request;
class Response;

namespace tls {

    class HTTPServer {
    public:
        explicit HTTPServer(const std::string& cert, const std::string& key, const std::string& password,
                            const std::function<void(Request& req, Response& res)>& onRequest);

        HTTPServer& operator=(const HTTPServer& webApp) = delete;

        void listen(in_port_t port, const std::function<void(int err)>& onError = nullptr);

    protected:
        std::function<void(Request& req, Response& res)> onRequest;

    private:
        std::string cert;
        std::string key;
        std::string password;
    };

} // namespace tls

#endif // TLSHTTPSERVER_H
