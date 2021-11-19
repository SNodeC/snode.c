/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#ifndef NET_SOCKET_BLUETOOTH_L2CAP_SOCKET_H
#define NET_SOCKET_BLUETOOTH_L2CAP_SOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "net/socket/Socket.h"
#include "net/socket/bluetooth/address/L2CapAddress.h"

namespace net::socket::bluetooth::l2cap {

    class Socket : public net::socket::Socket<net::socket::bluetooth::address::L2CapAddress> {
    protected:
        int create(int flags = 0) override;

    public:
        using SocketAddress = net::socket::bluetooth::address::L2CapAddress;

        class Server1 {
        public:
            virtual void listen(const SocketAddress& bindAddress, int backlog, const std::function<void(int)>& onError) const = 0;

            void listen(uint16_t psm, int backlog, const std::function<void(int)>& onError) {
                listen(SocketAddress(psm), backlog, onError);
            }

            void listen(const std::string& address, int backlog, const std::function<void(int)>& onError) {
                listen(SocketAddress(address), backlog, onError);
            }

            void listen(const std::string& address, uint16_t psm, int backlog, const std::function<void(int)>& onError) {
                listen(SocketAddress(address, psm), backlog, onError);
            }
        };

        class Client1 {
        public:
            virtual void connect(const SocketAddress& remoteAddress,
                                 const SocketAddress& bindAddress,
                                 const std::function<void(int)>& onError) const = 0;

            virtual void connect(const SocketAddress& remoteAddress, const std::function<void(int)>& onError) const = 0;

            void connect(const std::string& address, uint16_t psm, const std::function<void(int)>& onError) {
                connect(SocketAddress(address, psm), onError);
            }

            void
            connect(const std::string& address, uint16_t psm, const std::string& bindAddress, const std::function<void(int)>& onError) {
                connect(SocketAddress(address, psm), SocketAddress(bindAddress), onError);
            }

            void connect(const std::string& address,
                         uint16_t psm,
                         const std::string& bindAddress,
                         uint16_t bindPsm,
                         const std::function<void(int)>& onError) {
                connect(SocketAddress(address, psm), SocketAddress(bindAddress, bindPsm), onError);
            }
        };
    };

    class Server {
    public:
        using Socket = net::socket::bluetooth::l2cap::Socket;
        using SocketAddress = Socket::SocketAddress;

        virtual void listen(const SocketAddress& bindAddress, int backlog, const std::function<void(int)>& onError) const = 0;

        void listen(uint8_t channel, int backlog, const std::function<void(int)>& onError) {
            listen(SocketAddress(channel), backlog, onError);
        }

        void listen(const std::string& address, int backlog, const std::function<void(int)>& onError) {
            listen(SocketAddress(address), backlog, onError);
        }

        void listen(const std::string& address, uint8_t channel, int backlog, const std::function<void(int)>& onError) {
            listen(SocketAddress(address, channel), backlog, onError);
        }
    };

    class Client {
    public:
        using Socket = net::socket::bluetooth::l2cap::Socket;
        using SocketAddress = Socket::SocketAddress;

        virtual void
        connect(const SocketAddress& remoteAddress, const SocketAddress& bindAddress, const std::function<void(int)>& onError) const = 0;

        virtual void connect(const SocketAddress& remoteAddress, const std::function<void(int)>& onError) const = 0;

        void connect(const std::string& address, uint8_t channel, const std::function<void(int)>& onError) {
            connect(SocketAddress(address, channel), onError);
        }

        void connect(const std::string& address, uint8_t channel, const std::string& bindAddress, const std::function<void(int)>& onError) {
            connect(SocketAddress(address, channel), SocketAddress(bindAddress), onError);
        }

        void connect(const std::string& address,
                     uint8_t channel,
                     const std::string& bindAddress,
                     uint8_t bindChannel,
                     const std::function<void(int)>& onError) {
            connect(SocketAddress(address, channel), SocketAddress(bindAddress, bindChannel), onError);
        }
    };

} // namespace net::socket::bluetooth::l2cap

#endif // NET_SOCKET_BLUETOOTH_L2CAP_SOCKET_H
