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

#include "web/websocket/Receiver.h"

#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <endian.h> // for be16toh, be32toh, be64toh, htobe32
#include <iomanip>  // for operator<<, setfill, setw
#include <memory>   // for allocator
#include <sstream>  // for stringstream, basic_ostream, operator<<, bas...

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket {

    void Receiver::receive() {
        ssize_t ret = 0;
        bool parsingError = false;

        // dumpFrame(junk, junkLen);

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
                    parsingError = true;
                    reset();
                    break;
            };
        } while (ret > 0 && !parsingError); // && parserState != ParserState::BEGIN);
    }

    ssize_t Receiver::readOpcode() {
        char byte = 0;
        ssize_t ret = readFrameData(&byte, 1);

        if (ret > 0) {
            uint8_t opCodeByte = static_cast<uint8_t>(byte);

            fin = opCodeByte & 0b10000000;
            opCode = opCodeByte & 0b00001111;

            if (!continuation) {
                onMessageStart(opCode);
                parserState = ParserState::LENGTH;
            } else if (opCode == 0) {
                parserState = ParserState::LENGTH;
            } else {
                parserState = ParserState::ERROR;
                errorState = 1002;
                VLOG(0) << "Error opcode in continuation frame";
            }
            continuation = !fin;
        }

        return ret;
    }

    ssize_t Receiver::readLength() {
        char byte = 0;
        ssize_t ret = readFrameData(&byte, 1);

        if (ret > 0) {
            uint8_t lengthByte = static_cast<uint8_t>(byte);

            masked = lengthByte & 0b10000000;
            length = lengthByte & 0b01111111;

            if (length > 125) {
                switch (length) {
                    case 126:
                        elengthNumBytes = 2;
                        break;
                    case 127:
                        elengthNumBytes = 8;
                        break;
                }
                parserState = ParserState::ELENGTH;
                length = 0;
            } else {
                if (masked) {
                    parserState = ParserState::MASKINGKEY;
                } else if (length > 0) {
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

    ssize_t Receiver::readELength() {
        elengthNumBytesLeft = (elengthNumBytesLeft == 0) ? elengthNumBytes : elengthNumBytesLeft;

        ssize_t ret = readFrameData(elengthJunk, elengthNumBytesLeft);

        for (ssize_t i = 0; i < ret; i++) {
            length |= static_cast<uint64_t>(*reinterpret_cast<unsigned char*>(elengthJunk + i))
                      << (elengthNumBytes - elengthNumBytesLeft) * 8;

            elengthNumBytesLeft--;
        }

        if (elengthNumBytesLeft == 0) {
            switch (elengthNumBytes) {
                case 2: {
                    length = be16toh(static_cast<uint16_t>(length));
                } break;
                case 8:
                    length = be64toh(length);
                    break;
            }

            if (length & static_cast<uint64_t>(0x01) << 63) {
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

    ssize_t Receiver::readMaskingKey() {
        maskingKeyNumBytesLeft = (maskingKeyNumBytesLeft == 0) ? maskingKeyNumBytes : maskingKeyNumBytesLeft;

        ssize_t ret = readFrameData(maskingKeyJunk, maskingKeyNumBytesLeft);

        for (ssize_t i = 0; i < ret; i++) {
            maskingKey |= static_cast<uint32_t>(*reinterpret_cast<unsigned char*>(maskingKeyJunk + i))
                          << (maskingKeyNumBytes - maskingKeyNumBytesLeft) * 8;
            maskingKeyNumBytesLeft--;
        }

        if (maskingKeyNumBytesLeft == 0) {
            maskingKey = be32toh(maskingKey);
            if (length > 0) {
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

    ssize_t Receiver::readPayload() {
        std::size_t payloadJunkLeft = (MAX_PAYLOAD_JUNK_LEN <= length - payloadRead) ? static_cast<std::size_t>(MAX_PAYLOAD_JUNK_LEN)
                                                                                     : static_cast<std::size_t>(length - payloadRead);

        ssize_t ret = readFrameData(payloadJunk, payloadJunkLeft);

        if (ret > 0) {
            std::size_t payloadJunkLen = static_cast<std::size_t>(ret);
            MaskingKey maskingKeyAsArray = {.key = htobe32(maskingKey)};

            for (std::size_t i = 0; i < payloadJunkLen; i++) {
                *(payloadJunk + i) = *(payloadJunk + i) ^ *(maskingKeyAsArray.keyAsArray + (i + payloadRead) % 4);
            }

            onFrameReceived(payloadJunk, payloadJunkLen);

            payloadRead += payloadJunkLen;
        }

        if (payloadRead == length) {
            if (fin) {
                onMessageEnd();
            }
            reset();
        }

        return ret;
    }

    void Receiver::dumpFrame(char* frame, uint64_t frameLength) {
        unsigned long modul = 4;

        std::stringstream stringStream;

        for (std::size_t i = 0; i < frameLength; i++) {
            stringStream << std::setfill('0') << std::setw(2) << std::hex << (unsigned int) (unsigned char) frame[i] << " ";

            if ((i + 1) % modul == 0 || i == frameLength) {
                stringStream.str("");
            }
        }
    }

    void Receiver::reset() {
        parserState = ParserState::BEGIN;

        fin = false;
        continuation = false;
        masked = false;

        opCode = 0;
        length = 0;
        maskingKey = 0;

        elengthNumBytes = 0;
        elengthNumBytesLeft = 0;

        maskingKeyNumBytes = 4;
        maskingKeyNumBytesLeft = 0;

        payloadRead = 0;

        errorState = 0;
    }

} // namespace web::websocket
