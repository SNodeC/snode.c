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

#ifndef NET_SOCKET_IP_SOCKET_IPV4_STREAM_SOCKET_H
#define NET_SOCKET_IP_SOCKET_IPV4_STREAM_SOCKET_H

#include "net/socket/Socket.h"
#include "net/socket/ip/socket/ipv4/InetAddress.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::socket::ip::socket::ipv4::stream {

    class Socket : public net::socket::Socket<net::socket::ip::socket::ipv4::InetAddress> {
    protected:
        int create(int flags) override;

    public:
        using SocketAddress = net::socket::ip::socket::ipv4::InetAddress;

        class Server {
        public:
            virtual void listen(const SocketAddress& bindAddress, int backlog, const std::function<void(int)>& onError) const = 0;

            void listen(uint16_t port, int backlog, const std::function<void(int)>& onError) {
                listen(SocketAddress(port), backlog, onError);
            }

            void listen(const std::string& ipOrHostname, int backlog, const std::function<void(int)>& onError) {
                listen(SocketAddress(ipOrHostname), backlog, onError);
            }

            void listen(const std::string& ipOrHostname, uint16_t port, int backlog, const std::function<void(int)>& onError) {
                listen(SocketAddress(ipOrHostname, port), backlog, onError);
            }
        };

        class Client {
        public:
            virtual void connect(const SocketAddress& remoteAddress,
                                 const SocketAddress& bindAddress,
                                 const std::function<void(int)>& onError) const = 0;

            virtual void connect(const SocketAddress& remoteAddress, const std::function<void(int)>& onError) const = 0;

            void connect(const std::string& ipOrHostname, uint16_t port, const std::function<void(int)>& onError) {
                connect(SocketAddress(ipOrHostname, port), onError);
            }

            void connect(const std::string& ipOrHostname,
                         uint16_t port,
                         const std::string& bindIpOrHostname,
                         const std::function<void(int)>& onError) {
                connect(SocketAddress(ipOrHostname, port), SocketAddress(bindIpOrHostname), onError);
            }

            void connect(const std::string& ipOrHostname,
                         uint16_t port,
                         const std::string& bindIpOrHostname,
                         uint16_t bindPort,
                         const std::function<void(int)>& onError) {
                connect(SocketAddress(ipOrHostname, port), SocketAddress(bindIpOrHostname, bindPort), onError);
            }
        };
    };

    class Server {
    public:
        using Socket = net::socket::ip::socket::ipv4::stream::Socket;
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
        using Socket = net::socket::ip::socket::ipv4::stream::Socket;
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

} // namespace net::socket::ip::socket::ipv4::stream

#endif // NET_SOCKET_IP_SOCKET_IPV4_STREAM_SOCKET_H
