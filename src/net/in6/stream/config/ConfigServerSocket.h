/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#ifndef NET_IN6_STREAM_CONFIG_CONFIGSERVERSOCKET_H
#define NET_IN6_STREAM_CONFIG_CONFIGSERVERSOCKET_H

#include "net/config/ConfigBacklog.h"
#include "net/config/ConfigBase.h"
#include "net/config/ConfigConnection.h"
#include "net/config/ConfigLocal.h"
#include "net/in6/config/ConfigAddress.h"

// IWYU pragma: no_include "net/in6/config/ConfigAddress.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in6::stream::config {

    class ConfigServerSocket
        : public net::config::ConfigBase
        , public net::config::ConfigBacklog
        , public net::in6::config::ConfigAddress<net::config::ConfigLocal>
        , public net::config::ConfigConnection {
    public:
        explicit ConfigServerSocket(const std::string& name);
    };

} // namespace net::in6::stream::config

#endif // NET_IN6_STREAM_CONFIG_CONFIGSERVERSOCKET_H
