/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/dlfcn.h"

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::system {

    void* dlopen(const char* filename, int flags) {
        errno = 0;
        return ::dlopen(filename, flags);
    }

    int dlclose(void* handle) {
        errno = 0;
        return ::dlclose(handle);
    }

    void* dlsym(void* handle, const char* symbol) {
        errno = 0;
        return ::dlsym(handle, symbol);
    }

    char* dlerror() {
        errno = 0;
        return ::dlerror();
    }

} // namespace core::system
