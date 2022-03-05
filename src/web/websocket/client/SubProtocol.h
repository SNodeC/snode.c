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

#ifndef WEB_WEBSOCKET_CLIENT_SUBSPROTOCOL_H
#define WEB_WEBSOCKET_CLIENT_SUBSPROTOCOL_H

#include "web/websocket/SubProtocol.h"                 // IWYU pragma: export
#include "web/websocket/client/SocketContextUpgrade.h" // IWYU pragma: keep

// IWYU pragma: no_include "web/websocket/SubProtocol.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::client {

    class SubProtocol : public web::websocket::SubProtocol<web::websocket::client::SocketContextUpgrade> {
    private:
        using Super = web::websocket::SubProtocol<web::websocket::client::SocketContextUpgrade>;
        using Super::Super;

    public:
        /* Facade (API) to WSServerContext -> WSTransmitter to be used from SubProtocol-Subclasses */
        using Super::sendMessage;

        using Super::sendMessageEnd;
        using Super::sendMessageFrame;
        using Super::sendMessageStart;

        using Super::sendClose;
        using Super::sendPing;

        template <typename RequestT, typename ResponseT, typename SubProtocolT>
        friend class web::websocket::SocketContextUpgrade;
    };

} // namespace web::websocket::client

#endif // WEB_WEBSOCKET_CLIENT_SUBSPROTOCOL_H
