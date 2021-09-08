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

#ifndef WEB_WS_SERVER_CHANNELMANAGER_H
#define WEB_WS_SERVER_CHANNELMANAGER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <set>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    class SubProtocol;

    class ChannelManager {
    private:
        ChannelManager();
        ChannelManager(const ChannelManager&) = delete;

        ~ChannelManager();

        ChannelManager& operator=(const ChannelManager&) = delete;

    public:
        static ChannelManager* instance();

        const std::set<SubProtocol*>* subscribe(const std::string& channel, SubProtocol* subProtocol);

        void unsubscribe(SubProtocol* subProtocol);

    private:
        const std::set<SubProtocol*>* subscribe(SubProtocol* subProtocol);

        static ChannelManager* channelManager;

        std::map<std::string, std::set<SubProtocol*>> channels;

        friend class SubProtocol;
    };

} // namespace web::websocket::server

#endif // WEB_WS_SERVER_CHANNELMANAGER_H
