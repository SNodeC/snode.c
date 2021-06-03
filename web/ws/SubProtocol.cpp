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

#include "web/ws/SubProtocol.h"

#include "web/ws/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws {

    //    std::list<WSSubProtocol*> WSSubProtocol::clients;

    SubProtocol::SubProtocol(const std::string& name)
        : name(name) {
    }

    SubProtocol::~SubProtocol() {
    }

    void SubProtocol::sendMessage(const char* msg, std::size_t messageLength) {
        wSContext->sendMessage(2, msg, messageLength);
    }

    void SubProtocol::sendMessage(const std::string& msg) {
        wSContext->sendMessage(1, msg.data(), msg.length());
    }

    void SubProtocol::sendMessageStart(const char* message, std::size_t messageLength) {
        wSContext->sendMessageStart(2, message, messageLength);
    }

    void SubProtocol::sendMessageStart(const std::string& message) { // 1
        wSContext->sendMessageStart(1, message.data(), message.length());
    }

    void SubProtocol::sendMessageFrame(const char* message, std::size_t messageLength) {
        wSContext->sendMessageFrame(message, messageLength);
    }

    void SubProtocol::sendMessageFrame(const std::string& message) {
        sendMessageFrame(message.data(), message.length());
    }

    void SubProtocol::sendMessageEnd(const char* message, std::size_t messageLength) {
        wSContext->sendMessageEnd(message, messageLength);
    }

    void SubProtocol::sendMessageEnd(const std::string& message) {
        sendMessageEnd(message.data(), message.length());
    }

    void SubProtocol::sendPing(char* reason, std::size_t reasonLength) {
        wSContext->sendPing(reason, reasonLength);
    }

    void SubProtocol::sendClose(uint16_t statusCode, const char* reason, std::size_t reasonLength) {
        wSContext->close(statusCode, reason, reasonLength);
    }

    std::string SubProtocol::getLocalAddressAsString() const {
        return wSContext->getLocalAddressAsString();
    }

    std::string SubProtocol::getRemoteAddressAsString() const {
        return wSContext->getRemoteAddressAsString();
    }

    /* private members */
    void SubProtocol::setWSContext(SocketContext* wSServerContext) {
        wSContext = wSServerContext;
    }

    const std::string& SubProtocol::getName() {
        return name;
    }

} // namespace web::ws
