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
#include "net/DynamicLoader.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

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

    template <typename SubProtocolFactoryT>
    class SubProtocolFactorySelector {
    public:
        using SubProtocolFactory = SubProtocolFactoryT;

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
                        net::DynamicLoader::dlClose(handle, true);
                    }
                }
            } else if (handle != nullptr) {
                net::DynamicLoader::dlClose(handle, true);
            }
        }

    protected:
        SubProtocolFactory* load(const std::string& filePath) {
            SubProtocolFactory* subProtocolFactory = nullptr;

            void* handle = net::DynamicLoader::dlOpen(filePath.c_str(), RTLD_LAZY | RTLD_LOCAL);

            if (handle != nullptr) {
                SubProtocolFactory* (*getSubProtocolFactory)() =
                    net::DynamicLoader::dlSym<SubProtocolFactory* (*) ()>(handle, "getSubProtocolFactory");

                if (getSubProtocolFactory != nullptr) {
                    subProtocolFactory = getSubProtocolFactory();
                    if (subProtocolFactory != nullptr) {
                        add(subProtocolFactory, handle);
                    } else {
                        net::DynamicLoader::dlClose(handle, true);
                    }
                } else {
                    VLOG(0) << "Optaining function \"plugin()\" in plugin failed: " << net::DynamicLoader::dlError();
                }
            } else {
                VLOG(0) << "Error dlopen: " << net::DynamicLoader::dlError();
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
                    net::DynamicLoader::dlClose(subProtocolPlugin.handle, true);
                }

                subProtocolPlugins.erase(name);
            }
        }

        void linkSubProtocol(const std::string& subProtocolName, SubProtocolFactory* (*linkedPlugin)()) {
            linkedSubProtocolFactories[subProtocolName] = linkedPlugin;
        }

    private:
        std::map<std::string, SubProtocolPlugin<SubProtocolFactory>> subProtocolPlugins;
        std::map<std::string, SubProtocolFactory* (*) ()> linkedSubProtocolFactories;
        std::list<std::string> searchPaths;
    };

} // namespace web::websocket

#endif // WEB_WS_SUBPROTOCOLSELECTOR_H
