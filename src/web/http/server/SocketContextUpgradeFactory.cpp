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

#include "web/http/server/SocketContextUpgradeFactory.h"

#include "web/http/SocketContextUpgradeFactory.hpp" // IWYU pragma: keep
#include "web/http/server/SocketContextUpgradeFactorySelector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    SocketContextUpgradeFactory::SocketContextUpgradeFactory() {
    }

    void SocketContextUpgradeFactory::checkRefCount() {
        if (refCount == 0) {
            web::http::server::SocketContextUpgradeFactorySelector::instance()->unload(this);
        }
    }

    void SocketContextUpgradeFactory::link(const std::string& upgradeContextName, SocketContextUpgradeFactory* (*linkedPlugin)()) {
        web::http::server::SocketContextUpgradeFactorySelector::instance()->link(upgradeContextName, linkedPlugin);
    }

} // namespace web::http::server

template class web::http::SocketContextUpgradeFactory<web::http::server::Request, web::http::server::Response>;
