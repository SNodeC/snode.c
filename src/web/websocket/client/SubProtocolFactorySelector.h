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

#ifndef WEB_WEBSOCKET_CLIENT_SUBPROTOCOLSELECTOR_H
#define WEB_WEBSOCKET_CLIENT_SUBPROTOCOLSELECTOR_H

#include "web/websocket/SubProtocolFactorySelector.h" // IWYU pragma: export

namespace web::websocket {
    template <typename SubProtocolT>
    class SubProtocolFactory;

    namespace client {
        class SubProtocol;
    } // namespace client
} // namespace web::websocket

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::client {

    class SubProtocolFactorySelector
        : public web::websocket::SubProtocolFactorySelector<web::websocket::SubProtocolFactory<web::websocket::client::SubProtocol>> {
    public:
        SubProtocolFactorySelector(const SubProtocolFactorySelector&) = delete;

        SubProtocolFactorySelector& operator=(const SubProtocolFactorySelector&) = delete;

    private:
        using Super = web::websocket::SubProtocolFactorySelector<web::websocket::SubProtocolFactory<web::websocket::client::SubProtocol>>;

    public:
        static SubProtocolFactorySelector* instance();

        static void link(const std::string& subProtocolName, void* (*getSubProtocolFactory)());
        static void addSubProtocolSearchPath(const std::string& searchPath);

        using web::websocket::SubProtocolFactorySelector<SubProtocolFactory>::allowDlOpen;
        static void allowDlOpen();

    private:
        SubProtocolFactorySelector();
    };

} // namespace web::websocket::client

#endif // WEB_WEBSOCKET_CLIENT_SUBPROTOCOLSELECTOR_H
