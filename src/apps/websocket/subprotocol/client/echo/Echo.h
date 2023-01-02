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

#ifndef APPS_WEBSOCKET_SUBPROTOCOL_ECHO_SERVER_ECHO_H
#define APPS_WEBSOCKET_SUBPROTOCOL_ECHO_SERVER_ECHO_H

#include "web/websocket/client/SubProtocol.h"

namespace web::websocket {
    class SubProtocolContext;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace apps::websocket::subprotocol::echo::client {

    class Echo : public web::websocket::client::SubProtocol {
    public:
        Echo(web::websocket::SubProtocolContext* socketContextUpgradeBase, const std::string& name);

    private:
        void onConnected() override;
        void onMessageStart(int opCode) override;
        void onMessageData(const char* junk, std::size_t junkLen) override;
        void onMessageEnd() override;
        void onMessageError(uint16_t errnum) override;
        void onDisconnected() override;
        void onExit() override;

        std::string data;
    };

} // namespace apps::websocket::subprotocol::echo::client

#endif // APPS_WEBSOCKET_SUBPROTOCOL_ECHO_SERVER_ECHO_H
