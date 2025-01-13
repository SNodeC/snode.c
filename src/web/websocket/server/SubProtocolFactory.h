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

#ifndef WEB_WEBSOCKET_SERVER_SUBSPROTOCOLFACTORY_H
#define WEB_WEBSOCKET_SERVER_SUBSPROTOCOLFACTORY_H

#include "web/websocket/SubProtocolFactory.h"
#include "web/websocket/server/SubProtocolFactorySelector.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    template <typename SubProtocolT>
    class SubProtocolFactory : public web::websocket::SubProtocolFactory<SubProtocolT> {
    public:
        using SubProtocol = SubProtocolT;
        using Super = web::websocket::SubProtocolFactory<SubProtocol>;
        using Super::Super;

        static void link(const std::string& name, SubProtocolFactory* (*linkedPlugin)()) {
            linkedPlugin()->getName();
            SubProtocolFactorySelector::instance()->link(name, linkedPlugin);
        }
    };

} // namespace web::websocket::server

#endif // WEB_WEBSOCKET_SERVER_SUBSPROTOCOLFACTORY_H
