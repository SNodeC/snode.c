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

#include "web/ws/WSContext.h"
#include "web/ws/WSSubProtocol.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws {

    WSSubProtocol::WSSubProtocol(const std::string& name)
        : name(name) {
        clients.push_back(this);
    }

    WSSubProtocol::~WSSubProtocol() {
        clients.remove(this);
    }

    void WSSubProtocol::sendMessageStart(const std::string& message) { // 1
        wSContext->sendMessageStart(1, message.data(), message.length());
    }

    void WSSubProtocol::sendMessageStart(const char* message, std::size_t messageLength) { // 2
        wSContext->sendMessageStart(2, message, messageLength);
    }

    void WSSubProtocol::sendMessageFrame(const std::string& message) {
        sendMessageFrame(message.data(), message.length());
    }

    void WSSubProtocol::sendMessageFrame(const char* message, std::size_t messageLength) {
        wSContext->sendMessageFrame(message, messageLength);
    }

    void WSSubProtocol::sendMessage(const std::string& msg) {
        wSContext->sendMessage(1, msg.data(), msg.length());
    }

    void WSSubProtocol::sendMessage(const char* msg, std::size_t messageLength) {
        wSContext->sendMessage(2, msg, messageLength);
    }

    void WSSubProtocol::sendMessageEnd(const std::string& message) {
        sendMessageEnd(message.data(), message.length());
    }

    void WSSubProtocol::sendMessageEnd(const char* message, std::size_t messageLength) {
        wSContext->sendMessageEnd(message, messageLength);
    }

    void WSSubProtocol::sendPing(char* reason, std::size_t reasonLength) {
        wSContext->sendPing(reason, reasonLength);
    }

    void WSSubProtocol::sendClose(uint16_t statusCode, const char* reason, std::size_t reasonLength) {
        wSContext->close(statusCode, reason, reasonLength);
    }

    std::string WSSubProtocol::getLocalAddressAsString() const {
        return wSContext->getLocalAddressAsString();
    }

    std::string WSSubProtocol::getRemoteAddressAsString() const {
        return wSContext->getRemoteAddressAsString();
    }

    void WSSubProtocol::sendBroadcast(const std::string& message) {
        broadcast(1, message.data(), message.length());
    }

    void WSSubProtocol::sendBroadcast(const char* message, std::size_t messageLength) {
        broadcast(2, message, messageLength);
    }

    /* private members */
    void WSSubProtocol::setWSContext(WSContext* wSServerContext) {
        wSContext = wSServerContext;
    }

    std::list<WSSubProtocol*> WSSubProtocol::clients;

    void WSSubProtocol::broadcast(uint8_t opCode, const char* message, std::size_t messageLength) {
        for (WSSubProtocol* client : clients) {
            client->wSContext->sendMessage(opCode, message, messageLength);
        }
    }

    const std::string& WSSubProtocol::getName() {
        return name;
    }

} // namespace web::ws

#endif // WEB_WS_WSSERVERPROTOCOL_HPP
