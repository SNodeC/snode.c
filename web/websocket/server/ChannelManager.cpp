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

#include "web/websocket/server/ChannelManager.h"

#include "web/websocket/server/SubProtocol.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    ChannelManager* ChannelManager::channelManager = nullptr;

    ChannelManager::ChannelManager() {
        ChannelManager::channelManager = this;
    }

    ChannelManager::~ChannelManager() {
        ChannelManager::channelManager = nullptr;
    }

    ChannelManager* ChannelManager::instance() {
        return ChannelManager::channelManager == nullptr ? new ChannelManager() : ChannelManager::channelManager;
    }

    const std::set<SubProtocol*>* ChannelManager::subscribe(const std::string& channel, SubProtocol* subProtocol) {
        if (subProtocol->channel != channel) {
            channels[subProtocol->getName() + "/" + channel].insert(subProtocol);

            if (!subProtocol->channel.empty()) {
                unsubscribe(subProtocol);
            }

            subProtocol->channel = subProtocol->getName() + "/" + channel;
        }

        return &(channels[subProtocol->channel]);
    }

    const std::set<SubProtocol*>* ChannelManager::subscribe(SubProtocol* subProtocol) {
        return subscribe(subProtocol->getName(), subProtocol);
    }

    void ChannelManager::unsubscribe(SubProtocol* subProtocol) {
        if (channels.contains(subProtocol->channel)) {
            channels[subProtocol->channel].erase(subProtocol);

            if (channels[subProtocol->channel].size() == 0) {
                channels.erase(subProtocol->channel);
                if (channels.size() == 0) {
                    delete this;
                }
            }
        }
    }

} // namespace web::websocket::server
