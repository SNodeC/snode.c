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

#ifndef WEB_WS_SUBPROTOCOLSELECTOR_H
#define WEB_WS_SUBPROTOCOLSELECTOR_H

#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <dlfcn.h>
#include <list>
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket {

    template <typename SubProtocolFactory>
    struct SubProtocolPlugin {
        SubProtocolFactory* subProtocolFactory;
        void* handle = nullptr;
    };

    template <typename SubProtocolFactorySelectorT, typename SubProtocolFactoryT>
    class SubProtocolFactorySelector {
    public:
        using SubProtocolFactory = SubProtocolFactoryT;

        static SubProtocolFactorySelectorT* instance() {
            static SubProtocolFactorySelectorT subProtocolFactorySelector;

            return &subProtocolFactorySelector;
        }

    protected:
        SubProtocolFactorySelector() = default;

        SubProtocolFactorySelector(const SubProtocolFactorySelector&) = delete;
        SubProtocolFactorySelector& operator=(const SubProtocolFactorySelector&) = delete;

        virtual ~SubProtocolFactorySelector() = default;

    public:
        void add(SubProtocolFactory* subProtocolFactory, void* handle = nullptr) {
            SubProtocolPlugin<SubProtocolFactory> subProtocolPlugin = {.subProtocolFactory = subProtocolFactory, .handle = handle};

            if (subProtocolFactory != nullptr) {
                const auto [it, success] = subProtocolPlugins.insert({subProtocolFactory->name(), subProtocolPlugin});
                if (!success) {
                    VLOG(0) << "Subprotocol already existing: not using " << subProtocolFactory->name();
                    subProtocolFactory->destroy();
                    if (handle != nullptr) {
                        dlclose(handle);
                    }
                }
            } else if (handle != nullptr) {
                dlclose(handle);
            }
        }

    protected:
        SubProtocolFactory* load(const std::string& filePath) {
            SubProtocolFactory* subProtocolFactory = nullptr;

            void* handle = dlopen(filePath.c_str(), RTLD_LAZY | RTLD_LOCAL);

            if (handle != nullptr) {
                VLOG(0) << "dlopen: " << handle << " : " << filePath;

                SubProtocolFactory* (*plugin)() = reinterpret_cast<SubProtocolFactory* (*) ()>(dlsym(handle, "plugin"));

                if (plugin != nullptr) {
                    subProtocolFactory = plugin();
                    if (subProtocolFactory != nullptr) {
                        add(subProtocolFactory, handle);
                    } else {
                        dlclose(handle);
                    }
                } else {
                    VLOG(0) << "Optaining function \"plugin()\" in plugin failed: " << dlerror();
                }
            } else {
                VLOG(0) << "Error dlopen: " << dlerror();
            }

            return subProtocolFactory;
        }

        void addSubProtocolSearchPath(const std::string& searchPath) {
            searchPaths.push_back(searchPath);
        }

    public:
        SubProtocolFactory* select(const std::string& subProtocolName) {
            SubProtocolFactory* subProtocolFactory = nullptr;

            if (subProtocolPlugins.contains(subProtocolName)) {
                subProtocolFactory = subProtocolPlugins[subProtocolName].subProtocolFactory;
            } else {
                if (linkedSubProtocolFactories.contains(subProtocolName)) {
                    SubProtocolFactory* (*plugin)() = linkedSubProtocolFactories[subProtocolName];
                    subProtocolFactory = plugin();
                    if (subProtocolFactory != nullptr) {
                        add(subProtocolFactory, nullptr);
                    }
                } else {
                    for (const std::string& searchPath : searchPaths) {
                        subProtocolFactory = load(searchPath + "/libsnodec-websocket-" + subProtocolName + ".so");
                        if (subProtocolFactory != nullptr) {
                            break;
                        }
                    }
                }
            }

            return subProtocolFactory;
        }

        void unload(SubProtocolFactory* subProtocolFactory) {
            std::string name = subProtocolFactory->name();

            if (subProtocolPlugins.contains(name)) {
                subProtocolFactory->destroy();

                SubProtocolPlugin<SubProtocolFactory>& subProtocolPlugin = subProtocolPlugins[name];

                if (subProtocolPlugin.handle != nullptr) {
                    VLOG(0) << "dlclose: " << subProtocolPlugin.handle << " : " << name;
                    dlclose(subProtocolPlugin.handle);
                }

                subProtocolPlugins.erase(name);
            }
        }

        static void linkStatic(const std::string& subProtocolName, SubProtocolFactory* (*linkedPlugin)()) {
            instance()->linkedSubProtocolFactories[subProtocolName] = linkedPlugin;
        }

    private:
        std::map<std::string, SubProtocolPlugin<SubProtocolFactory>> subProtocolPlugins;
        std::map<std::string, SubProtocolFactory* (*) ()> linkedSubProtocolFactories;
        std::list<std::string> searchPaths;
    };

} // namespace web::websocket

#endif // WEB_WS_SUBPROTOCOLSELECTOR_H
