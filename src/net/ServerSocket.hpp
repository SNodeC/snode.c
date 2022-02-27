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

#include "net/ServerSocket.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    template <typename Config, typename Socket>
    ServerSocket<Config, Socket>::ServerSocket(const std::string& name)
        : config(std::make_shared<Config>(name)) {
    }

    template <typename Config, typename Socket>
    void ServerSocket<Config, Socket>::listen(const SocketAddress& localAddress,
                                              int backlog,
                                              const std::function<void(const SocketAddress&, int)>& onError) const {
        config->setLocalAddress(localAddress);
        config->setBacklog(backlog);

        listen(onError);
    }

    template <typename Config, typename Socket>
    Config& ServerSocket<Config, Socket>::getConfig() {
        return *config;
    }

} // namespace net
