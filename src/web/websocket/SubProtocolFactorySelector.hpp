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

#include "web/websocket/SubProtocolFactorySelector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <filesystem>
#include <list>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket {

    template <typename SubProtocolFactory>
    SubProtocolFactory* SubProtocolFactorySelector<SubProtocolFactory>::load(const std::string& subProtocolName,
                                                                             const std::string& subProtocolLibraryFile,
                                                                             const std::string& subProtocolFactoryFunctionName) {
        SubProtocolFactory* subProtocolFactory = nullptr;

        void* handle = core::DynamicLoader::dlOpen(subProtocolLibraryFile);

        if (handle != nullptr) {
            SubProtocolFactory* (*getSubProtocolFactory)() =
                reinterpret_cast<SubProtocolFactory* (*) ()>(core::DynamicLoader::dlSym(handle, subProtocolFactoryFunctionName));
            if (getSubProtocolFactory != nullptr) {
                subProtocolFactory = getSubProtocolFactory();
                if (subProtocolFactory != nullptr) {
                    subProtocolFactory->setHandle(handle);
                    LOG(DEBUG) << "WebSocket: SubProtocolFactory created successful: " << subProtocolName;
                } else {
                    LOG(DEBUG) << "WebSocket: SubProtocolFactory not created: " << subProtocolName;
                    core::DynamicLoader::dlClose(handle);
                }
            } else {
                LOG(DEBUG) << "WebSocket: Optaining function \"" << subProtocolFactoryFunctionName
                           << "\" in plugin failed: " << core::DynamicLoader::dlError();
                core::DynamicLoader::dlClose(handle);
            }
        }

        return subProtocolFactory;
    }

    template <typename SubProtocolFactory>
    SubProtocolFactory* SubProtocolFactorySelector<SubProtocolFactory>::select(const std::string& subProtocolName) {
        SubProtocolFactory* subProtocolFactory = nullptr;

        if (subProtocolFactories.contains(subProtocolName)) {
            subProtocolFactory = subProtocolFactories[subProtocolName];
        }

        return subProtocolFactory;
    }

    template <typename SubProtocolFactory>
    SubProtocolFactory* SubProtocolFactorySelector<SubProtocolFactory>::select(const std::string& subProtocolName,
                                                                               [[maybe_unused]] Role role) {
        SubProtocolFactory* subProtocolFactory = select(subProtocolName);

        if (subProtocolFactory == nullptr) {
            if (linkedSubProtocolFactories.contains(subProtocolName)) {
                SubProtocolFactory* (*plugin)() = linkedSubProtocolFactories[subProtocolName];
                subProtocolFactory = plugin();
            } else if (!onlyLinked) {
                subProtocolFactory = load(subProtocolName);
            }

            if (subProtocolFactory != nullptr) {
                subProtocolFactories.insert({subProtocolName, subProtocolFactory});
            }
        }

        return subProtocolFactory;
    }

    template <typename SubProtocolFactory>
    void SubProtocolFactorySelector<SubProtocolFactory>::allowDlOpen() {
        onlyLinked = false;
    }

} // namespace web::websocket
