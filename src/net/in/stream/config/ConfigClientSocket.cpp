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

#include "net/in/stream/config/ConfigClientSocket.h"

#include "net/config/ConfigLocalNew.hpp"
#include "net/config/ConfigRemoteNew.hpp"
#include "net/in/config/ConfigAddress.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in::stream::config {

    ConfigClientSocket::ConfigClientSocket(const std::string& name)
        : net::config::ConfigBase(name)
        , ConfigAddressRemote(baseSc)
        , ConfigAddressLocal(baseSc)
        , net::config::ConfigConnection(baseSc) {
        ConfigAddressRemote::required();
    }

} // namespace net::in::stream::config

namespace net::config {
    template class ConfigLocal<net::in::SocketAddress>;
}

namespace net::config {
    template class ConfigRemote<net::in::SocketAddress>;
}
