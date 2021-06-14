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

#include "log/Logger.h"
#include "web/http/server/SocketContextUpgradeFactory.h"
#include "web/http/server/SocketContextUpgradeInterface.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <dlfcn.h>
#include <type_traits> // for add_const<>::type

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    SocketContextUpgradeFactorySelector* SocketContextUpgradeFactorySelector::socketContextUpgradeFactorySelector = nullptr;

    void SocketContextUpgradeFactorySelector::loadSocketContexts() {
        void* handle = dlopen("/usr/local/lib/snodec/web/ws/libwebsocket.so", RTLD_LAZY | RTLD_GLOBAL);

        if (handle != nullptr) {
            SocketContextUpgradeInterface* (*plugin)() = reinterpret_cast<SocketContextUpgradeInterface* (*) ()>(dlsym(handle, "plugin"));

            SocketContextUpgradeInterface* socketContextUpgradeInterface = plugin();

            SocketContextUpgradeFactorySelector::instance()->registerSocketContextUpgradeFactory(socketContextUpgradeInterface->create(),
                                                                                                 handle);

            delete socketContextUpgradeInterface;
        }
    }

    void SocketContextUpgradeFactorySelector::unloadSocketContexts() {
        for (const auto& [name, socketContextPlugin] : serverSocketContextPlugins) {
            socketContextPlugin.socketContextUpgradeFactory->destroy();
            if (socketContextPlugin.handle != nullptr) {
                dlclose(socketContextPlugin.handle);
            }
        }

        for (const auto& [name, socketContextPlugin] : clientSocketContextPlugins) {
            socketContextPlugin.socketContextUpgradeFactory->destroy();
            if (socketContextPlugin.handle != nullptr) {
                dlclose(socketContextPlugin.handle);
            }
        }

        delete this;
    }

    SocketContextUpgradeFactorySelector* SocketContextUpgradeFactorySelector::instance() {
        if (socketContextUpgradeFactorySelector == nullptr) {
            socketContextUpgradeFactorySelector = new SocketContextUpgradeFactorySelector();
        }
        return socketContextUpgradeFactorySelector;
    }

    void SocketContextUpgradeFactorySelector::registerSocketContextUpgradeFactory(
        web::http::server::SocketContextUpgradeFactory* socketContextUpgradeFactory) {
        registerSocketContextUpgradeFactory(socketContextUpgradeFactory, nullptr);
    }

    void SocketContextUpgradeFactorySelector::registerSocketContextUpgradeFactory(
        web::http::server::SocketContextUpgradeFactory* socketContextUpgradeFactory, void* handle) {
        SocketContextPlugin socketContextPlugin = {.socketContextUpgradeFactory = socketContextUpgradeFactory, .handle = handle};

        if (socketContextUpgradeFactory->role() == SocketContextUpgradeFactory::ROLE::SERVER) {
            const auto [it, success] = serverSocketContextPlugins.insert({socketContextUpgradeFactory->name(), socketContextPlugin});
            if (!success) {
                VLOG(0) << "UpgradeSocketContext already existing: not using " << socketContextUpgradeFactory->name();
                socketContextUpgradeFactory->destroy();
                if (handle != nullptr) {
                    dlclose(handle);
                }
            }
        } else if (socketContextUpgradeFactory->role() == SocketContextUpgradeFactory::ROLE::CLIENT) {
            const auto [it, success] = clientSocketContextPlugins.insert({socketContextUpgradeFactory->name(), socketContextPlugin});
            if (!success) {
                VLOG(0) << "UpgradeSocketContext already existing: not using " << socketContextUpgradeFactory->name();
                socketContextUpgradeFactory->destroy();
                if (handle != nullptr) {
                    dlclose(handle);
                }
            }
        }
    }

    web::http::server::SocketContextUpgradeFactory* SocketContextUpgradeFactorySelector::select(const std::string& name) {
        web::http::server::SocketContextUpgradeFactory* socketContextFactory = nullptr;

        if (serverSocketContextPlugins.contains(name)) {
            socketContextFactory = serverSocketContextPlugins[name].socketContextUpgradeFactory;
        }

        return socketContextFactory;
    }

} // namespace web::http::server
