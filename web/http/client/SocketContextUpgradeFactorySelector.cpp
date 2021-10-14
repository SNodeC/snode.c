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
#include "web/http/client/Response.h"
#include "web/http/client/SocketContextUpgradeFactory.h"
#include "web/http/http_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <dlfcn.h>
#include <tuple> // for tie, tuple

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    //    SocketContextUpgradeFactorySelector* SocketContextUpgradeFactorySelector::socketContextUpgradeFactorySelector = nullptr;

    SocketContextUpgradeFactorySelector::SocketContextUpgradeFactorySelector() {
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

    void SocketContextUpgradeFactorySelector::unused(SocketContextUpgradeFactory* socketContextUpgradeFactory) {
        std::string upgradeContextNames = socketContextUpgradeFactory->name();

        if (socketContextUpgradePlugins.contains(upgradeContextNames)) {
            SocketContextPlugin& socketContextPlugin = socketContextUpgradePlugins[upgradeContextNames];

            if (socketContextPlugin.handle != nullptr) {
                socketContextUpgradeFactory->destroy();
                net::DynamicLoader::dlClose(socketContextPlugin.handle);
            }

            socketContextUpgradePlugins.erase(upgradeContextNames);
        }
    }

    bool SocketContextUpgradeFactorySelector::add(SocketContextUpgradeFactory* socketContextUpgradeFactory, void* handle) {
        bool success = false;

        if (socketContextUpgradeFactory != nullptr) {
            SocketContextPlugin socketContextPlugin = {.socketContextUpgradeFactory = socketContextUpgradeFactory, .handle = handle};

            if (socketContextUpgradeFactory->role() == SocketContextUpgradeFactory::Role::CLIENT) {
                std::tie(std::ignore, success) =
                    socketContextUpgradePlugins.insert({socketContextUpgradeFactory->name(), socketContextPlugin});
            }
        }

        return success;
    }

    bool SocketContextUpgradeFactorySelector::add(SocketContextUpgradeFactory* socketContextUpgradeFactory) {
        return add(socketContextUpgradeFactory, nullptr);
    }

    SocketContextUpgradeFactory* SocketContextUpgradeFactorySelector::load(const std::string& filePath) {
        SocketContextUpgradeFactory* socketContextUpgradeFactory = nullptr;

        void* handle = net::DynamicLoader::dlOpen(filePath.c_str(), RTLD_LAZY | RTLD_GLOBAL);

        if (handle != nullptr) {
            SocketContextUpgradeFactory* (*plugin)() = reinterpret_cast<SocketContextUpgradeFactory* (*) ()>(dlsym(handle, "plugin"));

            if (plugin != nullptr) {
                socketContextUpgradeFactory = plugin();

                if (socketContextUpgradeFactory != nullptr) {
                    VLOG(0) << "Upgrade Protocol: " << socketContextUpgradeFactory->name();
                    if (add(socketContextUpgradeFactory, handle)) {
                        VLOG(0) << "SocketContextUpgradeFactory created successfull: " << socketContextUpgradeFactory->name();
                    } else {
                        socketContextUpgradeFactory->destroy();
                        socketContextUpgradeFactory = nullptr;
                        net::DynamicLoader::dlClose(handle);
                        VLOG(0) << "SocketContextUpgradeFactory created successfull: " << socketContextUpgradeFactory->name();
                    }
                } else {
                    net::DynamicLoader::dlClose(handle);
                    VLOG(0) << "SocketContextUpgradeFactory not created: " << filePath;
                }
            } else {
                net::DynamicLoader::dlClose(handle);
                VLOG(0) << "Not a Plugin \"" << filePath;
            }
        } else {
            VLOG(0) << "Error dlopen: " << dlerror();
        }

        return socketContextUpgradeFactory;
    }

    SocketContextUpgradeFactory* SocketContextUpgradeFactorySelector::select(const std::string& upgradeContextName, bool doLoad) {
        SocketContextUpgradeFactory* socketContextUpgradeFactory = nullptr;

        if (socketContextUpgradePlugins.contains(upgradeContextName)) {
            socketContextUpgradeFactory = socketContextUpgradePlugins[upgradeContextName].socketContextUpgradeFactory;
        } else if (doLoad) {
            for (const std::string& searchPath : searchPaths) {
                socketContextUpgradeFactory = load(searchPath + "/libsnodec-" + upgradeContextName + ".so");

                if (socketContextUpgradeFactory != nullptr) {
                    break;
                }
            }
        }

        return socketContextUpgradeFactory;
    }

    SocketContextUpgradeFactory*
    SocketContextUpgradeFactorySelector::select(const std::string& _upgradeContextNames, Request& req, Response& res) {
        SocketContextUpgradeFactory* socketContextUpgradeFactory = nullptr;

        std::string upgradeContextNames =
            _upgradeContextNames; // Hack: read the context names from request and remove _upgradeContextNames from parameterlist

        if (!upgradeContextNames.empty()) {
            std::string upgradeContextName;
            std::string upgradeContextPriority;

            std::tie(upgradeContextName, upgradeContextNames) = httputils::str_split(upgradeContextNames, ',');
            std::tie(upgradeContextName, upgradeContextPriority) = httputils::str_split(upgradeContextName, '/');

            httputils::to_lower(upgradeContextName);

            socketContextUpgradeFactory = select(upgradeContextName);

            if (socketContextUpgradeFactory != nullptr) {
                socketContextUpgradeFactory->prepare(req, res); // Fill in the missing header fields into the request object
            }
        }

        return socketContextUpgradeFactory;
    }

    SocketContextUpgradeFactory* SocketContextUpgradeFactorySelector::select(Request& req, Response& res) {
        SocketContextUpgradeFactory* socketContextUpgradeFactory = nullptr;

        std::string upgradeContextNames = res.header("upgrade");

        if (!upgradeContextNames.empty()) {
            socketContextUpgradeFactory = select(upgradeContextNames, req, res);
        }

        return socketContextUpgradeFactory;
    }

} // namespace web::http::client
