/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#include "iot/mqtt/SubProtocol.h"
#include "log/Logger.h"
#include "utils/system/signal.h"
#include "web/websocket/SubProtocolContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cstring>
#include <iomanip>
#include <ostream>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt {

    template <typename WSSubProtocolRole>
    SubProtocol<WSSubProtocolRole>::SubProtocol(web::websocket::SubProtocolContext* subProtocolContext,
                                                const std::string& name,
                                                iot::mqtt::Mqtt* mqtt)
        : WSSubProtocolRole(subProtocolContext, name, 0)
        , iot::mqtt::MqttContext(mqtt)
        , onReceivedFromPeerEvent([this]([[maybe_unused]] const utils::Timeval& currentTime) -> void {
            iot::mqtt::MqttContext::onReceivedFromPeer();

            if (size > 0) {
                onReceivedFromPeerEvent.span();
            } else {
                buffer.clear();
                cursor = 0;
            }
        }) {
    }

    template <typename WSSubProtocolRole>
    std::size_t SubProtocol<WSSubProtocolRole>::recv(char* chunk, std::size_t chunklen) {
        std::size_t maxReturn = std::min(chunklen, size);

        std::copy(buffer.data() + cursor, buffer.data() + cursor + maxReturn, chunk);

        cursor += maxReturn;
        size -= maxReturn;

        return maxReturn;
    }

    template <typename WSSubProtocolRole>
    void SubProtocol<WSSubProtocolRole>::send(const char* chunk, std::size_t chunklen) {
        WSSubProtocolRole::sendMessage(chunk, chunklen);
    }

    template <typename WSSubProtocolRole>
    void SubProtocol<WSSubProtocolRole>::end([[maybe_unused]] bool fatal) {
        WSSubProtocolRole::sendClose();
    }

    template <typename WSSubProtocolRole>
    void SubProtocol<WSSubProtocolRole>::close() {
        WSSubProtocolRole::sendClose();
    }

    template <typename WSSubProtocolRole>
    void SubProtocol<WSSubProtocolRole>::onConnected() {
        LOG(INFO) << "WSMQTT: connected:";
        iot::mqtt::MqttContext::onConnected();
    }

    template <typename WSSubProtocolRole>
    void SubProtocol<WSSubProtocolRole>::onMessageStart(int opCode) {
        if (opCode == web::websocket::SubProtocolContext::OpCode::TEXT) {
            LOG(DEBUG) << "WSMQTT: Wrong Opcode: " << opCode;
            this->end(true);
        } else {
            LOG(TRACE) << "WSMQTT: Message START: " << opCode;
        }
    }

    template <typename WSSubProtocolRole>
    void SubProtocol<WSSubProtocolRole>::onMessageData(const char* chunk, std::size_t chunkLen) {
        data += std::string(chunk, chunkLen);

        std::stringstream ss;

        unsigned long i = 0;
        for (const char ch : std::string(chunk, chunkLen)) {
            if (i != 0 && i % 8 == 0 && i != data.size()) {
                ss << std::endl;
                ss << "                                                    ";
            }
            ++i;
            ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(static_cast<uint8_t>(ch)) << " ";
        }

        LOG(TRACE) << "WebSocket: Frame Data:\n"
                   << "                                                    " << ss.str();
    }

    template <typename WSSubProtocolRole>
    void SubProtocol<WSSubProtocolRole>::onMessageEnd() {
        LOG(TRACE) << "WSMQTT: Message END";

        buffer.insert(buffer.end(), data.begin(), data.end());
        size += data.size();
        data.clear();

        iot::mqtt::MqttContext::onReceivedFromPeer();

        if (size > 0) {
            onReceivedFromPeerEvent.span();
        } else {
            buffer.clear();
            cursor = 0;
            size = 0;
        }
    }

    template <typename WSSubProtocolRole>
    void SubProtocol<WSSubProtocolRole>::onMessageError(uint16_t errnum) {
        LOG(DEBUG) << "WSMQTT: Message error: " << errnum;
    }

    template <typename WSSubProtocolRole>
    void SubProtocol<WSSubProtocolRole>::onDisconnected() {
        iot::mqtt::MqttContext::onDisconnected();
        LOG(INFO) << "WSMQTT: disconnected:";
    }

    template <typename WSSubProtocolRole>
    bool SubProtocol<WSSubProtocolRole>::onSignal(int sig) {
        bool ret = iot::mqtt::MqttContext::onSignal(sig);
        LOG(INFO) << "WSMQTT: exit due to '" << strsignal(sig) << "' (SIG" << utils::system::sigabbrev_np(sig) << " = " << sig << ")";

        this->sendClose();

        return ret;
    }

    template <typename WSSubProtocolRole>
    core::socket::stream::SocketConnection* SubProtocol<WSSubProtocolRole>::getSocketConnection() {
        return WSSubProtocolRole::subProtocolContext->getSocketConnection();
    }

} // namespace iot::mqtt
