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

#ifndef WEB_WEBSOCKET_SUBPROTOCOLSELECTOR_H
#define WEB_WEBSOCKET_SUBPROTOCOLSELECTOR_H

#include "core/DynamicLoader.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <filesystem>
#include <list>
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket {

    template <typename SubProtocolFactoryT>
    class SubProtocolFactorySelector {
    public:
        SubProtocolFactorySelector(const SubProtocolFactorySelector&) = delete;
        SubProtocolFactorySelector& operator=(const SubProtocolFactorySelector&) = delete;

        using SubProtocolFactory = SubProtocolFactoryT;

        enum class Role { SERVER, CLIENT };

    protected:
        SubProtocolFactorySelector() = default;
        virtual ~SubProtocolFactorySelector() = default;

        virtual SubProtocolFactory* load(const std::string& subProtocolName, Role role) = 0;

        SubProtocolFactory* load(const std::string& subProtocolName,
                                 const std::string& subProtocolLibraryFile,
                                 const std::string& subProtocolFactoryFunctionName) {
            SubProtocolFactory* subProtocolFactory = nullptr;

            void* handle = dlOpen(subProtocolLibraryFile, RTLD_LAZY | RTLD_GLOBAL);

            if (handle != nullptr) {
                SubProtocolFactory* (*getSubProtocolFactory)() =
                    reinterpret_cast<SubProtocolFactory* (*) ()>(core::DynamicLoader::dlSym(handle, subProtocolFactoryFunctionName));
                if (getSubProtocolFactory != nullptr) {
                    subProtocolFactory = getSubProtocolFactory();
                    if (subProtocolFactory != nullptr) {
                        subProtocolFactory->setHandle(handle);
                        VLOG(0) << "SubProtocolFactory created successfull: " << subProtocolName;
                    } else {
                        VLOG(0) << "SubProtocolFactory not created: " << subProtocolName;
                        core::DynamicLoader::dlClose(handle);
                    }
                } else {
                    VLOG(0) << "Optaining function \"" << subProtocolFactoryFunctionName
                            << "\" in plugin failed: " << core::DynamicLoader::dlError();
                    core::DynamicLoader::dlClose(handle);
                }
            }

            return subProtocolFactory;
        }

    public:
        SubProtocolFactory* select(const std::string& subProtocolName) {
            SubProtocolFactory* subProtocolFactory = nullptr;

            if (subProtocolFactories.contains(subProtocolName)) {
                subProtocolFactory = subProtocolFactories[subProtocolName];
            }

            return subProtocolFactory;
        }

        SubProtocolFactory* select(const std::string& subProtocolName, Role role) {
            SubProtocolFactory* subProtocolFactory = select(subProtocolName);

            if (subProtocolFactory == nullptr) {
                if (linkedSubProtocolFactories.contains(subProtocolName)) {
                    SubProtocolFactory* (*plugin)() = linkedSubProtocolFactories[subProtocolName];
                    subProtocolFactory = plugin();
                } else if (!onlyLinked) {
                    subProtocolFactory = load(subProtocolName, role);
                }

                if (subProtocolFactory != nullptr) {
                    subProtocolFactories.insert({subProtocolName, subProtocolFactory});
                }
            }

            return subProtocolFactory;
        }

        void unload(SubProtocolFactory* subProtocolFactory) {
            std::string name = subProtocolFactory->getName();

            if (subProtocolFactories.contains(name)) {
                void* handle = subProtocolFactory->getHandle();
                delete subProtocolFactory;

                if (handle != nullptr) {
                    core::DynamicLoader::dlCloseDelayed(handle);
                }

                subProtocolFactories.erase(name);
            }
        }

    protected:
        void allowDlOpen() {
            onlyLinked = false;
        }

        template <typename SubProtocolFactory>
        void link(const std::string& subProtocolName, SubProtocolFactory* (*subProtocolFactory)()) {
            onlyLinked = true;
            linkedSubProtocolFactories[subProtocolName] =
                reinterpret_cast<SubProtocolFactorySelector::SubProtocolFactory* (*) ()>(subProtocolFactory);
        }

    private:
        std::map<std::string, SubProtocolFactory*> subProtocolFactories;
        std::map<std::string, SubProtocolFactory* (*) ()> linkedSubProtocolFactories;

        bool onlyLinked = false;
    };

} // namespace web::websocket

#endif // WEB_WEBSOCKET_SUBPROTOCOLSELECTOR_H
