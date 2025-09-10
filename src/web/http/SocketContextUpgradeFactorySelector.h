/*
 * SNode.C - A Slim Toolkit for Network Communication
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "web/http/SocketContextUpgrade.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <string> // IWYU pragma: export

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef WEB_HTTP_SOCKETCONTEXTUPGRADEFACTORYSELECTOR_H
#define WEB_HTTP_SOCKETCONTEXTUPGRADEFACTORYSELECTOR_H

namespace web::http {

    template <typename SocketContextUpgradeFactoryT, typename... Args>
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
        virtual SocketContextUpgradeFactory* select(Request& req, Response& res, Args&&... args) = 0;

        bool add(SocketContextUpgradeFactory* socketContextUpgradeFactory);

        void link(const std::string& socketContextUpgradeName, SocketContextUpgradeFactory* (*linkedPlugin)());

        void allowDlOpen();

        void unload(SocketContextUpgradeFactory* socketContextUpgradeFactory);

    protected:
        SocketContextUpgradeFactory* select(const std::string& socketContextUpgradeName);

        virtual SocketContextUpgradeFactory* load(const std::string& socketContextUpgradeName, Args&&... args) = 0;

        SocketContextUpgradeFactory* load(const std::string& socketContextUpgradeName,
                                          const std::string& socketContextUpgradeFactoryLibraryFile,
                                          const std::string& socketContextUpgradeFactoryFunctionName,
                                          Args&&... args);

        bool add(SocketContextUpgradeFactory* socketContextUpgradeFactory, void* handler);

    private:
        std::map<std::string, SocketContextPlugin> socketContextUpgradePlugins;
        std::map<std::string, SocketContextUpgradeFactory* (*) ()> linkedSocketContextUpgradePlugins;

        bool onlyLinked = false;
    };

} // namespace web::http

#endif // WEB_HTTP_SOCKETCONTEXTUPGRADEFACTORYSELECTOR_H
