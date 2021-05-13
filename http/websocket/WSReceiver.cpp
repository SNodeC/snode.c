#include "WSReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <endian.h>
#include <iostream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace http::websocket {

    WSReceiver::WSReceiver() {
    }

    void WSReceiver::receive(char* junk, std::size_t junkLen) {
        uint64_t consumed = 0;
        bool parsingError = false;

        while (consumed < junkLen && !parsingError) {
            switch (parserState) {
                case ParserState::BEGIN:
                    parserState = ParserState::OPCODE;
                    begin();
                    [[fallthrough]];
                case ParserState::OPCODE:
                    consumed += readOpcode(junk + consumed, junkLen - consumed);
                    break;
                case ParserState::LENGTH:
                    consumed += readLength(junk + consumed, junkLen - consumed);
                    break;
                case ParserState::ELENGTH:
                    consumed += readELength(junk + consumed, junkLen - consumed);
                    break;
                case ParserState::MASKINGKEY:
                    consumed += readMaskingKey(junk + consumed, junkLen - consumed);
                    break;
                case ParserState::PAYLOAD:
                    consumed += readPayload(junk + consumed, junkLen - consumed);
                    break;
                case ParserState::ERROR:
                    parsingError = true;
                    reset();
                    break;
            };
        }
    }

    uint64_t WSReceiver::readOpcode(char* junk, uint64_t junkLen) {
        uint64_t consumed = 0;
        if (junkLen > 0) {
            consumed++;

            uint8_t opCodeByte = *reinterpret_cast<uint8_t*>(junk);

            fin = opCodeByte & 0b10000000;
            opCode = opCodeByte & 0b00001111;

            if (opCode != 0) {
                if (!continuation) {
                    onMessageStart(opCode);
                } else {
                    std::cout << "Error opcode: provided in continuation frame" << std::endl;
                }
            }

            continuation = !fin;

            parserState = ParserState::LENGTH;
        }

        return consumed;
    }

    uint64_t WSReceiver::readLength(char* junk, uint64_t junkLen) {
        uint64_t consumed = 0;

        if (junkLen > 0) {
            uint8_t lengthByte = *reinterpret_cast<uint8_t*>(junk);

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
                parserState = ParserState::MASKINGKEY;
            }
            consumed++;
        }

        return consumed;
    }

    uint64_t WSReceiver::readELength(char* junk, uint64_t junkLen) {
        uint64_t consumed = 0;

        elengthNumBytesLeft = (elengthNumBytesLeft == 0) ? elengthNumBytes : elengthNumBytesLeft;

        while (consumed < junkLen && elengthNumBytesLeft > 0) {
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
                std::cout << "Error elength: msb == 1" << std::endl;
            }

            parserState = ParserState::MASKINGKEY;
        }

        return consumed;
    }

    uint64_t WSReceiver::readMaskingKey(char* junk, uint64_t junkLen) {
        uint64_t consumed = 0;

        if (masked) {
            maskingKeyNumBytesLeft = (maskingKeyNumBytesLeft == 0) ? maskingKeyNumBytes : maskingKeyNumBytesLeft;

            while (consumed < junkLen && maskingKeyNumBytesLeft > 0) {
                maskingKey |= static_cast<uint32_t>(*reinterpret_cast<unsigned char*>(junk + consumed))
                              << (maskingKeyNumBytes - maskingKeyNumBytesLeft) * 8;
                consumed++;
                maskingKeyNumBytesLeft--;
            }

            if (maskingKeyNumBytesLeft == 0) {
                maskingKey = be32toh(maskingKey);
                parserState = ParserState::PAYLOAD;
            }
        } else {
            parserState = ParserState::PAYLOAD;
        }

        return consumed;
    }

    uint64_t WSReceiver::readPayload(char* junk, uint64_t junkLen) {
        uint64_t consumed = 0;

        if (junkLen > 0 && length - payloadRead > 0) {
            uint64_t numBytesToRead = (junkLen <= length - payloadRead) ? junkLen : length - payloadRead;

            MaskingKeyAsArray maskingKeyAsArray = {.value = htobe32(maskingKey)};

            for (uint64_t i = 0; i < numBytesToRead; i++) {
                *(junk + i) = *(junk + i) ^ *(maskingKeyAsArray.array + (i + payloadRead) % 4);
            }

            onMessageData(junk, numBytesToRead);

            payloadRead += numBytesToRead;
            consumed = numBytesToRead;
        }

        if (payloadRead == length) {
            if (fin) {
                onMessageEnd();
            }
            reset();
        }

        return consumed;
    }

    void WSReceiver::reset() {
        payloadRead = 0;
        opCode = 0;
        parserState = ParserState::BEGIN;
        fin = false;
        masked = false;
        length = 0;
        elengthNumBytes = 0;
        elengthNumBytesLeft = 0;
        maskingKey = 0;
        maskingKeyNumBytes = 4;
        maskingKeyNumBytesLeft = 0;
        payloadRead = 0;
    }

} // namespace http::websocket
