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

#include "SocketContextUpgradeFactorySelector.h"

#include "config.h"
#include "web/http/http_utils.h"
#include "web/http/server/Request.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>   // for list
#include <string> // for string, all...
#include <tuple>  // for tie, tuple

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    SocketContextUpgradeFactorySelector::SocketContextUpgradeFactorySelector()
        : web::http::SocketContextUpgradeFactorySelector<SocketContextUpgradeFactory, Request, Response>(
              web::http::SocketContextUpgradeFactory<Request, Response>::Role::SERVER) {
#ifndef NDEBUG
#ifdef UPGRADECONTEXT_SERVER_COMPILE_PATH

        searchPaths.push_back(UPGRADECONTEXT_SERVER_COMPILE_PATH);

#endif // UPGRADECONTEXT_SERVER_COMPILE_PATH
#endif // NDEBUG

        searchPaths.push_back(UPGRADECONTEXT_SERVER_INSTALL_PATH);
    }

    SocketContextUpgradeFactorySelector* SocketContextUpgradeFactorySelector::instance() {
        static SocketContextUpgradeFactorySelector socketContextUpgradeFactorySelector;

        return &socketContextUpgradeFactorySelector;
    }

    /* do not remove */
    SocketContextUpgradeFactory* SocketContextUpgradeFactorySelector::select(Request& req, Response& res) {
        SocketContextUpgradeFactory* socketContextUpgradeFactory = nullptr;

        std::string upgradeContextNames = req.header("upgrade");

        if (!upgradeContextNames.empty()) {
            std::string upgradeContextName;
            std::string upgradeContextPriority;

            std::tie(upgradeContextName, upgradeContextNames) = httputils::str_split(upgradeContextNames, ',');
            std::tie(upgradeContextName, upgradeContextPriority) = httputils::str_split(upgradeContextName, '/');

            httputils::to_lower(upgradeContextName);

            socketContextUpgradeFactory = dynamic_cast<SocketContextUpgradeFactory*>(select(upgradeContextName));

            if (socketContextUpgradeFactory != nullptr) {
                socketContextUpgradeFactory->prepare(req, res);
            }
        }

        return socketContextUpgradeFactory;
    }

} // namespace web::http::server
