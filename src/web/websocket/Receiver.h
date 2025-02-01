/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

namespace core::socket::stream {

    class SocketConnection;

}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef MAX_PAYLOAD_JUNK_LEN
#define MAX_PAYLOAD_JUNK_LEN 16384
#endif

namespace web::websocket {

    class Receiver {
    public:
        Receiver(core::socket::stream::SocketConnection* socketConnection);

        Receiver(const Receiver&) = delete;
        Receiver& operator=(const Receiver&) = delete;

        virtual ~Receiver();

        std::size_t receive();

    private:
        union MaskingKey {
            uint32_t key;
            char keyAsArray[4];
        };

        std::size_t readOpcode();
        std::size_t readLength();
        std::size_t readELength();
        std::size_t readMaskingKey();
        std::size_t readPayload();

        virtual void onMessageStart(int opCode) = 0;
        virtual void onMessageData(const char* chunk, uint64_t chunkLen) = 0;
        virtual void onMessageEnd() = 0;
        virtual void onMessageError(uint16_t errnum) = 0;

        std::size_t readFrameData(char* chunk, std::size_t chunkLen);

        void reset();

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
        MaskingKey maskingKeyAsArray{};
        uint8_t maskingKeyNumBytes = 4;
        uint8_t maskingKeyNumBytesLeft = 4;

        uint16_t errorState = 0;

        char elengthChunk[8]{};
        char maskingKeyChunk[4]{};
        char payloadChunk[MAX_PAYLOAD_JUNK_LEN]{};

        core::socket::stream::SocketConnection* socketConnection = nullptr;
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
