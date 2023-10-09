
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

#include "core/DynamicLoader.h"
#include "web/http/SocketContextUpgradeFactorySelector.h"
#include "web/http/http_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstdlib>
#include <filesystem>
#include <tuple>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http {

    template <typename SocketContextUpgradeFactory>
    bool SocketContextUpgradeFactorySelector<SocketContextUpgradeFactory>::add(SocketContextUpgradeFactory* socketContextUpgradeFactory,
                                                                               void* handle) {
        bool success = false;

        if (socketContextUpgradeFactory != nullptr) {
            SocketContextPlugin socketContextPlugin = {.socketContextUpgradeFactory = socketContextUpgradeFactory, .handle = handle};
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
    SocketContextUpgradeFactorySelector<SocketContextUpgradeFactory>::load(const std::string& socketContextUpgradeName,
                                                                           const std::string& socketContextUpgradeFactoryLibraryFile,
                                                                           const std::string& socketContextUpgradeFactoryFunctionName) {
        SocketContextUpgradeFactory* socketContextUpgradeFactory = nullptr;

        void* handle = dlOpen(socketContextUpgradeFactoryLibraryFile, RTLD_LAZY | RTLD_GLOBAL);

        if (handle != nullptr) {
            SocketContextUpgradeFactory* (*getSocketContextUpgradeFactory)() = reinterpret_cast<SocketContextUpgradeFactory* (*) ()>(
                core::DynamicLoader::dlSym(handle, socketContextUpgradeFactoryFunctionName));

            if (getSocketContextUpgradeFactory != nullptr) {
                socketContextUpgradeFactory = getSocketContextUpgradeFactory();

                if (socketContextUpgradeFactory != nullptr) {
                    if (add(socketContextUpgradeFactory, handle)) {
                        LOG(DEBUG) << "SocketContextUpgradeFactory created successfull: " << socketContextUpgradeName;
                    } else {
                        LOG(DEBUG) << "UpgradeSocketContext already existing. Not using: " << socketContextUpgradeName;
                        delete socketContextUpgradeFactory;
                        socketContextUpgradeFactory = nullptr;
                        core::DynamicLoader::dlClose(handle);
                    }
                } else {
                    LOG(DEBUG) << "SocketContextUpgradeFactory not created: " << socketContextUpgradeName;
                    core::DynamicLoader::dlClose(handle);
                }
            } else {
                LOG(DEBUG) << "Optaining function \"" << socketContextUpgradeFactoryFunctionName
                           << "\" in plugin failed: " << core::DynamicLoader::dlError();
                core::DynamicLoader::dlClose(handle);
            }
        }

        return socketContextUpgradeFactory;
    }

    template <typename SocketContextUpgradeFactory>
    SocketContextUpgradeFactory*
    SocketContextUpgradeFactorySelector<SocketContextUpgradeFactory>::select(const std::string& socketContextUpgradeName) {
        SocketContextUpgradeFactory* socketContextUpgradeFactory = nullptr;

        if (socketContextUpgradePlugins.contains(socketContextUpgradeName)) {
            socketContextUpgradeFactory = socketContextUpgradePlugins[socketContextUpgradeName].socketContextUpgradeFactory;
        } else if (linkedSocketContextUpgradePlugins.contains(socketContextUpgradeName)) {
            socketContextUpgradeFactory = linkedSocketContextUpgradePlugins[socketContextUpgradeName]();
            add(socketContextUpgradeFactory);
        } else if (!onlyLinked) {
            socketContextUpgradeFactory = load(socketContextUpgradeName);
        }

        return socketContextUpgradeFactory;
    }

    template <typename SocketContextUpgradeFactory>
    void SocketContextUpgradeFactorySelector<SocketContextUpgradeFactory>::link(const std::string& socketContextUpgradeName,
                                                                                SocketContextUpgradeFactory* (*linkedPlugin)()) {
        if (!linkedSocketContextUpgradePlugins.contains(socketContextUpgradeName)) {
            linkedSocketContextUpgradePlugins[socketContextUpgradeName] = linkedPlugin;
        }

        onlyLinked = true;
    }

    template <typename SocketContextUpgradeFactory>
    void
    SocketContextUpgradeFactorySelector<SocketContextUpgradeFactory>::unload(SocketContextUpgradeFactory* socketContextUpgradeFactory) {
        std::string upgradeContextNames = socketContextUpgradeFactory->name();

        if (socketContextUpgradePlugins.contains(upgradeContextNames)) {
            SocketContextPlugin& socketContextPlugin = socketContextUpgradePlugins[upgradeContextNames];

            delete socketContextUpgradeFactory;

            if (socketContextPlugin.handle != nullptr) {
                core::DynamicLoader::dlCloseDelayed(socketContextPlugin.handle);
            }

            socketContextUpgradePlugins.erase(upgradeContextNames);
        }
    }

} // namespace web::http
