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

#include "web/http/SocketContextUpgradeFactory.h"
#include "web/http/SocketContextUpgradeFactorySelector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http {

    template <typename Request, typename Response>
    SocketContextUpgradeFactory<Request, Response>::SocketContextUpgradeFactory(Role role)
        : role(role) {
    }

    template <typename Request, typename Response>
    typename SocketContextUpgradeFactory<Request, Response>::Role SocketContextUpgradeFactory<Request, Response>::getRole() {
        return role;
    }

    template <typename Request, typename Response>
    void SocketContextUpgradeFactory<Request, Response>::prepare(Request& request, Response& response) {
        this->request = &request;
        this->response = &response;
    }

    template <typename Request, typename Response>
    void SocketContextUpgradeFactory<Request, Response>::SocketContextUpgradeFactory::destroy() {
        delete this;
    }

    template <typename Request, typename Response>
    void SocketContextUpgradeFactory<Request, Response>::SocketContextUpgradeFactory::incRefCount() {
        refCount++;
    }

} // namespace web::http
