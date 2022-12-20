/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "iot/mqtt/MqttSubProtocol.h" // IWYU pragma: export
#include "iot/mqtt/server/Mqtt.h"
#include "log/Logger.h"
#include "web/websocket/SubProtocolContext.h"

//

#include <algorithm>
#include <iomanip>
#include <ostream>

#define PING_INTERVAL 0

namespace iot::mqtt {

    template <typename SubProtocol>
    MqttSubProtocol<SubProtocol>::MqttSubProtocol(web::websocket::SubProtocolContext* subProtocolContext,
                                                  const std::string& name,
                                                  iot::mqtt::Mqtt* mqtt)
        : SubProtocol(subProtocolContext, name, PING_INTERVAL)
        , iot::mqtt::MqttContext(mqtt)
        , onReceivedFromPeerEvent([this]() -> void {
            iot::mqtt::MqttContext::onReceiveFromPeer();
        }) {
    }

    template <typename SubProtocol>
    std::size_t MqttSubProtocol<SubProtocol>::receive(char* junk, std::size_t junklen) {
        std::size_t maxReturn = std::min(junklen, size);

        std::copy(buffer.data() + cursor, buffer.data() + cursor + maxReturn, junk);

        cursor += maxReturn;
        size -= maxReturn;

        if (size > 0) {
            onReceivedFromPeerEvent.publish();
        } else {
            buffer.clear();
            cursor = 0;
            size = 0;
        }

        return maxReturn;
    }

    template <typename SubProtocol>
    void MqttSubProtocol<SubProtocol>::send(const char* junk, std::size_t junklen) {
        SubProtocol::sendMessage(junk, junklen);
    }

    template <typename SubProtocol>
    void MqttSubProtocol<SubProtocol>::end([[maybe_unused]] bool fatal) {
        SubProtocol::sendClose();
    }

    template <typename SubProtocol>
    void MqttSubProtocol<SubProtocol>::kill() {
        SubProtocol::sendClose();
    }

    template <typename SubProtocol>
    void MqttSubProtocol<SubProtocol>::onConnected() {
        VLOG(0) << "WS: Mqtt connected:";
        iot::mqtt::MqttContext::onConnected();
    }

    template <typename SubProtocol>
    void MqttSubProtocol<SubProtocol>::onMessageStart(int opCode) {
        if (opCode == 1) {
            VLOG(0) << "WS: Wrong Opcode: " << opCode;
            this->end(true);
        }
    }

    template <typename SubProtocol>
    void MqttSubProtocol<SubProtocol>::onMessageData(const char* junk, std::size_t junkLen) {
        data += std::string(junk, junkLen);
    }

    template <typename SubProtocol>
    void MqttSubProtocol<SubProtocol>::onMessageEnd() {
        std::stringstream ss;

        ss << "WS-Message: ";
        unsigned long i = 0;
        for (char ch : data) {
            if (i != 0 && i % 8 == 0 && i != data.size()) {
                ss << std::endl;
                ss << "                                            ";
            }
            ++i;
            ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(static_cast<uint8_t>(ch))
               << " "; // << " | ";
        }
        VLOG(0) << ss.str();

        buffer.insert(buffer.end(), data.begin(), data.end());
        size += data.size();
        data.clear();

        if (buffer.size() > 0) {
            iot::mqtt::MqttContext::onReceiveFromPeer();
        }
    }

    template <typename SubProtocol>
    void MqttSubProtocol<SubProtocol>::onMessageError(uint16_t errnum) {
        VLOG(0) << "Message error: " << errnum;
    }

    template <typename SubProtocol>
    void MqttSubProtocol<SubProtocol>::onDisconnected() {
        VLOG(0) << "MQTT disconnected:";
        iot::mqtt::MqttContext::onDisconnected();
    }

    template <typename SubProtocol>
    void MqttSubProtocol<SubProtocol>::onExit() {
        VLOG(0) << "MQTT exit:";
        iot::mqtt::MqttContext::onExit();
    }

    template <typename SubProtocol>
    core::socket::SocketConnection* MqttSubProtocol<SubProtocol>::getSocketConnection() {
        return SubProtocol::subProtocolContext->getSocketConnection();
    }

} // namespace iot::mqtt
