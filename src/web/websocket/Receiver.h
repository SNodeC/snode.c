/*
 * SNode.C - A Slim Toolkit for Network Communication
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

#ifndef WEB_WEBSOCKET_RECEVIER_H
#define WEB_WEBSOCKET_RECEVIER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket {

    class Receiver {
    public:
        Receiver(bool maskingExpected);

        Receiver(const Receiver&) = delete;
        Receiver& operator=(const Receiver&) = delete;

        virtual ~Receiver();

        std::size_t receive();

        std::size_t getPayloadTotalRead() const;

    private:
        std::size_t readFrameData(char* chunk, std::size_t chunkLen);

        std::size_t readOpcode();
        std::size_t readLength();
        std::size_t readELength();
        std::size_t readMaskingKey();
        std::size_t readPayload();

        std::size_t readELength2();
        std::size_t readELength8();

        virtual void onMessageStart(int opCode) = 0;
        virtual void onMessageData(const char* chunk, uint64_t chunkLen) = 0;
        virtual void onMessageEnd() = 0;
        virtual void onMessageError(uint16_t errnum) = 0;

        void reset();

        virtual std::size_t readFrameChunk(char* chunk, std::size_t chunkLen) const = 0;

        // Parser state
        enum struct ParserState { BEGIN, OPCODE, LENGTH, ELENGTH, MASKINGKEY, PAYLOAD, ERROR } parserState = ParserState::BEGIN;

        bool fin = false;
        bool continuation = false;
        bool masked = false;

        bool maskingExpected = false;

        uint8_t opCode = 0;

        char maskingKey[4];

        union ELength8 {
            uint64_t asValue;
            char asBytes[8];
        } eLength8{};

        union ELength2 {
            uint16_t asValue;
            char asBytes[2];
        } eLength2{};

        uint8_t elengthNumBytes = 0;
        uint8_t elengthNumBytesLeft = 0;

        uint64_t payloadNumBytes = 0;
        uint64_t payloadNumBytesLeft = 0;

        uint8_t maskingKeyNumBytes = 4;
        uint8_t maskingKeyNumBytesLeft = 4;

        uint16_t errorState = 0;

        std::size_t payloadTotalRead = 0;
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
