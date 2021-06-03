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

#include "web/ws/server/SubProtocol.h"

#include "web/ws/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws::server {

    SubProtocol::SubProtocol(const std::string& name)
        : web::ws::SubProtocol(name) {
    }

    SubProtocol::~SubProtocol() {
        clients->remove(this);
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

    /* private members */
    void SubProtocol::sendBroadcast(uint8_t opCode, const char* message, std::size_t messageLength) {
        for (SubProtocol* client : *clients) {
            client->Context->sendMessage(opCode, message, messageLength);
        }
    }

    void SubProtocol::sendBroadcastStart(uint8_t opCode, const char* message, std::size_t messageLength) {
        for (SubProtocol* client : *clients) {
            client->Context->sendMessageStart(opCode, message, messageLength);
        }
    }

    void SubProtocol::setClients(std::list<SubProtocol*>* clients) {
        this->clients = clients;
        clients->push_back(this);
    }

} // namespace web::ws::server
