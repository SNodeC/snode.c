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

#include "web/http/client/SocketContextUpgradeFactorySelector.h"

#include "config.h"
#include "web/http/SocketContextUpgradeFactorySelector.hpp"
#include "web/http/client/Response.h" // IWYU pragma: keep
#include "web/http/http_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list> // for list

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    SocketContextUpgradeFactorySelector::SocketContextUpgradeFactorySelector()
        : web::http::SocketContextUpgradeFactorySelector<SocketContextUpgradeFactory>(
              web::http::SocketContextUpgradeFactory<Request, Response>::Role::CLIENT) {
#ifndef NDEBUG
#ifdef UPGRADECONTEXT_CLIENT_COMPILE_PATH

        searchPaths.push_back(UPGRADECONTEXT_CLIENT_COMPILE_PATH);

#endif // UPGRADECONTEXT_CLIENT_COMPILE_PATH
#endif // NDEBUG

        searchPaths.push_back(UPGRADECONTEXT_CLIENT_INSTALL_PATH);
    }

    SocketContextUpgradeFactorySelector::SocketContextUpgradeFactory*
    SocketContextUpgradeFactorySelector::load(const std::string& upgradeContextName) {
        return web::http::SocketContextUpgradeFactorySelector<SocketContextUpgradeFactory>::load(upgradeContextName, Role::Client);
    }

    SocketContextUpgradeFactorySelector* SocketContextUpgradeFactorySelector::instance() {
        static SocketContextUpgradeFactorySelector socketContextUpgradeFactorySelector;

        return &socketContextUpgradeFactorySelector;
    }

    SocketContextUpgradeFactory* SocketContextUpgradeFactorySelector::select(const std::string& upgradeContextName, Request& req) {
        SocketContextUpgradeFactory* socketContextUpgradeFactory = nullptr;

        socketContextUpgradeFactory =
            web::http::SocketContextUpgradeFactorySelector<SocketContextUpgradeFactory>::select(upgradeContextName);

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

            socketContextUpgradeFactory =
                web::http::SocketContextUpgradeFactorySelector<SocketContextUpgradeFactory>::select(upgradeContextName);

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
