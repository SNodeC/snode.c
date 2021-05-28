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

#include "web/ws/subprotocol/Chooser.h"

#include "log/Logger.h"
#include "web/config.h"
#include "web/ws/WSProtocol.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <dlfcn.h>
#include <filesystem>
#include <sstream> // for basic_stringbuf<>::int_type, basic_st...

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws::subprotocol {

    Chooser::Chooser() {
        loadSubProtocols();
    }

    void Chooser::loadSubProtocols() {
        loadSubProtocolsIn(SUBPROTOCOL_PATH);
        loadSubProtocolsIn("/usr/lib/snodec/web/ws/subprotocol");
        loadSubProtocolsIn("/usr/local/lib/snodec/web/ws/subprotocol");
    }

    void Chooser::loadSubProtocolsIn(const std::string& path) {
        if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
            for (const std::filesystem::directory_entry& directoryEntry : std::filesystem::recursive_directory_iterator(path)) {
                if (std::filesystem::is_regular_file(directoryEntry) && directoryEntry.path().extension() == ".so") {
                    void* handle = dlopen(directoryEntry.path().c_str(), RTLD_NOW | RTLD_LOCAL);
                    if (handle != nullptr) {
                        SubProtocol subProtocol;
                        subProtocol.handle = handle;
                        subProtocol.name = reinterpret_cast<const char* (*) ()>(dlsym(handle, "name"));
                        subProtocol.create = reinterpret_cast<web::ws::WSProtocol* (*) ()>(dlsym(handle, "create"));
                        subProtocol.destroy = reinterpret_cast<void (*)(web::ws::WSProtocol * proto)>(dlsym(handle, "destroy"));
                        subProtocol.role = reinterpret_cast<web::ws::WSProtocol::Role (*)()>(dlsym(handle, "role"));

                        if (subProtocol.role() == web::ws::WSProtocol::Role::SERVER) {
                            serverSubprotocols.insert({subProtocol.name(), subProtocol});
                        } else {
                            clientSubprotocols.insert({subProtocol.name(), subProtocol});
                        }

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

    Chooser::~Chooser() {
        for (auto& [name, supprotocol] : serverSubprotocols) {
            dlclose(supprotocol.handle);
        }

        for (auto& [name, supprotocol] : clientSubprotocols) {
            dlclose(supprotocol.handle);
        }
    }

    WSProtocol* Chooser::select(const std::string& subProtocol, web::ws::WSProtocol::Role role) {
        WSProtocol* wSProtocol = nullptr;

        if (role == web::ws::WSProtocol::Role::SERVER) { // server
            if (serverSubprotocols.contains(subProtocol)) {
                wSProtocol = serverSubprotocols[subProtocol].create();
            }
        } else if (role == web::ws::WSProtocol::Role::CLIENT) { // client
            if (clientSubprotocols.contains(subProtocol)) {
                wSProtocol = clientSubprotocols[subProtocol].create();
            }
        }

        return wSProtocol;
    }

} // namespace web::ws::subprotocol
