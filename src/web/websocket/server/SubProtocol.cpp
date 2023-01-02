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

#include "web/websocket/server/SubProtocol.h"

#include "web/websocket/SubProtocol.hpp"
#include "web/websocket/server/GroupsManager.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    SubProtocol::SubProtocol(SubProtocolContext* socketContextUpgrade, const std::string& name, int pingInterval, int maxFlyingPings)
        : Super(socketContextUpgrade, name, pingInterval, maxFlyingPings) {
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

    void SubProtocol::forEachClient(const std::function<void(SubProtocol*)>& sendToClient, bool excludeSelf) {
        GroupsManager::instance()->forEachClient(group, sendToClient, excludeSelf ? this : nullptr);
    }

} // namespace web::websocket::server

template class web::websocket::SubProtocol<web::websocket::server::SocketContextUpgrade>;
