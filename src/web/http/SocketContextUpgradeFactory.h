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

#ifndef WEB_HTTP_SOCKETCONTEXTUPGRADEFACTORY_H
#define WEB_HTTP_SOCKETCONTEXTUPGRADEFACTORY_H

#include "core/socket/stream/SocketContextFactory.h"

namespace core::socket::stream {
    class SocketConnection;
    namespace stream {
        class SocketContext;
    }
} // namespace core::socket::stream

namespace web::http {
    template <typename RequestT, typename ResponseT>
    class SocketContextUpgrade;
} // namespace web::http

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http {

    template <typename RequestT, typename ResponseT>
    class SocketContextUpgradeFactory : public core::socket::stream::SocketContextFactory {
    public:
        using Request = RequestT;
        using Response = ResponseT;

    protected:
        SocketContextUpgradeFactory() = default;
        ~SocketContextUpgradeFactory() override = default;

    public:
        virtual std::string name() = 0;

    protected:
        std::size_t refCount = 0;

        void prepare(Request& request, Response& response);

    private:
        virtual SocketContextUpgrade<Request, Response>*
        create(core::socket::stream::SocketConnection* socketConnection, Request* request, Response* response) = 0;
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) final;

        void incRefCount();
        void decRefCount();

        virtual void checkRefCount() = 0;

        Request* request = nullptr;
        Response* response = nullptr;

        friend Response;
        friend class SocketContextUpgrade<Request, Response>;
    };

} // namespace web::http

#endif // WEB_HTTP_SOCKETCONTEXTUPGRADEFACTORY_H
