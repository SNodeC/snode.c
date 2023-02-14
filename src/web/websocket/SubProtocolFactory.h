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

#ifndef WEB_WEBSOCKET_SUBPROTOCOLPLUGININTERFACE_H
#define WEB_WEBSOCKET_SUBPROTOCOLPLUGININTERFACE_H

#include "core/socket/SocketContext.h"
#include "web/websocket/SubProtocolContext.h"

namespace web::websocket {
    template <typename SubProtocolT, typename RequestT, typename ResponseT>
    class SocketContextUpgrade;
} // namespace web::websocket

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket {

    template <typename SubProtocolT>
    class SubProtocolFactory {
    public:
        using SubProtocol = SubProtocolT;

        SubProtocolFactory(const std::string& name)
            : subProtocolName(name) {
        }

        SubProtocolFactory() = delete;

        virtual ~SubProtocolFactory() = default;

        SubProtocol* createSubProtocol(SubProtocolContext* subProtocolContext) {
            SubProtocol* subProtocol = create(subProtocolContext);

            //            if (subProtocol != nullptr) {
            refCount++;
            //            }

            return subProtocol;
        }

    private:
        virtual SubProtocol* create(SubProtocolContext* subProtocolContext) = 0;

    public:
        virtual std::size_t deleteSubProtocol(SubProtocol* subProtocol) {
            delete subProtocol;

            refCount--;

            return refCount;
        }

        const std::string& getName() {
            return subProtocolName;
        }

        void setHandle(void* handle) {
            this->handle = handle;
        }

        void* getHandle() {
            return handle;
        }

    private:
        std::size_t refCount = 0;

        std::string subProtocolName;

        void* handle = nullptr;
    };

} // namespace web::websocket

#endif // WEB_WEBSOCKET_SUBPROTOCOLPLUGININTERFACE_H
