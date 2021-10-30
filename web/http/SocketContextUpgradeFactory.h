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

#ifndef WEB_HTTP_SOCKETCONTEXTUPGRADEFACTORY_H
#define WEB_HTTP_SOCKETCONTEXTUPGRADEFACTORY_H

#include "net/socket/stream/SocketContextFactory.h" // IWYU pragma: export

namespace net::socket::stream {
    class SocketContext;
}

namespace web::http {
    template <typename RequestT, typename ResponseT>
    class SocketContextUpgrade;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http {

    template <typename RequestT, typename ResponseT>
    class SocketContextUpgradeFactory : public net::socket::stream::SocketContextFactory {
    public:
        using Request = RequestT;
        using Response = ResponseT;

        enum class Role { CLIENT, SERVER };

    protected:
        SocketContextUpgradeFactory(Role role);
        ~SocketContextUpgradeFactory() override = default;

    public:
        virtual std::string name() = 0;

        void prepare(Request& request, Response& response);

        SocketContextUpgradeFactory<Request, Response>::Role getRole() const;

        void destroy();

        void incRefCount();

        void decRefCount();

        virtual void checkRefCount() = 0;

    protected:
        std::size_t refCount = 0;

    private:
        Request* request;
        Response* response;

        virtual SocketContextUpgrade<Request, Response>*
        create(net::socket::stream::SocketConnection* socketConnection, Request* request, Response* response) = 0;

        net::socket::stream::SocketContext* create(net::socket::stream::SocketConnection* socketConnection) final;

        Role role;
    };

} // namespace web::http

#endif // WEB_HTTP_SOCKETCONTEXTUPGRADEFACTORY_H
