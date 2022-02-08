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

#ifndef NET_L2_STREAM_CONFIG_CONFIGCLIENTSOCKET_H
#define NET_L2_STREAM_CONFIG_CONFIGCLIENTSOCKET_H

#include "net/config/ConfigBase.h"
#include "net/config/ConfigConnection.h"
#include "net/config/ConfigLocalNew.h"
#include "net/config/ConfigRemoteNew.h"
#include "net/l2/config/ConfigAddress.h"

// IWYU pragma: no_include "net/l2/config/ConfigAddress.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::l2::stream::config {

    class ConfigClientSocket
        : public net::config::ConfigBase
        , public net::l2::config::ConfigAddress<net::config::ConfigRemote>
        , public net::l2::config::ConfigAddress<net::config::ConfigLocal>
        , public net::config::ConfigConnection {
        using ConfigAddressRemote = net::l2::config::ConfigAddress<net::config::ConfigRemote>;
        using ConfigAddressLocal = net::l2::config::ConfigAddress<net::config::ConfigLocal>;

    public:
        explicit ConfigClientSocket(const std::string& name);
    };

} // namespace net::l2::stream::config

#endif // NET_L2_STREAM_CONFIG_CONFIGCLIENTSOCKET_H
