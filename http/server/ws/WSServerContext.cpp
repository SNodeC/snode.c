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

#include "http/server/ws/WSServerContext.h"

#include "net/socket/stream/SocketConnectionBase.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstring>
#include <iostream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace http::websocket {

    void WSServerContext::parseReceivedData(const char* junk, std::size_t junkLen) {
        WSReceiver::receive(const_cast<char*>(junk), junkLen);
    }

    void WSServerContext::onReadError([[maybe_unused]] int errnum) {
    }

    void WSServerContext::onWriteError([[maybe_unused]] int errnum) {
    }

    void WSServerContext::onMessageStart(int opCode) {
        std::cout << "Message Start - OpCode: " << opCode << std::endl;

        closeReceived = (opCode == 8);
        pingReceived = (opCode == 9);
        pongReceived = (opCode == 10);
    }

    void WSServerContext::onMessageData(char* junk, uint64_t junkLen) {
        if (!closeReceived && !pingReceived && !pongReceived) {
            std::cout << "Data: " << std::string(junk, static_cast<std::size_t>(junkLen));
            if (fin) {
                std::cout << std::endl;
            }
        } else {
            // collect data for close and pong
        }
    }

    void WSServerContext::onMessageEnd() {
        if (closeReceived) {
            closeReceived = false;
            if (closeSent) {
                closeSent = false;
                std::cout << "Closed" << std::endl;
            } else {
                close();
                std::cout << "Close requested" << std::endl;
            }
            socketConnection->close();
        } else if (pingReceived) {
            pingReceived = false;
            pong();
        } else if (pongReceived) {
            pongReceived = false;
            /* Propagate connection alive to application */
        } else {
            /* Propagate Message back to application */
            message(1, "Hallo zurück", strlen("Hallo zurück"));
            std::cout << "Message End" << std::endl;
            //            close(1000);
        }
    }

    void WSServerContext::onError(uint16_t errnum) {
        std::cout << std::endl << "Message Error" << std::endl;
        close(errnum);
    }

    void WSServerContext::onFrameReady(char* frame, uint64_t frameLength) {
        socketConnection->enqueue(frame, static_cast<std::size_t>(frameLength));
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

    void WSServerContext::ping(const char* reason, std::size_t reasonLength) {
        message(9, reason, reasonLength);
    }

    void WSServerContext::pong(const char* reason, std::size_t reasonLength) {
        message(10, reason, reasonLength);
    }

} // namespace http::websocket
