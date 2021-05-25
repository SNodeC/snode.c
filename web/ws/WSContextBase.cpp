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

#include "web/ws/WSContextBase.h"

#include "log/Logger.h"
#include "net/socket/stream/SocketConnectionBase.h"
#include "web/ws/WSProtocol.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws {

    WSContextBase::WSContextBase(web::ws::WSProtocol* wSProtocol)
        : wSProtocol(wSProtocol) {
    }

    void WSContextBase::onMessageStart(int opCode) {
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
                wSProtocol->onMessageStart(opCode);
                break;
        }
    }

    void WSContextBase::onFrameReceived(const char* junk, uint64_t junkLen) {
        if (!closeReceived && !pingReceived && !pongReceived) {
            std::size_t junkOffset = 0;

            do {
                std::size_t sendJunkLen = (junkLen - junkOffset <= SIZE_MAX) ? static_cast<std::size_t>(junkLen - junkOffset) : SIZE_MAX;
                wSProtocol->onMessageData(junk + junkOffset, sendJunkLen);
                junkOffset += sendJunkLen;
            } while (junkLen - junkOffset > 0);
        } else {
            // collect data for close and pong
        }
    }

    void WSContextBase::onMessageEnd() {
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
            wSProtocol->onPongReceived();
        } else {
            wSProtocol->onMessageEnd();
        }
    }

    void WSContextBase::onPongReceived() {
        wSProtocol->onPongReceived();
    }

    void WSContextBase::onMessageError(uint16_t errnum) {
        VLOG(0) << "Message Error";

        wSProtocol->onMessageError(errnum);
        close(errnum, "hallo", std::string("hallo").length());
    }

    void WSContextBase::onProtocolConnect() {
        wSProtocol->onProtocolConnect();
    }

    void WSContextBase::onProtocolDisconnect() {
        wSProtocol->onProtocolDisconnect();
    }

    void WSContextBase::sendFrameData(uint8_t data) {
        sendToPeer(reinterpret_cast<char*>(&data), sizeof(uint8_t));
    }

    void WSContextBase::sendFrameData(uint16_t data) {
        sendToPeer(reinterpret_cast<char*>(&data), sizeof(uint16_t));
    }

    void WSContextBase::sendFrameData(uint32_t data) {
        sendToPeer(reinterpret_cast<char*>(&data), sizeof(uint32_t));
    }

    void WSContextBase::sendFrameData(uint64_t data) {
        sendToPeer(reinterpret_cast<char*>(&data), sizeof(uint64_t));
    }

    void WSContextBase::sendFrameData(const char* frame, uint64_t frameLength) {
        std::size_t frameOffset = 0;

        do {
            std::size_t sendJunkLen =
                (frameLength - frameOffset <= SIZE_MAX) ? static_cast<std::size_t>(frameLength - frameOffset) : SIZE_MAX;
            sendToPeer(frame + frameOffset, sendJunkLen);
            frameOffset += sendJunkLen;
        } while (frameLength - frameOffset > 0);
    }

    void WSContextBase::receiveFromPeer(const char* junk, std::size_t junkLen) {
        WSReceiver::receive(const_cast<char*>(junk), junkLen);
    }

    void WSContextBase::onReadError(int errnum) {
        VLOG(0) << "OnReadError: " << errnum;
    }

    void WSContextBase::onWriteError(int errnum) {
        VLOG(0) << "OnWriteError: " << errnum;
    }

    void WSContextBase::setWSServerContext(WSContextBase* wSServerContext) {
        wSProtocol->setWSServerContext(wSServerContext);
    }

    std::string WSContextBase::getLocalAddressAsString() const {
        return SocketProtocol::getLocalAddressAsString();
    }

    std::string WSContextBase::getRemoteAddressAsString() const {
        return SocketProtocol::getLocalAddressAsString();
    }

} // namespace web::ws
