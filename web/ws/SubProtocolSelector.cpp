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
#include "web/config.h" // IWYU pragma: keep
#include "web/ws/SubProtocol.h"
#include "web/ws/SubProtocolInterface.h" // for WSSubPr...

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <dlfcn.h>
#include <filesystem>
#include <sstream>     // for basic_stringbuf<>::int_type, basic_st...
#include <type_traits> // for add_const<>::type

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws {

    SubProtocolSelector::SubProtocolSelector() {
        loadSubProtocols();
    }

    SubProtocolSelector::~SubProtocolSelector() {
        for (const auto& [name, subProtocolPlugin] : serverSubprotocols) {
            delete subProtocolPlugin.subprotocolPluginInterface;
            if (subProtocolPlugin.handle != nullptr) {
                dlclose(subProtocolPlugin.handle);
            }
        }

        for (const auto& [name, subProtocolPlugin] : clientSubprotocols) {
            delete subProtocolPlugin.subprotocolPluginInterface;
            if (subProtocolPlugin.handle != nullptr) {
                dlclose(subProtocolPlugin.handle);
            }
        }
    }

    void SubProtocolSelector::loadSubProtocols() {
#ifndef NDEBUG
#ifdef SUBPROTOCOL_PATH
        loadSubProtocols(SUBPROTOCOL_PATH);
#endif
#endif
        loadSubProtocols("/usr/lib/snodec/web/ws/subprotocol");
        loadSubProtocols("/usr/local/lib/snodec/web/ws/subprotocol");
    }

    void SubProtocolSelector::registerSubProtocol(SubProtocolInterface* subProtocolInterface, void* handle) {
        SubProtocolPlugin subProtocolPlugin = {.subprotocolPluginInterface = subProtocolInterface, .handle = handle};

        if (subProtocolInterface != nullptr) {
            if (subProtocolInterface->role() == web::ws::SubProtocol::Role::SERVER) {
                const auto [it, success] = serverSubprotocols.insert({subProtocolInterface->name(), subProtocolPlugin});
                if (!success) {
                    delete subProtocolInterface;
                    if (handle != nullptr) {
                        dlclose(handle);
                    }
                }
            } else {
                const auto [it, success] = clientSubprotocols.insert({subProtocolInterface->name(), subProtocolPlugin});
                if (!success) {
                    delete subProtocolInterface;
                    if (handle != nullptr) {
                        dlclose(handle);
                    }
                }
            }
        } else if (handle != nullptr) {
            dlclose(handle);
        }
    }

    void SubProtocolSelector::loadSubProtocol(const std::string& filePath) {
        void* handle = dlopen(filePath.c_str(), RTLD_LAZY | RTLD_GLOBAL);

        if (handle != nullptr) {
            SubProtocolInterface* (*plugin)() = reinterpret_cast<SubProtocolInterface* (*) ()>(dlsym(handle, "plugin"));

            if (plugin != nullptr) {
                SubProtocolInterface* subProtocolInterface = plugin();
                if (subProtocolInterface != nullptr) {
                    registerSubProtocol(subProtocolInterface, handle);
                } else {
                    dlclose(handle);
                }
            } else {
                VLOG(0) << "Optaining function \"plugin()\" in plugin failed: " << dlerror();
            }

            VLOG(0) << "DLOpen: success: " << filePath;
        } else {
            VLOG(0) << "DLOpen: error: " << dlerror() << " - " << filePath;
        }
    }

    void SubProtocolSelector::loadSubProtocols(const std::string& directoryPath) {
        if (std::filesystem::exists(directoryPath) && std::filesystem::is_directory(directoryPath)) {
            for (const std::filesystem::directory_entry& directoryEntry : std::filesystem::recursive_directory_iterator(directoryPath)) {
                if (std::filesystem::is_regular_file(directoryEntry) && directoryEntry.path().extension() == ".so") {
                    loadSubProtocol(directoryEntry.path());
                } else {
                    VLOG(1) << "Not a library: Ignoring " << directoryEntry;
                }
            }
        } else {
            VLOG(1) << "Not a directory: Ignoring path: " << directoryPath;
        }
    }

} // namespace web::ws
