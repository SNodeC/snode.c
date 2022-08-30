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

#include "web/websocket/SubProtocol.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstddef>
#include <cstdint>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket {

    template <typename SocketContextUpgradeT>
    SubProtocol<SocketContextUpgradeT>::SubProtocol(const std::string& name, int pingInterval, int maxFlyingPings)
        : name(name)
        , socketContextUpgrade(nullptr) {
        if (pingInterval > 0) {
            pingTimer = core::timer::Timer::intervalTimer(
                [this, maxFlyingPings](const std::function<void()>& stop) -> void {
                    if (this->flyingPings < maxFlyingPings) {
                        LOG(INFO) << "Ping sent";
                        sendPing();
                        flyingPings++;
                    } else {
                        LOG(WARNING) << "MaxFlyingPings exceeded - closing";
                        sendClose();
                        stop();
                    }
                },
                pingInterval);
        }
    }

    template <typename SocketContextUpgradeT>
    SubProtocol<SocketContextUpgradeT>::~SubProtocol() {
        pingTimer.cancel();
    }

    template <typename SocketContextUpgradeT>
    void SubProtocol<SocketContextUpgradeT>::setSocketContextUpgrade(SocketContextUpgrade* socketContextUpgrade) {
        this->socketContextUpgrade = socketContextUpgrade;
    }

    template <typename SocketContextUpgradeT>
    void SubProtocol<SocketContextUpgradeT>::sendMessage(const char* message, std::size_t messageLength) {
        socketContextUpgrade->sendMessage(2, message, messageLength);
    }

    template <typename SocketContextUpgradeT>
    void SubProtocol<SocketContextUpgradeT>::sendMessage(const std::string& message) {
        socketContextUpgrade->sendMessage(1, message.data(), message.length());
    }

    template <typename SocketContextUpgradeT>
    void SubProtocol<SocketContextUpgradeT>::sendMessageStart(const char* message, std::size_t messageLength) {
        socketContextUpgrade->sendMessageStart(2, message, messageLength);
    }

    template <typename SocketContextUpgradeT>
    void SubProtocol<SocketContextUpgradeT>::sendMessageStart(const std::string& message) {
        socketContextUpgrade->sendMessageStart(1, message.data(), message.length());
    }

    template <typename SocketContextUpgradeT>
    void SubProtocol<SocketContextUpgradeT>::sendMessageFrame(const char* message, std::size_t messageLength) {
        socketContextUpgrade->sendMessageFrame(message, messageLength);
    }

    template <typename SocketContextUpgradeT>
    void SubProtocol<SocketContextUpgradeT>::sendMessageFrame(const std::string& message) {
        sendMessageFrame(message.data(), message.length());
    }

    template <typename SocketContextUpgradeT>
    void SubProtocol<SocketContextUpgradeT>::sendMessageEnd(const char* message, std::size_t messageLength) {
        socketContextUpgrade->sendMessageEnd(message, messageLength);
    }

    template <typename SocketContextUpgradeT>
    void SubProtocol<SocketContextUpgradeT>::sendMessageEnd(const std::string& message) {
        sendMessageEnd(message.data(), message.length());
    }

    template <typename SocketContextUpgradeT>
    void SubProtocol<SocketContextUpgradeT>::sendPing(const char* reason, std::size_t reasonLength) {
        socketContextUpgrade->sendPing(reason, reasonLength);
    }

    template <typename SocketContextUpgradeT>
    void SubProtocol<SocketContextUpgradeT>::sendClose(uint16_t statusCode, const char* reason, std::size_t reasonLength) {
        socketContextUpgrade->sendClose(statusCode, reason, reasonLength);
    }

    template <typename SocketContextUpgradeT>
    void SubProtocol<SocketContextUpgradeT>::onPongReceived() {
        LOG(INFO) << "Pong received";
        flyingPings = 0;
    }

    template <typename SocketContextUpgradeT>
    const std::string& SubProtocol<SocketContextUpgradeT>::getName() {
        return name;
    }

} // namespace web::websocket
