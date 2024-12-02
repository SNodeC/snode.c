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

#include "web/http/client/SocketContextUpgradeFactorySelector.h"

#include "web/http/SocketContextUpgradeFactorySelector.hpp"
#include "web/http/client/Response.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#if !defined(NDEBUG)

#include "log/Logger.h"

#include <cstdlib>
#endif

#include "web/http/http_utils.h"

#include <tuple>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    SocketContextUpgradeFactorySelector::SocketContextUpgradeFactory*
    SocketContextUpgradeFactorySelector::load(const std::string& socketContextUpgradeName) {
        const std::string socketContextUpgradeFactoryLibraryFile = socketContextUpgradeName + "-client.so." SOVERSION;
        const std::string socketContextUpgradeFactoryFunctionName = socketContextUpgradeName + "ClientContextUpgradeFactory";

        std::string httpUpgradeInstallLibdir = HTTP_UPGRADE_INSTALL_LIBDIR;

#if !defined(NDEBUG)
        if (const char* httpUpgradeInstallLibdirEnv = std::getenv("HTTP_UPGRADE_INSTALL_LIBDIR")) {
            LOG(WARNING) << "HTTP: Overriding http upgrade library dir";
            httpUpgradeInstallLibdir = std::string(httpUpgradeInstallLibdirEnv);
        }
#endif

        return Super::load(socketContextUpgradeName,
                           httpUpgradeInstallLibdir + "/libsnodec-" + socketContextUpgradeFactoryLibraryFile,
                           socketContextUpgradeFactoryFunctionName);
    }

    SocketContextUpgradeFactorySelector* SocketContextUpgradeFactorySelector::instance() {
        static SocketContextUpgradeFactorySelector socketContextUpgradeFactorySelector;

        return &socketContextUpgradeFactorySelector;
    }

    SocketContextUpgradeFactory* SocketContextUpgradeFactorySelector::select(const std::string& protocols, Request& req) {
        SocketContextUpgradeFactory* socketContextUpgradeFactory = nullptr;

        std::string upgradeContextNames = protocols;

        while (!upgradeContextNames.empty() && socketContextUpgradeFactory == nullptr) {
            std::string upgradeContextName;
            std::string upgradeContextVersion;

            std::tie(upgradeContextName, upgradeContextNames) = httputils::str_split(upgradeContextNames, ',');
            std::tie(upgradeContextName, upgradeContextVersion) = httputils::str_split(upgradeContextName, '/');

            httputils::str_trimm(upgradeContextName);
            httputils::to_lower(upgradeContextName);

            socketContextUpgradeFactory = select(upgradeContextName);
        }

        if (socketContextUpgradeFactory != nullptr) {
            socketContextUpgradeFactory->prepare(req); // Fill in the missing header fields into the request object
        }

        return socketContextUpgradeFactory;
    }

    SocketContextUpgradeFactory* SocketContextUpgradeFactorySelector::select(Request& req, Response& res) {
        SocketContextUpgradeFactory* socketContextUpgradeFactory = nullptr;

        std::string upgradeContextName = res.get("upgrade");

        if (!upgradeContextName.empty()) {
            httputils::to_lower(upgradeContextName);

            socketContextUpgradeFactory = select(upgradeContextName);

            if (socketContextUpgradeFactory != nullptr) {
                socketContextUpgradeFactory->prepare(req, res); // Fill in the missing header fields into the request object
            }
        }

        return socketContextUpgradeFactory;
    }

} // namespace web::http::client

template class web::http::SocketContextUpgradeFactorySelector<web::http::client::SocketContextUpgradeFactory>;
