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

#include "net/in/stream/tls/config/ConfigSocketClient.h"

#include "net/config/stream/tls/ConfigSocketClient.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::in::stream::tls::config {

    ConfigSocketClient::ConfigSocketClient(const std::string& name)
        : net::config::stream::tls::ConfigSocketClient<net::in::stream::config::ConfigSocketClient>(name) {
    }

    ConfigSocketClient::~ConfigSocketClient() {
    }

} // namespace net::in::stream::tls::config

template class net::config::stream::tls::ConfigSocketClient<net::in::stream::config::ConfigSocketClient>;
