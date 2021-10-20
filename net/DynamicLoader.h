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

#ifndef NET_DYNAMICLOADER_H
#define NET_DYNAMICLOADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <set>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    class DynamicLoader {
    private:
        DynamicLoader() = default;

        ~DynamicLoader() = default;

        static DynamicLoader* instance();

    public:
        static void* dlOpen(const std::string& libFile, int flags);
        static void dlClose(void* handle);

    private:
        static void execDlClose(void* handle);
        static void execDeleyedDlClose();
        static void execDlCloseAll();

        std::map<void*, std::string> dlOpenedLibraries;
        std::set<void*> registeredForDlClose;

        friend class EventLoop;
    };

} // namespace net

#endif // NET_DYNAMICLOADER_H
