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
#include "log/Logger.h"
#include "net/DynamicLoader.h"
#include "web/http/client/Request.h"
#include "web/http/client/Response.h"
#include "web/http/client/SocketContextUpgradeFactory.h"
#include "web/http/http_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <dlfcn.h>
#include <tuple> // for tie, tuple

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    SocketContextUpgradeFactorySelector::SocketContextUpgradeFactorySelector()
        : web::http::SocketContextUpgradeFactorySelector<Request, Response>(
              web::http::SocketContextUpgradeFactory<Request, Response>::Role::CLIENT) {
#ifndef NDEBUG
#ifdef UPGRADECONTEXT_CLIENT_COMPILE_PATH

        searchPaths.push_back(UPGRADECONTEXT_CLIENT_COMPILE_PATH);

#endif // UPGRADECONTEXT_CLIENT_COMPILE_PATH
#endif // NDEBUG

        searchPaths.push_back(UPGRADECONTEXT_CLIENT_INSTALL_PATH);
    }

    SocketContextUpgradeFactorySelector* SocketContextUpgradeFactorySelector::instance() {
        static SocketContextUpgradeFactorySelector socketContextUpgradeFactorySelector;

        return &socketContextUpgradeFactorySelector;
    }

    SocketContextUpgradeFactory* SocketContextUpgradeFactorySelector::select(const std::string& upgradeContextName, Request& req) {
        SocketContextUpgradeFactory* socketContextUpgradeFactory = nullptr;

        socketContextUpgradeFactory = dynamic_cast<SocketContextUpgradeFactory*>(select(upgradeContextName));

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

            socketContextUpgradeFactory = dynamic_cast<SocketContextUpgradeFactory*>(select(upgradeContextName));

            if (socketContextUpgradeFactory != nullptr) {
                socketContextUpgradeFactory->prepare(req, res); // Fill in the missing header fields into the request object
            }
        }

        return socketContextUpgradeFactory;
    }

} // namespace web::http::client
