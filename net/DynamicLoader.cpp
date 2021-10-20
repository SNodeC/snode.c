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

#include <dlfcn.h>
#include <utility> // for pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

//#define USE_SYNC

namespace net {

    std::map<void*, std::string> DynamicLoader::dlOpenedLibraries;
    std::set<void*> DynamicLoader::registeredForDlClose;

    void* DynamicLoader::dlOpen(const std::string& libFile, int flags) {
        void* handle = dlopen(libFile.c_str(), flags);

        VLOG(0) << "dlOpen: " << handle << " : " << libFile.c_str();

#ifndef USE_SYNC
        if (handle != nullptr) {
            dlOpenedLibraries[handle] = libFile;
        }
#endif

        return handle;
    }

    void DynamicLoader::dlClose(void* handle) {
#ifdef USE_SYNC
        VLOG(0) << "dlClose: " << handle;
        dlclose(handle);
#else
        VLOG(0) << "dlClose: " << handle << " : " << dlOpenedLibraries[handle];
        if (dlOpenedLibraries.contains(handle) && !registeredForDlClose.contains(handle)) {
            registeredForDlClose.insert(handle);
        }
#endif
    }

    void DynamicLoader::execDlClose([[maybe_unused]] void* handle) {
#ifndef USE_SYNC
        VLOG(0) << "execDLClose: " << handle << " : " << dlOpenedLibraries[handle];

        if (dlclose(handle) != 0) {
            VLOG(0) << "Error during dlclose: " << dlerror();
        }

        dlOpenedLibraries.erase(handle);
#endif
    }

    void DynamicLoader::execDeleyedDlClose() {
#ifndef USE_SYNC
        for (void* handle : registeredForDlClose) {
            execDlClose(handle);
        }

        registeredForDlClose.clear();
#endif
    }

    void DynamicLoader::execDlCloseAll() {
#ifndef USE_SYNC

        execDeleyedDlClose();

        std::map<void*, std::string>::iterator it = dlOpenedLibraries.begin();

        while (it != dlOpenedLibraries.end()) {
            std::map<void*, std::string>::iterator tmpIt = it;
            ++it;
            execDlClose(tmpIt->first);
        }
#endif
    }

} // namespace net
