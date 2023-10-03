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

#ifndef NET_UN_STREAM_TLS_CONFIG_CONFIGSOCKETSERVER_H
#define NET_UN_STREAM_TLS_CONFIG_CONFIGSOCKETSERVER_H

#include "net/config/stream/tls/ConfigSocketServer.h"
#include "net/un/stream/config/ConfigSocketServer.h" // IWYU pragma: export

// IWYU pragma: no_include "net/config/stream/tls/ConfigSocketServer.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::un::stream::tls::config {

    class ConfigSocketServer final
        : public net::config::stream::tls::ConfigSocketServer<net::un::stream::config::ConfigSocketServer> {
    public:
        explicit ConfigSocketServer(const std::string& name);

        ~ConfigSocketServer() override;
    };

} // namespace net::un::stream::tls::config

extern template class net::config::stream::tls::ConfigSocketServer<net::un::stream::config::ConfigSocketServer>;

#endif // NET_UN_STREAM_TLS_CONFIG_CONFIGSOCKETSERVER_H
