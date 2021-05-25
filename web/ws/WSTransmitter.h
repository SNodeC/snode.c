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

#ifndef WEB_WS_WSTRANSMITTER_H
#define WEB_WS_WSTRANSMITTER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <random>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws {

    class WSTransmitter {
    protected:
        void sendMessageStart(uint8_t opCode, const char* message, std::size_t messageLength, bool masked);
        void sendMessageFrame(const char* message, std::size_t messageLength, bool masked);
        void sendMessageEnd(const char* message, std::size_t messageLength, bool masked);

        void sendMessage(uint8_t opCode, const char* message, std::size_t messageLength, bool masked);

    private:
        void send(bool end, uint8_t opCode, const char* message, std::size_t messageLength, bool masked);

        void sendFrame(bool fin, uint8_t opCode, const char* payload, uint64_t payloadLength, bool masked);
        void dumpFrame(char* frame, uint64_t frameLength);

        virtual void sendFrameData(uint8_t data) = 0;
        virtual void sendFrameData(uint16_t data) = 0;
        virtual void sendFrameData(uint32_t data) = 0;
        virtual void sendFrameData(uint64_t data) = 0;
        virtual void sendFrameData(const char* frame, uint64_t frameLength) = 0;

        std::default_random_engine generator;
        std::uniform_int_distribution<uint32_t> distribution{1, UINT32_MAX};

        //        virtual void onFrameReady(char* frame, uint64_t frameLength) = 0;

        //        void sendFrame1(bool fin, uint8_t opCode, uint32_t maskingKey, char* payload, uint64_t payloadLength);
    };

} // namespace web::ws

#endif // WEB_WS_WSTRANSMITTER_H
