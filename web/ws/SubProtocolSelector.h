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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws {

    class SubProtocolInterface;

    struct SubProtocolPlugin {
        SubProtocolInterface* subprotocolPluginInterface;
        void* handle = nullptr;
    };

    class SubProtocolSelector {
    protected:
        SubProtocolSelector();
        virtual ~SubProtocolSelector();

        SubProtocolSelector(const SubProtocolSelector&) = delete;
        SubProtocolSelector& operator=(const SubProtocolSelector&) = delete;

    public:
        void loadSubProtocol(const std::string& filePath);
        void loadSubProtocols(const std::string& directoryPath);
        void registerSubProtocol(SubProtocolInterface* subProtocolPluginInterface, void* handle = nullptr);

        void unloadSubProtocols();

        virtual SubProtocolInterface* select(const std::string& subProtocolName) = 0;

    protected:
        void loadSubProtocols();

        std::map<std::string, SubProtocolPlugin> serverSubprotocols;
        std::map<std::string, SubProtocolPlugin> clientSubprotocols;
    };

} // namespace web::ws

#endif // WEB_WS_SUBPROTOCOLSELECTOR_H
