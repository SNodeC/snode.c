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

#ifndef WEB_WEBSOCKET_TRANSMITTER_H
#define WEB_WEBSOCKET_TRANSMITTER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <random>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket {

    class Transmitter {
    public:
        Transmitter() = delete;

        Transmitter(const Transmitter&) = delete;
        Transmitter& operator=(const Transmitter&) = delete;

        virtual ~Transmitter();

    protected:
        Transmitter(bool masking);

        void sendMessage(uint8_t opCode, const char* message, std::size_t messageLength);

        void sendMessageStart(uint8_t opCode, const char* message, std::size_t messageLength);
        void sendMessageFrame(const char* message, std::size_t messageLength);
        void sendMessageEnd(const char* message, std::size_t messageLength);

        std::size_t getPayloadTotalSent() const;

    private:
        void send(bool end, uint8_t opCode, const char* message, std::size_t messageLength);

        void sendFrame(bool fin, uint8_t opCode, const char* payload, uint64_t payloadLength);

        void sendFrameData(uint8_t data) const;
        void sendFrameData(uint16_t data) const;
        void sendFrameData(uint32_t data) const;
        void sendFrameData(uint64_t data) const;
        void sendFrameData(const char* frame, uint64_t frameLength) const;

        virtual void sendFrameChunk(const char* data, std::size_t dataLength) const = 0;

        std::random_device randomDevice;
        std::uniform_int_distribution<uint32_t> distribution{0, UINT32_MAX};

        bool masking = false;

    protected:
        bool closeSent = false;

    private:
        std::size_t payloadTotalSent = 0;
    };

} // namespace web::websocket

#endif // WEB_WEBSOCKET_TRANSMITTER_H
