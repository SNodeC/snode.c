/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include "core/socket/stream/SocketConnection.h"
#include "web/websocket/SubProtocol.h"
#include "web/websocket/SubProtocolContext.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstddef>
#include <cstdint>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket {

    template <typename SocketContextUpgrade>
    SubProtocol<SocketContextUpgrade>::SubProtocol(SubProtocolContext* subProtocolContext,
                                                   const std::string& name,
                                                   int pingInterval,
                                                   int maxFlyingPings)
        : name(name)
        , subProtocolContext(subProtocolContext) {
        if (pingInterval > 0) {
            pingTimer = core::timer::Timer::intervalTimer(
                [this, maxFlyingPings](const std::function<void()>& stop) {
                    if (flyingPings < maxFlyingPings) {
                        LOG(DEBUG) << "WebSocket: Ping sent";
                        sendPing();
                        flyingPings++;
                    } else {
                        LOG(WARNING) << "WebSocket: MaxFlyingPings exceeded - closing";
                        sendClose();
                        stop();
                    }
                },
                pingInterval);
        }

        getSocketConnection()->setTimeout(0);
    }

    template <typename SocketContextUpgrade>
    SubProtocol<SocketContextUpgrade>::~SubProtocol() {
        pingTimer.cancel();
    }

    template <typename SocketContextUpgrade>
    void SubProtocol<SocketContextUpgrade>::sendMessage(const char* message, std::size_t messageLength) const {
        subProtocolContext->sendMessage(2, message, messageLength);
    }

    template <typename SocketContextUpgrade>
    void SubProtocol<SocketContextUpgrade>::sendMessage(const std::string& message) const {
        subProtocolContext->sendMessage(1, message.data(), message.length());
    }

    template <typename SocketContextUpgrade>
    void SubProtocol<SocketContextUpgrade>::sendMessageStart(const char* message, std::size_t messageLength) const {
        subProtocolContext->sendMessageStart(2, message, messageLength);
    }

    template <typename SocketContextUpgrade>
    void SubProtocol<SocketContextUpgrade>::sendMessageStart(const std::string& message) const {
        subProtocolContext->sendMessageStart(1, message.data(), message.length());
    }

    template <typename SocketContextUpgrade>
    void SubProtocol<SocketContextUpgrade>::sendMessageFrame(const char* message, std::size_t messageLength) const {
        subProtocolContext->sendMessageFrame(message, messageLength);
    }

    template <typename SocketContextUpgrade>
    void SubProtocol<SocketContextUpgrade>::sendMessageFrame(const std::string& message) const {
        sendMessageFrame(message.data(), message.length());
    }

    template <typename SocketContextUpgrade>
    void SubProtocol<SocketContextUpgrade>::sendMessageEnd(const char* message, std::size_t messageLength) const {
        subProtocolContext->sendMessageEnd(message, messageLength);
    }

    template <typename SocketContextUpgrade>
    void SubProtocol<SocketContextUpgrade>::sendMessageEnd(const std::string& message) const {
        sendMessageEnd(message.data(), message.length());
    }

    template <typename SocketContextUpgrade>
    void SubProtocol<SocketContextUpgrade>::sendPing(const char* reason, std::size_t reasonLength) const {
        subProtocolContext->sendPing(reason, reasonLength);
    }

    template <typename SocketContextUpgrade>
    void SubProtocol<SocketContextUpgrade>::sendClose(uint16_t statusCode, const char* reason, std::size_t reasonLength) {
        subProtocolContext->sendClose(statusCode, reason, reasonLength);
    }

    template <typename SocketContextUpgrade>
    void SubProtocol<SocketContextUpgrade>::onPongReceived() {
        LOG(DEBUG) << "WebSocket: Pong received";
        flyingPings = 0;
    }

    template <typename SocketContextUpgrade>
    const std::string& SubProtocol<SocketContextUpgrade>::getName() {
        return name;
    }

    template <typename SocketContextUpgrade>
    core::socket::stream::SocketConnection* SubProtocol<SocketContextUpgrade>::getSocketConnection() {
        return subProtocolContext->getSocketConnection();
    }

} // namespace web::websocket
