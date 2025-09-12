
/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "core/DynamicLoader.h"
#include "web/http/SocketContextUpgradeFactorySelector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "web/http/http_utils.h"

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

        void* handle = core::DynamicLoader::dlOpen(socketContextUpgradeFactoryLibraryFile);

        if (handle != nullptr) {
            SocketContextUpgradeFactory* (*getSocketContextUpgradeFactory)() = reinterpret_cast<SocketContextUpgradeFactory* (*) ()>(
                core::DynamicLoader::dlSym(handle, socketContextUpgradeFactoryFunctionName));

            if (getSocketContextUpgradeFactory != nullptr) {
                socketContextUpgradeFactory = getSocketContextUpgradeFactory();

                if (socketContextUpgradeFactory != nullptr) {
                    if (add(socketContextUpgradeFactory, handle)) {
                        LOG(TRACE) << "HTTP: SocketContextUpgradeFactory create success: " << socketContextUpgradeName;
                    } else {
                        LOG(TRACE) << "HTTP: SocketContextUpgradeFactory already existing: " << socketContextUpgradeName;
                        delete socketContextUpgradeFactory;
                        socketContextUpgradeFactory = nullptr;
                        core::DynamicLoader::dlClose(handle);
                    }
                } else {
                    LOG(ERROR) << "HTTP: SocketContextUpgradeFactory create failed: " << socketContextUpgradeName;
                    core::DynamicLoader::dlClose(handle);
                }
            } else {
                LOG(ERROR) << "HTTP: Optaining function \"" << socketContextUpgradeFactoryFunctionName
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
            SocketContextPlugin socketContextPlugin = socketContextUpgradePlugins[upgradeContextNames];
            socketContextUpgradePlugins.erase(upgradeContextNames);

            delete socketContextUpgradeFactory;

            if (socketContextPlugin.handle != nullptr) {
                core::DynamicLoader::dlCloseDelayed(socketContextPlugin.handle);
            }
        }
    }

} // namespace web::http
