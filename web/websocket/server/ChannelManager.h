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

#ifndef WEB_WEBSOCKET_SERVER_CHANNELMANAGER_H
#define WEB_WEBSOCKET_SERVER_CHANNELMANAGER_H

namespace web::websocket::server {
    class SubProtocol;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef> // for size_t
#include <functional>
#include <map>
#include <set>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    class ChannelManager {
    private:
        ChannelManager() = default;
        ChannelManager(const ChannelManager&) = delete;

        ~ChannelManager();

        ChannelManager& operator=(const ChannelManager&) = delete;

    public:
        static ChannelManager* instance();

        void subscribe(SubProtocol* subProtocol, std::string channel = "");

        void unsubscribe(SubProtocol* subProtocol);

        void sendBroadcast(const std::string& channel, const char* message, std::size_t messageLength, const SubProtocol* excludedClient);
        void
        sendBroadcastStart(const std::string& channel, const char* message, std::size_t messageLength, const SubProtocol* excludedClient);

        void sendBroadcast(const std::string& channel, const std::string& message, const SubProtocol* excludedClient);
        void sendBroadcastStart(const std::string& channel, const std::string& message, const SubProtocol* excludedClient);

        void
        sendBroadcastFrame(const std::string& channel, const char* message, std::size_t messageLength, const SubProtocol* excludedClient);
        void sendBroadcastFrame(const std::string& channel, const std::string& message, const SubProtocol* excludedClient);

        void
        sendBroadcastEnd(const std::string& channel, const char* message, std::size_t messageLength, const SubProtocol* excludedClient);
        void sendBroadcastEnd(const std::string& channel, const std::string& message, const SubProtocol* excludedClient);

        void
        forEachClient(const std::string& channel, const std::function<void(SubProtocol*)>& sendToClient, const SubProtocol* excludedClient);

        static ChannelManager* channelManager;

        std::map<std::string, std::set<SubProtocol*>> channels;
    };

} // namespace web::websocket::server

#endif // WEB_WEBSOCKET_SERVER_CHANNELMANAGER_H
