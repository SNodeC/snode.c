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

#include "net/in6/stream/SocketClient.h"
#include "net/stream/SocketClient.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in6::stream {

    template <typename Config>
    void SocketClient<Config>::connect(const std::string& ipOrHostname,
                                       uint16_t port,
                                       const std::function<void(const SocketAddress&, int)>& onError) const {
        connect(SocketAddress(ipOrHostname, port), onError);
    }

    template <typename Config>
    void SocketClient<Config>::connect(const std::string& ipOrHostname,
                                       uint16_t port,
                                       const std::string& bindIpOrHostname,
                                       const std::function<void(const SocketAddress&, int)>& onError) const {
        connect(SocketAddress(ipOrHostname, port), SocketAddress(bindIpOrHostname), onError);
    }

    template <typename Config>
    void SocketClient<Config>::connect(const std::string& ipOrHostname,
                                       uint16_t port,
                                       uint16_t bindPort,
                                       const std::function<void(const SocketAddress&, int)>& onError) const {
        connect(SocketAddress(ipOrHostname, port), SocketAddress(bindPort), onError);
    }

    template <typename Config>
    void SocketClient<Config>::connect(const std::string& ipOrHostname,
                                       uint16_t port,
                                       const std::string& bindIpOrHostname,
                                       uint16_t bindPort,
                                       const std::function<void(const SocketAddress&, int)>& onError) const {
        connect(SocketAddress(ipOrHostname, port), SocketAddress(bindIpOrHostname, bindPort), onError);
    }

} // namespace net::in6::stream
