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

#include "Echo.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/system/signal.h"

#include <cstring>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MAX_FLYING_PINGS 3
#define PING_INTERVAL 90

namespace apps::websocket::subprotocol::echo::client {

    Echo::Echo(web::websocket::SubProtocolContext* socketContextUpgradeBase, const std::string& name)
        : web::websocket::client::SubProtocol(socketContextUpgradeBase, name, PING_INTERVAL, MAX_FLYING_PINGS) {
    }

    void Echo::onConnected() {
        VLOG(1) << "Echo connected:";
    }

    void Echo::onMessageStart(int opCode) {
        VLOG(2) << "Message Start - OpCode: " << opCode;
    }

    void Echo::onMessageData(const char* junk, std::size_t junkLen) {
        data += std::string(junk, junkLen);

        VLOG(2) << "Message Fragment: " << std::string(junk, junkLen);
    }

    void Echo::onMessageEnd() {
        VLOG(1) << "Message Full Data: " << data;
        VLOG(1) << "Message End";
        /*
                forEachClient([&data = this->data](SubProtocol* client) {
                    client->sendMessage(data);
                });
        */
        // sendMessage(data);

        data.clear();
    }

    void Echo::onMessageError(uint16_t errnum) {
        VLOG(1) << "Message error: " << errnum;
    }

    void Echo::onDisconnected() {
        VLOG(1) << "Echo disconnected:";
    }

    void Echo::onExit(int sig) {
        VLOG(1) << "SubProtocol 'echo' exit due to '" << strsignal(sig) << "' (SIG" << utils::system::sigabbrev_np(sig) << " = " << sig
                << ")";

        sendClose();
    }

} // namespace apps::websocket::subprotocol::echo::client
