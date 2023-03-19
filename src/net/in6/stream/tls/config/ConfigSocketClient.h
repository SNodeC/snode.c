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

#ifndef NET_IN6_STREAM_TLS_CONFIG_CONFIGSOCKETCLIENT_H
#define NET_IN6_STREAM_TLS_CONFIG_CONFIGSOCKETCLIENT_H

#include "net/in6/stream/config/ConfigSocketClient.h" // IWYU pragma: export
#include "net/stream/config/tls/ConfigSocketClient.h"

// IWYU pragma: no_include "net/stream/config/tls/ConfigSocketClient.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::in6::stream::tls::config {

    class ConfigSocketClient : public net::stream::config::tls::ConfigSocketClient<net::in6::stream::config::ConfigSocketClient> {
    public:
        explicit ConfigSocketClient(const std::string& name);

        ~ConfigSocketClient() override;
    };

} // namespace net::in6::stream::tls::config

#endif // NET_IN6_STREAM_TLS_CONFIG_CONFIGSOCKETCLIENT_H
