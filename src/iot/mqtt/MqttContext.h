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

#ifndef IOT_MQTT_MQTTCONTEXT_H
#define IOT_MQTT_MQTTCONTEXT_H

namespace core::socket {
    class SocketConnection;
}

namespace iot::mqtt {
    class Mqtt;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h"

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt {

    class MqttContext {
    public:
        explicit MqttContext(Mqtt* mqtt);
        MqttContext(const MqttContext&) = default;

        MqttContext& operator=(const MqttContext&) = default;

        virtual ~MqttContext();

        virtual std::size_t receive(char* junk, std::size_t junklen) = 0;
        virtual void send(const char* junk, std::size_t junklen) = 0;

        virtual core::socket::SocketConnection* getSocketConnection() = 0;

        virtual void end(bool fatal = false) = 0;
        virtual void kill() = 0;

        void onConnected();
        std::size_t onProcess();
        void onDisconnected();
        void onExit();

    protected:
        Mqtt* mqtt;
    };

} // namespace iot::mqtt

#endif // IOT_MQTT_MQTTCONTEXT_H
