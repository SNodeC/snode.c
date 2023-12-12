/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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
#include "web/websocket/SubProtocolContext.h"

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
#include <endian.h>
#include <iomanip>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define CLOSE_SOCKET_TIMEOUT 10

namespace web::websocket {

    template <typename SubProtocolT, typename RequestT, typename ResponseT>
    class SocketContextUpgrade
        : public web::http::SocketContextUpgrade<RequestT, ResponseT>
        , public web::websocket::SubProtocolContext {
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

        SocketContextUpgrade(core::socket::stream::SocketConnection* socketConnection,
                             web::http::SocketContextUpgradeFactory<Request, Response>* socketContextUpgradeFactory,
                             Role role)
            : Super(socketConnection, socketContextUpgradeFactory)
            , web::websocket::SubProtocolContext(role == Role::CLIENT) {
        }

    public:
        ~SocketContextUpgrade() override = default;

    private:
        void sendMessage(uint8_t opCode, const char* message, std::size_t messageLength) override {
            Transmitter::sendMessage(opCode, message, messageLength);
        }

        void sendMessageStart(uint8_t opCode, const char* message, std::size_t messageLength) override {
            Transmitter::sendMessageStart(opCode, message, messageLength);
        }

        void sendMessageFrame(const char* message, std::size_t messageLength) override {
            Transmitter::sendMessageFrame(message, messageLength);
        }

        void sendMessageEnd(const char* message, std::size_t messageLength) override {
            Transmitter::sendMessageEnd(message, messageLength);
        }

        void sendPing(const char* reason = nullptr, std::size_t reasonLength = 0) override {
            sendMessage(9, reason, reasonLength);
        }

        void sendPong(const char* reason = nullptr, std::size_t reasonLength = 0) override {
            sendMessage(10, reason, reasonLength);
        }

        void sendClose(uint16_t statusCode = 1000, const char* reason = nullptr, std::size_t reasonLength = 0) override {
            std::size_t closePayloadLength = reasonLength + 2;
            char* closePayload = new char[closePayloadLength];

            *reinterpret_cast<uint16_t*>(closePayload) = htobe16(statusCode); // cppcheck-suppress uninitdata

            if (reasonLength > 0) {
                memcpy(closePayload + 2, reason, reasonLength);
            }

            sendClose(closePayload, closePayloadLength);

            delete[] closePayload;
        }

        void sendClose(const char* message, std::size_t messageLength) override {
            if (!closeSent) {
                LOG(DEBUG) << "WebSocket: Sending close to peer";

                sendMessage(8, message, messageLength);
                shutdownWrite();

                setTimeout(CLOSE_SOCKET_TIMEOUT);

                closeSent = true;
            }
        }

        core::socket::stream::SocketConnection* getSocketConnection() override {
            return web::http::SocketContextUpgrade<RequestT, ResponseT>::getSocketConnection();
        }

        /* WSReceiver */
        void onMessageStart(int opCode) override {
            receivedOpCode = opCode;

            switch (opCode) {
                case SubProtocolContext::OpCode::CLOSE:
                    [[fallthrough]];
                case SubProtocolContext::OpCode::PING:
                    [[fallthrough]];
                case SubProtocolContext::OpCode::PONG:
                    break;
                default:
                    subProtocol->onMessageStart(opCode);
                    break;
            }
        }

        void onMessageData(const char* junk, uint64_t junkLen) override {
            switch (receivedOpCode) {
                case SubProtocolContext::OpCode::CLOSE:
                    [[fallthrough]];
                case SubProtocolContext::OpCode::PING:
                    [[fallthrough]];
                case SubProtocolContext::OpCode::PONG:
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
                case SubProtocolContext::OpCode::CLOSE:
                    if (closeSent) { // active close
                        closeSent = false;
                        LOG(DEBUG) << "WebSocket: Close confirmed from peer";
                    } else { // passive close
                        LOG(DEBUG) << "WebSocket: Close request received - replying with close";
                        sendClose(pongCloseData.data(), pongCloseData.length());
                        pongCloseData.clear();
                    }
                    break;
                case SubProtocolContext::OpCode::PING:
                    sendPong(pongCloseData.data(), pongCloseData.length());
                    pongCloseData.clear();
                    break;
                case SubProtocolContext::OpCode::PONG:
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
            LOG(INFO) << "WebSocket: connected";
            subProtocol->onConnected();
        }

        void onDisconnected() override {
            subProtocol->onDisconnected();
            LOG(INFO) << "WebSocket: disconnected";
        }

        void onExit(int sig) override {
            subProtocol->onExit(sig);
        }

        /* Facade to SocketContext used from WSTransmitter */
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

        /* SocketContext */
        std::size_t onReceivedFromPeer() override {
            return Receiver::receive();
        }

    protected:
        SubProtocol* subProtocol = nullptr;

    private:
        bool closeSent = false;

        int receivedOpCode = 0;

        std::string pongCloseData;

        void dumpFrame(char* frame, uint64_t frameLength) {
            const unsigned int modul = 4;

            std::stringstream stringStream;

            for (std::size_t i = 0; i < frameLength; i++) {
                stringStream << std::setfill('0') << std::setw(2) << std::hex
                             << static_cast<unsigned int>(static_cast<unsigned char>(frame[i])) << " ";

                if ((i + 1) % modul == 0 || i == frameLength) { // cppcheck-suppress knownConditionTrueFalse
                    LOG(TRACE) << "WebSocket: Frame = " << stringStream.str();
                    stringStream.str("");
                }
            }
        }
    };

} // namespace web::websocket

#endif // WEB_WEBSOCKET_SOCKETCONTEXT_H
