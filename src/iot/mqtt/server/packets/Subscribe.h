/*
 * SNode.C - A Slim Toolkit for Network Communication
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef IOT_MQTT_SERVER_PACKETS_DESERIALIZER_SUBSCRIBE_H
#define IOT_MQTT_SERVER_PACKETS_DESERIALIZER_SUBSCRIBE_H

#include "iot/mqtt/packets/Subscribe.h"                // IWYU pragma: export
#include "iot/mqtt/server/ControlPacketDeserializer.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt::server::packets {

    class Subscribe
        : public iot::mqtt::packets::Subscribe
        , public iot::mqtt::server::ControlPacketDeserializer {
    public:
        Subscribe(uint32_t remainingLength, uint8_t flags);

    private:
        std::size_t deserializeVP(iot::mqtt::MqttContext* mqttContext) override;
        void deliverPacket(iot::mqtt::server::Mqtt* mqtt) override;

        iot::mqtt::types::String topic;
        iot::mqtt::types::UInt8 qoS;

        int state = 0;
    };

} // namespace iot::mqtt::server::packets

#endif // IOT_MQTT_SERVER_PACKETS_DESERIALIZER_SUBSCRIBE_H
