/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "iot/mqtt/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace iot::mqtt {

    SocketContext::SocketContext(core::socket::stream::SocketConnection* socketConnection, Mqtt* mqtt)
        : core::socket::stream::SocketContext(socketConnection)
        , iot::mqtt::MqttContext(mqtt) {
    }

    void SocketContext::onConnected() {
        iot::mqtt::MqttContext::onConnected();
    }

    void SocketContext::onDisconnected() {
        iot::mqtt::MqttContext::onDisconnected();
    }

    void SocketContext::onExit() {
        iot::mqtt::MqttContext::onExit();
    }

    core::socket::stream::SocketConnection* SocketContext::getSocketConnection() {
        return core::socket::stream::SocketContext::getSocketConnection();
    }

    std::size_t SocketContext::onReceivedFromPeer() {
        return MqttContext::onProcess();
    }

    std::size_t SocketContext::recv(char* junk, std::size_t junklen) {
        return core::socket::stream::SocketContext::readFromPeer(junk, junklen);
    }

    void SocketContext::send(const char* junk, std::size_t junklen) {
        core::socket::stream::SocketContext::sendToPeer(junk, junklen);
    }

    void SocketContext::end(bool fatal) {
        core::socket::stream::SocketContext::shutdown(fatal);
    }

    void SocketContext::close() {
        core::socket::stream::SocketContext::close();
    }

} // namespace iot::mqtt
