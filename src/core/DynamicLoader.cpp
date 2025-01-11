/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include "core/DynamicLoader.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <algorithm>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    std::map<void*, DynamicLoader::Library> DynamicLoader::dlOpenedLibraries;
    std::list<void*> DynamicLoader::closeHandles;

    void* DynamicLoader::dlRegisterHandle(void* handle, const std::string& libFile) {
        if (handle != nullptr) {
            if (!dlOpenedLibraries.contains(handle)) {
                dlOpenedLibraries[handle].fileName = libFile;
                dlOpenedLibraries[handle].handle = handle;
            }
            LOG(TRACE) << "DynLoader: dlOpen: " << libFile << ": success";
        } else {
            LOG(TRACE) << "DynLoader: dlOpen: " << DynamicLoader::dlError();
        }

        return handle;
    }

    void DynamicLoader::dlCloseDelayed(void* handle) {
        if (handle != nullptr) {
            if (dlOpenedLibraries.contains(handle)) {
                if (std::find(closeHandles.begin(), closeHandles.end(), handle) == closeHandles.end()) {
                    LOG(TRACE) << "DynLoader: dlCloseDelayed: " << dlOpenedLibraries[handle].fileName;

                    closeHandles.push_back(handle);
                } else {
                    LOG(TRACE) << "DynLoader: dlCloseDelayed: " << dlOpenedLibraries[handle].fileName << ": already registered: ";
                }
            } else {
                LOG(TRACE) << "DynLoader: dlCloseDelayed: " << handle << ": not opened using dlOpen";
            }
        } else {
            LOG(TRACE) << "DynLoader: dlCloseDelayed: handle is nullptr";
        }
    }

    int DynamicLoader::dlClose(void* handle) {
        int ret = 0;

        if (handle != nullptr) {
            if (dlOpenedLibraries.contains(handle)) {
                ret = dlClose(dlOpenedLibraries[handle]);

                dlOpenedLibraries.erase(handle);
            } else {
                LOG(TRACE) << "DynLoader: dlCloseDelayed: " << handle << ": not opened using dlOpen";
            }
        } else {
            LOG(TRACE) << "DynLoader: dlClose handle: nullptr";
        }

        return ret;
    }

    void* DynamicLoader::dlSym(void* handle, const std::string& symbol) {
        return core::system::dlsym(handle, symbol.c_str());
    }

    char* DynamicLoader::dlError() {
        return core::system::dlerror();
    }

    int DynamicLoader::realExecDlClose(const Library& library) {
        return core::system::dlclose(library.handle);
    }

    int DynamicLoader::dlClose(const Library& library) {
        int ret = 0;
        ret = realExecDlClose(library);

        if (ret != 0) {
            LOG(TRACE) << "  dlClose: " << DynamicLoader::dlError();
        } else {
            LOG(TRACE) << "  dlClose: " << library.fileName << ": success";
        }

        return ret;
    }

    void DynamicLoader::execDlCloseDeleyed() {
        if (!closeHandles.empty()) {
            LOG(TRACE) << "DynLoader: execDlCloseDeleyed";

            for (void* handle : closeHandles) {
                dlClose(dlOpenedLibraries[handle]);
                dlOpenedLibraries.erase(handle);
            }

            closeHandles.clear();

            LOG(TRACE) << "DynLoader: execDlCloseDeleyed done";
        }
    }

    void DynamicLoader::execDlCloseAll() {
        LOG(TRACE) << "DynLoader: execDlCloseAll";

        for (auto& [handle, library] : dlOpenedLibraries) {
            dlClose(library);
        }

        dlOpenedLibraries.clear();
        closeHandles.clear();

        LOG(TRACE) << "DynLoader: execDlCloseAll done";
    }

} // namespace core
