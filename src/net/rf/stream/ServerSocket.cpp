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

#include "net/rf/stream/ServerSocket.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::rf::stream {

    ServerSocket::ServerSocket(const std::string& name)
        : Super(name)
        , serverConfig(name) {
    }

    void ServerSocket::listen(const std::function<void(const Socket&, int)>& onError) {
        serverConfig.parse(true);

        listen(serverConfig.getBindInterface(), serverConfig.getChannel(), serverConfig.getBacklog(), onError);
    }

    void ServerSocket::listen(uint8_t channel, int backlog, const std::function<void(const Socket& socket, int)>& onError) {
        listen(SocketAddress(channel), backlog, onError);
    }

    void ServerSocket::listen(const std::string& address, int backlog, const std::function<void(const Socket& socket, int)>& onError) {
        listen(SocketAddress(address), backlog, onError);
    }

    void ServerSocket::listen(const std::string& address,
                              uint8_t channel,
                              int backlog,
                              const std::function<void(const Socket& socket, int)>& onError) {
        listen(SocketAddress(address, channel), backlog, onError);
    }

    ServerConfig& ServerSocket::getServerConfig() {
        return serverConfig;
    }

} // namespace net::rf::stream
