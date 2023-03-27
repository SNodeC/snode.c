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

#ifndef NET_L2_STREAM_SOCKETSERVER_H
#define NET_L2_STREAM_SOCKETSERVER_H

#include "net/l2/stream/PhysicalServerSocket.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::l2::stream {

    template <typename SocketServerT>
    class SocketServer : public SocketServerT {
    private:
        using Super = SocketServerT;

    protected:
        using Super::Super;

    public:
        using Super::listen;

        void listen(uint16_t psm, const std::function<void(const SocketAddress&, int)>& onError) const {
            listen(SocketAddress(psm), onError);
        }

        void listen(uint16_t psm, int backlog, const std::function<void(const SocketAddress&, int)>& onError) const {
            listen(SocketAddress(psm), backlog, onError);
        }

        void listen(const std::string& btAddress, uint16_t psm, const std::function<void(const SocketAddress&, int)>& onError) const {
            listen(SocketAddress(btAddress, psm), onError);
        }

        void listen(const std::string& btAddress,
                    uint16_t psm,
                    int backlog,
                    const std::function<void(const SocketAddress& SocketAddress, int)>& onError) const {
            listen(SocketAddress(btAddress, psm), backlog, onError);
        }
    };

} // namespace net::l2::stream

#endif // NET_L2_STREAM_SOCKETSERVER_H
