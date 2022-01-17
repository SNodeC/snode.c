/*
 * snode.c - a slim toolkit for network communication
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

#ifndef WEB_WEBSOCKET_RECEVIER_H
#define WEB_WEBSOCKET_RECEVIER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // for size_t
#include <cstdint>
#include <sys/types.h> // for ssize_t

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef MAX_PAYLOAD_JUNK_LEN
#define MAX_PAYLOAD_JUNK_LEN 16384
#endif

namespace web::websocket {

    class Receiver {
    public:
        Receiver() = default;

        Receiver(const Receiver&) = delete;
        Receiver& operator=(const Receiver&) = delete;

        void receive();

    private:
        union MaskingKey {
            uint32_t key;       // cppcheck-suppress unusedStructMember
            char keyAsArray[4]; // cppcheck-suppress unusedStructMember
        };

        ssize_t readOpcode();
        ssize_t readLength();
        ssize_t readELength();
        ssize_t readMaskingKey();
        ssize_t readPayload();

        virtual void onMessageStart(int opCode) = 0;
        virtual void onMessageData(const char* junk, uint64_t junkLen) = 0;
        virtual void onMessageEnd() = 0;
        virtual void onMessageError(uint16_t errnum) = 0;

        virtual ssize_t readFrameData(char* junk, std::size_t junkLen) = 0;

        void reset();

        void dumpFrame(char* frame, uint64_t frameLength);

        // Parser state
        enum struct ParserState { BEGIN, OPCODE, LENGTH, ELENGTH, MASKINGKEY, PAYLOAD, ERROR } parserState = ParserState::BEGIN;

        bool fin = false;
        bool continuation = false;
        bool masked = false;

        uint8_t opCode = 0;

        uint8_t elengthNumBytes = 0;
        uint8_t elengthNumBytesLeft = 0;

        uint64_t payLoadNumBytes = 0;
        uint64_t payLoadNumBytesLeft = 0;

        uint32_t maskingKey = 0;
        MaskingKey maskingKeyAsArray;
        uint8_t maskingKeyNumBytes = 4;
        uint8_t maskingKeyNumBytesLeft = 4;

        uint16_t errorState = 0;

        char elengthJunk[8];
        char maskingKeyJunk[4];
        char payloadJunk[MAX_PAYLOAD_JUNK_LEN];
    };

} // namespace web::websocket

#endif // WEB_WEBSOCKET_RECEVIER_H

//   |Opcode  | Meaning                             | Reference |
//  -+--------+-------------------------------------+-----------|
//   | 0      | Continuation Frame                  | RFC XXXX  |
//  -+--------+-------------------------------------+-----------|
//   | 1      | Text Frame                          | RFC XXXX  |
//  -+--------+-------------------------------------+-----------|
//   | 2      | Binary Frame                        | RFC XXXX  |
//  -+--------+-------------------------------------+-----------|
//   | 8      | Connection Close Frame              | RFC XXXX  |
//  -+--------+-------------------------------------+-----------|
//   | 9      | Ping Frame                          | RFC XXXX  |
//  -+--------+-------------------------------------+-----------|
//   | 10     | Pong Frame                          | RFC XXXX  |
//  -+--------+-------------------------------------+-----------|
