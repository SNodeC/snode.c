
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

#include "core/DynamicLoader.h"
#include "web/http/SocketContextUpgradeFactorySelector.h"
#include "web/http/http_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <tuple> // for tie, tuple

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http {

    template <typename SocketContextUpgradeFactory>
    void
    SocketContextUpgradeFactorySelector<SocketContextUpgradeFactory>::addSocketContextUpgradeSearchPath(const std::string& searchPath) {
        searchPaths.push_front(searchPath);
    }

    template <typename SocketContextUpgradeFactory>
    bool SocketContextUpgradeFactorySelector<SocketContextUpgradeFactory>::add(SocketContextUpgradeFactory* socketContextUpgradeFactory,
                                                                               void* handle) {
        bool success = false;

        if (socketContextUpgradeFactory != nullptr) {
            SocketContextPlugin<SocketContextUpgradeFactory> socketContextPlugin = {
                .socketContextUpgradeFactory = socketContextUpgradeFactory, .handle = handle};
            std::tie(std::ignore, success) = socketContextUpgradePlugins.insert({socketContextUpgradeFactory->name(), socketContextPlugin});
        }

        return success;
    }

    template <typename SocketContextUpgradeFactory>
    bool SocketContextUpgradeFactorySelector<SocketContextUpgradeFactory>::add(SocketContextUpgradeFactory* socketContextUpgradeFactory) {
        return add(socketContextUpgradeFactory, nullptr);
    }

    template <typename SocketContextUpgradeFactory>
    void SocketContextUpgradeFactorySelector<SocketContextUpgradeFactory>::allowDlOpen() {
        onlyLinked = false;
    }

    template <typename SocketContextUpgradeFactory>
    SocketContextUpgradeFactory*
    SocketContextUpgradeFactorySelector<SocketContextUpgradeFactory>::load(const std::string& upgradeContextName,
                                                                           core::socket::SocketContext::Role role) {
        SocketContextUpgradeFactory* socketContextUpgradeFactory = nullptr;

        for (const std::string& searchPath : searchPaths) {
            void* handle =
                core::DynamicLoader::dlOpen((searchPath + "/libsnodec-" + upgradeContextName + ".so").c_str(), RTLD_LAZY | RTLD_GLOBAL);

            if (handle != nullptr) {
                SocketContextUpgradeFactory* (*getSocketContextUpgradeFactory)() =
                    reinterpret_cast<SocketContextUpgradeFactory* (*) ()>(core::DynamicLoader::dlSym(
                        handle,
                        upgradeContextName + (role == core::socket::SocketContext::Role::SERVER ? "Server" : "Client") +
                            "ContextUpgradeFactory"));

                if (getSocketContextUpgradeFactory != nullptr) {
                    socketContextUpgradeFactory = getSocketContextUpgradeFactory();

                    if (socketContextUpgradeFactory != nullptr) {
                        if (add(socketContextUpgradeFactory, handle)) {
                            VLOG(0) << "SocketContextUpgradeFactory created successfull: " << socketContextUpgradeFactory->name();
                        } else {
                            VLOG(0) << "UpgradeSocketContext already existing. Not using: " << socketContextUpgradeFactory->name();
                            socketContextUpgradeFactory->destroy();
                            socketContextUpgradeFactory = nullptr;
                            core::DynamicLoader::dlCloseDelayed(handle);
                        }
                        break;
                    } else {
                        core::DynamicLoader::dlCloseDelayed(handle);
                        VLOG(0) << "SocketContextUpgradeFactory not created: " << upgradeContextName;
                    }
                } else {
                    core::DynamicLoader::dlCloseDelayed(handle);
                    VLOG(0) << "Not a Plugin \"" << upgradeContextName;
                }
            } else {
                VLOG(0) << "Error dlopen: " << core::DynamicLoader::dlError();
            }
        }

        return socketContextUpgradeFactory;
    }

    template <typename SocketContextUpgradeFactory>
    SocketContextUpgradeFactory*
    SocketContextUpgradeFactorySelector<SocketContextUpgradeFactory>::select(const std::string& upgradeContextName) {
        SocketContextUpgradeFactory* socketContextUpgradeFactory = nullptr;

        if (socketContextUpgradePlugins.contains(upgradeContextName)) {
            socketContextUpgradeFactory = socketContextUpgradePlugins[upgradeContextName].socketContextUpgradeFactory;
        } else if (linkedSocketContextUpgradePlugins.contains(upgradeContextName)) {
            socketContextUpgradeFactory = linkedSocketContextUpgradePlugins[upgradeContextName]();
            add(socketContextUpgradeFactory);
        } else if (!onlyLinked) {
            socketContextUpgradeFactory = load(upgradeContextName);
        }

        return socketContextUpgradeFactory;
    }

    template <typename SocketContextUpgradeFactory>
    void SocketContextUpgradeFactorySelector<SocketContextUpgradeFactory>::link(const std::string& upgradeContextName,
                                                                                SocketContextUpgradeFactory* (*linkedPlugin)()) {
        if (!linkedSocketContextUpgradePlugins.contains(upgradeContextName)) {
            linkedSocketContextUpgradePlugins[upgradeContextName] = linkedPlugin;
        }

        onlyLinked = true;
    }

    template <typename SocketContextUpgradeFactory>
    void
    SocketContextUpgradeFactorySelector<SocketContextUpgradeFactory>::unload(SocketContextUpgradeFactory* socketContextUpgradeFactory) {
        std::string upgradeContextNames = socketContextUpgradeFactory->name();

        if (socketContextUpgradePlugins.contains(upgradeContextNames)) {
            SocketContextPlugin<SocketContextUpgradeFactory>& socketContextPlugin = socketContextUpgradePlugins[upgradeContextNames];

            socketContextUpgradeFactory->destroy();

            if (socketContextPlugin.handle != nullptr) {
                core::DynamicLoader::dlCloseDelayed(socketContextPlugin.handle);
            }

            socketContextUpgradePlugins.erase(upgradeContextNames);
        }
    }

} // namespace web::http
