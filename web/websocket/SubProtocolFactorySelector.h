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

#ifndef WEB_WEBSOCKET_SUBPROTOCOLSELECTOR_H
#define WEB_WEBSOCKET_SUBPROTOCOLSELECTOR_H

#include "log/Logger.h"
#include "core/DynamicLoader.h"

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

        enum class Role { SERVER, CLIENT };

    protected:
        SubProtocolFactorySelector() = default;

        SubProtocolFactorySelector(const SubProtocolFactorySelector&) = delete;
        SubProtocolFactorySelector& operator=(const SubProtocolFactorySelector&) = delete;

        virtual ~SubProtocolFactorySelector() = default;

    private:
        void add(SubProtocolFactory* subProtocolFactory, void* handle = nullptr) {
            SubProtocolPlugin<SubProtocolFactory> subProtocolPlugin = {.subProtocolFactory = subProtocolFactory, .handle = handle};

            if (subProtocolFactory != nullptr) {
                const auto [it, success] = subProtocolPlugins.insert({subProtocolFactory->getName(), subProtocolPlugin});
                if (!success) {
                    VLOG(0) << "Subprotocol already existing: not using " << subProtocolFactory->getName();
                    delete subProtocolFactory;
                    if (handle != nullptr) {
                        net::DynamicLoader::dlClose(handle);
                    }
                }
            } else if (handle != nullptr) {
                net::DynamicLoader::dlClose(handle);
            }
        }

    protected:
        virtual SubProtocolFactory* load(const std::string& subProtocolName) = 0;

        SubProtocolFactory* load(const std::string& subProtocolName, Role role) {
            SubProtocolFactory* subProtocolFactory = nullptr;

            for (const std::string& searchPath : searchPaths) {
                void* handle =
                    net::DynamicLoader::dlOpen(searchPath + "/libsnodec-websocket-" + subProtocolName + ".so", RTLD_LAZY | RTLD_LOCAL);

                if (handle != nullptr) {
                    SubProtocolFactory* (*getSubProtocolFactory)() = net::DynamicLoader::dlSym<SubProtocolFactory* (*) ()>(
                        handle, subProtocolName + (role == Role::SERVER ? "Server" : "Client") + "SubProtocolFactory");
                    if (getSubProtocolFactory != nullptr) {
                        subProtocolFactory = getSubProtocolFactory();
                        if (subProtocolFactory != nullptr) {
                            add(subProtocolFactory, handle);
                            break;
                        } else {
                            net::DynamicLoader::dlClose(handle);
                        }
                    } else {
                        VLOG(0) << "Optaining function \"plugin()\" in plugin failed: " << net::DynamicLoader::dlError();
                    }
                } else {
                    VLOG(0) << "Error dlopen: " << net::DynamicLoader::dlError();
                }
            }

            return subProtocolFactory;
        }

        void addSubProtocolSearchPath(const std::string& searchPath) {
            searchPaths.push_front(searchPath);
        }

    public:
        SubProtocolFactory* select(const std::string& subProtocolName) {
            SubProtocolFactory* subProtocolFactory = nullptr;

            if (subProtocolPlugins.contains(subProtocolName)) {
                subProtocolFactory = subProtocolPlugins[subProtocolName].subProtocolFactory;
            } else if (linkedSubProtocolFactories.contains(subProtocolName)) {
                SubProtocolFactory* (*plugin)() = linkedSubProtocolFactories[subProtocolName];
                subProtocolFactory = plugin();
                if (subProtocolFactory != nullptr) {
                    add(subProtocolFactory, nullptr);
                }
            } else if (!onlyLinked) {
                subProtocolFactory = load(subProtocolName);
            }

            return subProtocolFactory;
        }

    private:
        std::string doUnload(SubProtocolFactory* subProtocolFactory) {
            std::string name = subProtocolFactory->getName();

            if (subProtocolPlugins.contains(name)) {
                delete subProtocolFactory;

                SubProtocolPlugin<SubProtocolFactory>& subProtocolPlugin = subProtocolPlugins[name];

                if (subProtocolPlugin.handle != nullptr) {
                    net::DynamicLoader::dlClose(subProtocolPlugin.handle);
                }
            }

            return name;
        }

    public:
        void unload(SubProtocolFactory* subProtocolFactory) {
            subProtocolPlugins.erase(doUnload(subProtocolFactory));
        }

    protected:
        void allowDlOpen() {
            onlyLinked = false;
        }

        void linkSubProtocol(const std::string& subProtocolName, SubProtocolFactory* (*linkedPlugin)()) {
            onlyLinked = true;
            linkedSubProtocolFactories[subProtocolName] = linkedPlugin;
        }

    private:
        std::map<std::string, SubProtocolPlugin<SubProtocolFactory>> subProtocolPlugins;
        std::map<std::string, SubProtocolFactory* (*) ()> linkedSubProtocolFactories;
        std::list<std::string> searchPaths;

        bool onlyLinked = false;
    };

} // namespace web::websocket

#endif // WEB_WEBSOCKET_SUBPROTOCOLSELECTOR_H
