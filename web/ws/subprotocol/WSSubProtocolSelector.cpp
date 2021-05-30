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

#include "web/ws/subprotocol/WSSubProtocolSelector.h"

#include "log/Logger.h"
#include "web/config.h"
#include "web/ws/WSSubProtocol.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <dlfcn.h>
#include <filesystem>
#include <sstream> // for basic_stringbuf<>::int_type, basic_st...

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws::subprotocol {

    WSSubProtocolSelector::WSSubProtocolSelector() {
        loadSubProtocols();
    }

    void WSSubProtocolSelector::loadSubProtocols() {
        loadSubProtocols(SUBPROTOCOL_PATH);
        loadSubProtocols("/usr/lib/snodec/web/ws/subprotocol");
        loadSubProtocols("/usr/local/lib/snodec/web/ws/subprotocol");
    }

    void WSSubProtocolSelector::loadSubProtocol(const WSSubProtocolPluginInterface& subProtocol) {
        if (subProtocol.role() == web::ws::WSSubProtocol::Role::SERVER) {
            const auto [it, success] = serverSubprotocols.insert({subProtocol.name(), subProtocol});
            if (!success && subProtocol.handle) {
                dlclose(subProtocol.handle);
            }
        } else {
            const auto [it, success] = clientSubprotocols.insert({subProtocol.name(), subProtocol});
            if (!success && subProtocol.handle) {
                dlclose(subProtocol.handle);
            }
        }
    }

    void WSSubProtocolSelector::loadSubProtocols(const std::string& path) {
        if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
            for (const std::filesystem::directory_entry& directoryEntry : std::filesystem::recursive_directory_iterator(path)) {
                if (std::filesystem::is_regular_file(directoryEntry) && directoryEntry.path().extension() == ".so") {
                    void* handle = dlopen(directoryEntry.path().c_str(), RTLD_NOW | RTLD_LOCAL);
                    if (handle != nullptr) {
                        WSSubProtocolPluginInterface (*wSProtocolPlugin)(void*) =
                            reinterpret_cast<WSSubProtocolPluginInterface (*)(void*)>(dlsym(handle, "plugin"));

                        loadSubProtocol(wSProtocolPlugin(handle));

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

    WSSubProtocolSelector::~WSSubProtocolSelector() {
        for (auto& [name, subProtocol] : serverSubprotocols) {
            if (subProtocol.handle != nullptr) {
                dlclose(subProtocol.handle);
            }
        }

        for (auto& [name, subProtocol] : clientSubprotocols) {
            if (subProtocol.handle != nullptr) {
                dlclose(subProtocol.handle);
            }
        }
    }

    WSSubProtocolPluginInterface* WSSubProtocolSelector::select(const std::string& subProtocolName, web::ws::WSSubProtocol::Role role) {
        WSSubProtocolPluginInterface* subProtocol = nullptr;

        if (role == web::ws::WSSubProtocol::Role::SERVER) { // server
            if (serverSubprotocols.contains(subProtocolName)) {
                subProtocol = &serverSubprotocols[subProtocolName];
            }
        } else if (role == web::ws::WSSubProtocol::Role::CLIENT) { // client
            if (clientSubprotocols.contains(subProtocolName)) {
                subProtocol = &clientSubprotocols[subProtocolName];
            }
        }

        return subProtocol;
    }

} // namespace web::ws::subprotocol
