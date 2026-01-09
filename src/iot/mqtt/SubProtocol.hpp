/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

#include "core/socket/stream/SocketConnection.h"
#include "iot/mqtt/SubProtocol.h"
#include "log/Logger.h"
#include "utils/system/signal.h"
#include "web/websocket/SubProtocolContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/hexdump.h"

#include <algorithm>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt {

    template <typename WSSubProtocolRole>
    SubProtocol<WSSubProtocolRole>::SubProtocol(web::websocket::SubProtocolContext* subProtocolContext,
                                                const std::string& name,
                                                iot::mqtt::Mqtt* mqtt)
        : WSSubProtocolRole(subProtocolContext, name, 0)
        , iot::mqtt::MqttContext(mqtt)
        , onReceivedFromPeerEvent([this]([[maybe_unused]] const utils::Timeval& currentTime) {
            iot::mqtt::MqttContext::onReceivedFromPeer();

            if (size > 0) {
                onReceivedFromPeerEvent.span();
            } else {
                buffer.clear();
                cursor = 0;
            }
        }) {
    }

    template <typename WSSubProtocolRole>
    std::size_t SubProtocol<WSSubProtocolRole>::recv(char* chunk, std::size_t chunklen) {
        std::size_t maxReturn = std::min(chunklen, size);

        std::copy(buffer.data() + cursor, buffer.data() + cursor + maxReturn, chunk);

        cursor += maxReturn;
        size -= maxReturn;

        return maxReturn;
    }

    template <typename WSSubProtocolRole>
    void SubProtocol<WSSubProtocolRole>::send(const char* chunk, std::size_t chunklen) {
        WSSubProtocolRole::sendMessage(chunk, chunklen);
    }

    template <typename WSSubProtocolRole>
    void SubProtocol<WSSubProtocolRole>::end() {
        WSSubProtocolRole::sendClose();
    }

    template <typename WSSubProtocolRole>
    void SubProtocol<WSSubProtocolRole>::close() {
        const std::string reason = "Protocol error";

        WSSubProtocolRole::sendClose(1002, reason.data(), reason.length());
    }

    template <typename WSSubProtocolRole>
    void SubProtocol<WSSubProtocolRole>::onConnected() {
        LOG(INFO) << getSocketConnection()->getConnectionName() << " WsMqtt: connected:";
        iot::mqtt::MqttContext::onConnected();
    }

    template <typename WSSubProtocolRole>
    void SubProtocol<WSSubProtocolRole>::onMessageStart(int opCode) {
        if (opCode == web::websocket::SubProtocolContext::OpCode::TEXT) {
            LOG(ERROR) << getSocketConnection()->getConnectionName() << " WsMqtt: Wrong Opcode: " << opCode << " (TEXT)";
            this->close();
        } else {
            LOG(DEBUG) << getSocketConnection()->getConnectionName() << " WsMqtt: Message START: " << opCode << " (BIN)";
        }
    }

    template <typename WSSubProtocolRole>
    void SubProtocol<WSSubProtocolRole>::onMessageData(const char* chunk, std::size_t chunkLen) {
        data.append(std::string(chunk, chunkLen));

        LOG(DEBUG) << getSocketConnection()->getConnectionName() << " WsMqtt: Frame Data:\n"
                   << std::string(32, ' ').append(utils::hexDump(std::vector<char>(chunk, chunk + chunkLen), 32));
    }

    template <typename WSSubProtocolRole>
    void SubProtocol<WSSubProtocolRole>::onMessageEnd() {
        LOG(DEBUG) << getSocketConnection()->getConnectionName() << " WsMqtt: Message END";

        buffer.insert(buffer.end(), data.begin(), data.end());
        size += data.size();
        data.clear();

        if (size > 0) {
            onReceivedFromPeerEvent.span();
        } else {
            buffer.clear();
            cursor = 0;
        }
    }

    template <typename WSSubProtocolRole>
    void SubProtocol<WSSubProtocolRole>::onMessageError(uint16_t errnum) {
        LOG(ERROR) << getSocketConnection()->getConnectionName() << " WsMqtt: Message error: " << errnum;
    }

    template <typename WSSubProtocolRole>
    void SubProtocol<WSSubProtocolRole>::onDisconnected() {
        iot::mqtt::MqttContext::onDisconnected();
        LOG(DEBUG) << getSocketConnection()->getConnectionName() << " WsMqtt: disconnected";
    }

    template <typename WSSubProtocolRole>
    bool SubProtocol<WSSubProtocolRole>::onSignal(int sig) {
        bool ret = iot::mqtt::MqttContext::onSignal(sig);
        LOG(INFO) << getSocketConnection()->getConnectionName() << " WsMqtt: exit due to '" << strsignal(sig) << "' (SIG"
                  << utils::system::sigabbrev_np(sig) << " = " << sig << ")";

        this->sendClose();

        return ret;
    }

    template <typename WSSubProtocolRole>
    core::socket::stream::SocketConnection* SubProtocol<WSSubProtocolRole>::getSocketConnection() const {
        return WSSubProtocolRole::subProtocolContext->getSocketConnection();
    }

} // namespace iot::mqtt
