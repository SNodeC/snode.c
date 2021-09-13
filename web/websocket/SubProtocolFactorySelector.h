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

#ifndef WEB_WS_SUBPROTOCOLSELECTOR_H
#define WEB_WS_SUBPROTOCOLSELECTOR_H

namespace web::websocket {
    template <typename S>
    class SubProtocolFactory;
} // namespace web::websocket

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket {

    template <typename SubProtocolT>
    struct SubProtocolPlugin {
        SubProtocolFactory<SubProtocolT>* subProtocolFactory;
        void* handle = nullptr;
    };

    template <typename SubProtocolT>
    class SubProtocolFactorySelector {
    protected:
        SubProtocolFactorySelector() = default;
        virtual ~SubProtocolFactorySelector() = default;

        SubProtocolFactorySelector(const SubProtocolFactorySelector&) = delete;
        SubProtocolFactorySelector& operator=(const SubProtocolFactorySelector&) = delete;

    public:
        SubProtocolFactory<SubProtocolT>* select(const std::string& subProtocolName);

        void add(SubProtocolFactory<SubProtocolT>* subProtocolFactory, void* handle = nullptr);

        void unload();

    protected:
        SubProtocolFactory<SubProtocolT>* load(const std::string& filePath);

        void addSubProtocolSearchPath(const std::string& searchPath);

        std::map<std::string, SubProtocolPlugin<SubProtocolT>> subProtocolPlugins;
        std::list<std::string> searchPaths;
    };

} // namespace web::websocket

#endif // WEB_WS_SUBPROTOCOLSELECTOR_H
