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

#ifndef NET_IN_STREAM_LEGACY_CONFIG_CONFIGSOCKETSERVER_H
#define NET_IN_STREAM_LEGACY_CONFIG_CONFIGSOCKETSERVER_H

#include "net/in/stream/config/ConfigSocketServer.h" // IWYU pragma: export
#include "net/config/socket/stream/legacy/ConfigSocketServer.h"

// IWYU pragma: no_include "net/config/socket/stream/legacy/ConfigSocketServer.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::in::stream::legacy::config {

    class ConfigSocketServer final : public net::stream::config::legacy::ConfigSocketServer<net::in::stream::config::ConfigSocketServer> {
    public:
        explicit ConfigSocketServer(const std::string& name);

        ~ConfigSocketServer() override;
    };

} // namespace net::in::stream::legacy::config

extern template class net::stream::config::legacy::ConfigSocketServer<net::in::stream::config::ConfigSocketServer>;

#endif // NET_IN_STREAM_LEGACY_CONFIG_CONFIGSOCKETSERVER_H
