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

#ifndef WEB_WS_SOCKETCONTEXT_H
#define WEB_WS_SOCKETCONTEXT_H

#include "log/Logger.h"
#include "web/http/SocketContextUpgrade.h"
#include "web/websocket/Receiver.h"
#include "web/websocket/Transmitter.h"

namespace net::socket::stream {
    class SocketConnection;
} // namespace net::socket::stream

namespace web::http {
    template <typename RequestT, typename ResponseT>
    class SocketContextUpgradeFactory;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define CLOSE_SOCKET_TIMEOUT 10

namespace web::websocket {

    template <typename RequestT, typename ResponseT, typename SubProtocolT>
    class SocketContextUpgrade
        : public web::http::SocketContextUpgrade<RequestT, ResponseT>
        , protected web::websocket::Receiver
        , protected web::websocket::Transmitter {
    public:
        using SubProtocol = SubProtocolT;
        using Request = RequestT;
        using Response = ResponseT;

    private:
        using web::http::SocketContextUpgrade<Request, Response>::sendToPeer;
        using web::http::SocketContextUpgrade<Request, Response>::readFromPeer;
        using web::http::SocketContextUpgrade<Request, Response>::close;
        using web::http::SocketContextUpgrade<Request, Response>::setTimeout;

    protected:
        enum class Role { SERVER, CLIENT };

    protected:
        SocketContextUpgrade(net::socket::stream::SocketConnection* socketConnection,
                             web::http::SocketContextUpgradeFactory<Request, Response>* socketContextUpgradeFactory,
                             SubProtocol* subProtocol,
                             Role role)
            : web::http::SocketContextUpgrade<Request, Response>(socketConnection, socketContextUpgradeFactory)
            , Transmitter(role == Role::CLIENT)
            , subProtocol(subProtocol) {
        }

        SocketContextUpgrade() = delete;
        SocketContextUpgrade(const SocketContextUpgrade&) = delete;
        SocketContextUpgrade& operator=(const SocketContextUpgrade&) = delete;

        virtual ~SocketContextUpgrade() = default;

    public:
        void sendMessage(uint8_t opCode, const char* message, std::size_t messageLength) {
            Transmitter::sendMessage(opCode, message, messageLength);
        }

        void sendMessageStart(uint8_t opCode, const char* message, std::size_t messageLength) {
            Transmitter::sendMessageStart(opCode, message, messageLength);
        }

        void sendMessageFrame(const char* message, std::size_t messageLength) {
            Transmitter::sendMessageFrame(message, messageLength);
        }

        void sendMessageEnd(const char* message, std::size_t messageLength) {
            Transmitter::sendMessageEnd(message, messageLength);
        }

        void sendPing(const char* reason = nullptr, std::size_t reasonLength = 0) {
            sendMessage(9, reason, reasonLength);
        }

        void sendPong(const char* reason = nullptr, std::size_t reasonLength = 0) {
            sendMessage(10, reason, reasonLength);
        }

        void sendClose(uint16_t statusCode = 1000, const char* reason = nullptr, std::size_t reasonLength = 0) {
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

            sendClose(closePayload, closePayloadLength);

            if (statusCode != 0) {
                delete[] closePayload;
            }

            setTimeout(CLOSE_SOCKET_TIMEOUT);

            closeSent = true;
        }

        SubProtocol* getSubProtocol() const {
            return subProtocol;
        }

    protected:
        SubProtocol* subProtocol = nullptr;

    private:
        void sendClose(const char* message, std::size_t messageLength) {
            sendMessage(8, message, messageLength);
            close();
        }

        /* WSReceiver */
        void onMessageStart(int opCode) override {
            opCodeReceived = opCode;

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

        void onFrameReceived(const char* junk, uint64_t junkLen) override {
            switch (opCodeReceived) {
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
            switch (opCodeReceived) {
                case OpCode::CLOSE:
                    if (closeSent) { // active close
                        closeSent = false;
                        VLOG(0) << "Close confirmed from peer";
                    } else { // passive close
                        VLOG(0) << "Close request received - replying with close";
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

        ssize_t readFrameData(char* junk, std::size_t junkLen) override {
            return readFromPeer(junk, junkLen);
        }

        /* Callbacks (API) socketConnection -> WSProtocol */
        void onConnected() override {
            subProtocol->onConnected();
        }

        void onDisconnected() override {
            subProtocol->onDisconnected();
        }

        /* Facade to SocketProtocol used from WSTransmitter */
        void sendFrameData(uint8_t data) override {
            if (!closeSent) {
                sendToPeer(reinterpret_cast<char*>(&data), sizeof(uint8_t));
            }
        }

        void sendFrameData(uint16_t data) override {
            if (!closeSent) {
                uint16_t sendData = htobe16(data);
                sendToPeer(reinterpret_cast<char*>(&sendData), sizeof(uint16_t));
            }
        }

        void sendFrameData(uint32_t data) override {
            if (!closeSent) {
                uint32_t sendData = htobe32(data);
                sendToPeer(reinterpret_cast<char*>(&sendData), sizeof(uint32_t));
            }
        }

        void sendFrameData(uint64_t data) override {
            if (!closeSent) {
                uint64_t sendData = htobe64(data);
                sendToPeer(reinterpret_cast<char*>(&sendData), sizeof(uint64_t));
            }
        }

        void sendFrameData(const char* frame, uint64_t frameLength) override {
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

        /* SocketProtocol */
        void onReceiveFromPeer() override {
            Receiver::receive();
        }

        void onReadError(int errnum) override {
            if (errnum != 0) {
                PLOG(INFO) << "OnReadError:";
            }
        }

        void onWriteError(int errnum) override {
            if (errnum != 0) {
                PLOG(INFO) << "OnWriteError:";
            }
        }

        void stop() override {
            Receiver::stop();
        }

        bool closeSent = false;

        int opCodeReceived = 0;

        std::string pongCloseData;

        enum OpCode { CLOSE = 0x08, PING = 0x09, PONG = 0x0A };
    };

} // namespace web::websocket

#endif // WEB_WS_SOCKETCONTEXT_H
