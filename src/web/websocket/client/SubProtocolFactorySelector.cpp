/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

#include "web/websocket/client/SubProtocolFactorySelector.h"

#include "utils/Config.h"
#include "web/websocket/SubProtocolFactory.h"
#include "web/websocket/SubProtocolFactorySelector.hpp"

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
        instance()->Super::allowDlOpen();
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

template class web::websocket::SubProtocolFactorySelector<web::websocket::SubProtocolFactory<web::websocket::client::SubProtocol>>;
