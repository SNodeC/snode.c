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

#ifndef WEB_WS_WSSERVERPROTOCOL_HPP
#define WEB_WS_WSSERVERPROTOCOL_HPP

#include "web/ws/WSProtocol.h"

#include "web/ws/WSContextBase.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws {

    WSProtocol::WSProtocol() {
        clients.push_back(this);
    }

    WSProtocol::~WSProtocol() {
        clients.remove(this);
    }

    void WSProtocol::sendMessageStart(const std::string& message) { // 1
        messageStart(1, const_cast<std::string&>(message).data(), message.length());
    }

    void WSProtocol::sendMessageStart(const char* message, std::size_t messageLength) { // 2
        messageStart(2, message, messageLength);
    }

    void WSProtocol::sendMessageFrame(const std::string& message) {
        sendMessageFrame(const_cast<std::string&>(message).data(), message.length());
    }

    void WSProtocol::sendMessageFrame(const char* message, std::size_t messageLength) {
        wSServerContext->sendMessageFrame(message, messageLength);
    }

    void WSProtocol::sendMessage(const std::string& msg) {
        message(1, const_cast<std::string&>(msg).data(), msg.length());
    }

    void WSProtocol::sendMessage(const char* msg, std::size_t messageLength) {
        message(2, msg, messageLength);
    }

    void WSProtocol::sendMessageEnd(const std::string& message) {
        sendMessageEnd(const_cast<std::string&>(message).data(), message.length());
    }

    void WSProtocol::sendMessageEnd(const char* message, std::size_t messageLength) {
        wSServerContext->sendMessageEnd(message, messageLength);
    }

    void WSProtocol::sendBroadcast(const std::string& message) {
        broadcast(1, const_cast<std::string&>(message).data(), message.length());
    }

    void WSProtocol::sendBroadcast(const char* message, std::size_t messageLength) {
        broadcast(2, message, messageLength);
    }

    void WSProtocol::sendPing(char* reason, std::size_t reasonLength) {
        wSServerContext->sendPing(reason, reasonLength);
    }

    void WSProtocol::sendClose(uint16_t statusCode, const char* reason, std::size_t reasonLength) {
        wSServerContext->close(statusCode, reason, reasonLength);
    }

    /* private members */
    void WSProtocol::messageStart(uint8_t opCode, const char* message, std::size_t messageLength) {
        wSServerContext->sendMessageStart(opCode, message, messageLength);
    }

    void WSProtocol::message(uint8_t opCode, const char* message, std::size_t messageLength) {
        wSServerContext->sendMessage(opCode, message, messageLength);
    }

    std::list<WSProtocol*> WSProtocol::clients;

    void WSProtocol::broadcast(uint8_t opCode, const char* message, std::size_t messageLength) {
        for (WSProtocol* client : clients) {
            client->message(opCode, message, messageLength);
        }
    }

    std::string WSProtocol::getLocalAddressAsString() const {
        return wSServerContext->getLocalAddressAsString();
    }
    std::string WSProtocol::getRemoteAddressAsString() const {
        return wSServerContext->getRemoteAddressAsString();
    }

    void WSProtocol::setWSServerContext(WSContextBase* wSServerContext) {
        this->wSServerContext = wSServerContext;
    }

} // namespace web::ws

#endif // WEB_WS_WSSERVERPROTOCOL_HPP
