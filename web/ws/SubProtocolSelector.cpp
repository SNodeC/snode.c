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

#include "SubProtocolInterface.h" // for WSSubPr...
#include "log/Logger.h"
#include "web/config.h"
#include "web/ws/SubProtocol.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <dlfcn.h>
#include <filesystem>
#include <sstream> // for basic_stringbuf<>::int_type, basic_st...

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws {

    SubProtocolSelector::SubProtocolSelector() {
        loadSubProtocols();
    }

    SubProtocolSelector::~SubProtocolSelector() {
        for (auto& [name, subProtocol] : serverSubprotocols) {
            if (subProtocol.handle != nullptr) {
                dlclose(subProtocol.handle);
            }
            delete subProtocol.subprotocolPluginInterface;
        }

        for (auto& [name, subProtocol] : clientSubprotocols) {
            if (subProtocol.handle != nullptr) {
                dlclose(subProtocol.handle);
            }
            delete subProtocol.subprotocolPluginInterface;
        }
    }

    void SubProtocolSelector::loadSubProtocols() {
#ifndef NDEBUG
        loadSubProtocols(SUBPROTOCOL_PATH);
#endif
        loadSubProtocols("/usr/lib/snodec/web/ws/subprotocol");
        loadSubProtocols("/usr/local/lib/snodec/web/ws/subprotocol");
    }

    void SubProtocolSelector::registerSubProtocol(SubProtocolInterface* subProtocolPluginInterface) {
        registerSubProtocol(subProtocolPluginInterface, nullptr);
    }

    void SubProtocolSelector::registerSubProtocol(SubProtocolInterface* subProtocolPluginInterface, void* handle) {
        SubProtocolPlugin subProtocolPlugin = {.subprotocolPluginInterface = subProtocolPluginInterface, .handle = handle};

        if (subProtocolPluginInterface->role() == web::ws::SubProtocol::Role::SERVER) {
            const auto [it, success] = serverSubprotocols.insert({subProtocolPluginInterface->name(), subProtocolPlugin});
            if (!success) {
                delete subProtocolPluginInterface;
                if (handle != nullptr) {
                    dlclose(handle);
                }
            }
        } else {
            const auto [it, success] = clientSubprotocols.insert({subProtocolPluginInterface->name(), subProtocolPlugin});
            if (!success) {
                delete subProtocolPluginInterface;
                if (handle != nullptr) {
                    dlclose(handle);
                }
            }
        }
    }

    void SubProtocolSelector::loadSubProtocol(const std::string& filePath) {
        void* handle = dlopen(filePath.c_str(), RTLD_NOW | RTLD_LOCAL);
        if (handle != nullptr) {
            SubProtocolInterface* (*subProtocolPluginInterface)() = reinterpret_cast<SubProtocolInterface* (*) ()>(dlsym(handle, "plugin"));

            if (subProtocolPluginInterface != nullptr) {
                registerSubProtocol(subProtocolPluginInterface(), handle);
            } else {
                VLOG(0) << "Optaining function \"plugin()\" in plugin failed: " << dlerror();
            }

            VLOG(1) << "DLOpen: success: " << filePath;
        } else {
            VLOG(1) << "DLOpen: error: " << dlerror() << " - " << filePath;
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
