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

#include "web/server/ws/WSServerContext.h"

#include "log/Logger.h"
#include "net/socket/stream/SocketConnectionBase.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstring>  // for memcpy
#include <endian.h> // for htobe16
#include <ostream>  // for endl

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket {

    WSServerContext::WSServerContext(
        const std::function<void(WSServerContext* wSServerContext, int opCode)>& _onMessageStart,
        const std::function<void(WSServerContext* wSServerContext, const char* junk, std::size_t junkLen)>& _onFrameData,
        const std::function<void(WSServerContext* wSServerContext)>& _onMessageEnd)
        : _onMessageStart(_onMessageStart)
        , _onFrameData(_onFrameData)
        , _onMessageEnd(_onMessageEnd) {
    }

    void WSServerContext::messageStart(uint8_t opCode, const char* message, std::size_t messageLength, uint32_t messageKey) {
        if (!closeSent) {
            WSTransmitter::messageStart(opCode, message, messageLength, messageKey);
        }
    }

    void WSServerContext::sendFrame(const char* message, std::size_t messageLength, uint32_t messageKey) {
        if (!closeSent) {
            WSTransmitter::sendFrame(message, messageLength, messageKey);
        }
    }

    void WSServerContext::messageEnd(const char* message, std::size_t messageLength, uint32_t messageKey) {
        if (!closeSent) {
            WSTransmitter::messageEnd(message, messageLength, messageKey);
        }
    }

    void WSServerContext::message(uint8_t opCode, const char* message, std::size_t messageLength, uint32_t messageKey) {
        if (!closeSent) {
            WSTransmitter::message(opCode, message, messageLength, messageKey);
        }
    }

    void WSServerContext::receiveFromPeer(const char* junk, std::size_t junkLen) {
        WSReceiver::receive(const_cast<char*>(junk), junkLen);
    }

    void WSServerContext::onReadError(int errnum) {
        VLOG(0) << "OnReadError: " << errnum;
    }

    void WSServerContext::onWriteError(int errnum) {
        VLOG(0) << "OnWriteError: " << errnum;
    }

    void WSServerContext::onMessageStart(int opCode) {
        closeReceived = (opCode == 0x08);
        pingReceived = (opCode == 0x09);
        pongReceived = (opCode == 0x0A);

        switch (opCode) {
            case 0x08:
                closeReceived = true;
                VLOG(0) << "Close requested";
                break;
            case 0x09:
                pingReceived = true;
                VLOG(0) << "Ping received";
                break;
            case 0x0A:
                pongReceived = true;
                VLOG(0) << "Pong received";
                break;
            default:
                _onMessageStart(this, opCode);
                break;
        }
    }

    void WSServerContext::onFrameData(const char* junk, uint64_t junkLen) {
        if (!closeReceived && !pingReceived && !pongReceived) {
            std::size_t junkOffset = 0;

            do {
                std::size_t sendJunkLen = (junkLen - junkOffset <= SIZE_MAX) ? static_cast<std::size_t>(junkLen - junkOffset) : SIZE_MAX;
                _onFrameData(this, junk + junkOffset, sendJunkLen);
                junkOffset += sendJunkLen;
            } while (junkLen - junkOffset > 0);
        } else {
            // collect data for close and pong
        }
    }

    void WSServerContext::onMessageEnd() {
        if (closeReceived) {
            closeReceived = false;
            if (closeSent) { // active close
                closeSent = false;
                VLOG(0) << "Request close";
            } else { // passive close
                VLOG(0) << "Closed";
                close();
            }
            socketConnection->getSocketProtocol()->close();
        } else if (pingReceived) {
            pingReceived = false;
            sendPong();
        } else if (pongReceived) {
            pongReceived = false;
            /* Propagate connection alive to application? */
        } else {
            _onMessageEnd(this);
        }
    }

    void WSServerContext::onError(uint16_t errnum) {
        VLOG(0) << std::endl << "Message Error";
        close(errnum);
    }

    void WSServerContext::onFrameReady(char* frame, uint64_t frameLength) {
        sendToPeer(frame, static_cast<std::size_t>(frameLength));
    }

    void WSServerContext::close(uint16_t statusCode, const char* reason, std::size_t reasonLength) {
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

        closeSent = true;
    }

    void WSServerContext::sendPing(const char* reason, std::size_t reasonLength) {
        message(9, reason, reasonLength);
    }

    void WSServerContext::sendPong(const char* reason, std::size_t reasonLength) {
        message(10, reason, reasonLength);
    }

} // namespace web::websocket
