/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include "ControlPacketDeserializer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt {

    ControlPacketDeserializer::ControlPacketDeserializer(uint32_t remainingLength, uint8_t flags, uint8_t mustFlags)
        : error(flags != mustFlags)
        , remainingLength(remainingLength) {
    }

    ControlPacketDeserializer::~ControlPacketDeserializer() {
    }

    std::size_t ControlPacketDeserializer::deserialize(iot::mqtt::MqttContext* mqttContext) {
        const std::size_t currentConsumed = deserializeVP(mqttContext);
        consumed += currentConsumed;

        error = (complete && consumed != this->getRemainingLength()) || (!complete && consumed >= this->getRemainingLength());

        return currentConsumed;
    }

    uint32_t ControlPacketDeserializer::getRemainingLength() const {
        return remainingLength;
    }

    bool ControlPacketDeserializer::isComplete() const {
        return complete;
    }

    bool ControlPacketDeserializer::isError() const {
        return error;
    }

    std::size_t ControlPacketDeserializer::getConsumed() const {
        return consumed;
    }

} // namespace iot::mqtt
