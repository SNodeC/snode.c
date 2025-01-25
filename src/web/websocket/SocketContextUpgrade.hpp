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
#include "web/websocket/SocketContextUpgrade.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket {

    template <typename SubProtocol, typename Request, typename Response>
    SocketContextUpgrade<SubProtocol, Request, Response>::SocketContextUpgrade(
        core::socket::stream::SocketConnection* socketConnection,
        web::http::SocketContextUpgradeFactory<Request, Response>* socketContextUpgradeFactory,
        Role role)
        : Super(socketConnection, socketContextUpgradeFactory)
        , web::websocket::SubProtocolContext(role == Role::CLIENT) {
    }

    template <typename SubProtocol, typename Request, typename Response>
    void web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::sendMessage(uint8_t opCode,
                                                                                           const char* message,
                                                                                           std::size_t messageLength) {
        Transmitter::sendMessage(opCode, message, messageLength);
    }

    template <typename SubProtocol, typename Request, typename Response>
    void web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::sendMessageStart(uint8_t opCode,
                                                                                                const char* message,
                                                                                                std::size_t messageLength) {
        Transmitter::sendMessageStart(opCode, message, messageLength);
    }

    template <typename SubProtocol, typename Request, typename Response>
    void web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::sendMessageFrame(const char* message,
                                                                                                std::size_t messageLength) {
        Transmitter::sendMessageFrame(message, messageLength);
    }

    template <typename SubProtocol, typename Request, typename Response>
    void web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::sendMessageEnd(const char* message,
                                                                                              std::size_t messageLength) {
        Transmitter::sendMessageEnd(message, messageLength);
    }

    template <typename SubProtocol, typename Request, typename Response>
    void web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::sendPing(const char* reason, std::size_t reasonLength) {
        sendMessage(9, reason, reasonLength);
    }

    template <typename SubProtocol, typename Request, typename Response>
    void web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::sendPong(const char* reason, std::size_t reasonLength) {
        sendMessage(10, reason, reasonLength);
    }

    template <typename SubProtocol, typename Request, typename Response>
    void web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::sendClose(uint16_t statusCode,
                                                                                         const char* reason,
                                                                                         std::size_t reasonLength) {
        std::size_t closePayloadLength = reasonLength + 2;
        char* closePayload = new char[closePayloadLength];

        *reinterpret_cast<uint16_t*>(closePayload) = htobe16(statusCode); // cppcheck-suppress uninitdata

        if (reasonLength > 0) {
            memcpy(closePayload + 2, reason, reasonLength);
        }

        sendClose(closePayload, closePayloadLength);

        delete[] closePayload;
    }

    template <typename SubProtocol, typename Request, typename Response>
    void web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::sendClose(const char* message, std::size_t messageLength) {
        if (!closeSent) {
            LOG(DEBUG) << this->getSocketConnection()->getInstanceName() << " WebSocket: Sending close to peer";

            sendMessage(8, message, messageLength);

            setTimeout(CLOSE_SOCKET_TIMEOUT);

            closeSent = true;
        }
    }

    template <typename SubProtocol, typename Request, typename Response>
    core::socket::stream::SocketConnection* web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::getSocketConnection() {
        return web::http::SocketContextUpgrade<Request, Response>::getSocketConnection();
    }

    template <typename SubProtocol, typename Request, typename Response>
    void web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::onMessageStart(int opCode) {
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

    template <typename SubProtocol, typename Request, typename Response>
    void web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::onMessageData(const char* chunk, uint64_t chunkLen) {
        switch (receivedOpCode) {
            case SubProtocolContext::OpCode::CLOSE:
                [[fallthrough]];
            case SubProtocolContext::OpCode::PING:
                [[fallthrough]];
            case SubProtocolContext::OpCode::PONG:
                pongCloseData += std::string(chunk, static_cast<std::size_t>(chunkLen));
                break;
            default:
                std::size_t chunkOffset = 0;

                do {
                    std::size_t sendChunkLen =
                        (chunkLen - chunkOffset <= SIZE_MAX) ? static_cast<std::size_t>(chunkLen - chunkOffset) : SIZE_MAX;
                    subProtocol->onMessageData(chunk + chunkOffset, sendChunkLen);
                    chunkOffset += sendChunkLen;
                } while (chunkLen - chunkOffset > 0);
                break;
        }
    }

    template <typename SubProtocol, typename Request, typename Response>
    void web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::onMessageEnd() {
        switch (receivedOpCode) {
            case SubProtocolContext::OpCode::CLOSE:
                if (closeSent) { // active close
                    closeSent = false;
                    LOG(DEBUG) << this->getSocketConnection()->getInstanceName() << " WebSocket: Close confirmed from peer";
                } else { // passive close
                    LOG(DEBUG) << this->getSocketConnection()->getInstanceName()
                               << " WebSocket: Close request received - replying with close";
                    sendClose(pongCloseData.data(), pongCloseData.length());
                    pongCloseData.clear();
                    shutdownWrite();
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

    template <typename SubProtocol, typename Request, typename Response>
    void web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::onMessageError(uint16_t errnum) {
        subProtocol->onMessageError(errnum);
        sendClose(errnum);
    }

    template <typename SubProtocol, typename Request, typename Response>
    std::size_t web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::readFrameData(char* chunk, std::size_t chunkLen) {
        return readFromPeer(chunk, chunkLen);
    }

    template <typename SubProtocol, typename Request, typename Response>
    void web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::onConnected() {
        LOG(INFO) << this->getSocketConnection()->getInstanceName() << " WebSocket: connected";
        subProtocol->onConnected();
    }

    template <typename SubProtocol, typename Request, typename Response>
    void web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::onDisconnected() {
        subProtocol->onDisconnected();
        LOG(INFO) << this->getSocketConnection()->getInstanceName() << " WebSocket: disconnected";
    }

    template <typename SubProtocol, typename Request, typename Response>
    bool web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::onSignal(int sig) {
        return subProtocol->onSignal(sig);
    }

    template <typename SubProtocol, typename Request, typename Response>
    void web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::sendFrameData(uint8_t data) const {
        if (!closeSent) {
            sendToPeer(reinterpret_cast<char*>(&data), sizeof(uint8_t));
        }
    }

    template <typename SubProtocol, typename Request, typename Response>
    void web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::sendFrameData(uint16_t data) const {
        if (!closeSent) {
            uint16_t sendData = htobe16(data);
            sendToPeer(reinterpret_cast<char*>(&sendData), sizeof(uint16_t));
        }
    }

    template <typename SubProtocol, typename Request, typename Response>
    void web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::sendFrameData(uint32_t data) const {
        if (!closeSent) {
            uint32_t sendData = htobe32(data);
            sendToPeer(reinterpret_cast<char*>(&sendData), sizeof(uint32_t));
        }
    }

    template <typename SubProtocol, typename Request, typename Response>
    void web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::sendFrameData(uint64_t data) const {
        if (!closeSent) {
            uint64_t sendData = htobe64(data);
            sendToPeer(reinterpret_cast<char*>(&sendData), sizeof(uint64_t));
        }
    }

    template <typename SubProtocol, typename Request, typename Response>
    void web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::sendFrameData(const char* frame,
                                                                                             uint64_t frameLength) const {
        if (!closeSent) {
            uint64_t frameOffset = 0;

            do {
                std::size_t sendChunkLen =
                    (frameLength - frameOffset <= SIZE_MAX) ? static_cast<std::size_t>(frameLength - frameOffset) : SIZE_MAX;
                sendToPeer(frame + frameOffset, sendChunkLen);
                frameOffset += sendChunkLen;
            } while (frameLength - frameOffset > 0);
        }
    }

    template <typename SubProtocol, typename Request, typename Response>
    std::size_t web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::onReceivedFromPeer() {
        return Receiver::receive();
    }

    template <typename SubProtocol, typename Request, typename Response>
    void web::websocket::SocketContextUpgrade<SubProtocol, Request, Response>::dumpFrame(char* frame, uint64_t frameLength) {
        const unsigned int modul = 4;

        std::stringstream stringStream;

        for (std::size_t i = 0; i < frameLength; i++) {
            stringStream << std::setfill('0') << std::setw(2) << std::hex << static_cast<unsigned int>(static_cast<unsigned char>(frame[i]))
                         << " ";

            if ((i + 1) % modul == 0 || i == frameLength) {
                LOG(DEBUG) << "WebSocket: Frame = " << stringStream.str();
                stringStream.str("");
            }
        }
    }

} // namespace web::websocket
