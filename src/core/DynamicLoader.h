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

#ifndef CORE_DYNAMICLOADER_H
#define CORE_DYNAMICLOADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/dlfcn.h" // IWYU pragma: keep

#include <cstddef>
#include <map>
#include <set>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class DynamicLoader {
    private:
        struct Library {
            std::string fileName = "";
            std::size_t refCount = 0;
            void* handle = nullptr;
        };

    public:
        DynamicLoader() = delete;
        ~DynamicLoader() = delete;

        inline static void* dlOpen(const std::string& libFile, int flags) {
            void* handle = nullptr;

            //        if (std::filesystem::exists(libFile)) {
            handle = core::system::dlopen(libFile.c_str(), flags);

            if (handle != nullptr) {
                if (!dlOpenedLibraries.contains(handle)) {
                    dlOpenedLibraries[handle].fileName = libFile;
                    dlOpenedLibraries[handle].handle = handle;
                }
                dlOpenedLibraries[handle].refCount++;
            } else {
            }
            //        } else {
            //            LOG(WARNING) << "dlOpen file = " << libFile << ": not existing";
            //        }

            return handle;
        }

        static void dlCloseDelayed(void* handle);
        static int dlClose(void* handle);

        static int dlClose(Library& library);

        static void* dlSym(void* handle, const std::string& symbol);

        static char* dlError();

    private:
        static int execDlClose(void* handle);
        static int execDlClose(Library& library);
        static void execDlCloseDeleyed();
        static void execDlCloseAll();

        static std::map<void*, Library> dlOpenedLibraries;
        static std::set<void*> closeHandles;

        friend class EventLoop;
        friend class EventMultiplexer;
    };

} // namespace core

#endif // CORE_DYNAMICLOADER_H
