/*
 * SNode.C - A Slim Toolkit for Network Communication
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "core/socket/stream/SocketConnection.h"
#include "web/websocket/SocketContextUpgrade.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/hexdump.h"

#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket {

    template <typename SubProtocol, typename Request, typename Response>
    SocketContextUpgrade<SubProtocol, Request, Response>::SocketContextUpgrade(
        core::socket::stream::SocketConnection* socketConnection,
        web::http::SocketContextUpgradeFactory<Request, Response>* socketContextUpgradeFactory,
        Role role)
        : Super(socketConnection, socketContextUpgradeFactory)
        , SubProtocolContext(role == Role::CLIENT) {
    }

    template <typename SubProtocol, typename Request, typename Response>
    void SocketContextUpgrade<SubProtocol, Request, Response>::sendMessage(uint8_t opCode, const char* message, std::size_t messageLength) {
        Transmitter::sendMessage(opCode, message, messageLength);
    }

    template <typename SubProtocol, typename Request, typename Response>
    void
    SocketContextUpgrade<SubProtocol, Request, Response>::sendMessageStart(uint8_t opCode, const char* message, std::size_t messageLength) {
        Transmitter::sendMessageStart(opCode, message, messageLength);
    }

    template <typename SubProtocol, typename Request, typename Response>
    void SocketContextUpgrade<SubProtocol, Request, Response>::sendMessageFrame(const char* message, std::size_t messageLength) {
        Transmitter::sendMessageFrame(message, messageLength);
    }

    template <typename SubProtocol, typename Request, typename Response>
    void SocketContextUpgrade<SubProtocol, Request, Response>::sendMessageEnd(const char* message, std::size_t messageLength) {
        Transmitter::sendMessageEnd(message, messageLength);
    }

    template <typename SubProtocol, typename Request, typename Response>
    void SocketContextUpgrade<SubProtocol, Request, Response>::sendPing(const char* reason, std::size_t reasonLength) {
        sendMessage(9, reason, reasonLength);
    }

    template <typename SubProtocol, typename Request, typename Response>
    void SocketContextUpgrade<SubProtocol, Request, Response>::sendPong(const char* reason, std::size_t reasonLength) {
        sendMessage(10, reason, reasonLength);
    }

    template <typename SubProtocol, typename Request, typename Response>
    void
    SocketContextUpgrade<SubProtocol, Request, Response>::sendClose(uint16_t statusCode, const char* reason, std::size_t reasonLength) {
        const std::size_t closePayloadLength = reasonLength + 2;
        std::vector<char> closePayload(closePayloadLength);

        // Avoid unaligned stores.
        const uint16_t beStatus = htobe16(statusCode);
        std::memcpy(closePayload.data(), &beStatus, sizeof(beStatus));

        if (reasonLength > 0) {
            std::memcpy(closePayload.data() + 2, reason, reasonLength);
        }

        sendClose(closePayload.data(), closePayloadLength);
    }

    template <typename SubProtocol, typename Request, typename Response>
    void SocketContextUpgrade<SubProtocol, Request, Response>::sendClose(const char* message, std::size_t messageLength) {
        if (!closeSent) {
            LOG(DEBUG) << this->getSocketConnection()->getConnectionName() << " WebSocketContext: Subprotocol '" << subProtocol->name
                       << "' sending close to peer";

            sendMessage(8, message, messageLength);

            setTimeout(CLOSE_SOCKET_TIMEOUT);

            closeSent = true;
        }
    }

    template <typename SubProtocol, typename Request, typename Response>
    void SocketContextUpgrade<SubProtocol, Request, Response>::sendFrameChunk(const char* message, std::size_t messageLength) const {
        Super::sendToPeer(message, messageLength);
    }

    template <typename SubProtocol, typename Request, typename Response>
    std::size_t SocketContextUpgrade<SubProtocol, Request, Response>::readFrameChunk(char* chunk, std::size_t chunkLen) const {
        return Super::readFromPeer(chunk, chunkLen);
    }

    template <typename SubProtocol, typename Request, typename Response>
    core::socket::stream::SocketConnection* SocketContextUpgrade<SubProtocol, Request, Response>::getSocketConnection() const {
        return web::http::SocketContextUpgrade<Request, Response>::getSocketConnection();
    }

    template <typename SubProtocol, typename Request, typename Response>
    void SocketContextUpgrade<SubProtocol, Request, Response>::onMessageStart(int opCode) {
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
    void SocketContextUpgrade<SubProtocol, Request, Response>::onMessageData(const char* chunk, uint64_t chunkLen) {
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
    void SocketContextUpgrade<SubProtocol, Request, Response>::onMessageEnd() {
        switch (receivedOpCode) {
            case SubProtocolContext::OpCode::CLOSE:
                if (closeSent) { // active close
                    closeSent = false;
                    LOG(DEBUG) << getSocketConnection()->getConnectionName() << " WebSocketContext: Subprotocol '" << subProtocol->name
                               << "' close confirmed from peer";

                    shutdownWrite();
                } else { // passive close
                    LOG(DEBUG) << getSocketConnection()->getConnectionName() << " WebSocketContext: Subprotocol '" << subProtocol->name
                               << "' close request received - replying with close";

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

    template <typename SubProtocol, typename Request, typename Response>
    void SocketContextUpgrade<SubProtocol, Request, Response>::onMessageError(uint16_t errnum) {
        subProtocol->onMessageError(errnum);
        sendClose(errnum);
    }

    template <typename SubProtocol, typename Request, typename Response>
    void SocketContextUpgrade<SubProtocol, Request, Response>::onConnected() {
        LOG(INFO) << getSocketConnection()->getConnectionName() << " WebSocketContext: Subprotocol '" << subProtocol->name << "' connect";
        subProtocol->attach();
    }

    template <typename SubProtocol, typename Request, typename Response>
    void SocketContextUpgrade<SubProtocol, Request, Response>::onDisconnected() {
        subProtocol->detach();
        LOG(INFO) << getSocketConnection()->getConnectionName() << " WebSocketContext:  Subprotocol '" << subProtocol->name
                  << "' disconnected";
    }

    template <typename SubProtocol, typename Request, typename Response>
    bool SocketContextUpgrade<SubProtocol, Request, Response>::onSignal(int sig) {
        return subProtocol->onSignal(sig);
    }

    template <typename SubProtocol, typename Request, typename Response>
    std::size_t SocketContextUpgrade<SubProtocol, Request, Response>::onReceivedFromPeer() {
        return Receiver::receive();
    }

    template <typename SubProtocol, typename Request, typename Response>
    std::size_t SocketContextUpgrade<SubProtocol, Request, Response>::getPayloadTotalSent() const {
        return Transmitter::getPayloadTotalSent();
    }

    template <typename SubProtocol, typename Request, typename Response>
    std::size_t SocketContextUpgrade<SubProtocol, Request, Response>::getPayloadTotalRead() const {
        return Receiver::getPayloadTotalRead();
    }

    template <typename SubProtocol, typename Request, typename Response>
    std::string SocketContextUpgrade<SubProtocol, Request, Response>::getOnlineSince() const {
        return Super::getOnlineSince();
    }

    template <typename SubProtocol, typename Request, typename Response>
    std::string SocketContextUpgrade<SubProtocol, Request, Response>::getOnlineDuration() const {
        return Super::getOnlineDuration();
    }

} // namespace web::websocket
