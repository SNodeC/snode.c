/*+
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

#ifndef WEB_WS_SUBPROTOCOLSELECTOR_HPP
#define WEB_WS_SUBPROTOCOLSELECTOR_HPP

#include "SubProtocolFactorySelector.h" // IWYU pragma: export
#include "log/Logger.h"
#include "web/websocket/SubProtocolFactory.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <dlfcn.h>
#include <type_traits> // for add_const<>::type

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket {

    template <typename SubProtocolT>
    SubProtocolFactory<SubProtocolT>* SubProtocolFactorySelector<SubProtocolT>::select(const std::string& subProtocolName) {
        SubProtocolFactory<SubProtocolT>* subProtocolFactory = nullptr;

        if (subProtocolPlugins.contains(subProtocolName)) {
            subProtocolFactory = subProtocolPlugins[subProtocolName].subProtocolFactory;
        } else {
            for (const std::string& searchPath : searchPaths) {
                subProtocolFactory = load(searchPath + "/lib" + subProtocolName + ".so");
                if (subProtocolFactory != nullptr) {
                    break;
                }
            }
        }

        return subProtocolFactory;
    }

    template <typename SubProtocolT>
    SubProtocolFactory<SubProtocolT>* SubProtocolFactorySelector<SubProtocolT>::load(const std::string& filePath) {
        SubProtocolFactory<SubProtocolT>* subProtocolFactory = nullptr;

        void* handle = dlopen(filePath.c_str(), RTLD_LAZY | RTLD_LOCAL);

        if (handle != nullptr) {
            VLOG(0) << "SubProtocol loaded successfully: " << filePath;

            SubProtocolFactory<SubProtocolT>* (*plugin)() =
                reinterpret_cast<SubProtocolFactory<SubProtocolT>* (*) ()>(dlsym(handle, "plugin"));

            if (plugin != nullptr) {
                subProtocolFactory = plugin();
                if (subProtocolFactory != nullptr) {
                    add(subProtocolFactory, handle);
                } else {
                    dlclose(handle);
                }
            } else {
                VLOG(0) << "Optaining function \"plugin()\" in plugin failed: " << dlerror();
            }
        } else {
            VLOG(0) << "Error dlopen: " << dlerror();
        }

        return subProtocolFactory;
    }

    template <typename SubProtocolT>
    void SubProtocolFactorySelector<SubProtocolT>::unload() {
        for ([[maybe_unused]] const auto& [name, subProtocolPlugin] : subProtocolPlugins) {
            subProtocolPlugin.subProtocolFactory->destroy();
            if (subProtocolPlugin.handle != nullptr) {
                dlclose(subProtocolPlugin.handle);
            }
        }
    }

    template <typename SubProtocolT>
    void SubProtocolFactorySelector<SubProtocolT>::add(SubProtocolFactory<SubProtocolT>* subProtocolFactory, void* handle) {
        SubProtocolPlugin<SubProtocolT> subProtocolPlugin = {.subProtocolFactory = subProtocolFactory, .handle = handle};

        if (subProtocolFactory != nullptr) {
            const auto [it, success] = subProtocolPlugins.insert({subProtocolFactory->name(), subProtocolPlugin});
            if (!success) {
                VLOG(0) << "Subprotocol already existing: not using " << subProtocolFactory->name();
                subProtocolFactory->destroy();
                if (handle != nullptr) {
                    dlclose(handle);
                }
            }
        } else if (handle != nullptr) {
            dlclose(handle);
        }
    }

    template <typename SubProtocolT>
    void SubProtocolFactorySelector<SubProtocolT>::addSubProtocolSearchPath(const std::string& searchPath) {
        searchPaths.push_back(searchPath);
    }

} // namespace web::websocket

#endif // WEB_WS_SUBPROTOCOLSELECTOR_HPP
