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

#include "web/websocket/server/SubProtocol.h"

#include "web/websocket/SubProtocol.hpp"
#include "web/websocket/server/GroupsManager.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    SubProtocol::SubProtocol(SubProtocolContext* subProtocolContext, const std::string& name, int pingInterval, int maxFlyingPings)
        : Super(subProtocolContext, name, pingInterval, maxFlyingPings) {
        GroupsManager::instance()->subscribe(this);
    }

    SubProtocol::~SubProtocol() {
        GroupsManager::instance()->unsubscribe(this);
    }

    void SubProtocol::subscribe(const std::string& group) {
        GroupsManager::instance()->subscribe(this, group);
        // subscibe(group, this);
        // unsubscribe(this->group, this);
        // this->group = group;
    }

    void SubProtocol::sendBroadcast(const std::string& message, bool excludeSelf) {
        GroupsManager::instance()->sendBroadcast(group, message, excludeSelf ? this : nullptr);
    }

    void SubProtocol::sendBroadcast(const char* message, std::size_t messageLength, bool excludeSelf) {
        GroupsManager::instance()->sendBroadcast(group, message, messageLength, excludeSelf ? this : nullptr);
    }

    void SubProtocol::sendBroadcastStart(const char* message, std::size_t messageLength, bool excludeSelf) {
        GroupsManager::instance()->sendBroadcastStart(group, message, messageLength, excludeSelf ? this : nullptr);
    }

    void SubProtocol::sendBroadcastStart(const std::string& message, bool excludeSelf) {
        GroupsManager::instance()->sendBroadcastStart(group, message, excludeSelf ? this : nullptr);
    }

    void SubProtocol::sendBroadcastFrame(const char* message, std::size_t messageLength, bool excludeSelf) {
        GroupsManager::instance()->sendBroadcastFrame(group, message, messageLength, excludeSelf ? this : nullptr);
    }

    void SubProtocol::sendBroadcastFrame(const std::string& message, bool excludeSelf) {
        GroupsManager::instance()->sendBroadcastFrame(group, message.data(), message.length(), excludeSelf ? this : nullptr);
    }

    void SubProtocol::sendBroadcastEnd(const char* message, std::size_t messageLength, bool excludeSelf) {
        GroupsManager::instance()->sendBroadcastEnd(group, message, messageLength, excludeSelf ? this : nullptr);
    }

    void SubProtocol::sendBroadcastEnd(const std::string& message, bool excludeSelf) {
        GroupsManager::instance()->sendBroadcastEnd(group, message.data(), message.length(), excludeSelf ? this : nullptr);
    }

    void SubProtocol::forEachClient(const std::function<void(const SubProtocol*)>& sendToClient, bool excludeSelf) {
        GroupsManager::instance()->forEachClient(group, sendToClient, excludeSelf ? this : nullptr);
    }

} // namespace web::websocket::server

template class web::websocket::SubProtocol<web::websocket::server::SocketContextUpgrade>;
