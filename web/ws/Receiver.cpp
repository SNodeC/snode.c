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

#include "web/ws/Receiver.h"

#include "SocketContext.h"
#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <endian.h> // for be16toh, be32toh, be64toh, htobe32
#include <iomanip>  // for operator<<, setfill, setw
#include <memory>   // for allocator
#include <sstream>  // for stringstream, basic_ostream, operator<<, bas...

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws {

    Receiver::Receiver(net::socket::stream::SocketContext* socketContext)
        : socketContext(socketContext) {
    }

    void Receiver::receive() {
        std::size_t consumed = 0;
        bool parsingError = false;

        // dumpFrame(junk, junkLen);

        do {
            switch (parserState) {
                case ParserState::BEGIN:
                    parserState = ParserState::OPCODE;
                    [[fallthrough]];
                case ParserState::OPCODE:
                    consumed = readOpcode();
                    break;
                case ParserState::LENGTH:
                    consumed = readLength();
                    break;
                case ParserState::ELENGTH:
                    consumed = readELength();
                    break;
                case ParserState::MASKINGKEY:
                    consumed = readMaskingKey();
                    break;
                case ParserState::PAYLOAD:
                    consumed = readPayload();
                    break;
                case ParserState::ERROR:
                    onMessageError(errorState);
                    parsingError = true;
                    reset();
                    break;
            };
        } while (consumed > 0 && !parsingError);
    }

    std::size_t Receiver::readOpcode() {
        char ch = 0;
        std::size_t consumed = socketContext->readFromPeer(&ch, 1);

        if (consumed > 0) {
            uint8_t opCodeByte = static_cast<uint8_t>(ch);

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

        return consumed;
    }

    std::size_t Receiver::readLength() {
        char ch = 0;
        std::size_t consumed = socketContext->readFromPeer(&ch, 1);

        if (consumed > 0) {
            uint8_t lengthByte = static_cast<uint8_t>(ch);

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

        return consumed;
    }

    std::size_t Receiver::readELength() {
        std::size_t consumed = 0;

        elengthNumBytesLeft = (elengthNumBytesLeft == 0) ? elengthNumBytes : elengthNumBytesLeft;

        char junk[8];
        std::size_t numBytesRead = socketContext->readFromPeer(junk, elengthNumBytesLeft);

        while (numBytesRead - consumed > 0) {
            length |= static_cast<uint64_t>(*reinterpret_cast<unsigned char*>(junk + consumed))
                      << (elengthNumBytes - elengthNumBytesLeft) * 8;

            consumed++;
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

        return consumed;
    }

    std::size_t Receiver::readMaskingKey() {
        std::size_t consumed = 0;

        maskingKeyNumBytesLeft = (maskingKeyNumBytesLeft == 0) ? maskingKeyNumBytes : maskingKeyNumBytesLeft;

        char junk[4];
        std::size_t numBytesRead = socketContext->readFromPeer(junk, maskingKeyNumBytesLeft);

        while (numBytesRead - consumed > 0) {
            maskingKey |= static_cast<uint32_t>(*reinterpret_cast<unsigned char*>(junk + consumed))
                          << (maskingKeyNumBytes - maskingKeyNumBytesLeft) * 8;
            consumed++;
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

        return consumed;
    }

    std::size_t Receiver::readPayload() {
        std::size_t consumed = 0;

        std::size_t numBytesToRead = (1024 <= length - payloadRead) ? 1024 : static_cast<std::size_t>(length - payloadRead);
        char junk[1024];

        ssize_t numBytesRead = socketContext->readFromPeer(junk, numBytesToRead);

        if (numBytesRead > 0) {
            MaskingKey maskingKeyAsArray = {.key = htobe32(maskingKey)};

            for (ssize_t i = 0; i < numBytesRead; i++) {
                *(junk + i) = *(junk + i) ^ *(maskingKeyAsArray.keyAsArray + (i + payloadRead) % 4);
            }

            onFrameReceived(junk, numBytesRead);

            payloadRead += numBytesRead;
            consumed = numBytesRead;
        }

        if (payloadRead == length) {
            if (fin) {
                onMessageEnd();
            }
            reset();
        }

        return consumed;
    }

    void Receiver::dumpFrame(char* frame, uint64_t frameLength) {
        int modul = 4;

        std::stringstream stringStream;

        for (std::size_t i = 0; i < frameLength; i++) {
            stringStream << std::setfill('0') << std::setw(2) << std::hex << (unsigned int) (unsigned char) frame[i] << " ";

            if ((i + 1) % modul == 0 || i == frameLength) {
                VLOG(0) << "Frame: " << stringStream.str();
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

} // namespace web::ws
