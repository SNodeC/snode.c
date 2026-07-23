/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

#include "web/websocket/Receiver.h"

#include "web/websocket/SemanticLog.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/SemanticLogger.h"
#include "utils/hexdump.h"

#include <endian.h>
#include <string>
#include <limits>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef MAX_PAYLOAD_CHUNK_LEN
#define MAX_PAYLOAD_CHUNK_LEN 16384
#endif

namespace web::websocket {

    Receiver::Receiver(bool maskingExpected)
        : maskingExpected(maskingExpected) {
    }

    const logger::BoundaryLogger& Receiver::frameLog() const {
        const unsigned long generation = logger::LogManager::generation();
        if (!frameLog_ || frameLogGeneration_ != generation) {
            frameLogGeneration_ = generation;
            frameLog_.emplace(semantic::webSocketFrameLog());
        }
        return *frameLog_;
    }

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
        } while (ret > 0 && parserState != ParserState::BEGIN);

        return consumed;
    }

    std::size_t Receiver::getPayloadTotalRead() const {
        return payloadTotalRead;
    }

    std::size_t Receiver::readFrameData(char* chunk, std::size_t chunkLen) {
        return readFrameChunk(chunk, chunkLen);
    }

    namespace {
        constexpr uint16_t ProtocolError = 1002;

        bool isControlOpCode(uint8_t opCode) {
            return opCode >= 0x08;
        }

        bool isKnownOpCode(uint8_t opCode) {
            return opCode == 0x00 || opCode == 0x01 || opCode == 0x02 || opCode == 0x08 || opCode == 0x09 || opCode == 0x0A;
        }

        bool isValidCloseStatus(uint16_t status) {
            return (status >= 1000 && status <= 1014 && status != 1004 && status != 1005 && status != 1006) ||
                   (status >= 3000 && status <= 4999);
        }
    } // namespace

    std::size_t Receiver::readOpcode() {
        char byte = 0;
        const std::size_t ret = readFrameData(&byte, 1);

        if (ret > 0) {
            const uint8_t opCodeByte = static_cast<uint8_t>(byte);
            const bool rsvSet = (opCodeByte & 0b01110000) != 0;

            fin = (opCodeByte & 0b10000000) != 0;
            opCode = opCodeByte & 0b00001111;
            currentFrameIsControl = isControlOpCode(opCode);

            if (rsvSet) {
                parserState = ParserState::ERROR;
                errorState = ProtocolError;
                frameLog().error() << "RSV bits are not supported without negotiated extensions";
            } else if (!isKnownOpCode(opCode)) {
                parserState = ParserState::ERROR;
                errorState = ProtocolError;
                frameLog().error() << "Reserved or unknown opcode";
            } else if (currentFrameIsControl && !fin) {
                parserState = ParserState::ERROR;
                errorState = ProtocolError;
                frameLog().error() << "Control frames must not be fragmented";
            } else if (opCode == 0) {
                if (!fragmentedMessageActive) {
                    parserState = ParserState::ERROR;
                    errorState = ProtocolError;
                    frameLog().error() << "Continuation frame without active fragmented message";
                } else {
                    parserState = ParserState::LENGTH;
                }
            } else if (currentFrameIsControl) {
                onMessageStart(opCode);
                parserState = ParserState::LENGTH;
            } else if (fragmentedMessageActive) {
                parserState = ParserState::ERROR;
                errorState = ProtocolError;
                frameLog().error() << "New data frame before fragmented message completed";
            } else {
                onMessageStart(opCode);
                if (!fin) {
                    fragmentedMessageActive = true;
                }
                parserState = ParserState::LENGTH;
            }
        }

        return ret;
    }

    std::size_t Receiver::readLength() {
        controlPayload.clear();
        char byte = 0;
        const std::size_t ret = readFrameData(&byte, 1);

        if (ret > 0) {
            const uint8_t lengthByte = static_cast<uint8_t>(byte);

            masked = (lengthByte & 0b10000000) != 0;
            if (masked == maskingExpected) {
                payloadLengthCode = lengthByte & 0b01111111;
                payloadNumBytes = payloadNumBytesLeft = payloadLengthCode;

                if (currentFrameIsControl && payloadLengthCode > 125) {
                    parserState = ParserState::ERROR;
                    errorState = ProtocolError;
                    frameLog().error() << "Control frame uses extended payload length";
                } else if (payloadNumBytes > 125) {
                    switch (payloadNumBytes) {
                        case 126:
                            elengthNumBytes = elengthNumBytesLeft = 2;
                            break;
                        case 127:
                            elengthNumBytes = elengthNumBytesLeft = 8;
                            break;
                    }
                    parserState = ParserState::ELENGTH;
                    payloadNumBytes = payloadNumBytesLeft = 0;
                } else {
                    if (masked) {
                        parserState = ParserState::MASKINGKEY;
                    } else if (payloadNumBytes > 0) {
                        parserState = ParserState::PAYLOAD;
                    } else {
                        if (fin) {
                            onMessageEnd();
                        }
                        reset();
                    }
                }
            } else {
                parserState = ParserState::ERROR;
                errorState = ProtocolError;
                frameLog().error() << "Frame masking does not match endpoint role";
            }
        }

        return ret;
    }

    std::size_t Receiver::readELength2() {
        char elengthChunk[2]{};

        const std::size_t ret = readFrameData(elengthChunk, elengthNumBytesLeft);

        const std::size_t cursor = static_cast<std::size_t>(elengthNumBytes - elengthNumBytesLeft);
        for (std::size_t i = 0; i < ret; i++) {
            eLength2.asBytes[cursor + i] = elengthChunk[i];
            elengthNumBytesLeft--;
        }

        if (elengthNumBytesLeft == 0) {
            payloadNumBytes = payloadNumBytesLeft = be16toh(eLength2.asValue);
        }

        return ret;
    }

    std::size_t Receiver::readELength8() {
        char elengthChunk[8]{};

        const std::size_t ret = readFrameData(elengthChunk, elengthNumBytesLeft);

        const std::size_t cursor = static_cast<std::size_t>(elengthNumBytes - elengthNumBytesLeft);
        for (std::size_t i = 0; i < ret; i++) {
            eLength8.asBytes[cursor + i] = elengthChunk[i];
            elengthNumBytesLeft--;
        }

        if (elengthNumBytesLeft == 0) {
            payloadNumBytes = payloadNumBytesLeft = be64toh(eLength8.asValue);
        }

        return ret;
    }

    std::size_t Receiver::readELength() {
        std::size_t ret = 0;

        switch (elengthNumBytes) {
            case 2:
                ret = readELength2();
                break;
            case 8:
                ret = readELength8();
                break;
        }

        if (elengthNumBytesLeft == 0) {
            if ((payloadNumBytes & (static_cast<uint64_t>(0x01) << 63)) != 0) {
                parserState = ParserState::ERROR;
                errorState = ProtocolError;
                frameLog().error() << "64-bit payload length has MSB set";
            } else if ((payloadLengthCode == 126 && payloadNumBytes < 126) || (payloadLengthCode == 127 && payloadNumBytes < 65536)) {
                parserState = ParserState::ERROR;
                errorState = ProtocolError;
                frameLog().error() << "Non-minimal payload length encoding";
            } else if (payloadNumBytes > static_cast<uint64_t>(std::numeric_limits<std::size_t>::max())) {
                parserState = ParserState::ERROR;
                errorState = ProtocolError;
                frameLog().error() << "Payload length exceeds platform size limits";
            } else if (masked) {
                parserState = ParserState::MASKINGKEY;
            } else {
                parserState = ParserState::PAYLOAD;
            }
        }

        return ret;
    }

    std::size_t Receiver::readMaskingKey() {
        char maskingKeyChunk[4]{};

        const std::size_t ret = readFrameData(maskingKeyChunk, maskingKeyNumBytesLeft);

        const std::size_t cursor = static_cast<std::size_t>(maskingKeyNumBytes - maskingKeyNumBytesLeft);
        for (std::size_t i = 0; i < ret; i++) {
            maskingKey[cursor + i] = maskingKeyChunk[i];
            maskingKeyNumBytesLeft -= 1;
        }

        if (maskingKeyNumBytesLeft == 0) {
            if (payloadNumBytes > 0) {
                parserState = ParserState::PAYLOAD;
            } else {
                if (opCode == 0 && fin) {
                    fragmentedMessageActive = false;
                }
                if (fin) {
                    onMessageEnd();
                }
                reset();
            }
        }

        return ret;
    }

    std::size_t Receiver::readPayload() {
        char payloadChunk[MAX_PAYLOAD_CHUNK_LEN]{};

        const std::size_t payloadChunkLeft = (MAX_PAYLOAD_CHUNK_LEN <= payloadNumBytesLeft)
                                                 ? static_cast<std::size_t>(MAX_PAYLOAD_CHUNK_LEN)
                                                 : static_cast<std::size_t>(payloadNumBytesLeft);
        const std::size_t ret = readFrameData(payloadChunk, payloadChunkLeft);

        if (ret > 0) {
            const std::size_t payloadChunkLen = static_cast<std::size_t>(ret);

            if (masked) {
                for (std::size_t i = 0; i < payloadChunkLen; i++) {
                    *(payloadChunk + i) = *(payloadChunk + i) ^ *(maskingKey + (i + payloadNumBytes - payloadNumBytesLeft) % 4);
                }
            }

            if (frameLog().enabled(logger::LogLevel::Trace)) {
                const auto dump = utils::hexDumpPresentation(payloadChunk, payloadChunkLen, 32, true);
                frameLog().emit(logger::LogLevel::Trace,
                                logger::PresentedMessage{.plain = "receive: Frame data\n" + dump.plain,
                                                         .terminal = "receive: Frame data\n" + dump.terminal});
            }

            if (currentFrameIsControl) {
                controlPayload.append(payloadChunk, payloadChunkLen);
            }

            onMessageData(payloadChunk, payloadChunkLen);

            payloadNumBytesLeft -= payloadChunkLen;
            payloadTotalRead += payloadChunkLen;
        }

        if (payloadNumBytesLeft == 0) {
            if (opCode == 0 && fin) {
                fragmentedMessageActive = false;
            }
            if (opCode == 8) {
                if (controlPayload.size() == 1) {
                    parserState = ParserState::ERROR;
                    errorState = ProtocolError;
                    frameLog().error() << "Close frame payload length 1 is invalid";
                } else if (controlPayload.size() >= 2) {
                    const uint16_t closeStatus = (static_cast<uint16_t>(static_cast<unsigned char>(controlPayload[0])) << 8) |
                                                 static_cast<uint16_t>(static_cast<unsigned char>(controlPayload[1]));
                    if (!isValidCloseStatus(closeStatus)) {
                        parserState = ParserState::ERROR;
                        errorState = ProtocolError;
                        frameLog().error() << "Close frame status code is invalid";
                    }
                }
            }
            if (parserState != ParserState::ERROR) {
                if (fin) {
                    onMessageEnd();
                }
                reset();
            }
        }

        return ret;
    }

    void Receiver::reset() {
        parserState = ParserState::BEGIN;

        fin = false;
        masked = false;
        currentFrameIsControl = false;

        opCode = 0;
        payloadLengthCode = 0;

        elengthNumBytes = 0;
        elengthNumBytesLeft = 0;

        payloadNumBytes = 0;
        payloadNumBytesLeft = 0;

        maskingKeyNumBytes = 4;
        maskingKeyNumBytesLeft = 4;

        errorState = 0;
        controlPayload.clear();
    }

} // namespace web::websocket
