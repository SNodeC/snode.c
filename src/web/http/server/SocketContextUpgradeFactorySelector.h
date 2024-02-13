/*
 * snode.c - a slim toolkit for network communication
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

#ifndef WEB_HTTP_SERVER_SOCKETCONTEXTUPGRADEFACTORYSELECTOR_H
#define WEB_HTTP_SERVER_SOCKETCONTEXTUPGRADEFACTORYSELECTOR_H

#include "web/http/SocketContextUpgradeFactorySelector.h"
#include "web/http/server/SocketContextUpgradeFactory.h" // IWYU pragma: export

// IWYU pragma: no_include "web/http/SocketContextUpgradeFactorySelector.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    class SocketContextUpgradeFactorySelector
        : public web::http::SocketContextUpgradeFactorySelector<web::http::server::SocketContextUpgradeFactory> {
    private:
        using Super = web::http::SocketContextUpgradeFactorySelector<web::http::server::SocketContextUpgradeFactory>;

        using Super::load;
        SocketContextUpgradeFactory* load(const std::string& socketContextUpgradeName) override;

    public:
        static SocketContextUpgradeFactorySelector* instance();

        using Super::select;
        SocketContextUpgradeFactory* select(Request& req, Response& res) override;
    };

} // namespace web::http::server

extern template class web::http::SocketContextUpgradeFactorySelector<web::http::server::SocketContextUpgradeFactory>;

#endif // WEB_HTTP_SERVER_SOCKETCONTEXTUPGRADEFACTORYSELECTOR_H
