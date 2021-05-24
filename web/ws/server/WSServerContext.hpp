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

#ifndef WEB_WS_SERVER_WSSERVERCONTEXT_HPP
#define WEB_WS_SERVER_WSSERVERCONTEXT_HPP -

#include "log/Logger.h"
#include "net/socket/stream/SocketConnectionBase.h"
#include "web/ws/server/WSServerContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstring>  // for memcpy
#include <endian.h> // for htobe16
#include <ostream>  // for endl

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define CLOSE_SOCKET_TIMEOUT 10

namespace web::ws::server {

    template <typename WSServerProtocol>
    WSServerContext<WSServerProtocol>::WSServerContext()
        : wSServerProtocol(new WSServerProtocol()) {
        wSServerProtocol->wSServerContext = this;
    }

    template <typename WSServerProtocol>
    WSServerContext<WSServerProtocol>::~WSServerContext() {
        delete wSServerProtocol;
    }

    template <typename WSServerProtocol>
    void WSServerContext<WSServerProtocol>::messageStart(uint8_t opCode, char* message, std::size_t messageLength, uint32_t messageKey) {
        if (!closeSent) {
            WSTransmitter::messageStart(opCode, message, messageLength, messageKey);
        }
    }

    template <typename WSServerProtocol>
    void WSServerContext<WSServerProtocol>::sendFrame(char* message, std::size_t messageLength, uint32_t messageKey) {
        if (!closeSent) {
            WSTransmitter::sendFrame(message, messageLength, messageKey);
        }
    }

    template <typename WSServerProtocol>
    void WSServerContext<WSServerProtocol>::messageEnd(char* message, std::size_t messageLength, uint32_t messageKey) {
        if (!closeSent) {
            WSTransmitter::messageEnd(message, messageLength, messageKey);
        }
    }

    template <typename WSServerProtocol>
    void WSServerContext<WSServerProtocol>::message(uint8_t opCode, char* message, std::size_t messageLength, uint32_t messageKey) {
        if (!closeSent) {
            WSTransmitter::message(opCode, message, messageLength, messageKey);
        }
    }

    template <typename WSServerProtocol>
    void WSServerContext<WSServerProtocol>::onConnect() {
        wSServerProtocol->onConnect();
    }

    template <typename WSServerProtocol>
    void WSServerContext<WSServerProtocol>::onDisconnect() {
        wSServerProtocol->onDisconnect();
    }

    template <typename WSServerProtocol>
    void WSServerContext<WSServerProtocol>::receiveFromPeer(const char* junk, std::size_t junkLen) {
        WSReceiver::receive(const_cast<char*>(junk), junkLen);
    }

    template <typename WSServerProtocol>
    void WSServerContext<WSServerProtocol>::onReadError(int errnum) {
        VLOG(0) << "OnReadError: " << errnum;
    }

    template <typename WSServerProtocol>
    void WSServerContext<WSServerProtocol>::onWriteError(int errnum) {
        VLOG(0) << "OnWriteError: " << errnum;
    }

    template <typename WSServerProtocol>
    void WSServerContext<WSServerProtocol>::onMessageStart(int opCode) {
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
                wSServerProtocol->onMessageStart(opCode);
                break;
        }
    }

    template <typename WSServerProtocol>
    void WSServerContext<WSServerProtocol>::onFrameReceived(const char* junk, uint64_t junkLen) {
        if (!closeReceived && !pingReceived && !pongReceived) {
            std::size_t junkOffset = 0;

            do {
                std::size_t sendJunkLen = (junkLen - junkOffset <= SIZE_MAX) ? static_cast<std::size_t>(junkLen - junkOffset) : SIZE_MAX;
                wSServerProtocol->onFrameData(junk + junkOffset, sendJunkLen);
                junkOffset += sendJunkLen;
            } while (junkLen - junkOffset > 0);
        } else {
            // collect data for close and pong
        }
    }

    template <typename WSServerProtocol>
    void WSServerContext<WSServerProtocol>::onMessageEnd() {
        if (closeReceived) {
            closeReceived = false;
            if (closeSent) { // active close
                closeSent = false;
                VLOG(0) << "Close confirmed from peer";
            } else { // passive close
                VLOG(0) << "Close request received - replying with close";
                close();
            }
            socketConnection->getSocketProtocol()->close();
        } else if (pingReceived) {
            pingReceived = false;
            replyPong();
        } else if (pongReceived) {
            pongReceived = false;
            wSServerProtocol->onPongReceived();
        } else {
            wSServerProtocol->onMessageEnd();
        }
    }

    template <typename WSServerProtocol>
    void WSServerContext<WSServerProtocol>::onMessageError(uint16_t errnum) {
        VLOG(0) << std::endl << "Message Error";

        wSServerProtocol->onMessageError(errnum);
        close(errnum);
    }

    template <typename WSServerProtocol>
    void WSServerContext<WSServerProtocol>::onFrameReady(char* frame, uint64_t frameLength) {
        sendToPeer(frame, static_cast<std::size_t>(frameLength));
    }

    template <typename WSServerProtocol>
    void WSServerContext<WSServerProtocol>::close(uint16_t statusCode, const char* reason, std::size_t reasonLength) {
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

        message(8, closePayload, closePayloadLength);

        if (statusCode != 0) {
            delete[] closePayload;
        }

        setTimeout(CLOSE_SOCKET_TIMEOUT);

        closeSent = true;
    }

    template <typename WSServerProtocol>
    std::string WSServerContext<WSServerProtocol>::getLocalAddressAsString() const {
        return SocketProtocol::getLocalAddressAsString();
    }

    template <typename WSServerProtocol>
    std::string WSServerContext<WSServerProtocol>::getRemoteAddressAsString() const {
        return SocketProtocol::getLocalAddressAsString();
    }

    template <typename WSServerProtocol>
    WSServerProtocol* WSServerContext<WSServerProtocol>::getWSServerProtocol() {
        return wSServerProtocol;
    }

    template <typename WSServerProtocol>
    void WSServerContext<WSServerProtocol>::sendPing(char* reason, std::size_t reasonLength) {
        message(9, reason, reasonLength);
    }

    template <typename WSServerProtocol>
    void WSServerContext<WSServerProtocol>::replyPong(char* reason, std::size_t reasonLength) {
        message(10, reason, reasonLength);
    }

    template <typename WSServerProtocol>
    void WSServerContext<WSServerProtocol>::sendFrameData(uint8_t data) {
        sendToPeer(reinterpret_cast<char*>(&data), sizeof(uint8_t));
    }

    template <typename WSServerProtocol>
    void WSServerContext<WSServerProtocol>::sendFrameData(uint16_t data) {
        sendToPeer(reinterpret_cast<char*>(&data), sizeof(uint16_t));
    }

    template <typename WSServerProtocol>
    void WSServerContext<WSServerProtocol>::sendFrameData(uint32_t data) {
        sendToPeer(reinterpret_cast<char*>(&data), sizeof(uint32_t));
    }

    template <typename WSServerProtocol>
    void WSServerContext<WSServerProtocol>::sendFrameData(uint64_t data) {
        sendToPeer(reinterpret_cast<char*>(&data), sizeof(uint64_t));
    }

    template <typename WSServerProtocol>
    void WSServerContext<WSServerProtocol>::sendFrameData(char* frame, uint64_t frameLength) {
        std::size_t frameOffset = 0;

        do {
            std::size_t sendJunkLen =
                (frameLength - frameOffset <= SIZE_MAX) ? static_cast<std::size_t>(frameLength - frameOffset) : SIZE_MAX;
            sendToPeer(frame + frameOffset, sendJunkLen);
            frameOffset += sendJunkLen;
        } while (frameLength - frameOffset > 0);
    }

} // namespace web::ws::server

#endif // WEB_WS_SERVER_WSSERVERCONTEXT_HPP
