/*
 * SNode.C - a slim toolkit for network communication
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

#include "web/http/SocketContextUpgrade.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <string> // IWYU pragma: export

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef WEB_HTTP_SOCKETCONTEXTUPGRADEFACTORYSELECTOR_H
#define WEB_HTTP_SOCKETCONTEXTUPGRADEFACTORYSELECTOR_H

namespace web::http {

    template <typename SocketContextUpgradeFactoryT>
    class SocketContextUpgradeFactorySelector {
    public:
        using SocketContextUpgradeFactory = SocketContextUpgradeFactoryT;
        using Request = typename SocketContextUpgradeFactory::Request;
        using Response = typename SocketContextUpgradeFactory::Response;

    protected:
        SocketContextUpgradeFactorySelector() = default;
        virtual ~SocketContextUpgradeFactorySelector() = default;

    private:
        struct SocketContextPlugin {
            SocketContextUpgradeFactory* socketContextUpgradeFactory;
            void* handle = nullptr;
        };

    public:
        virtual SocketContextUpgradeFactory* select(Request& req, Response& res) = 0;

        bool add(SocketContextUpgradeFactory* socketContextUpgradeFactory);

        void link(const std::string& socketContextUpgradeName, SocketContextUpgradeFactory* (*linkedPlugin)());

        void allowDlOpen();

        void unload(SocketContextUpgradeFactory* socketContextUpgradeFactory);

    protected:
        SocketContextUpgradeFactory* select(const std::string& socketContextUpgradeName);

        virtual SocketContextUpgradeFactory* load(const std::string& socketContextUpgradeName) = 0;

        SocketContextUpgradeFactory* load(const std::string& socketContextUpgradeName,
                                          const std::string& socketContextUpgradeFactoryLibraryFile,
                                          const std::string& socketContextUpgradeFactoryFunctionName);

        bool add(SocketContextUpgradeFactory* socketContextUpgradeFactory, void* handler);

    private:
        std::map<std::string, SocketContextPlugin> socketContextUpgradePlugins;
        std::map<std::string, SocketContextUpgradeFactory* (*) ()> linkedSocketContextUpgradePlugins;

        bool onlyLinked = false;
    };

} // namespace web::http

#endif // WEB_HTTP_SOCKETCONTEXTUPGRADEFACTORYSELECTOR_H
