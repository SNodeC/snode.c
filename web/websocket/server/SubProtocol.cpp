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

#include "web/websocket/server/SubProtocol.h"

#include "web/websocket/SocketContext.h"
#include "web/websocket/server/ChannelManager.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    SubProtocol::SubProtocol(const std::string& name)
        : web::websocket::SubProtocol(name) {
        clients = ChannelManager::instance()->subscribe(this);
    }

    SubProtocol::~SubProtocol() {
        ChannelManager::instance()->unsubscribe(this);
    }

    void SubProtocol::subscribe(const std::string& channel) {
        clients = ChannelManager::instance()->subscribe(channel, this);
    }

    void SubProtocol::sendBroadcast(const std::string& message) {
        sendBroadcast(1, message.data(), message.length());
    }

    void SubProtocol::sendBroadcast(const char* message, std::size_t messageLength) {
        sendBroadcast(2, message, messageLength);
    }

    void SubProtocol::sendBroadcastStart(const char* message, std::size_t messageLength) {
        sendBroadcastStart(2, message, messageLength);
    }
    void SubProtocol::sendBroadcastStart(const std::string& message) {
        sendBroadcastStart(1, message.data(), message.length());
    }

    void SubProtocol::sendBroadcastFrame(const char* message, std::size_t messageLength) {
        for (SubProtocol* client : *clients) {
            client->sendMessageFrame(message, messageLength);
        }
    }

    void SubProtocol::sendBroadcastFrame(const std::string& message) {
        sendBroadcastFrame(message.data(), message.length());
    }

    void SubProtocol::sendBroadcastEnd(const char* message, std::size_t messageLength) {
        for (SubProtocol* client : *clients) {
            client->sendMessageEnd(message, messageLength);
        }
    }

    void SubProtocol::sendBroadcastEnd(const std::string& message) {
        sendBroadcastEnd(message.data(), message.length());
    }

    void SubProtocol::forEachClient(const std::function<void(SubProtocol*)>& forEachClient) {
        for (SubProtocol* client : *clients) {
            forEachClient(client);
        }
    }

    /* private members */
    void SubProtocol::sendBroadcast(uint8_t opCode, const char* message, std::size_t messageLength) {
        for (SubProtocol* client : *clients) {
            client->context->sendMessage(opCode, message, messageLength);
        }
    }

    void SubProtocol::sendBroadcastStart(uint8_t opCode, const char* message, std::size_t messageLength) {
        for (SubProtocol* client : *clients) {
            client->context->sendMessageStart(opCode, message, messageLength);
        }
    }

} // namespace web::websocket::server
