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

#include "net/rc/stream/SocketClient.h"
#include "net/stream/SocketClient.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::rc::stream {

    template <typename Config>
    SocketClient<Config>::SocketClient(const std::string& name)
        : Super(name) {
    }

    template <typename Config>
    void SocketClient<Config>::connect(const std::string& address,
                                       uint8_t channel,
                                       const std::function<void(const SocketAddress&, int)>& onError) const {
        connect(SocketAddress(address, channel), onError);
    }

    template <typename Config>
    void SocketClient<Config>::connect(const std::string& address,
                                       uint8_t channel,
                                       const std::string& localAddress,
                                       const std::function<void(const SocketAddress&, int)>& onError) const {
        connect(SocketAddress(address, channel), SocketAddress(localAddress), onError);
    }

    template <typename Config>
    void SocketClient<Config>::connect(const std::string& address,
                                       uint8_t channel,
                                       const std::string& localAddress,
                                       uint8_t bindChannel,
                                       const std::function<void(const SocketAddress&, int)>& onError) const {
        connect(SocketAddress(address, channel), SocketAddress(localAddress, bindChannel), onError);
    }

} // namespace net::rc::stream
