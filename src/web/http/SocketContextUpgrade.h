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

#ifndef WEB_HTTP_SOCKETCONTEXTUPGRADE_H
#define WEB_HTTP_SOCKETCONTEXTUPGRADE_H

#include "core/socket/stream/SocketContext.h" // IWYU pragma: export

namespace core::socket::stream {
    class SocketConnection;
}

namespace web::http {
    template <typename RequestT, typename ResponseT>
    class SocketContextUpgradeFactory;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http {

    template <typename RequestT, typename ResponseT>
    class SocketContextUpgrade : public core::socket::stream::SocketContext {
    public:
        using Request = RequestT;
        using Response = ResponseT;

        SocketContextUpgrade(core::socket::stream::SocketConnection* socketConnection,
                             SocketContextUpgradeFactory<Request, Response>* socketContextUpgradeFactory)
            : core::socket::stream::SocketContext(socketConnection)
            , socketContextUpgradeFactory(socketContextUpgradeFactory) {
            socketContextUpgradeFactory->incRefCount();
        }

        virtual ~SocketContextUpgrade() {
            socketContextUpgradeFactory->decRefCount();
        }

        using core::socket::stream::SocketContext::setTimeout;

    protected:
        SocketContextUpgradeFactory<Request, Response>* socketContextUpgradeFactory = nullptr;
    };

} // namespace web::http

#endif // WEB_HTTP_SOCKETCONTEXTUPGRADE_H
