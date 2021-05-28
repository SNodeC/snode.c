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

#include "web/config.h"
#include "web/ws/WSProtocol.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <dlfcn.h>
#include <filesystem>
#include <iostream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws::subprotocol {

    Chooser::Chooser() {
        loadSubprotocols();
    }

    void Chooser::loadSubprotocols() {
        for (const std::filesystem::directory_entry& directoryEntry : std::filesystem::recursive_directory_iterator(SUBPROTOCOL_PATH)) {
            if (std::filesystem::is_regular_file(directoryEntry)) {
                {
                    if (directoryEntry.path().extension() == ".so") {
                        void* handle = dlopen(directoryEntry.path().c_str(), RTLD_NOW | RTLD_GLOBAL);
                        if (handle != nullptr) {
                            SubprotocolInterface subprotocolInterface;
                            subprotocolInterface.handle = handle;
                            subprotocolInterface.name = reinterpret_cast<const char* (*) ()>(dlsym(handle, "name"));
                            subprotocolInterface.create = reinterpret_cast<web::ws::WSProtocol* (*) ()>(dlsym(handle, "create"));
                            subprotocolInterface.destroy =
                                reinterpret_cast<void (*)(web::ws::WSProtocol * proto)>(dlsym(handle, "destroy"));
                            subprotocolInterface.role = reinterpret_cast<web::ws::WSTransmitter::Role (*)()>(dlsym(handle, "role"));

                            if (subprotocolInterface.role() == web::ws::WSTransmitter::Role::SERVER) {
                                serverSubprotocols.insert({subprotocolInterface.name(), subprotocolInterface});
                            } else {
                                clientSubprotocols.insert({subprotocolInterface.name(), subprotocolInterface});
                            }
                            std::cout << "DLOpen: success: " << directoryEntry.path().c_str() << std::endl;
                        } else {
                            std::cout << "DLOpen: error: " << dlerror() << " - " << directoryEntry.path().c_str() << std::endl;
                        }
                    }
                }
            }
        }
    }

    Chooser::~Chooser() {
        for (std::pair<const std::string&, SubprotocolInterface> pair : serverSubprotocols) {
            dlclose(pair.second.handle);
        }

        for (std::pair<const std::string&, SubprotocolInterface> pair : clientSubprotocols) {
            dlclose(pair.second.handle);
        }
    }

    WSProtocol* Chooser::select(const std::string& subProtocol, web::ws::WSTransmitter::Role role) {
        WSProtocol* wSProtocol = nullptr;

        if (role == web::ws::WSTransmitter::Role::SERVER) { // server
            if (serverSubprotocols.contains(subProtocol)) {
                wSProtocol = serverSubprotocols[subProtocol].create();
            }
        } else { // client
            if (clientSubprotocols.contains(subProtocol)) {
                wSProtocol = clientSubprotocols[subProtocol].create();
            }
        }
        return wSProtocol;
    }

} // namespace web::ws::subprotocol
