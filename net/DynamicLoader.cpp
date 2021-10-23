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
        VLOG(0) << "dlOpen: " << libFile;

        void* handle = net::system::dlopen(libFile.c_str(), flags);

        if (handle != nullptr) {
            dlOpenedLibraries[handle] = libFile;
        }

        return handle;
    }

    void DynamicLoader::dlCloseDelayed(void* handle) {
        if (handle != nullptr) {
            if (dlOpenedLibraries.contains(handle) && !registeredForDlClose.contains(handle)) {
                VLOG(0) << "dlCloseDelayed: " << dlOpenedLibraries[handle];

                registeredForDlClose.insert(handle);
            } else {
                VLOG(0) << "dlCloseDelayed: Handle" << handle << " opened using dlOpen.";
            }
        } else {
            VLOG(0) << "dlCloseDelayed: Handle is nullptr";
        }
    }

    int DynamicLoader::dlClose(void* handle) {
        int ret = 0;

        if (handle != nullptr) {
            if (dlOpenedLibraries.contains(handle) && !registeredForDlClose.contains(handle)) {
                VLOG(0) << "dlCloseSync: " << dlOpenedLibraries[handle];

                ret = execDlClose(handle);
            } else {
                VLOG(0) << "dlCloseSync: Handle" << handle << " opened using dlOpen.";
            }
        } else {
            VLOG(0) << "dlCloseSync: Handle is nullptr";
        }

        return ret;
    }
    char* DynamicLoader::dlError() {
        return net::system::dlerror();
    }

    int DynamicLoader::execDlClose(void* handle) {
        int ret = -1;

        if (handle != nullptr) {
            if (dlOpenedLibraries.contains(handle)) {
                VLOG(0) << "execDLClose: " << dlOpenedLibraries[handle];

                ret = net::system::dlclose(handle);

                dlOpenedLibraries.erase(handle);
            }
        }

        return ret;
    }

    void DynamicLoader::execDlCloseDeleyed() {
        for (void* handle : registeredForDlClose) {
            int ret = execDlClose(handle);

            if (ret != 0) {
                VLOG(0) << "Error execDeleyedDlClose: " << net::DynamicLoader::dlError();
            }
        }

        registeredForDlClose.clear();
    }

    void DynamicLoader::execDlCloseAll() {
        execDlCloseDeleyed();

        std::map<void*, std::string>::iterator it = dlOpenedLibraries.begin();

        while (it != dlOpenedLibraries.end()) {
            std::map<void*, std::string>::iterator tmpIt = it;
            ++it;
            int ret = execDlClose(tmpIt->first);

            if (ret != 0) {
                VLOG(0) << "Error execDlCloseAll: " << net::DynamicLoader::dlError();
            }
        }
    }

} // namespace net
