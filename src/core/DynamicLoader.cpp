#include <SemanticLog.h>
/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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
            dlOpenedLibraries[handle].refCount++;
            snode::semantic::appLog().trace() << "dlOpen file = " << libFile << ": success";
        } else {
            snode::semantic::appLog().warn() << "dlOpen " << DynamicLoader::dlError();
        }

        return handle;
    }

    void DynamicLoader::dlCloseDelayed(void* handle) {
        if (handle != nullptr) {
            if (dlOpenedLibraries.contains(handle)) {
                if (std::find(closeHandles.begin(), closeHandles.end(), handle) == closeHandles.end()) {
                    snode::semantic::appLog().trace() << "dlCloseDelayed file = " << dlOpenedLibraries[handle].fileName << ": registered";

                    closeHandles.push_back(handle);
                } else {
                    snode::semantic::appLog().error() << "dlCloseDelayed file = " << dlOpenedLibraries[handle].fileName
                               << ": already registered for dlCloseDelayed";
                }
            } else {
                snode::semantic::appLog().warn() << "dlCloseDelayed handle = " << handle << ": not opened using dlOpen";
            }
        } else {
            snode::semantic::appLog().error() << "dlCloseDelayed handle: nullptr";
        }
    }

    int DynamicLoader::dlClose(void* handle) {
        int ret = 0;

        if (handle != nullptr) {
            if (std::find(closeHandles.begin(), closeHandles.end(), handle) == closeHandles.end()) {
                if (dlOpenedLibraries.contains(handle)) {
                    ret = dlClose(dlOpenedLibraries[handle]);

                    dlOpenedLibraries.erase(handle);
                } else {
                    snode::semantic::appLog().warn() << "dlClose handle = " << handle << ": not opened with dlOpen";
                }
            } else {
                snode::semantic::appLog().error() << "dlClose handle = " << handle << ": already registered for dlCloseDelayed";
            }
        } else {
            snode::semantic::appLog().error() << "dlClose handle: nullptr";
        }

        return ret;
    }

    int DynamicLoader::dlClose(Library& library) {
        int ret = 0;

        while (library.refCount-- > 0 && ret == 0) {
            ret = execDlClose(library);

            if (ret != 0) {
                snode::semantic::appLog().warn() << "dlClose: " << DynamicLoader::dlError();
            } else {
                snode::semantic::appLog().trace() << "dlClose file = " << library.fileName << ": closed";
            }
        }

        return ret;
    }

    void* DynamicLoader::dlSym(void* handle, const std::string& symbol) {
        return core::system::dlsym(handle, symbol.c_str());
    }

    char* DynamicLoader::dlError() {
        return core::system::dlerror();
    }

    int DynamicLoader::execDlClose(void* handle) {
        return core::system::dlclose(handle);
    }

    int DynamicLoader::execDlClose(Library& library) {
        return execDlClose(library.handle);
    }

    void DynamicLoader::execDlCloseDeleyed() {
        for (void* handle : closeHandles) {
            Library& library = dlOpenedLibraries[handle];

            if (execDlClose(library) != 0) {
                snode::semantic::appLog().warn() << "execDlCloseDeleyed file = " << library.fileName << ": " << DynamicLoader::dlError();
            } else {
                snode::semantic::appLog().trace() << "execDlCloseDeleyed file = " << library.fileName << ": closed";
            }

            dlOpenedLibraries.erase(handle);
        }

        closeHandles.clear();
    }

    void DynamicLoader::execDlCloseAll() {
        execDlCloseDeleyed();

        for (auto& [handle, library] : dlOpenedLibraries) {
            int ret = dlClose(library);

            if (ret != 0) {
                snode::semantic::appLog().warn() << "execDlCloseAll file = " << library.fileName << ": " << DynamicLoader::dlError();
            } else {
                snode::semantic::appLog().trace() << "execDlCloseAll file = " << library.fileName << ": closed";
            }
        }

        dlOpenedLibraries.clear();
    }

} // namespace core
