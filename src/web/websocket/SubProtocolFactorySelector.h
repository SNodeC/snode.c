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

#ifndef WEB_WEBSOCKET_SUBPROTOCOLSELECTOR_H
#define WEB_WEBSOCKET_SUBPROTOCOLSELECTOR_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/DynamicLoader.h"

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

        virtual SubProtocolFactory* load(const std::string& subProtocolName) = 0;
        static SubProtocolFactory* load(const std::string& subProtocolName,
                                        const std::string& subProtocolLibraryFile,
                                        const std::string& subProtocolFactoryFunctionName);

    public:
        SubProtocolFactory* select(const std::string& subProtocolName);
        SubProtocolFactory* select(const std::string& subProtocolName, Role role);

        template <typename SubProtocolFactory>
        void unload(SubProtocolFactory* subProtocolFactory) {
            std::string name = subProtocolFactory->getName();

            if (subProtocolFactories.contains(name)) {
                subProtocolFactories.erase(name);

                void* handle = subProtocolFactory->getHandle();
                delete subProtocolFactory;

                if (handle != nullptr) {
                    core::DynamicLoader::dlCloseDelayed(handle);
                }
            }
        }

    protected:
        void allowDlOpen();

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
