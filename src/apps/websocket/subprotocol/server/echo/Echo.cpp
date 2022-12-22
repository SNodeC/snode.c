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

#include "Echo.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MAX_FLYING_PINGS 3
#define PING_INTERVAL 90

namespace apps::websocket::subprotocol::echo::server {

    Echo::Echo(web::websocket::SubProtocolContext* socketContextUpgradeBase, const std::string& name)
        : web::websocket::server::SubProtocol(socketContextUpgradeBase, name, PING_INTERVAL, MAX_FLYING_PINGS) {
    }

    void Echo::onConnected() {
        VLOG(0) << "Echo connected:";

        sendMessage("Welcome to SimpleChat");
        sendMessage("=====================");
    }

    void Echo::onMessageStart(int opCode) {
        VLOG(0) << "Message Start - OpCode: " << opCode;
    }

    void Echo::onMessageData(const char* junk, std::size_t junkLen) {
        data += std::string(junk, junkLen);

        VLOG(0) << "Message Fragment: " << std::string(junk, junkLen);
    }

    void Echo::onMessageEnd() {
        VLOG(0) << "Message Full Data: " << data;
        VLOG(0) << "Message End";
        /*
                forEachClient([&data = this->data](SubProtocol* client) {
                    client->sendMessage(data);
                });
        */
        sendBroadcast(data);

        data.clear();
    }

    void Echo::onMessageError(uint16_t errnum) {
        VLOG(0) << "Message error: " << errnum;
    }

    void Echo::onDisconnected() {
        VLOG(0) << "Echo disconnected:";
    }

    void Echo::onExit() {
        VLOG(0) << "Echo exit:";
    }

} // namespace apps::websocket::subprotocol::echo::server
