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

#include "web/http/SocketContextUpgrade.h"
#include "web/http/SocketContextUpgradeFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http {

    template <typename Request, typename Response>
    SocketContextUpgradeFactory<Request, Response>::SocketContextUpgradeFactory() {
    }

    template <typename Request, typename Response>
    void SocketContextUpgradeFactory<Request, Response>::prepare(Request& request, Response& response) {
        this->request = &request;
        this->response = &response;
    }

    template <typename Request, typename Response>
    core::socket::SocketContext*
    SocketContextUpgradeFactory<Request, Response>::create(core::socket::stream::SocketConnection* socketConnection) {
        SocketContextUpgrade<Request, Response>* socketContext = create(socketConnection, request, response);

        checkRefCount();

        return socketContext;
    }

    template <typename Request, typename Response>
    void SocketContextUpgradeFactory<Request, Response>::destroy() {
        delete this;
    }

    template <typename Request, typename Response>
    void SocketContextUpgradeFactory<Request, Response>::incRefCount() {
        ++refCount;
    }

    template <typename Request, typename Response>
    void SocketContextUpgradeFactory<Request, Response>::decRefCount() {
        --refCount;

        checkRefCount();
    }

} // namespace web::http
