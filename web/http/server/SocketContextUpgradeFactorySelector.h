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

#ifndef WEB_HTTP_SERVER_SOCKETCONTEXTUPGRADEFACTORYSELECTOR_H
#define WEB_HTTP_SERVER_SOCKETCONTEXTUPGRADEFACTORYSELECTOR_H

namespace web::http::server {
    class SocketContextUpgradeFactory;
    class Request;
    class Response;
} // namespace web::http::server

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::server {

    struct SocketContextPlugin {
        web::http::server::SocketContextUpgradeFactory* socketContextUpgradeFactory;
        void* handle = nullptr;
    };

    class SocketContextUpgradeFactorySelector {
    private:
        SocketContextUpgradeFactorySelector();
        ~SocketContextUpgradeFactorySelector() = default;

    public:
        static SocketContextUpgradeFactorySelector* instance();

        web::http::server::SocketContextUpgradeFactory* select(web::http::server::Request& req, web::http::server::Response& res);

        void unload();

        bool add(web::http::server::SocketContextUpgradeFactory* socketContextUpgradeFactory);

    protected:
        web::http::server::SocketContextUpgradeFactory* load(const std::string& socketContextName);

        bool add(web::http::server::SocketContextUpgradeFactory* socketContextUpgradeFactory, void* handler);

        std::map<std::string, SocketContextPlugin> socketContextUpgradePlugins;
        std::list<std::string> searchPaths;

    private:
        static SocketContextUpgradeFactorySelector* socketContextUpgradeFactorySelector;
    };

} // namespace web::http::server

#endif // WEB_HTTP_SERVER_SOCKETCONTEXTUPGRADEFACTORYSELECTOR_H
