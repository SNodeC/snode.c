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

#include "http/server/ws/WSServerContext.h"

#include "net/socket/stream/SocketConnectionBase.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstring>
#include <iostream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace http::websocket {

    void WSServerContext::receiveData(const char* junk, std::size_t junkLen) {
        receive(const_cast<char*>(junk), junkLen);
    }

    void WSServerContext::onReadError([[maybe_unused]] int errnum) {
        socketConnection->close();
    }

    void WSServerContext::onWriteError([[maybe_unused]] int errnum) {
        socketConnection->close();
    }

    void WSServerContext::onMessageStart(int opCode) {
        std::cout << "Message Start - OpCode: " << opCode << std::endl;
    }

    void WSServerContext::onMessageData(char* junk, uint64_t junkLen) {
        std::cout << std::string(junk, static_cast<std::size_t>(junkLen));
    }

    void WSServerContext::onMessageEnd() {
        std::cout << std::endl << "Message End" << std::endl;
        message(1, "Hallo zurück", strlen("Hallo zurück"));
    }

    void WSServerContext::onError([[maybe_unused]] int errno) {
        std::cout << std::endl << "Message Error" << std::endl;

        socketConnection->close();
    }

    void WSServerContext::onFrameReady(char* frame, uint64_t frameLength) {
        socketConnection->enqueue(frame, static_cast<std::size_t>(frameLength));
    }

} // namespace http::websocket
