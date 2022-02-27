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

#include "net/ClientSocket.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    template <typename Config, typename Socket>
    ClientSocket<Config, Socket>::ClientSocket(const std::string& name)
        : config(std::make_shared<Config>(name)) {
    }

    template <typename Config, typename Socket>
    void ClientSocket<Config, Socket>::connect(const SocketAddress& remoteAddress,
                                               const SocketAddress& localAddress,
                                               const std::function<void(const SocketAddress&, int)>& onError) const {
        config->setLocalAddress(localAddress);

        connect(remoteAddress, onError);
    }

    template <typename Config, typename Socket>
    void ClientSocket<Config, Socket>::connect(const SocketAddress& remoteAddress,
                                               const std::function<void(const SocketAddress&, int)>& onError) const {
        config->setRemoteAddress(remoteAddress);

        connect(onError);
    }

    template <typename Config, typename Socket>
    Config& ClientSocket<Config, Socket>::getConfig() {
        return *config;
    }

} // namespace net
