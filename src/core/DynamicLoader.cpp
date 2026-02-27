/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "core/DynamicLoader.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <filesystem>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    std::map<std::string, DynamicLoader::Library> DynamicLoader::dlOpenedLibraries;
    std::map<void*, std::string> DynamicLoader::dlOpenedLibrariesByHandle;
    std::list<std::string> DynamicLoader::closeQueue;

    std::string DynamicLoader::canonicalizePath(const std::string& libFile) {
        std::string result = libFile;

        try {
            const std::filesystem::path p(libFile);
            // Only try to resolve symlinks / normalize if the file actually exists.
            // Otherwise (e.g. "libfoo.so" to be found via ld search path), keep as-is.
            if (std::filesystem::exists(p)) {
                result = std::filesystem::canonical(p).string();
            }
        } catch (...) {
            // keep as-is
        }

        return result;
    }

    void* DynamicLoader::dlOpen(const std::string& libFile, int flags) {
        void* handle = nullptr;

        const std::string canonicalFile = canonicalizePath(libFile);

        auto it = dlOpenedLibraries.find(canonicalFile);
        if (it != dlOpenedLibraries.end()) {
            Library& lib = it->second;
            ++lib.refCount;
            lib.closePending = false;

            LOG(TRACE) << "DynLoader: dlOpen: " << lib.fileName << ": already open (refCount=" << lib.refCount << ")";
            handle = lib.handle;
        } else {
            // Clear possible stale error
            (void) DynamicLoader::dlError();

            handle = core::system::dlopen(libFile.c_str(), flags);
            if (handle != nullptr) {
                Library lib;
                lib.fileName = libFile;
                lib.canonicalFileName = canonicalFile;
                lib.handle = handle;
                lib.refCount = 1;
                lib.closePending = false;

                dlOpenedLibraries.emplace(canonicalFile, lib);
                dlOpenedLibrariesByHandle.emplace(handle, canonicalFile);

                LOG(TRACE) << "DynLoader: dlOpen: " << libFile << ": success";
            } else {
                LOG(TRACE) << "DynLoader: dlOpen: " << libFile << ": " << DynamicLoader::dlError();
            }
        }

        return handle;
    }

    void DynamicLoader::dlCloseDelayed(void* handle) {
        if (handle == nullptr) {
            LOG(TRACE) << "DynLoader: dlCloseDelayed: handle is nullptr";
        } else {
            auto itHandle = dlOpenedLibrariesByHandle.find(handle);
            if (itHandle == dlOpenedLibrariesByHandle.end()) {
                LOG(TRACE) << "DynLoader: dlCloseDelayed: " << handle << ": not opened using dlOpen";
            } else {
                auto itLib = dlOpenedLibraries.find(itHandle->second);
                if (itLib == dlOpenedLibraries.end()) {
                    LOG(TRACE) << "DynLoader: dlCloseDelayed: internal error: handle known but library record missing";
                } else {
                    Library& lib = itLib->second;

                    if (lib.refCount > 0) {
                        --lib.refCount;
                    }

                    if (lib.refCount == 0) {
                        lib.closePending = true;
                        closeQueue.push_back(lib.canonicalFileName);
                        LOG(TRACE) << "DynLoader: dlCloseDelayed: " << lib.fileName;
                    } else {
                        LOG(TRACE) << "DynLoader: dlCloseDelayed: " << lib.fileName << ": still referenced (refCount=" << lib.refCount
                                   << ")";
                    }
                }
            }
        }
    }

    int DynamicLoader::dlClose(void* handle) {
        int ret = 0;

        if (handle == nullptr) {
            LOG(TRACE) << "DynLoader: dlClose: handle is nullptr";
        } else {
            auto itHandle = dlOpenedLibrariesByHandle.find(handle);
            if (itHandle == dlOpenedLibrariesByHandle.end()) {
                LOG(TRACE) << "DynLoader: dlClose: " << handle << ": not opened using dlOpen";
            } else {
                auto itLib = dlOpenedLibraries.find(itHandle->second);
                if (itLib == dlOpenedLibraries.end()) {
                    LOG(TRACE) << "DynLoader: dlClose: internal error: handle known but library record missing";
                } else {
                    Library& lib = itLib->second;

                    if (lib.refCount > 0) {
                        --lib.refCount;
                    }

                    if (lib.refCount != 0) {
                        LOG(TRACE) << "DynLoader: dlClose: " << lib.fileName << ": still referenced (refCount=" << lib.refCount << ")";
                    } else {
                        lib.closePending = false;
                        ret = dlClose(lib);

                        dlOpenedLibrariesByHandle.erase(lib.handle);
                        dlOpenedLibraries.erase(itLib);
                    }
                }
            }
        }

        return ret;
    }

    void* DynamicLoader::dlSym(void* handle, const std::string& symbol) {
        void* sym = core::system::dlsym(handle, symbol.c_str());
        return sym;
    }

    const char* DynamicLoader::dlError() {
        const char* err = core::system::dlerror();
        return err;
    }

    int DynamicLoader::realExecDlClose(const Library& library) {
        const int ret = core::system::dlclose(library.handle);
        return ret;
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
        if (!closeQueue.empty()) {
            LOG(TRACE) << "DynLoader: execDlCloseDeleyed";

            for (const std::string& canonicalFile : closeQueue) {
                auto it = dlOpenedLibraries.find(canonicalFile);
                if (it == dlOpenedLibraries.end()) {
                    continue;
                }

                Library& lib = it->second;
                if (!lib.closePending || lib.refCount != 0) {
                    continue;
                }

                lib.closePending = false;
                (void) dlClose(lib);
                dlOpenedLibrariesByHandle.erase(lib.handle);
                dlOpenedLibraries.erase(it);
            }

            closeQueue.clear();

            LOG(TRACE) << "DynLoader: execDlCloseDeleyed done";
        }
    }

    void DynamicLoader::execDlCloseAll() {
        LOG(TRACE) << "DynLoader: execDlCloseAll";

        for (auto& [canonical, library] : dlOpenedLibraries) {
            (void) dlClose(library);
        }

        dlOpenedLibraries.clear();
        dlOpenedLibrariesByHandle.clear();
        closeQueue.clear();

        LOG(TRACE) << "DynLoader: execDlCloseAll done";
    }

} // namespace core
