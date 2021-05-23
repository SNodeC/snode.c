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

#ifndef WS_SERVER_WSSERVERPROTOCOL_HPP
#define WS_SERVER_WSSERVERPROTOCOL_HPP

#include "web/ws/server/WSServerProtocol.h"

#include "web/ws/server/WSServerContext.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws::server {

    WSServerProtocol::WSServerProtocol() {
        clients.push_back(this);
    }

    WSServerProtocol::~WSServerProtocol() {
        clients.remove(this);
    }

    void WSServerProtocol::messageStart(char* message, std::size_t messageLength, uint32_t messageKey) { // 2
        messageStart(2, message, messageLength, messageKey);
    }

    void WSServerProtocol::sendFrame(char* message, std::size_t messageLength, uint32_t messageKey) {
        wSServerContext->sendFrame(message, messageLength, messageKey);
    }

    void WSServerProtocol::message(char* message, std::size_t messageLength, uint32_t messageKey) {
        messageStart(2, message, messageLength, messageKey);
    }

    void WSServerProtocol::messageEnd(char* message, std::size_t messageLength, uint32_t messageKey) {
        wSServerContext->messageEnd(message, messageLength, messageKey);
    }

    void WSServerProtocol::messageStart(const std::string& message, uint32_t messageKey) { // 1
        messageStart(1, const_cast<std::string&>(message).data(), message.length(), messageKey);
    }

    void WSServerProtocol::sendFrame(const std::string& message, uint32_t messageKey) {
        sendFrame(const_cast<std::string&>(message).data(), message.length(), messageKey);
    }

    void WSServerProtocol::message(const std::string& msg, uint32_t messageKey) {
        message(1, const_cast<std::string&>(msg).data(), msg.length(), messageKey);
    }

    void WSServerProtocol::messageEnd(const std::string& message, uint32_t messageKey) {
        messageEnd(const_cast<std::string&>(message).data(), message.length(), messageKey);
    }

    void WSServerProtocol::broadcast(char* message, std::size_t messageLength, uint32_t messageKey) {
        broadcast(2, message, messageLength, messageKey);
    }
    void WSServerProtocol::broadcast(const std::string& message, uint32_t messageKey) {
        broadcast(1, const_cast<std::string&>(message).data(), message.length(), messageKey);
    }

    void WSServerProtocol::sendPing(char* reason, std::size_t reasonLength) {
        wSServerContext->sendPing(reason, reasonLength);
    }

    void WSServerProtocol::close(uint16_t statusCode, const char* reason, std::size_t reasonLength) {
        wSServerContext->close(statusCode, reason, reasonLength);
    }

    /* private members */
    void WSServerProtocol::messageStart(uint8_t opCode, char* message, std::size_t messageLength, uint32_t messageKey) {
        wSServerContext->messageStart(opCode, message, messageLength, messageKey);
    }

    void WSServerProtocol::message(uint8_t opCode, char* message, std::size_t messageLength, uint32_t messageKey) {
        wSServerContext->message(opCode, message, messageLength, messageKey);
    }

    std::list<WSServerProtocol*> WSServerProtocol::clients;

    void WSServerProtocol::broadcast(uint8_t opCode, char* message, std::size_t messageLength, uint32_t messageKey) {
        for (WSServerProtocol* client : clients) {
            client->message(opCode, message, messageLength, messageKey);
        }
    }

} // namespace web::ws::server

#endif // WS_SERVER_WSSERVERPROTOCOL_HPP
