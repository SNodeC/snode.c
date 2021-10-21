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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef WEB_HTTP_SOCKETCONTEXTUPGRADEFACTORYSELECTOR_H
#define WEB_HTTP_SOCKETCONTEXTUPGRADEFACTORYSELECTOR_H

namespace web::http {

    template <typename SocketContextUpgradeFactory>
    struct SocketContextPlugin {
        SocketContextUpgradeFactory* socketContextUpgradeFactory;
        void* handle = nullptr;
    };

    template <typename SocketContextUpgradeFactoryT>
    class SocketContextUpgradeFactorySelector {
    public:
        using SocketContextUpgradeFactory = SocketContextUpgradeFactoryT;
        using Request = typename SocketContextUpgradeFactory::Request;
        using Response = typename SocketContextUpgradeFactory::Response;

    protected:
        enum class Role { Server, Client };

        SocketContextUpgradeFactorySelector(typename SocketContextUpgradeFactory::Role role);
        virtual ~SocketContextUpgradeFactorySelector() = default;

    public:
        virtual SocketContextUpgradeFactory* select(Request& req, Response& res) = 0;

        bool add(SocketContextUpgradeFactory* socketContextUpgradeFactory);

        void linkSocketUpgradeContext(const std::string& upgradeContextName, SocketContextUpgradeFactory* (*linkedPlugin)());

        void unload(SocketContextUpgradeFactory* socketContextUpgradeFactory);

    protected:
        SocketContextUpgradeFactory* select(const std::string& upgradeContextName);

        virtual SocketContextUpgradeFactory* load(const std::string& upgradeContextName) = 0;
        SocketContextUpgradeFactory* load(const std::string& upgradeContextName, Role role);

        bool add(SocketContextUpgradeFactory* socketContextUpgradeFactory, void* handler);

        std::map<std::string, SocketContextPlugin<SocketContextUpgradeFactory>> socketContextUpgradePlugins;
        std::map<std::string, SocketContextUpgradeFactory* (*) ()> linkedSocketContextUpgradePlugins;
        std::list<std::string> searchPaths;

        const typename SocketContextUpgradeFactory::Role role;
    };

} // namespace web::http

#endif // WEB_HTTP_SOCKETCONTEXTUPGRADEFACTORYSELECTOR_H
