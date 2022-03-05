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

#ifndef WEB_WEBSOCKET_SUBSPROTOCOL_H
#define WEB_WEBSOCKET_SUBSPROTOCOL_H

namespace web::websocket {
    template <typename SubProtocolT, typename RequestT, typename ResponseT>
    class SocketContextUpgrade;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstddef>
#include <cstdint>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket {

    template <typename SocketContextUpgradeT>
    class SubProtocol {
    public:
        using SocketContextUpgrade = SocketContextUpgradeT;

    protected:
        SubProtocol() = delete;
        SubProtocol(const std::string& name)
            : socketContextUpgrade(nullptr) {
            this->name = name;
        }

        SubProtocol(const SubProtocol&) = delete;
        SubProtocol& operator=(const SubProtocol&) = delete;

        virtual ~SubProtocol() = default;

    public:
        /* Facade (API) to WSServerContext -> WSTransmitter to be used from SubProtocol-Subclasses */
        void sendMessage(const char* message, std::size_t messageLength) {
            socketContextUpgrade->sendMessage(2, message, messageLength);
        }

        void sendMessage(const std::string& message) {
            socketContextUpgrade->sendMessage(1, message.data(), message.length());
        }

        void sendMessageStart(const char* message, std::size_t messageLength) {
            socketContextUpgrade->sendMessageStart(2, message, messageLength);
        }

        void sendMessageStart(const std::string& message) {
            socketContextUpgrade->sendMessageStart(1, message.data(), message.length());
        }

        void sendMessageFrame(const char* message, std::size_t messageLength) {
            socketContextUpgrade->sendMessageFrame(message, messageLength);
        }

        void sendMessageFrame(const std::string& message) {
            sendMessageFrame(message.data(), message.length());
        }

        void sendMessageEnd(const char* message, std::size_t messageLength) {
            socketContextUpgrade->sendMessageEnd(message, messageLength);
        }

        void sendMessageEnd(const std::string& message) {
            sendMessageEnd(message.data(), message.length());
        }

        void sendPing(char* reason = nullptr, std::size_t reasonLength = 0) {
            socketContextUpgrade->sendPing(reason, reasonLength);
        }

        void sendClose(uint16_t statusCode = 1000, const char* reason = nullptr, std::size_t reasonLength = 0) {
            socketContextUpgrade->sendClose(statusCode, reason, reasonLength);
        }

        std::string getLocalAddressAsString() const {
            return socketContextUpgrade->getLocalAddressAsString();
        }
        std::string getRemoteAddressAsString() const {
            return socketContextUpgrade->getRemoteAddressAsString();
        }

        const std::string& getName() {
            return name;
        }

    private:
        /* Callbacks (API) WSReceiver -> SubProtocol-Subclasses */
        virtual void onMessageStart(int opCode) = 0;
        virtual void onMessageData(const char* junk, std::size_t junkLen) = 0;
        virtual void onMessageEnd() = 0;
        virtual void onPongReceived() = 0;
        virtual void onMessageError(uint16_t errnum) = 0;

        /* Callbacks (API) socketConnection -> SubProtocol-Subclasses */
        virtual void onConnected() = 0;
        virtual void onDisconnected() = 0;

        void setSocketContextUpgrade(SocketContextUpgrade* socketContextUpgrade) {
            this->socketContextUpgrade = socketContextUpgrade;
        }

        void setName(const std::string& name) {
            this->name = name;
        }

        SocketContextUpgrade* socketContextUpgrade;

        std::string name;

        template <typename SubProtocolT, typename RequestT, typename ResponseT>
        friend class web::websocket::SocketContextUpgrade;

        friend SocketContextUpgrade;
    };

} // namespace web::websocket

#endif // WEB_WEBSOCKET_SUBSPROTOCOL_H
