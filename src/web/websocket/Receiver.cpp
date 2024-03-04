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

#include "web/websocket/Receiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <endian.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket {

    Receiver::~Receiver() {
    }

    std::size_t Receiver::receive() {
        std::size_t ret = 0;
        std::size_t consumed = 0;

        // dumpFrame(chunk, chunkLen);

        do {
            switch (parserState) {
                case ParserState::BEGIN:
                    parserState = ParserState::OPCODE;
                    [[fallthrough]];
                case ParserState::OPCODE:
                    ret = readOpcode();
                    break;
                case ParserState::LENGTH:
                    ret = readLength();
                    break;
                case ParserState::ELENGTH:
                    ret = readELength();
                    break;
                case ParserState::MASKINGKEY:
                    ret = readMaskingKey();
                    break;
                case ParserState::PAYLOAD:
                    ret = readPayload();
                    break;
                case ParserState::ERROR:
                    onMessageError(errorState);
                    reset();
                    break;
            }
            consumed += ret;
        } while (ret > 0 && parserState != ParserState::BEGIN && parserState != ParserState::ERROR);

        return consumed;
    }

    std::size_t Receiver::readOpcode() {
        char byte = 0;
        const std::size_t ret = readFrameData(&byte, 1);

        if (ret > 0) {
            const uint8_t opCodeByte = static_cast<uint8_t>(byte);

            fin = (opCodeByte & 0b10000000) != 0;
            opCode = opCodeByte & 0b00001111;

            if (!continuation) {
                onMessageStart(opCode);
                parserState = ParserState::LENGTH;
            } else if (opCode == 0) {
                parserState = ParserState::LENGTH;
            } else {
                parserState = ParserState::ERROR;
                errorState = 1002;
                LOG(DEBUG) << "WebSocket: Error opcode in continuation frame";
            }
            continuation = !fin;
        }

        return ret;
    }

    std::size_t Receiver::readLength() {
        char byte = 0;
        const std::size_t ret = readFrameData(&byte, 1);

        if (ret > 0) {
            const uint8_t lengthByte = static_cast<uint8_t>(byte);

            masked = (lengthByte & 0b10000000) != 0;
            payLoadNumBytes = payLoadNumBytesLeft = lengthByte & 0b01111111;

            if (payLoadNumBytes > 125) {
                switch (payLoadNumBytes) {
                    case 126:
                        elengthNumBytes = elengthNumBytesLeft = 2;
                        break;
                    case 127:
                        elengthNumBytes = elengthNumBytesLeft = 8;
                        break;
                }
                parserState = ParserState::ELENGTH;
                payLoadNumBytes = payLoadNumBytesLeft = 0;
            } else {
                if (masked) {
                    parserState = ParserState::MASKINGKEY;
                } else if (payLoadNumBytes > 0) {
                    parserState = ParserState::PAYLOAD;
                } else {
                    if (fin) {
                        onMessageEnd();
                    }
                    reset();
                }
            }
        }

        return ret;
    }

    std::size_t Receiver::readELength() {
        const std::size_t ret = readFrameData(elengthChunk, elengthNumBytesLeft);

        for (std::size_t i = 0; i < ret; i++) {
            payLoadNumBytes |= *reinterpret_cast<uint64_t*>(elengthChunk + i) << (elengthNumBytes - elengthNumBytesLeft) * 8;

            elengthNumBytesLeft--;
        }

        if (elengthNumBytesLeft == 0) {
            switch (elengthNumBytes) {
                case 2:
                    payLoadNumBytes = payLoadNumBytesLeft = be16toh(static_cast<uint16_t>(payLoadNumBytes));
                    break;
                case 8:
                    payLoadNumBytes = payLoadNumBytesLeft = be64toh(payLoadNumBytes);
                    break;
            }

            if ((payLoadNumBytes & static_cast<uint64_t>(0x01) << 63) != 0) {
                parserState = ParserState::ERROR;
                errorState = 1004;
            } else if (masked) {
                parserState = ParserState::MASKINGKEY;
            } else {
                parserState = ParserState::PAYLOAD;
            }
        }

        return ret;
    }

    std::size_t Receiver::readMaskingKey() {
        const std::size_t ret = readFrameData(maskingKeyChunk, maskingKeyNumBytesLeft);

        for (std::size_t i = 0; i < ret; i++) {
            maskingKey |= static_cast<uint32_t>(*reinterpret_cast<unsigned char*>(maskingKeyChunk + i))
                          << (maskingKeyNumBytes - maskingKeyNumBytesLeft) * 8;
            maskingKeyNumBytesLeft--;
        }

        if (maskingKeyNumBytesLeft == 0) {
            maskingKeyAsArray = {.key = maskingKey};

            if (payLoadNumBytes > 0) {
                parserState = ParserState::PAYLOAD;
            } else {
                if (fin) {
                    onMessageEnd();
                }
                reset();
            }
        }

        return ret;
    }

    std::size_t Receiver::readPayload() {
        const std::size_t payloadChunkLeft = (MAX_PAYLOAD_JUNK_LEN <= payLoadNumBytesLeft) ? static_cast<std::size_t>(MAX_PAYLOAD_JUNK_LEN)
                                                                                           : static_cast<std::size_t>(payLoadNumBytesLeft);

        const std::size_t ret = readFrameData(payloadChunk, payloadChunkLeft);

        if (ret > 0) {
            const std::size_t payloadChunkLen = static_cast<std::size_t>(ret);

            if (masked) {
                for (std::size_t i = 0; i < payloadChunkLen; i++) {
                    *(payloadChunk + i) =
                        *(payloadChunk + i) ^ *(maskingKeyAsArray.keyAsArray + (i + payLoadNumBytes - payLoadNumBytesLeft) % 4);
                }
            }

            onMessageData(payloadChunk, payloadChunkLen);

            payLoadNumBytesLeft -= payloadChunkLen;
        }

        if (payLoadNumBytesLeft == 0) {
            if (fin) {
                onMessageEnd();
            }
            reset();
        }

        return ret;
    }

    void Receiver::reset() {
        parserState = ParserState::BEGIN;

        fin = false;
        continuation = false;
        masked = false;

        opCode = 0;

        elengthNumBytes = 0;
        elengthNumBytesLeft = 0;

        payLoadNumBytes = 0;
        payLoadNumBytesLeft = 0;

        maskingKey = 0;
        maskingKeyAsArray = {.key = 0};
        maskingKeyNumBytes = 4;
        maskingKeyNumBytesLeft = 4;

        errorState = 0;
    }

} // namespace web::websocket
