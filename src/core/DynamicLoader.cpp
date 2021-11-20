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

namespace core {

    std::map<void*, DynamicLoader::Library> DynamicLoader::dlOpenedLibraries;
    std::map<void*, std::size_t> DynamicLoader::registeredForDlClose;

    void* DynamicLoader::dlOpen(const std::string& libFile, int flags) {
        VLOG(0) << "dlOpen: " << libFile;

        void* handle = core::system::dlopen(libFile.c_str(), flags);

        if (handle != nullptr) {
            if (!dlOpenedLibraries.contains(handle)) {
                dlOpenedLibraries[handle].fileName = libFile;
            }
            dlOpenedLibraries[handle].refCount++;
        }

        return handle;
    }

    void DynamicLoader::dlCloseDelayed(void* handle) {
        if (handle != nullptr) {
            if (dlOpenedLibraries.contains(handle)) {
                VLOG(0) << "dlCloseDelayed: " << dlOpenedLibraries[handle].fileName;

                registeredForDlClose[handle]++;
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
                VLOG(0) << "dlClose: " << dlOpenedLibraries[handle].fileName;

                ret = execDlClose(handle);
            } else {
                VLOG(0) << "dlClose: Handle" << handle << " either not opened with dlOpen or already registered for dlCloseDelayedopened.";
            }
        } else {
            VLOG(0) << "dlClose: Handle is nullptr";
        }

        return ret;
    }

    char* DynamicLoader::dlError() {
        return core::system::dlerror();
    }

    int DynamicLoader::execDlClose(void* handle) {
        int ret = -1;

        if (handle != nullptr) {
            if (dlOpenedLibraries.contains(handle)) {
                VLOG(0) << "execDLClose: " << dlOpenedLibraries[handle].fileName;

                ret = core::system::dlclose(handle);
                dlOpenedLibraries[handle].refCount--;

                if (dlOpenedLibraries[handle].refCount == 0) {
                    dlOpenedLibraries.erase(handle);
                }
            }
        }

        return ret;
    }

    void DynamicLoader::execDlCloseDeleyed() {
        for (auto& [handle, refCount] : registeredForDlClose) {
            do {
                int ret = execDlClose(handle);

                if (ret != 0) {
                    VLOG(0) << "Error execDeleyedDlClose: " << core::DynamicLoader::dlError();
                }
            } while (--refCount > 0);
        }

        registeredForDlClose.clear();
    }

    void DynamicLoader::execDlCloseAll() {
        execDlCloseDeleyed();

        std::map<void*, Library>::iterator it = dlOpenedLibraries.begin();

        while (it != dlOpenedLibraries.end()) {
            std::map<void*, Library>::iterator tmpIt = it;
            ++it;

            do {
                int ret = execDlClose(tmpIt->first);

                if (ret != 0) {
                    VLOG(0) << "Error execDlCloseAll: " << core::DynamicLoader::dlError();
                }
            } while (it->second.refCount > 0);
        }
    }

} // namespace core
