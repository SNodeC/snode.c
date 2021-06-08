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

#include "web/http/server/SocketContextUpgradeFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <dlfcn.h>
#include <type_traits> // for add_const<>::type

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    SocketContextUpgradeFactorySelector SocketContextUpgradeFactorySelector::socketContextUpgradeFactorySelector;

    SocketContextUpgradeFactorySelector::SocketContextUpgradeFactorySelector() {
    }

    SocketContextUpgradeFactorySelector::~SocketContextUpgradeFactorySelector() {
        for (const auto& [name, socketContextPlugin] : serverSocketContexts) {
            if (socketContextPlugin.handle != nullptr) {
                dlclose(socketContextPlugin.handle);
            }
            delete socketContextPlugin.socketContextFactory;
        }

        for (const auto& [name, socketContextPlugin] : clientSocketContexts) {
            if (socketContextPlugin.handle != nullptr) {
                dlclose(socketContextPlugin.handle);
            }
            delete socketContextPlugin.socketContextFactory;
        }
    }

    SocketContextUpgradeFactorySelector& SocketContextUpgradeFactorySelector::instance() {
        return SocketContextUpgradeFactorySelector::socketContextUpgradeFactorySelector;
    }

    void SocketContextUpgradeFactorySelector::registerSubProtocol(web::http::server::SocketContextUpgradeFactory* socketContextFactory) {
        registerSubProtocol(socketContextFactory, nullptr);
    }

    void SocketContextUpgradeFactorySelector::registerSubProtocol(web::http::server::SocketContextUpgradeFactory* socketContextFactory,
                                                                  void* handle) {
        SocketContextPlugin socketContextPlugin = {.socketContextFactory = socketContextFactory, .handle = handle};

        if (socketContextFactory->type() == "server") {
            [[maybe_unused]] const auto [it, success] = serverSocketContexts.insert({socketContextFactory->name(), socketContextPlugin});
        } else {
            [[maybe_unused]] const auto [it, success] = clientSocketContexts.insert({socketContextFactory->name(), socketContextPlugin});
        }
    }

    web::http::server::SocketContextUpgradeFactory* SocketContextUpgradeFactorySelector::select(const std::string& name) {
        web::http::server::SocketContextUpgradeFactory* socketContextFactory = nullptr;

        if (serverSocketContexts.contains(name)) {
            socketContextFactory = serverSocketContexts[name].socketContextFactory;
        }

        return socketContextFactory;
    }

} // namespace web::http::server
