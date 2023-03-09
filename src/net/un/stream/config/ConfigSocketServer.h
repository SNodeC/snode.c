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

#ifndef NET_UN_STREAM_CONFIG_CONFIGSOCKETSERVER_H
#define NET_UN_STREAM_CONFIG_CONFIGSOCKETSERVER_H

#include "net/stream/config/ConfigSocketServer.h" // IWYU pragma: export
//
#include "net/un/config/ConfigAddress.h"

namespace net::config {
    class ConfigInstance;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un::stream::config {

    class ConfigSocketServer : public net::stream::config::ConfigSocketServer<net::un::config::ConfigAddress> {
    public:
        explicit ConfigSocketServer(net::config::ConfigInstance* instance);
    };

} // namespace net::un::stream::config

#endif // NET_UN_STREAM_CONFIG_CONFIGSOCKETSERVER_H
