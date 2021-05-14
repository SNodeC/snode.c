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

#ifndef WSRECEVIER_H
#define WSRECEVIER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace http::websocket {

    class WSReceiver {
    public:
        WSReceiver();

        void receive(char* junk, std::size_t junkLen);

    protected:
        union MaskingKeyAsArray {
            uint32_t value;
            char array[4];
        };

        void begin() {
        }

        uint64_t readOpcode(char* junk, uint64_t junkLen);
        uint64_t readLength(char* junk, uint64_t junkLen);
        uint64_t readELength(char* junk, uint64_t junkLen);
        uint64_t readMaskingKey(char* junk, uint64_t junkLen);
        uint64_t readPayload(char* junk, uint64_t junkLen);

        virtual void onMessageStart(int opCode) = 0;
        virtual void onMessageData(char* junk, uint64_t junkLen) = 0;
        virtual void onMessageEnd() = 0;

        void reset();

        // Parser state
        enum struct ParserState { BEGIN, OPCODE, LENGTH, ELENGTH, MASKINGKEY, PAYLOAD, ERROR } parserState = ParserState::BEGIN;

        bool fin = false;
        bool continuation = false;
        bool masked = false;
        uint8_t opCode = 0;
        uint64_t length = 0;
        uint32_t maskingKey = 0;

        uint8_t elengthNumBytes = 0;
        uint8_t elengthNumBytesLeft = 0;
        uint8_t maskingKeyNumBytes = 4;
        uint8_t maskingKeyNumBytesLeft = 0;
        uint64_t payloadRead = 0;
    };

} // namespace http::websocket

#endif // WSRECEVIER_H

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

//   | Version Number | Reference                               |
//  -+----------------+-----------------------------------------+-
//   | 0              + draft-ietf-hybi-thewebsocketprotocol-00 |
//  -+----------------+-----------------------------------------+-
//   | 1              + draft-ietf-hybi-thewebsocketprotocol-01 |
//  -+----------------+-----------------------------------------+-
//   | 2              + draft-ietf-hybi-thewebsocketprotocol-02 |
//  -+----------------+-----------------------------------------+-
//   | 3              + draft-ietf-hybi-thewebsocketprotocol-03 |
//  -+----------------+-----------------------------------------+-
//   | 4              + draft-ietf-hybi-thewebsocketprotocol-04 |
//  -+----------------+-----------------------------------------+-
//   | 5              + draft-ietf-hybi-thewebsocketprotocol-05 |
//  -+----------------+-----------------------------------------+-
//   | 6              + draft-ietf-hybi-thewebsocketprotocol-06 |
//  -+----------------+-----------------------------------------+-
//   | 7              + draft-ietf-hybi-thewebsocketprotocol-07 |
//  -+----------------+-----------------------------------------+-
//   | 8              + draft-ietf-hybi-thewebsocketprotocol-08 |
//  -+----------------+-----------------------------------------+-
//   | 9              + draft-ietf-hybi-thewebsocketprotocol-09 |
//  -+----------------+-----------------------------------------+-
