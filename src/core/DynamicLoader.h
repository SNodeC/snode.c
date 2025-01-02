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

#ifndef CORE_DYNAMICLOADER_H
#define CORE_DYNAMICLOADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class DynamicLoader {
    private:
        struct Library {
            std::string fileName;
            void* handle = nullptr;
        };

    public:
        DynamicLoader() = delete;
        ~DynamicLoader() = delete;

#define dlOpen(libFile) dlOpenReal(libFile)

        static void* dlOpenReal(const std::string& libFile);

    private:
        static void* dlRegisterHandle(void* handle, const std::string& libFile);

    public:
        static void dlCloseDelayed(void* handle);
        static int dlClose(void* handle);
        static void* dlSym(void* handle, const std::string& symbol);
        static char* dlError();

    private:
        static int dlClose(const Library& library);

        static int realExecDlClose(const Library& library);
        static void execDlCloseDeleyed();
        static void execDlCloseAll();

        static std::map<void*, Library> dlOpenedLibraries;
        static std::list<void*> closeHandles;

        friend class EventLoop;
        friend class EventMultiplexer;
    };

} // namespace core

#endif // CORE_DYNAMICLOADER_H
