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

#ifndef WEB_HTTP_SERVER_REQUESTCONTEXTBASE_H
#define WEB_HTTP_SERVER_REQUESTCONTEXTBASE_H

namespace web::http {

    class SocketContext;

} // namespace web::http

namespace core::socket {
    class SocketContext;
    namespace stream {
        class SocketContextFactory;
    }
} // namespace core::socket

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    class RequestContextBase {
    public:
        RequestContextBase(web::http::SocketContext* socketContext);

        virtual ~RequestContextBase();

        void socketContextGone();

        core::socket::SocketContext* switchSocketContext(core::socket::stream::SocketContextFactory* socketContextUpgradeFactory);

        void sendToPeer(const char* junk, std::size_t junkLen);
        void sendToPeerCompleted();
        void close();

    private:
        web::http::SocketContext* socketContext = nullptr;
    };

} // namespace web::http::server

#endif // WEB_HTTP_SERVER_REQUESTCONTEXTBASE_H
