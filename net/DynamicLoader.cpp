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

#include "DynamicLoader.h"

#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <utility> // for pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    std::map<void*, std::string> DynamicLoader::dlOpenedLibraries;
    std::set<void*> DynamicLoader::registeredForDlClose;

    void* DynamicLoader::dlOpen(const std::string& libFile, int flags) {
        void* handle = net::system::dlopen(libFile.c_str(), flags);

        VLOG(0) << "dlOpen: " << libFile.c_str();

        if (handle != nullptr) {
            dlOpenedLibraries[handle] = libFile;
        }

        return handle;
    }

    void DynamicLoader::dlClose(void* handle, bool sync) {
        VLOG(0) << "dlClose: " << dlOpenedLibraries[handle];
        if (dlOpenedLibraries.contains(handle) && !registeredForDlClose.contains(handle)) {
            if (sync) {
                execDlClose(handle);
            } else {
                registeredForDlClose.insert(handle);
            }
        }
    }

    char* DynamicLoader::dlError() {
        return net::system::dlerror();
    }

    void DynamicLoader::execDlClose(void* handle) {
        VLOG(0) << "execDLClose: " << dlOpenedLibraries[handle];

        if (net::system::dlclose(handle) != 0) {
            VLOG(0) << "Error during dlclose: " << net::system::dlerror();
        }

        dlOpenedLibraries.erase(handle);
    }

    void DynamicLoader::execDeleyedDlClose() {
        for (void* handle : registeredForDlClose) {
            execDlClose(handle);
        }

        registeredForDlClose.clear();
    }

    void DynamicLoader::execDlCloseAll() {
        execDeleyedDlClose();

        std::map<void*, std::string>::iterator it = dlOpenedLibraries.begin();

        while (it != dlOpenedLibraries.end()) {
            std::map<void*, std::string>::iterator tmpIt = it;
            ++it;
            execDlClose(tmpIt->first);
        }
    }

} // namespace net
