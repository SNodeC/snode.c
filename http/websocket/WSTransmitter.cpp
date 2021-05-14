/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#include "http/websocket/WSTransmitter.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <endian.h>
#include <iomanip>
#include <iostream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace http::websocket {

    WSTransmitter::WSTransmitter() {
    }

    void WSTransmitter::messageStart(uint8_t opCode, const char* message, std::size_t messageLength, uint32_t messageKey) {
        send(false, opCode, message, messageLength, messageKey);
    }

    void WSTransmitter::message(const char* message, std::size_t messageLength, uint32_t messageKey) {
        send(false, 0, message, messageLength, messageKey);
    }

    void WSTransmitter::messageEnd(const char* message, std::size_t messageLength, uint32_t messageKey) {
        send(true, 0, message, messageLength, messageKey);
    }

    void WSTransmitter::message(uint8_t opCode, const char* message, std::size_t messageLength, uint32_t messageKey) {
        send(true, opCode, message, messageLength, messageKey);
    }

    void WSTransmitter::send(bool end, uint8_t opCode, const char* message, std::size_t messageLength, uint32_t messageKey) {
        std::size_t messageOffset = 0;

        while (messageLength - messageOffset > 0) {
            std::size_t sendMessageLength =
                (messageLength - messageOffset <= WSPAYLOADLENGTH) ? messageLength - messageOffset : WSPAYLOADLENGTH;
            bool fin = sendMessageLength == messageLength - messageOffset;
            sendFrame(fin && end, opCode, messageKey, message + messageOffset, sendMessageLength);
            messageOffset += sendMessageLength;
            opCode = 0; // continuation
        }
    }

    void WSTransmitter::sendFrame(bool fin, uint8_t opCode, uint32_t maskingKey, const char* payload, uint64_t payloadLength) {
        uint64_t frameLength = 2 + payloadLength;
        uint8_t opCodeOffset = 0;
        uint8_t lengthOffset = 1;
        uint8_t eLengthOffset = 2;
        uint8_t maskingKeyOffset = 2;
        uint8_t payloadOffset = 2;

        if (maskingKey > 0) {
            frameLength += 4;
            payloadOffset += 4;
        }

        char* frame = nullptr;
        uint64_t length = 0;

        if (payloadLength < 126) {
            frame = new char[frameLength];
            length = payloadLength;
        } else if (payloadLength < 0x10000) {
            frameLength += 2;
            maskingKeyOffset += 2;
            payloadOffset += 2;
            frame = new char[frameLength];
            *reinterpret_cast<uint16_t*>(frame + eLengthOffset) = htobe16(*reinterpret_cast<uint16_t*>(&payloadLength));
            length = 126;
        } else {
            frameLength += 8;
            maskingKeyOffset += 8;
            payloadOffset += 8;
            frame = new char[frameLength];
            *reinterpret_cast<uint64_t*>(frame + eLengthOffset) = htobe64(*reinterpret_cast<uint64_t*>(&payloadLength));
            length = 127;
        }

        if (maskingKey > 0) {
            *reinterpret_cast<uint32_t*>(frame + maskingKeyOffset) = htobe32(maskingKey);
        }

        *reinterpret_cast<uint8_t*>(frame + opCodeOffset) = static_cast<uint8_t>((fin ? 0b10000000 : 0) | opCode);
        *reinterpret_cast<uint8_t*>(frame + lengthOffset) = static_cast<uint8_t>(((maskingKey > 0) ? 0b10000000 : 0) | length);

        MaskingKeyAsArray maskingKeyAsArray = {.value = htobe32(maskingKey)};

        for (uint64_t i = 0; i < payloadLength; i++) {
            *(frame + payloadOffset + i) = *(payload + i) ^ *(maskingKeyAsArray.array + i % 4);
        }

        onFrameReady(frame, frameLength);

        delete[] frame;
    }

    void WSTransmitter::dumpFrame(char* frame, uint64_t frameLength) {
        for (std::size_t i = 0; i < frameLength; i++) {
            std::cout << std::setfill('0') << std::setw(2) << std::hex << (unsigned int) (unsigned char) frame[i] << " ";

            if ((i + 1) % 4 == 0) {
                std::cout << std::endl;
            }
        }
        std::cout << std::endl;
    }

} // namespace http::websocket
