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

#include "SubProtocolSelector.h"

#include "log/Logger.h"
#include "web/websocket/SubProtocol.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <dlfcn.h>
#include <filesystem>
#include <sstream>     // for basic_stringbuf<>::int_type, basic_st...
#include <type_traits> // for add_const<>::type

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket {

    SubProtocolSelector::SubProtocolSelector(SubProtocolInterface::Role role)
        : role(role) {
    }

    void SubProtocolSelector::destroy(web::websocket::SubProtocol* subProtocol) {
        if (subProtocols.contains(subProtocol->getName())) {
            web::websocket::SubProtocolInterface* subProtocolInterface =
                static_cast<SubProtocolInterface*>(subProtocols.find(subProtocol->getName())->second.subProtocolInterface);

            subProtocolInterface->destroy(subProtocol);
        }
    }

    SubProtocolInterface* SubProtocolSelector::load(const std::string& filePath) {
        SubProtocolInterface* subProtocolInterface = nullptr;

        void* handle = dlopen(filePath.c_str(), RTLD_LAZY | RTLD_LOCAL);

        if (handle != nullptr) {
            VLOG(0) << "SubProtocol loaded successfully: " << filePath;

            SubProtocolInterface* (*plugin)() = reinterpret_cast<SubProtocolInterface* (*) ()>(dlsym(handle, "plugin"));

            if (plugin != nullptr) {
                subProtocolInterface = plugin();
                if (subProtocolInterface != nullptr) {
                    add(subProtocolInterface, handle);
                } else {
                    dlclose(handle);
                }
            } else {
                VLOG(0) << "Optaining function \"plugin()\" in plugin failed: " << dlerror();
            }
        } else {
            VLOG(0) << "DLOpen: error: " << dlerror() << " - " << filePath;
        }

        return subProtocolInterface;
    }

    void SubProtocolSelector::add(SubProtocolInterface* subProtocolInterface, void* handle) {
        SubProtocolPlugin subProtocolPlugin = {.subProtocolInterface = subProtocolInterface, .handle = handle};

        if (subProtocolInterface != nullptr) {
            if (subProtocolInterface->role() == role) {
                const auto [it, success] = subProtocols.insert({subProtocolInterface->name(), subProtocolPlugin});
                if (!success) {
                    VLOG(0) << "Subprotocol already existing: not using " << subProtocolInterface->name();
                    subProtocolInterface->destroy();
                    if (handle != nullptr) {
                        dlclose(handle);
                    }
                }
            } else if (handle != nullptr) {
                subProtocolInterface->destroy();
                dlclose(handle);
            }
        } else if (handle != nullptr) {
            dlclose(handle);
        }
    }

    void SubProtocolSelector::unload() {
        for (const auto& [name, subProtocolPlugin] : subProtocols) {
            subProtocolPlugin.subProtocolInterface->destroy();
            if (subProtocolPlugin.handle != nullptr) {
                dlclose(subProtocolPlugin.handle);
            }
        }
    }

    void SubProtocolSelector::addSubProtocolSearchPath(const std::string& searchPath) {
        searchPaths.push_back(searchPath);
    }

    SubProtocolInterface* SubProtocolSelector::selectSubProtocolInterface(const std::string& subProtocolName) {
        SubProtocolInterface* subProtocolInterface = nullptr;

        if (subProtocols.contains(subProtocolName)) {
            subProtocolInterface = subProtocols.find(subProtocolName)->second.subProtocolInterface;
        } else {
            for (const std::string& searchPath : searchPaths) {
                subProtocolInterface = dynamic_cast<SubProtocolInterface*>(load(searchPath + "/lib" + subProtocolName + ".so"));
                if (subProtocolInterface != nullptr) {
                    break;
                }
            }
        }

        return subProtocolInterface;
    }

} // namespace web::websocket
