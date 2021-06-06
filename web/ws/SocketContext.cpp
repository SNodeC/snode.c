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

#include "web/ws/SocketContext.h"

#include "log/Logger.h"
#include "net/socket/stream/SocketConnectionBase.h"
#include "web/ws/SubProtocol.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstring>  // for memcpy
#include <endian.h> // for htobe16

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define CLOSE_SOCKET_TIMEOUT 10

namespace web::ws {

    SocketContext::SocketContext(net::socket::stream::SocketConnectionBase* socketConnection,
                                 web::ws::SubProtocol* subProtocol,
                                 web::ws::SubProtocol::Role role)
        : net::socket::stream::SocketContext(socketConnection)
        , Receiver(this)
        , Transmitter(role == web::ws::SubProtocol::Role::CLIENT)
        , subProtocol(subProtocol) {
        subProtocol->setWSContext(this);
    }

    void SocketContext::sendMessageStart(uint8_t opCode, const char* message, std::size_t messageLength) {
        Transmitter::sendMessageStart(opCode, message, messageLength);
    }

    void SocketContext::sendMessageFrame(const char* message, std::size_t messageLength) {
        Transmitter::sendMessageFrame(message, messageLength);
    }

    void SocketContext::sendMessageEnd(const char* message, std::size_t messageLength) {
        Transmitter::sendMessageEnd(message, messageLength);
    }

    void SocketContext::sendMessage(uint8_t opCode, const char* message, std::size_t messageLength) {
        Transmitter::sendMessage(opCode, message, messageLength);
    }

    void SocketContext::onMessageStart(int opCode) {
        switch (opCode) {
            case 0x08:
                closeReceived = true;
                break;
            case 0x09:
                pingReceived = true;
                VLOG(0) << "Ping received";
                break;
            case 0x0A:
                pongReceived = true;
                break;
            default:
                subProtocol->onMessageStart(opCode);
                break;
        }
    }

    void SocketContext::onFrameReceived(const char* junk, uint64_t junkLen) {
        if (!closeReceived && !pingReceived && !pongReceived) {
            std::size_t junkOffset = 0;

            do {
                std::size_t sendJunkLen = (junkLen - junkOffset <= SIZE_MAX) ? static_cast<std::size_t>(junkLen - junkOffset) : SIZE_MAX;
                subProtocol->onMessageData(junk + junkOffset, sendJunkLen);
                junkOffset += sendJunkLen;
            } while (junkLen - junkOffset > 0);
        } else {
            pongCloseData += std::string(junk, static_cast<std::size_t>(junkLen));
        }
    }

    void SocketContext::onMessageEnd() {
        if (closeReceived) {
            closeReceived = false;
            if (closeSent) { // active close
                closeSent = false;
                VLOG(0) << "Close confirmed from peer";
            } else { // passive close
                VLOG(0) << "Close request received - replying with close";
                close(pongCloseData.data(), pongCloseData.length());
                pongCloseData.clear();
            }
            net::socket::stream::SocketContext::socketConnection->getSocketContext()->close();
        } else if (pingReceived) {
            pingReceived = false;
            replyPong(pongCloseData.data(), pongCloseData.length());
            pongCloseData.clear();
        } else if (pongReceived) {
            pongReceived = false;
            subProtocol->onPongReceived();
        } else {
            subProtocol->onMessageEnd();
        }
    }

    void SocketContext::onPongReceived() {
        subProtocol->onPongReceived();
    }

    void SocketContext::onMessageError(uint16_t errnum) {
        VLOG(0) << "Message Error";

        subProtocol->onMessageError(errnum);
        close(errnum, "hallo", std::string("hallo").length());
    }

    void SocketContext::onProtocolConnected() {
        subProtocol->onProtocolConnected();
    }

    void SocketContext::onProtocolDisconnected() {
        subProtocol->onProtocolDisconnected();
    }

    void SocketContext::sendPing(const char* reason, std::size_t reasonLength) {
        sendMessage(9, reason, reasonLength);
    }

    void SocketContext::replyPong(const char* reason, std::size_t reasonLength) {
        sendMessage(10, reason, reasonLength);
    }

    void SocketContext::close(uint16_t statusCode, const char* reason, std::size_t reasonLength) {
        char* closePayload = const_cast<char*>(reason);
        std::size_t closePayloadLength = reasonLength;

        if (statusCode != 0) {
            closePayload = new char[reasonLength + 2];
            *reinterpret_cast<uint16_t*>(closePayload) = htobe16(statusCode);
            closePayloadLength += 2;
            if (reasonLength > 0) {
                memcpy(closePayload + 2, reason, reasonLength);
            }
        }

        close(closePayload, closePayloadLength);

        if (statusCode != 0) {
            delete[] closePayload;
        }

        setTimeout(CLOSE_SOCKET_TIMEOUT);

        closeSent = true;
    }

    void SocketContext::close(const char* message, std::size_t messageLength) {
        sendMessage(8, message, messageLength);
    }

    void SocketContext::sendFrameData(uint8_t data) {
        if (!closeSent) {
            sendToPeer(reinterpret_cast<char*>(&data), sizeof(uint8_t));
        }
    }

    void SocketContext::sendFrameData(uint16_t data) {
        if (!closeSent) {
            uint16_t sendData = htobe16(data);
            sendToPeer(reinterpret_cast<char*>(&sendData), sizeof(uint16_t));
        }
    }

    void SocketContext::sendFrameData(uint32_t data) {
        if (!closeSent) {
            uint32_t sendData = htobe32(data);
            sendToPeer(reinterpret_cast<char*>(&sendData), sizeof(uint32_t));
        }
    }

    void SocketContext::sendFrameData(uint64_t data) {
        if (!closeSent) {
            uint64_t sendData = htobe64(data);
            sendToPeer(reinterpret_cast<char*>(&sendData), sizeof(uint64_t));
        }
    }

    void SocketContext::sendFrameData(const char* frame, uint64_t frameLength) {
        if (!closeSent) {
            std::size_t frameOffset = 0;

            do {
                std::size_t sendJunkLen =
                    (frameLength - frameOffset <= SIZE_MAX) ? static_cast<std::size_t>(frameLength - frameOffset) : SIZE_MAX;
                sendToPeer(frame + frameOffset, sendJunkLen);
                frameOffset += sendJunkLen;
            } while (frameLength - frameOffset > 0);
        }
    }

    void SocketContext::onReceiveFromPeer(const char* junk, std::size_t junkLen) {
        Receiver::receive(const_cast<char*>(junk), junkLen);
    }

    void SocketContext::onReadError(int errnum) {
        VLOG(0) << "OnReadError: " << errnum;
    }

    void SocketContext::onWriteError(int errnum) {
        VLOG(0) << "OnWriteError: " << errnum;
    }

    std::string SocketContext::getLocalAddressAsString() const {
        return net::socket::stream::SocketContext::getLocalAddressAsString();
    }

    std::string SocketContext::getRemoteAddressAsString() const {
        return net::socket::stream::SocketContext::getRemoteAddressAsString();
    }

} // namespace web::ws
