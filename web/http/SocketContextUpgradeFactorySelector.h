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

namespace web::http {
    template <typename RequestT, typename ResponseT>
    class SocketContextUpgradeFactory;
} // namespace web::http

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef WEB_HTTP_SOCKETCONTEXTUPGRADEFACTORYSELECTOR_H
#define WEB_HTTP_SOCKETCONTEXTUPGRADEFACTORYSELECTOR_H

namespace web::http {

    template <typename RequestT, typename ResponseT>
    struct SocketContextPlugin {
        SocketContextUpgradeFactory<RequestT, ResponseT>* socketContextUpgradeFactory;
        void* handle = nullptr;
    };

    template <typename RequestT, typename ResponseT>
    class SocketContextUpgradeFactorySelector {
    public:
        using Request = RequestT;
        using Response = ResponseT;

    protected:
        SocketContextUpgradeFactorySelector(typename SocketContextUpgradeFactory<Request, Response>::Role role);
        virtual ~SocketContextUpgradeFactorySelector() = default;

    public:
        virtual SocketContextUpgradeFactory<Request, Response>* select(Request& req, Response& res) = 0;

        bool add(SocketContextUpgradeFactory<Request, Response>* socketContextUpgradeFactory);

        void setLinkedPlugin(const std::string& upgradeContextName, SocketContextUpgradeFactory<Request, Response>* (*linkedPlugin)());

        void unload(SocketContextUpgradeFactory<Request, Response>* socketContextUpgradeFactory);

    protected:
        SocketContextUpgradeFactory<Request, Response>* select(const std::string& upgradeContextName);

        SocketContextUpgradeFactory<Request, Response>* load(const std::string& socketContextName);

        bool add(SocketContextUpgradeFactory<Request, Response>* socketContextUpgradeFactory, void* handler);

        std::map<std::string, SocketContextPlugin<Request, Response>> socketContextUpgradePlugins;
        std::map<std::string, SocketContextUpgradeFactory<Request, Response>* (*) ()> linkedSocketContextUpgradePlugins;
        std::list<std::string> searchPaths;

        const typename SocketContextUpgradeFactory<Request, Response>::Role role;
    };

} // namespace web::http

#endif // WEB_HTTP_SOCKETCONTEXTUPGRADEFACTORYSELECTOR_H
