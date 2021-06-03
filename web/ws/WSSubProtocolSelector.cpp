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

#include "WSSubProtocolSelector.h"

#include "WSSubProtocolPluginInterface.h" // for WSSubPr...
#include "log/Logger.h"
#include "web/config.h"
#include "web/ws/WSSubProtocol.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <dlfcn.h>
#include <filesystem>
#include <sstream> // for basic_stringbuf<>::int_type, basic_st...

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws {

    WSSubProtocolSelector::WSSubProtocolSelector() {
        loadSubProtocols();
    }

    WSSubProtocolSelector::~WSSubProtocolSelector() {
        for (auto& [name, subProtocol] : serverSubprotocols) {
            if (subProtocol.handle != nullptr) {
                dlclose(subProtocol.handle);
            }
            delete subProtocol.wSSubprotocolPluginInterface;
        }

        for (auto& [name, subProtocol] : clientSubprotocols) {
            if (subProtocol.handle != nullptr) {
                dlclose(subProtocol.handle);
            }
            delete subProtocol.wSSubprotocolPluginInterface;
        }
    }

    void WSSubProtocolSelector::loadSubProtocols() {
        loadSubProtocols(SUBPROTOCOL_PATH);
        loadSubProtocols("/usr/lib/snodec/web/ws/subprotocol");
        loadSubProtocols("/usr/local/lib/snodec/web/ws/subprotocol");
    }

    void WSSubProtocolSelector::registerSubProtocol(WSSubProtocolPluginInterface* wSSubProtocolPluginInterface) {
        registerSubProtocol(wSSubProtocolPluginInterface, nullptr);
    }

    void WSSubProtocolSelector::registerSubProtocol(WSSubProtocolPluginInterface* wSSubProtocolPluginInterface, void* handle) {
        WSSubProtocolPlugin wSSubProtocolPlugin = {.wSSubprotocolPluginInterface = wSSubProtocolPluginInterface, .handle = handle};

        if (wSSubProtocolPluginInterface->role() == web::ws::WSSubProtocol::Role::SERVER) {
            const auto [it, success] = serverSubprotocols.insert({wSSubProtocolPluginInterface->name(), wSSubProtocolPlugin});
            if (!success) {
                if (handle != nullptr) {
                    dlclose(handle);
                }
                delete wSSubProtocolPluginInterface;
            }
        } else {
            const auto [it, success] = clientSubprotocols.insert({wSSubProtocolPluginInterface->name(), wSSubProtocolPlugin});
            if (!success) {
                if (handle != nullptr) {
                    dlclose(handle);
                }
                delete wSSubProtocolPluginInterface;
            }
        }
    }

    void WSSubProtocolSelector::loadSubProtocols(const std::string& path) {
        if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
            for (const std::filesystem::directory_entry& directoryEntry : std::filesystem::recursive_directory_iterator(path)) {
                if (std::filesystem::is_regular_file(directoryEntry) && directoryEntry.path().extension() == ".so") {
                    void* handle = dlopen(directoryEntry.path().c_str(), RTLD_NOW | RTLD_LOCAL);
                    if (handle != nullptr) {
                        WSSubProtocolPluginInterface* (*wSSubProtocolPlugin)() =
                            reinterpret_cast<WSSubProtocolPluginInterface* (*) ()>(dlsym(handle, "plugin"));

                        registerSubProtocol(wSSubProtocolPlugin(), handle);

                        VLOG(1) << "DLOpen: success: " << directoryEntry.path().c_str();
                    } else {
                        VLOG(1) << "DLOpen: error: " << dlerror() << " - " << directoryEntry.path().c_str();
                    }
                } else {
                    VLOG(1) << "Not a library: Ignoring direntry " << directoryEntry;
                }
            }
        } else {
            VLOG(1) << "Not a directory: Ignoring path: " << path;
        }
    }

} // namespace web::ws
