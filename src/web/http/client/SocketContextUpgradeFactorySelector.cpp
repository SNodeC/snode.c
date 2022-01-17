/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "web/http/client/SocketContextUpgradeFactorySelector.h"

#include "web/http/SocketContextUpgradeFactorySelector.hpp"
#include "web/http/client/Response.h" // IWYU pragma: keep
#include "web/http/http_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    SocketContextUpgradeFactorySelector::SocketContextUpgradeFactorySelector()
        : web::http::SocketContextUpgradeFactorySelector<SocketContextUpgradeFactory>() {
        addSocketContextUpgradeSearchPath(HTTP_SOCKETCONTEXTUPGRADE_CLIENT_INSTALL_LIBDIR);

#if !defined(NDEBUG) && defined(HTTP_SOCKETCONTEXTUPGRADE_CLIENT_COMPILE_LIBDIR)

        addSocketContextUpgradeSearchPath(HTTP_SOCKETCONTEXTUPGRADE_CLIENT_COMPILE_LIBDIR);

#endif // !defined(NDEBUG) && defined(HTTP_SOCKETCONTEXTUPGRADE_CLIENT_COMPILE_LIBDIR)
    }

    SocketContextUpgradeFactorySelector* SocketContextUpgradeFactorySelector::instance() {
        static SocketContextUpgradeFactorySelector socketContextUpgradeFactorySelector;

        return &socketContextUpgradeFactorySelector;
    }

    SocketContextUpgradeFactorySelector::SocketContextUpgradeFactory*
    SocketContextUpgradeFactorySelector::load(const std::string& upgradeContextName) {
        return load(upgradeContextName, core::socket::SocketContext::Role::CLIENT);
    }

    SocketContextUpgradeFactory* SocketContextUpgradeFactorySelector::select(const std::string& upgradeContextName, Request& req) {
        SocketContextUpgradeFactory* socketContextUpgradeFactory = nullptr;

        socketContextUpgradeFactory = select(upgradeContextName);

        if (socketContextUpgradeFactory != nullptr) {
            socketContextUpgradeFactory->prepare(req); // Fill in the missing header fields into the request object
        }

        return socketContextUpgradeFactory;
    }

    SocketContextUpgradeFactory* SocketContextUpgradeFactorySelector::select(Request& req, Response& res) {
        SocketContextUpgradeFactory* socketContextUpgradeFactory = nullptr;

        std::string upgradeContextName = res.header("upgrade");

        if (!upgradeContextName.empty()) {
            httputils::to_lower(upgradeContextName);

            socketContextUpgradeFactory = select(upgradeContextName);

            if (socketContextUpgradeFactory != nullptr) {
                socketContextUpgradeFactory->web::http::SocketContextUpgradeFactory<Request, Response>::prepare(
                    req, res); // Fill in the missing header fields into the request object
            }
        }

        return socketContextUpgradeFactory;
    }

} // namespace web::http::client

namespace web::http {
    template class web::http::SocketContextUpgradeFactorySelector<web::http::client::SocketContextUpgradeFactory>;
}
