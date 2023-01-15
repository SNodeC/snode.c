/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#ifndef EXPRESS_WEBAPPT_H
#define EXPRESS_WEBAPPT_H

#include "express/WebApp.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express {

    template <typename ServerT>
    class WebAppT
        : public WebApp
        , public ServerT {
    private:
        using Server = ServerT;

    public:
        using SocketConnection = typename Server::SocketConnection;
        using SocketAddress = typename Server::SocketAddress;

        explicit WebAppT()
            : WebAppT("") {
        }

        explicit WebAppT(const std::string& name)
            : WebAppT(name, Router()) {
        }

        WebAppT(const std::string& name, const Router& router)
            : WebApp(router)
            , Server(name,
                     [rootRoute = this->rootRoute](Request& req,
                                                   Response& res) -> void { // onRequestReady
                         rootRoute->dispatch(Controller(req, res));
                     }) {
        }
    };

} // namespace express

#endif // EXPRESS_WEBAPPT_H
