/*
 * SNode.C - a slim toolkit for network communication
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

#ifndef WEB_WEBSOCKET_SERVER_CHANNELMANAGER_H
#define WEB_WEBSOCKET_SERVER_CHANNELMANAGER_H

namespace web::websocket::server {
    class SubProtocol;
} // namespace web::websocket::server

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <functional>
#include <map>
#include <set>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    class GroupsManager {
    public:
        GroupsManager() = default;
        GroupsManager(const GroupsManager&) = delete;

        GroupsManager& operator=(const GroupsManager&) = delete;

    private:
        ~GroupsManager();

    public:
        static GroupsManager* instance();

        void subscribe(SubProtocol* subProtocol, std::string group = "");
        void unsubscribe(SubProtocol* subProtocol);

        void sendBroadcast(const std::string& group, const char* message, std::size_t messageLength, const SubProtocol* excludedClient);
        void sendBroadcast(const std::string& group, const std::string& message, const SubProtocol* excludedClient);

        void
        sendBroadcastStart(const std::string& group, const char* message, std::size_t messageLength, const SubProtocol* excludedClient);
        void sendBroadcastStart(const std::string& group, const std::string& message, const SubProtocol* excludedClient);

        void
        sendBroadcastFrame(const std::string& group, const char* message, std::size_t messageLength, const SubProtocol* excludedClient);
        void sendBroadcastFrame(const std::string& group, const std::string& message, const SubProtocol* excludedClient);

        void sendBroadcastEnd(const std::string& group, const char* message, std::size_t messageLength, const SubProtocol* excludedClient);
        void sendBroadcastEnd(const std::string& group, const std::string& message, const SubProtocol* excludedClient);

        void forEachClient(const std::string& group,
                           const std::function<void(const SubProtocol*)>& sendToClient,
                           const SubProtocol* excludedClient);

        static GroupsManager* groupsManager;

    private:
        std::map<std::string, std::set<SubProtocol*>> groups;
    };

} // namespace web::websocket::server

#endif // WEB_WEBSOCKET_SERVER_CHANNELMANAGER_H
