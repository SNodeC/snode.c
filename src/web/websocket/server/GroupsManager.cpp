/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#include "web/websocket/server/GroupsManager.h"

#include "web/websocket/server/SubProtocol.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    GroupsManager* GroupsManager::groupsManager = nullptr;

    GroupsManager::~GroupsManager() {
        GroupsManager::groupsManager = nullptr;
    }

    GroupsManager* GroupsManager::instance() {
        return GroupsManager::groupsManager == nullptr ? GroupsManager::groupsManager = new GroupsManager() : GroupsManager::groupsManager;
    }

    void GroupsManager::subscribe(SubProtocol* subProtocol, std::string group) {
        if (group.empty()) {
            group = subProtocol->getName();
        }

        if (subProtocol->group != group) {
            const std::string newChannel = subProtocol->getName() + "/" + group;

            groups[newChannel].insert(subProtocol);

            if (!subProtocol->group.empty()) {
                unsubscribe(subProtocol);
            }

            subProtocol->group = newChannel;
        }
    }

    void GroupsManager::unsubscribe(SubProtocol* subProtocol) {
        if (groups.contains(subProtocol->group)) {
            groups[subProtocol->group].erase(subProtocol);

            if (groups[subProtocol->group].empty()) {
                groups.erase(subProtocol->group);
                if (groups.empty()) {
                    delete this;
                }
            }
        }
    }

    void GroupsManager::sendBroadcast(const std::string& group,
                                      const char* message,
                                      std::size_t messageLength,
                                      const SubProtocol* excludedClient) {
        if (groups.contains(group)) {
            for (const SubProtocol* client : groups[group]) {
                if (client != excludedClient) {
                    client->sendMessage(message, messageLength);
                }
            }
        }
    }

    void GroupsManager::sendBroadcast(const std::string& group, const std::string& message, const SubProtocol* excludedClient) {
        if (groups.contains(group)) {
            for (const SubProtocol* client : groups[group]) {
                if (client != excludedClient) {
                    client->sendMessage(message);
                }
            }
        }
    }

    void GroupsManager::sendBroadcastStart(const std::string& group,
                                           const char* message,
                                           std::size_t messageLength,
                                           const SubProtocol* excludedClient) {
        if (groups.contains(group)) {
            for (const SubProtocol* client : groups[group]) {
                if (client != excludedClient) {
                    client->sendMessageStart(message, messageLength);
                }
            }
        }
    }

    void GroupsManager::sendBroadcastStart(const std::string& group, const std::string& message, const SubProtocol* excludedClient) {
        sendBroadcastStart(group, message.data(), message.length(), excludedClient);
    }

    void GroupsManager::sendBroadcastFrame(const std::string& group,
                                           const char* message,
                                           std::size_t messageLength,
                                           const SubProtocol* excludedClient) {
        if (groups.contains(group)) {
            for (const SubProtocol* client : groups[group]) {
                if (client != excludedClient) {
                    client->sendMessageFrame(message, messageLength);
                }
            }
        }
    }

    void GroupsManager::sendBroadcastFrame(const std::string& group, const std::string& message, const SubProtocol* excludedClient) {
        sendBroadcastFrame(group, message.data(), message.length(), excludedClient);
    }

    void GroupsManager::sendBroadcastEnd(const std::string& group,
                                         const char* message,
                                         std::size_t messageLength,
                                         const SubProtocol* excludedClient) {
        if (groups.contains(group)) {
            for (const SubProtocol* client : groups[group]) {
                if (client != excludedClient) {
                    client->sendMessageEnd(message, messageLength);
                }
            }
        }
    }

    void GroupsManager::sendBroadcastEnd(const std::string& group, const std::string& message, const SubProtocol* excludedClient) {
        sendBroadcastEnd(group, message.data(), message.length(), excludedClient);
    }

    void GroupsManager::forEachClient(const std::string& group,
                                      const std::function<void(const SubProtocol*)>& sendToClient,
                                      const SubProtocol* excludedClient) {
        if (groups.contains(group)) {
            for (const SubProtocol* client : groups[group]) {
                if (client != excludedClient) {
                    sendToClient(client);
                }
            }
        }
    }

} // namespace web::websocket::server
