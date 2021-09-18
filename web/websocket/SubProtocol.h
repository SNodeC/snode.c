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

#ifndef WEB_WS_SUBSPROTOCOL_H
#define WEB_WS_SUBSPROTOCOL_H

namespace web::websocket {
    template <typename SubProtocolT>
    class SocketContext;
} // namespace web::websocket

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket {

    template <typename SocketContextT>
    class SubProtocol {
    public:
        using SocketContext = SocketContextT;

        enum class Role { SERVER, CLIENT };

    protected:
        SubProtocol(const std::string& name)
            : name(name) {
        }

        SubProtocol() = delete;
        SubProtocol(const SubProtocol&) = delete;
        SubProtocol& operator=(const SubProtocol&) = delete;

    public:
        virtual ~SubProtocol() = default;

        /* Facade (API) to WSServerContext -> WSTransmitter to be used from SubProtocol-Subclasses */
        void sendMessage(const char* message, std::size_t messageLength) {
            context->sendMessage(2, message, messageLength);
        }

        void sendMessage(const std::string& message) {
            context->sendMessage(1, message.data(), message.length());
        }

        void sendMessageStart(const char* message, std::size_t messageLength) {
            context->sendMessageStart(2, message, messageLength);
        }

        void sendMessageStart(const std::string& message) {
            context->sendMessageStart(1, message.data(), message.length());
        }

        void sendMessageFrame([[maybe_unused]] const char* message, [[maybe_unused]] std::size_t messageLength) {
            context->sendMessageFrame(message, messageLength);
        }

        void sendMessageFrame(const std::string& message) {
            sendMessageFrame(message.data(), message.length());
        }

        void sendMessageEnd([[maybe_unused]] const char* message, [[maybe_unused]] std::size_t messageLength) {
            context->sendMessageEnd(message, messageLength);
        }

        void sendMessageEnd(const std::string& message) {
            sendMessageEnd(message.data(), message.length());
        }

        void sendPing(char* reason = nullptr, std::size_t reasonLength = 0) {
            context->sendPing(reason, reasonLength);
        }

        void sendClose(uint16_t statusCode = 1000, const char* reason = nullptr, std::size_t reasonLength = 0) {
            context->sendClose(statusCode, reason, reasonLength);
        }

        std::string getLocalAddressAsString() const {
            return context->getLocalAddressAsString();
        }
        std::string getRemoteAddressAsString() const {
            return context->getRemoteAddressAsString();
        }

        const std::string& getName() {
            return name;
        }

        /* Callbacks (API) WSReceiver -> SubProtocol-Subclasses */
        virtual void onMessageStart(int opCode) = 0;
        virtual void onMessageData(const char* junk, std::size_t junkLen) = 0;
        virtual void onMessageEnd() = 0;
        virtual void onPongReceived() = 0;
        virtual void onMessageError(uint16_t errnum) = 0;

        /* Callbacks (API) socketConnection -> SubProtocol-Subclasses */
        virtual void onConnected() = 0;
        virtual void onDisconnected() = 0;

        void setSocketContext(SocketContext* serverContext) {
            context = serverContext;
        }

    protected:
        SocketContext* context;

        const std::string name;
    };

} // namespace web::websocket

#endif // WEB_WS_SUBSPROTOCOL_H
