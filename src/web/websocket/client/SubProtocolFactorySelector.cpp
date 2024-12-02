/*
 * SNode.C - a slim toolkit for network communication
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

#include "web/websocket/client/SubProtocolFactorySelector.h"

#include "utils/Config.h"
#include "web/websocket/SubProtocolFactory.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#if !defined(NDEBUG)
#include "log/Logger.h"

#include <cstdlib>
#endif

#include <filesystem>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::client {

    SubProtocolFactorySelector* SubProtocolFactorySelector::instance() {
        static SubProtocolFactorySelector subProtocolFactorySelector;

        return &subProtocolFactorySelector;
    }

    SubProtocolFactorySelector::~SubProtocolFactorySelector() {
    }

    void SubProtocolFactorySelector::allowDlOpen() {
        SubProtocolFactorySelector::instance()->Super::allowDlOpen();
    }

    SubProtocolFactorySelector::SubProtocolFactory* SubProtocolFactorySelector::load(const std::string& subProtocolName) {
        const std::string websocketSubprotocolLibraryFile = "libsnodec-websocket-" + subProtocolName + "-client.so." SOVERSION;
        const std::string websocketSubprotocolFunctionName = subProtocolName + "ClientSubProtocolFactory";

        std::string websocketSubprotocolInstallLibdir = WEBSOCKET_SUBPROTOCO_INSTALL_LIBDIR;

#if !defined(NDEBUG)
        if (const char* websocketSubprotocolInstallLibdirEnv = std::getenv("WEBSOCKET_SUBPROTOCOL_INSTALL_LIBDIR")) {
            LOG(WARNING) << "WebSocket: Overriding websocket subprotocol library dir";
            websocketSubprotocolInstallLibdir = std::string(websocketSubprotocolInstallLibdirEnv);
        }
#endif

        SubProtocolFactorySelector::SubProtocolFactory* subProtocolFactory = nullptr;

        if (std::filesystem::is_directory(websocketSubprotocolInstallLibdir + "/" + utils::Config::getApplicationName())) {
            subProtocolFactory = Super::load(subProtocolName,
                                             websocketSubprotocolInstallLibdir + "/" + utils::Config::getApplicationName() + "/" +
                                                 websocketSubprotocolLibraryFile,
                                             websocketSubprotocolFunctionName);
        } else {
            subProtocolFactory = Super::load(subProtocolName,
                                             websocketSubprotocolInstallLibdir + "/" + websocketSubprotocolLibraryFile,
                                             websocketSubprotocolFunctionName);
        }

        return subProtocolFactory;
    }

} // namespace web::websocket::client
