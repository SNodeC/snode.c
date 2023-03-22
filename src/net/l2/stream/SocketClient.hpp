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

#include "net/LogicalSocketClient.hpp"
#include "net/l2/stream/SocketClient.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::l2::stream {

    template <typename Config>
    void SocketClient<Config>::connect(const std::string& btAddress,
                                       uint16_t psm,
                                       const std::function<void(const SocketAddress&, int)>& onError) const {
        connect(SocketAddress(btAddress, psm), onError);
    }

    template <typename Config>
    void SocketClient<Config>::connect(const std::string& btAddress,
                                       uint16_t psm,
                                       const std::string& bindBtAddress,
                                       const std::function<void(const SocketAddress&, int)>& onError) const {
        connect(SocketAddress(btAddress, psm), SocketAddress(bindBtAddress), onError);
    }

    template <typename Config>
    void SocketClient<Config>::connect(const std::string& btAddress,
                                       uint16_t psm,
                                       uint16_t bindPsm,
                                       const std::function<void(const SocketAddress&, int)>& onError) const {
        connect(SocketAddress(btAddress, psm), SocketAddress(bindPsm), onError);
    }

    template <typename Config>
    void SocketClient<Config>::connect(const std::string& btAddress,
                                       uint16_t psm,
                                       const std::string& bindBtAddress,
                                       uint16_t bindPsm,
                                       const std::function<void(const SocketAddress&, int)>& onError) const {
        connect(SocketAddress(btAddress, psm), SocketAddress(bindBtAddress, bindPsm), onError);
    }

} // namespace net::l2::stream
