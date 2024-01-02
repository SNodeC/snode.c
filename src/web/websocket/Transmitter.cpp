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

#include "web/websocket/Transmitter.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <endian.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

constexpr int WSMAXFRAMEPAYLOADLENGTH = 1024;

namespace web::websocket {

    Transmitter::Transmitter(bool masking)
        : masking(masking) {
    }

    Transmitter::~Transmitter() {
    }

    void Transmitter::sendMessage(uint8_t opCode, const char* message, std::size_t messageLength) {
        send(true, opCode, message, messageLength);
    }

    void Transmitter::sendMessageStart(uint8_t opCode, const char* message, std::size_t messageLength) {
        send(false, opCode, message, messageLength);
    }

    void Transmitter::sendMessageFrame(const char* message, std::size_t messageLength) {
        send(false, 0, message, messageLength);
    }

    void Transmitter::sendMessageEnd(const char* message, std::size_t messageLength) {
        send(true, 0, message, messageLength);
    }

    void Transmitter::send(bool end, uint8_t opCode, const char* message, std::size_t messageLength) {
        std::size_t messageOffset = 0;

        do {
            const std::size_t sendFrameLength =
                (messageLength - messageOffset <= WSMAXFRAMEPAYLOADLENGTH) ? messageLength - messageOffset : WSMAXFRAMEPAYLOADLENGTH;

            const bool fin = (sendFrameLength == messageLength - messageOffset) && end; // cppcheck-suppress knownConditionTrueFalse

            sendFrame(fin, opCode, message + messageOffset, sendFrameLength);

            messageOffset += sendFrameLength;

            opCode = 0; // continuation
        } while (messageLength - messageOffset > 0);
    }

    void Transmitter::sendFrame(bool fin, uint8_t opCode, const char* payload, uint64_t payloadLength) {
        uint64_t length = 0;

        if (payloadLength < 126) {
            length = payloadLength;
        } else if (payloadLength < 0x10000) {
            length = 126;
        } else {
            length = 127;
        }

        char header[2];
        header[0] = static_cast<char>((fin ? 0b10000000 : 0) | opCode);

        header[1] = static_cast<char>((masking ? 0b10000000 : 0) | length);

        sendFrameData(header, 2);

        switch (length) {
            case 126:
                sendFrameData(static_cast<uint16_t>(payloadLength));
                break;
            case 127:
                sendFrameData(payloadLength);
                break;
        }

        union MaskingKey {
            uint32_t keyAsValue;
            char keyAsBytes[4];
        };

        MaskingKey maskingKeyAsArray = {.keyAsValue = distribution(randomDevice)};

        if (masking) {
            sendFrameData(htobe32(maskingKeyAsArray.keyAsValue));

            for (uint64_t i = 0; i < payloadLength; i++) {
                *(const_cast<char*>(payload) + i) = static_cast<char>(*(payload + i) ^ *(maskingKeyAsArray.keyAsBytes + i % 4));
            }
        }

        sendFrameData(payload, payloadLength);

        if (masking) {
            for (uint64_t i = 0; i < payloadLength; i++) {
                *(const_cast<char*>(payload) + i) = static_cast<char>(*(payload + i) ^ *(maskingKeyAsArray.keyAsBytes + i % 4));
            }
        }
    }

} // namespace web::websocket
