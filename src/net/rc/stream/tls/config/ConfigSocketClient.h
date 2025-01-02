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

#ifndef NET_RC_STREAM_TLS_CONFIG_CONFIGSOCKETCLIENT_H
#define NET_RC_STREAM_TLS_CONFIG_CONFIGSOCKETCLIENT_H

#include "net/config/stream/tls/ConfigSocketClient.h" // IWYU pragma: export
#include "net/rc/stream/config/ConfigSocketClient.h"  // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::rc::stream::tls::config {

    class ConfigSocketClient final : public net::config::stream::tls::ConfigSocketClient<net::rc::stream::config::ConfigSocketClient> {
    public:
        explicit ConfigSocketClient(const std::string& name);

        ~ConfigSocketClient() override;
    };

} // namespace net::rc::stream::tls::config

extern template class net::config::stream::tls::ConfigSocketClient<net::rc::stream::config::ConfigSocketClient>;

#endif // NET_RC_STREAM_TLS_CONFIG_CONFIGSOCKETCLIENT_H
