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

#include "SubProtocolFactorySelector.h"

#include "log/Logger.h"
#include "web/websocket/SubProtocol.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <dlfcn.h>
#include <type_traits> // for add_const<>::type
#include <utility>     // for pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket {

    SubProtocolFactorySelector::SubProtocolFactorySelector(SubProtocolFactory::Role role)
        : role(role) {
    }

    void SubProtocolFactorySelector::destroy(SubProtocol* subProtocol) {
        if (subProtocolPlugins.contains(subProtocol->getName())) {
            SubProtocolFactory* subProtocolFactory = subProtocolPlugins.find(subProtocol->getName())->second.subProtocolFactory;

            subProtocolFactory->destroy(subProtocol);
        }
    }

    SubProtocolFactory* SubProtocolFactorySelector::select(const std::string& subProtocolName) {
        SubProtocolFactory* subProtocolFactory = nullptr;

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

    SubProtocolFactory* SubProtocolFactorySelector::load(const std::string& filePath) {
        SubProtocolFactory* subProtocolFactory = nullptr;

        void* handle = dlopen(filePath.c_str(), RTLD_LAZY | RTLD_LOCAL);

        if (handle != nullptr) {
            VLOG(0) << "SubProtocol loaded successfully: " << filePath;

            SubProtocolFactory* (*plugin)() = reinterpret_cast<SubProtocolFactory* (*) ()>(dlsym(handle, "plugin"));

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

    void SubProtocolFactorySelector::unload() {
        for ([[maybe_unused]] const auto& [name, subProtocolPlugin] : subProtocolPlugins) {
            subProtocolPlugin.subProtocolFactory->destroy();
            if (subProtocolPlugin.handle != nullptr) {
                dlclose(subProtocolPlugin.handle);
            }
        }
    }

    void SubProtocolFactorySelector::add(SubProtocolFactory* subProtocolFactory, void* handle) {
        SubProtocolPlugin subProtocolPlugin = {.subProtocolFactory = subProtocolFactory, .handle = handle};

        if (subProtocolFactory != nullptr) {
            if (subProtocolFactory->role() == role) {
                const auto [it, success] = subProtocolPlugins.insert({subProtocolFactory->name(), subProtocolPlugin});
                if (!success) {
                    VLOG(0) << "Subprotocol already existing: not using " << subProtocolFactory->name();
                    subProtocolFactory->destroy();
                    if (handle != nullptr) {
                        dlclose(handle);
                    }
                }
            } else {
                subProtocolFactory->destroy();
                if (handle != nullptr) {
                    dlclose(handle);
                }
            }
        } else if (handle != nullptr) {
            dlclose(handle);
        }
    }

    void SubProtocolFactorySelector::addSubProtocolSearchPath(const std::string& searchPath) {
        searchPaths.push_back(searchPath);
    }

} // namespace web::websocket
