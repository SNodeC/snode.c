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

#include "Echo.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/system/signal.h"

#include <cstring>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MAX_FLYING_PINGS 3
#define PING_INTERVAL 90

namespace apps::websocket::subprotocol::echo::client {

    Echo::Echo(web::websocket::SubProtocolContext* socketContextUpgradeBase, const std::string& name)
        : web::websocket::client::SubProtocol(socketContextUpgradeBase, name, PING_INTERVAL, MAX_FLYING_PINGS) {
    }

    void Echo::onConnected() {
        VLOG(1) << "Echo connected";

        sendMessage("Welcome to SimpleChat");
        sendMessage("=====================");
    }

    void Echo::onMessageStart(int opCode) {
        VLOG(2) << "Message Start - OpCode: " << opCode;
    }

    void Echo::onMessageData(const char* chunk, std::size_t chunkLen) {
        data += std::string(chunk, chunkLen);

        VLOG(2) << "Message Fragment: " << std::string(chunk, chunkLen);
    }

    void Echo::onMessageEnd() {
        VLOG(1) << "Message Data: " << data;

        // To do ping-pong
        //        sendMessage(data);

        data.clear();
    }

    void Echo::onMessageError(uint16_t errnum) {
        VLOG(1) << "Message error: " << errnum;
    }

    void Echo::onDisconnected() {
        VLOG(1) << "Echo disconnected:";
    }

    bool Echo::onSignal(int sig) {
        VLOG(1) << "SubProtocol 'echo' exit due to '" << strsignal(sig) << "' (SIG" << utils::system::sigabbrev_np(sig) << " = " << sig
                << ")";

        sendClose();

        return false;
    }

} // namespace apps::websocket::subprotocol::echo::client
