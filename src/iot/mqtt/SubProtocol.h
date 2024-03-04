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

#ifndef IOT_MQTT_MQTTSUBPROTOCOL_H
#define IOT_MQTT_MQTTSUBPROTOCOL_H

namespace core::socket::stream {
    class SocketConnection;
}

namespace web::websocket {
    class SubProtocolContext;
}

namespace utils {
    class Timeval;
}

namespace iot::mqtt {
    class Mqtt;
}

#include "core/EventReceiver.h"
#include "iot/mqtt/MqttContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt {

    class OnReceivedFromPeerEvent : public core::EventReceiver {
    public:
        explicit OnReceivedFromPeerEvent(const std::function<void(const utils::Timeval&)>& onReceivedFromPeer);

    private:
        void onEvent(const utils::Timeval& currentTime) override;

        std::function<void(const utils::Timeval&)> onReceivedFromPeer;
    };

    template <typename WSSubProtocolRoleT>
    class SubProtocol
        : public WSSubProtocolRoleT
        , private iot::mqtt::MqttContext {
    private:
        using WSSubProtocolRole = WSSubProtocolRoleT;

    public:
        SubProtocol(web::websocket::SubProtocolContext* subProtocolContext, const std::string& name, iot::mqtt::Mqtt* mqtt);
        ~SubProtocol() override = default;

        std::size_t recv(char* chunk, std::size_t chunklen) override;
        void send(const char* chunk, std::size_t chunklen) override;

        void end(bool fatal = false) override;
        void close() override;

    private:
        void onConnected() override;
        void onMessageStart(int opCode) override;
        void onMessageData(const char* chunk, std::size_t chunkLen) override;
        void onMessageEnd() override;
        void onMessageError(uint16_t errnum) override;
        void onDisconnected() override;
        bool onSignal(int sig) override;

        core::socket::stream::SocketConnection* getSocketConnection() override;

        OnReceivedFromPeerEvent onReceivedFromPeerEvent;

        std::string data;
        std::vector<char> buffer;
        std::size_t cursor = 0;
        std::size_t size = 0;
    };

} // namespace iot::mqtt

#endif // IOT_MQTT_MQTTSUBPROTOCOL_H
