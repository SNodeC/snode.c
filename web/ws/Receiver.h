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

#ifndef WEB_WS_RECEVIER_H
#define WEB_WS_RECEVIER_H

namespace net::socket::stream {
    class SocketConnectionBase;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws {

    class Receiver {
    public:
        Receiver(net::socket::stream::SocketConnectionBase* socketConnection);

        Receiver(const Receiver&) = delete;
        Receiver& operator=(const Receiver&) = delete;

        void receive(char* junk, std::size_t junkLen);

    private:
        net::socket::stream::SocketConnectionBase* socketConnection = nullptr;

        union MaskingKey {
            uint32_t key;
            char keyAsArray[4];
        };

        uint64_t readOpcode(char* junk, uint64_t junkLen);
        uint64_t readLength(char* junk, uint64_t junkLen);
        uint64_t readELength(char* junk, uint64_t junkLen);
        uint64_t readMaskingKey(char* junk, uint64_t junkLen);
        uint64_t readPayload(char* junk, uint64_t junkLen);

        virtual void onMessageStart(int opCode) = 0;
        virtual void onFrameReceived(const char* junk, uint64_t junkLen) = 0;
        virtual void onMessageEnd() = 0;
        virtual void onMessageError(uint16_t errnum) = 0;

        void reset();

        void dumpFrame(char* frame, uint64_t frameLength);

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

        uint16_t errorState = 0;
    };

} // namespace web::ws

#endif // WEB_WS_RECEVIER_H

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
