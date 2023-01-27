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

#ifndef NET_L2_STREAM_LEGACY_CONFIG_CONFIGSOCKETCLIENT_H
#define NET_L2_STREAM_LEGACY_CONFIG_CONFIGSOCKETCLIENT_H

#include "net/stream/config/legacy/ConfigSocketClient.hpp"
//
#include "net/l2/stream/config/ConfigSocketClient.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::l2::stream::legacy::config {

    class ConfigSocketClient : public net::stream::config::legacy::ConfigSocketClient<net::l2::stream::config::ConfigSocketClient> {
    public:
        explicit ConfigSocketClient(const std::string& name)
            : net::stream::config::legacy::ConfigSocketClient<net::l2::stream::config::ConfigSocketClient>(name) {
        }
    };

} // namespace net::l2::stream::legacy::config

#endif // NET_L2_STREAM_LEGACY_CONFIG_CONFIGSOCKETCLIENT_H
