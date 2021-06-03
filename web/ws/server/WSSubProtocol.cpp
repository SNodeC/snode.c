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

#include "web/ws/server/WSSubProtocol.h"

#include "web/ws/WSContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws::server {

    WSSubProtocol::WSSubProtocol(const std::string& name)
        : web::ws::WSSubProtocol(name) {
    }

    WSSubProtocol::~WSSubProtocol() {
        clients->remove(this);
    }

    void WSSubProtocol::sendBroadcast(const std::string& message) {
        sendBroadcast(1, message.data(), message.length());
    }

    void WSSubProtocol::sendBroadcast(const char* message, std::size_t messageLength) {
        sendBroadcast(2, message, messageLength);
    }

    void WSSubProtocol::sendBroadcastStart(const char* message, std::size_t messageLength) {
        sendBroadcastStart(2, message, messageLength);
    }
    void WSSubProtocol::sendBroadcastStart(const std::string& message) {
        sendBroadcastStart(1, message.data(), message.length());
    }

    void WSSubProtocol::sendBroadcastFrame(const char* message, std::size_t messageLength) {
        for (WSSubProtocol* client : *clients) {
            client->sendMessageFrame(message, messageLength);
        }
    }

    void WSSubProtocol::sendBroadcastFrame(const std::string& message) {
        sendBroadcastFrame(message.data(), message.length());
    }

    void WSSubProtocol::sendBroadcastEnd(const char* message, std::size_t messageLength) {
        for (WSSubProtocol* client : *clients) {
            client->sendMessageEnd(message, messageLength);
        }
    }

    void WSSubProtocol::sendBroadcastEnd(const std::string& message) {
        sendBroadcastEnd(message.data(), message.length());
    }

    /* private members */
    void WSSubProtocol::sendBroadcast(uint8_t opCode, const char* message, std::size_t messageLength) {
        for (WSSubProtocol* client : *clients) {
            client->wSContext->sendMessage(opCode, message, messageLength);
        }
    }

    void WSSubProtocol::sendBroadcastStart(uint8_t opCode, const char* message, std::size_t messageLength) {
        for (WSSubProtocol* client : *clients) {
            client->wSContext->sendMessageStart(opCode, message, messageLength);
        }
    }

    void WSSubProtocol::setClients(std::list<WSSubProtocol*>* clients) {
        this->clients = clients;
        clients->push_back(this);
    }

} // namespace web::ws::server
