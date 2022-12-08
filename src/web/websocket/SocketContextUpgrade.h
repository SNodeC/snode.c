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

#ifndef WEB_WEBSOCKET_SOCKETCONTEXT_H
#define WEB_WEBSOCKET_SOCKETCONTEXT_H

#include "web/http/SocketContextUpgrade.h" // IWYU pragma: export
#include "web/websocket/SocketContextUpgradeBase.h"

namespace core::socket {
    class SocketConnection;
} // namespace core::socket

namespace web::http {
    template <typename RequestT, typename ResponseT>
    class SocketContextUpgradeFactory;
} // namespace web::http

namespace web::websocket {
    template <typename SubProtocolT>
    class SubProtocolFactory;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstdint>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define CLOSE_SOCKET_TIMEOUT 10

namespace web::websocket {

    template <typename SubProtocolT, typename RequestT, typename ResponseT>
    class SocketContextUpgrade
        : public web::http::SocketContextUpgrade<RequestT, ResponseT>
        , protected web::websocket::SocketContextUpgradeBase {
    public:
        SocketContextUpgrade() = delete;
        SocketContextUpgrade(const SocketContextUpgrade&) = delete;
        SocketContextUpgrade& operator=(const SocketContextUpgrade&) = delete;

    private:
        using Super = web::http::SocketContextUpgrade<RequestT, ResponseT>;

    public:
        using SubProtocol = SubProtocolT;
        using Request = RequestT;
        using Response = ResponseT;

    private:
        using Super::readFromPeer;
        using Super::sendToPeer;
        using Super::setTimeout;
        using Super::shutdownWrite;

    protected:
        enum class Role { SERVER, CLIENT };

        SocketContextUpgrade(core::socket::SocketConnection* socketConnection,
                             web::http::SocketContextUpgradeFactory<Request, Response>* socketContextUpgradeFactory,
                             web::websocket::SubProtocolFactory<SubProtocol>* subProtocolFactory,
                             Role role)
            : Super(socketConnection, socketContextUpgradeFactory)
            , web::websocket::SocketContextUpgradeBase(role == Role::CLIENT)
            , subProtocol(subProtocolFactory->create(this)) {
            subProtocol->setSocketContextUpgrade(this);
        }

        ~SocketContextUpgrade() override = default;

    private:
        void sendMessage(uint8_t opCode, const char* message, std::size_t messageLength) const override {
            Transmitter::sendMessage(opCode, message, messageLength);
        }

        void sendMessageStart(uint8_t opCode, const char* message, std::size_t messageLength) const override {
            Transmitter::sendMessageStart(opCode, message, messageLength);
        }

        void sendMessageFrame(const char* message, std::size_t messageLength) const override {
            Transmitter::sendMessageFrame(message, messageLength);
        }

        void sendMessageEnd(const char* message, std::size_t messageLength) const override {
            Transmitter::sendMessageEnd(message, messageLength);
        }

        void sendPing(const char* reason = nullptr, std::size_t reasonLength = 0) const override {
            sendMessage(9, reason, reasonLength);
        }

        void sendPong(const char* reason = nullptr, std::size_t reasonLength = 0) const override {
            sendMessage(10, reason, reasonLength);
        }

        void sendClose(uint16_t statusCode = 1000, const char* reason = nullptr, std::size_t reasonLength = 0) override {
            std::size_t closePayloadLength = reasonLength + 2;
            char* closePayload = new char[closePayloadLength];

            *reinterpret_cast<uint16_t*>(closePayload) = htobe16(statusCode);

            if (reasonLength > 0) {
                memcpy(closePayload + 2, reason, reasonLength);
            }

            sendClose(closePayload, closePayloadLength);

            delete[] closePayload;

            setTimeout(CLOSE_SOCKET_TIMEOUT);

            closeSent = true;
        }

        void sendClose(const char* message, std::size_t messageLength) override {
            sendMessage(8, message, messageLength);
            shutdownWrite();
        }

        core::socket::SocketConnection* getSocketConnection() override {
            return Super::getSocketConnection();
        }

        /* WSReceiver */
        void onMessageStart(int opCode) override {
            receivedOpCode = opCode;

            switch (opCode) {
                case OpCode::CLOSE:
                    [[fallthrough]];
                case OpCode::PING:
                    [[fallthrough]];
                case OpCode::PONG:
                    break;
                default:
                    subProtocol->onMessageStart(opCode);
                    break;
            }
        }

        void onMessageData(const char* junk, uint64_t junkLen) override {
            switch (receivedOpCode) {
                case OpCode::CLOSE:
                    [[fallthrough]];
                case OpCode::PING:
                    [[fallthrough]];
                case OpCode::PONG:
                    pongCloseData += std::string(junk, static_cast<std::size_t>(junkLen));
                    break;
                default:
                    std::size_t junkOffset = 0;

                    do {
                        std::size_t sendJunkLen =
                            (junkLen - junkOffset <= SIZE_MAX) ? static_cast<std::size_t>(junkLen - junkOffset) : SIZE_MAX;
                        subProtocol->onMessageData(junk + junkOffset, sendJunkLen);
                        junkOffset += sendJunkLen;
                    } while (junkLen - junkOffset > 0);
                    break;
            }
        }

        void onMessageEnd() override {
            switch (receivedOpCode) {
                case OpCode::CLOSE:
                    if (closeSent) { // active close
                        closeSent = false;
                        LOG(INFO) << "Close confirmed from peer";
                    } else { // passive close
                        LOG(INFO) << "Close request received - replying with close";
                        sendClose(pongCloseData.data(), pongCloseData.length());
                        pongCloseData.clear();
                    }
                    break;
                case OpCode::PING:
                    sendPong(pongCloseData.data(), pongCloseData.length());
                    pongCloseData.clear();
                    break;
                case OpCode::PONG:
                    subProtocol->onPongReceived();
                    break;
                default:
                    subProtocol->onMessageEnd();
                    break;
            }
        }

        void onMessageError(uint16_t errnum) override {
            subProtocol->onMessageError(errnum);
            sendClose(errnum);
        }

        std::size_t readFrameData(char* junk, std::size_t junkLen) override {
            return readFromPeer(junk, junkLen);
        }

        /* Callbacks (API) socketConnection -> WSProtocol */
        void onConnected() override {
            VLOG(0) << "Websocket connected";
            subProtocol->onConnected();
        }

        void onDisconnected() override {
            subProtocol->onDisconnected();
            VLOG(0) << "Websocket disconnected";
        }

        /* Facade to SocketProtocol used from WSTransmitter */
        void sendFrameData(uint8_t data) const override {
            if (!closeSent) {
                sendToPeer(reinterpret_cast<char*>(&data), sizeof(uint8_t));
            }
        }

        void sendFrameData(uint16_t data) const override {
            if (!closeSent) {
                uint16_t sendData = htobe16(data);
                sendToPeer(reinterpret_cast<char*>(&sendData), sizeof(uint16_t));
            }
        }

        void sendFrameData(uint32_t data) const override {
            if (!closeSent) {
                uint32_t sendData = htobe32(data);
                sendToPeer(reinterpret_cast<char*>(&sendData), sizeof(uint32_t));
            }
        }

        void sendFrameData(uint64_t data) const override {
            if (!closeSent) {
                uint64_t sendData = htobe64(data);
                sendToPeer(reinterpret_cast<char*>(&sendData), sizeof(uint64_t));
            }
        }

        void sendFrameData(const char* frame, uint64_t frameLength) const override {
            if (!closeSent) {
                uint64_t frameOffset = 0;

                do {
                    std::size_t sendJunkLen =
                        (frameLength - frameOffset <= SIZE_MAX) ? static_cast<std::size_t>(frameLength - frameOffset) : SIZE_MAX;
                    sendToPeer(frame + frameOffset, sendJunkLen);
                    frameOffset += sendJunkLen;
                } while (frameLength - frameOffset > 0);
            }
        }

        /* SocketProtocol */
        std::size_t onReceiveFromPeer() override {
            return Receiver::receive();
        }

    protected:
        SubProtocol* subProtocol = nullptr;

    private:
        bool closeSent = false;

        int receivedOpCode = 0;

        std::string pongCloseData;

        enum OpCode { CLOSE = 0x08, PING = 0x09, PONG = 0x0A };
    };

} // namespace web::websocket

#endif // WEB_WEBSOCKET_SOCKETCONTEXT_H
