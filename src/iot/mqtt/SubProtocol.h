/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

namespace web::websocket {
    class SubProtocolContext;
}

namespace utils {
    class Timeval;
}

namespace iot::mqtt::server {
    class Mqtt;
}

#include "core/EventReceiver.h"
#include "core/socket/SocketConnection.h"
#include "iot/mqtt/MqttContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt {

    class OnDataEvent : public core::EventReceiver {
    public:
        explicit OnDataEvent(const std::function<void(const utils::Timeval& currentTime)>& event)
            : core::EventReceiver("WS-OnData")
            , event(event) {
        }

    private:
        void onEvent(const utils::Timeval& currentTime) override {
            event(currentTime);
        }

        std::function<void(const utils::Timeval& currentTime)> event;
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

        std::size_t receive(char* junk, std::size_t junklen) override;
        void send(const char* junk, std::size_t junklen) override;

        void end(bool fatal = false) override;
        void kill() override;

    private:
        void onConnected() override;
        void onMessageStart(int opCode) override;
        void onMessageData(const char* junk, std::size_t junkLen) override;
        void onMessageEnd() override;
        void onMessageError(uint16_t errnum) override;
        void onDisconnected() override;
        void onExit() override;

        core::socket::SocketConnection* getSocketConnection() override;

        OnDataEvent onData;

        std::string data;
        std::vector<char> buffer;
        std::size_t cursor = 0;
        std::size_t size = 0;
    };

} // namespace iot::mqtt

#endif // IOT_MQTT_MQTTSUBPROTOCOL_H
