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

#include "web/ws/WSTransmitter.h"

#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <endian.h> // for htobe32, htobe16, htobe64
#include <iomanip>  // for operator<<, setfill, setw
#include <memory>   // for allocator
#include <sstream>  // for stringstream, basic_ostream, operator<<, bas...

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define WSMAXFRAMEPAYLOADLENGTH 1024

namespace web::ws {

    WSTransmitter::WSTransmitter(bool masking)
        : masking(masking) {
    }

    void WSTransmitter::sendMessageStart(uint8_t opCode, const char* message, std::size_t messageLength) {
        send(false, opCode, message, messageLength);
    }

    void WSTransmitter::sendMessageFrame(const char* message, std::size_t messageLength) {
        send(false, 0, message, messageLength);
    }

    void WSTransmitter::sendMessageEnd(const char* message, std::size_t messageLength) {
        send(true, 0, message, messageLength);
    }

    void WSTransmitter::sendMessage(uint8_t opCode, const char* message, std::size_t messageLength) {
        send(true, opCode, message, messageLength);
    }

    void WSTransmitter::send(bool end, uint8_t opCode, const char* message, std::size_t messageLength) {
        std::size_t messageOffset = 0;

        do {
            std::size_t sendMessageLength =
                (messageLength - messageOffset <= WSMAXFRAMEPAYLOADLENGTH) ? messageLength - messageOffset : WSMAXFRAMEPAYLOADLENGTH;

            bool fin = sendMessageLength == messageLength - messageOffset;

            sendFrame(fin && end, opCode, message + messageOffset, sendMessageLength);

            messageOffset += sendMessageLength;

            opCode = 0; // continuation
        } while (messageLength - messageOffset > 0);
    }

    /*
        void WSTransmitter::sendFrame1(bool fin, uint8_t opCode, char* payload, uint64_t payloadLength, uint32_t maskingKey, ) {
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
                *reinterpret_cast<uint16_t*>(frame + eLengthOffset) = htobe16(static_cast<uint16_t>(payloadLength));
                length = 126;
            } else {
                frameLength += 8;
                maskingKeyOffset += 8;
                payloadOffset += 8;
                frame = new char[frameLength];
                *reinterpret_cast<uint64_t*>(frame + eLengthOffset) = htobe64(payloadLength);
                length = 127;
            }

            if (maskingKey > 0) {
                *reinterpret_cast<uint32_t*>(frame + maskingKeyOffset) = htobe32(maskingKey);
            }

            *reinterpret_cast<uint8_t*>(frame + opCodeOffset) = static_cast<uint8_t>((fin ? 0b10000000 : 0) | opCode);
            *reinterpret_cast<uint8_t*>(frame + lengthOffset) = static_cast<uint8_t>(((maskingKey > 0) ? 0b10000000 : 0) | length);

            union MaskingKey {
                uint32_t key;
                char keyAsArray[4];
            };

            MaskingKey maskingKeyAsArray = {.key = htobe32(maskingKey)};

            for (uint64_t i = 0; i < payloadLength; i++) {
                *(frame + payloadOffset + i) = *(payload + i) ^ *(maskingKeyAsArray.keyAsArray + i % 4);
            }

            // dumpFrame(frame, frameLength);

            onFrameReady(frame, frameLength);

            delete[] frame;
        }
    */
    void WSTransmitter::sendFrame(bool fin, uint8_t opCode, const char* payload, uint64_t payloadLength) {
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
                sendFrameData(htobe16(static_cast<uint16_t>(payloadLength)));
                break;
            case 127:
                sendFrameData(htobe64(static_cast<uint64_t>(payloadLength)));
                break;
        }

        union MaskingKey {
            uint32_t keyAsValue;
            char keyAsBytes[4];
        };

        MaskingKey maskingKeyAsArray = {.keyAsValue = distribution(generator)};

        if (masking) {
            sendFrameData(htobe32(maskingKeyAsArray.keyAsValue));

            for (uint64_t i = 0; i < payloadLength; i++) {
                *(const_cast<char*>(payload) + i) = *(payload + i) ^ *(maskingKeyAsArray.keyAsBytes + i % 4);
            }
        }

        sendFrameData(payload, payloadLength);

        if (masking) {
            for (uint64_t i = 0; i < payloadLength; i++) {
                *(const_cast<char*>(payload) + i) = *(payload + i) ^ *(maskingKeyAsArray.keyAsBytes + i % 4);
            }
        }
    }

    void WSTransmitter::dumpFrame(char* frame, uint64_t frameLength) {
        unsigned int modul = 4;

        std::stringstream stringStream;

        for (std::size_t i = 0; i < frameLength; i++) {
            stringStream << std::setfill('0') << std::setw(2) << std::hex << (unsigned int) (unsigned char) frame[i] << " ";

            if ((i + 1) % modul == 0 || i == frameLength) {
                VLOG(0) << "Frame: " << stringStream.str();
                stringStream.str("");
            }
        }
    }

} // namespace web::ws
