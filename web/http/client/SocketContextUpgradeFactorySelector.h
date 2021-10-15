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

#ifndef WEB_HTTP_CLIENT_SOCKETCONTEXTUPGRADEFACTORYSELECTOR_H
#define WEB_HTTP_CLIENT_SOCKETCONTEXTUPGRADEFACTORYSELECTOR_H

namespace web::http::client {
    class SocketContextUpgradeFactory;
    class Request;
    class Response;
} // namespace web::http::client

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    struct SocketContextPlugin {
        SocketContextUpgradeFactory* socketContextUpgradeFactory;
        void* handle = nullptr;
    };

    class SocketContextUpgradeFactorySelector {
    private:
        SocketContextUpgradeFactorySelector();
        ~SocketContextUpgradeFactorySelector() = default;

    public:
        static SocketContextUpgradeFactorySelector* instance();

        SocketContextUpgradeFactory* select(const std::string& upgradeContextName, Request& req);
        SocketContextUpgradeFactory* select(Request& req, Response& res);

        bool add(SocketContextUpgradeFactory* socketContextUpgradeFactory);

        void setLinkedPlugin(const std::string& upgradeContextName, SocketContextUpgradeFactory* (*linkedPlugin)());

        void unused(SocketContextUpgradeFactory* socketContextUpgradeFactory);

    protected:
        SocketContextUpgradeFactory* select(const std::string& upgradeContextName);

        web::http::client::SocketContextUpgradeFactory* load(const std::string& socketContextName);

        bool add(SocketContextUpgradeFactory* socketContextUpgradeFactory, void* handler);

        std::map<std::string, SocketContextPlugin> socketContextUpgradePlugins;
        std::map<std::string, SocketContextUpgradeFactory* (*) ()> linkedSocketContextUpgradePlugins;
        std::list<std::string> searchPaths;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_SOCKETCONTEXTUPGRADEFACTORYSELECTOR_H
