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

#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    struct SocketContextPlugin {
        web::http::client::SocketContextUpgradeFactory* socketContextUpgradeFactory;
        void* handle = nullptr;
    };

    class SocketContextUpgradeFactorySelector {
    private:
        SocketContextUpgradeFactorySelector() = default;
        ~SocketContextUpgradeFactorySelector() = default;

    public:
        void registerSocketContextUpgradeFactory(web::http::client::SocketContextUpgradeFactory* socketContextUpgradeFactory);
        void registerSocketContextUpgradeFactory(web::http::client::SocketContextUpgradeFactory* socketContextUpgradeFactory,
                                                 void* handler);

        void loadSocketContexts();
        void unloadSocketContexts();

        web::http::client::SocketContextUpgradeFactory*
        select(const std::string& subProtocolName, web::http::client::Request& req, web::http::client::Response& res);

        static SocketContextUpgradeFactorySelector* instance();

    protected:
        std::map<std::string, SocketContextPlugin> serverSocketContextPlugins;
        std::map<std::string, SocketContextPlugin> clientSocketContextPlugins;

    public:
        static SocketContextUpgradeFactorySelector* socketContextUpgradeFactorySelector;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_SOCKETCONTEXTUPGRADEFACTORYSELECTOR_H
