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

#ifndef MARIADBSOCKETCONNECTOR_H
#define MARIADBSOCKETCONNECTOR_H

#include <mysql.h>
#include "log/Logger.h"

#include "net/ConnectEventReceiver.h"
#include "net/system/socket.h"


namespace database::mariadb {
    class SocketContextFactory;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace database::mariadb {

    template <typename SocketConnectionT>
    class MariaDBSocketConnector
        : public SocketConnectionT::Socket
        , public ConnectEventReceiver {
        MariaDBSocketConnector() = delete;
        MariaDBSocketConnector(const MariaDBSocketConnector&) = delete;
        MariaDBSocketConnector& operator=(const MariaDBSocketConnector&) = delete;

    public:
        using SocketConnection = SocketConnectionT;
        using Socket = typename SocketConnection::Socket;
        using SocketAddress = typename Socket::SocketAddress;
        using Socket::getFd;

        MariaDBSocketConnector(
            const std::shared_ptr<SocketContextFactory>& socketContextFactory,
            const std::function<void(const SocketAddress&, const SocketAddress&)>& onConnect,
            const std::function<void(SocketConnection*)>& onConnected,
            const std::function<void(SocketConnection*)>& onDisconnect,
            const std::map<std::string, std::any>& options)
                : socketContextFactory(socketContextFactory)
                , options(options)
                , onConnect(onConnect)
                , onConnected(onConnected)
                , onDisconnect(onDisconnect) {
        }

        virtual ~MariaDBSocketConnector() = default;

        void connect(const std::function<void(int)>& onError) {
            this->onError = onError;

            std::string hostname = "localhost";
            std::string username = "rathalin";
            std::string password = "rathalin";
            unsigned int port = 3306;
            std::string socket_name = "/run/mysqld/mysqld.sock";
            std::string db_name = "snodec";
            unsigned int flags = 0;

            MYSQL mysql;
            MYSQL* connection;
            MYSQL_RES* result;
            
            mysql_init(&mysql);
            mysql_options(&mysql, MYSQL_OPT_NONBLOCK, 0);

            mysql_real_connect_start(
                &connection, 
                &mysql, 
                hostname.c_str(), 
                username.c_str(), 
                password.c_str(), 
                db_name.c_str(), 
                port,
                socket_name.c_str(), 
                flags
            );
            net::ConnectEventReceiver::enable(mysql_get_socket(mysql));


            // Socket::open(
            //     [this, &bindAddress, &remoteAddress, &onError](int errnum) -> void {
            //         if (errnum > 0) {
            //             onError(errnum);
            //             destruct();
            //         } else {
            //             Socket::bind(bindAddress, [this, &remoteAddress, &onError](int errnum) -> void {
            //                 if (errnum > 0) {
            //                     onError(errnum);
            //                     destruct();
            //                 } else {
            //                     int ret = net::system::connect(getFd(), &remoteAddress.getSockAddr(), remoteAddress.getSockAddrLen());

            //                     if (ret == 0 || errno == EINPROGRESS) {
            //                         ConnectEventReceiver::enable(getFd());
            //                     } else {
            //                         onError(errno);
            //                         destruct();
            //                     }
            //                 }
            //             });
            //         }
            //     },
            //     SOCK_NONBLOCK);
        }

    private:
        void connectEvent() override {

            VLOG(0) << "connectEvent in MariaDBSocketConnector.h";
//            int cErrno = -1; 
//            socklen_t cErrnoLen = sizeof(cErrno);

//            int err = net::system::getsockopt(Socket::getFd(), SOL_SOCKET, SO_ERROR, &cErrno, &cErrnoLen);

//            if (err == 0) {
//                errno = cErrno;
//                if (errno != EINPROGRESS) {
//                    if (errno == 0) {
//                        typename SocketAddress::SockAddr localAddress{};
//                        socklen_t localAddressLength = sizeof(localAddress);

//                        typename SocketAddress::SockAddr remoteAddress{};
//                        socklen_t remoteAddressLength = sizeof(remoteAddress);

//                        if (net::system::getsockname(getFd(), reinterpret_cast<sockaddr*>(&localAddress), &localAddressLength) == 0 &&
//                            net::system::getpeername(getFd(), reinterpret_cast<sockaddr*>(&remoteAddress), &remoteAddressLength) == 0) {
//                            SocketConnection* socketConnection = new SocketConnection(
//                                getFd(),
//                                socketContextFactory,
//                                SocketAddress(localAddress),
//                                SocketAddress(remoteAddress),
//                                onConnect,
//                                onDisconnect
//                            );
//                            MariaDBSocketConnector::dontClose(true);
//                            MariaDBSocketConnector::ConnectEventReceiver::disable();

//                            onConnected(socketConnection);
//                            onError(0);
//                        } else {
//                            MariaDBSocketConnector::ConnectEventReceiver::disable();
//                            onError(errno);
//                        }
//                    } else {
//                        MariaDBSocketConnector::ConnectEventReceiver::disable();
//                        onError(errno);
//                    }
//                } else {
//                    // connect() still in progress
//                }
//            } else {
//                MariaDBSocketConnector::ConnectEventReceiver::disable();
//                onError(errno);
//            }
        }

        void unobserved() override {
            destruct();
        }

    protected:
        void destruct() {
            delete this;
        }

    private:
        std::shared_ptr<SocketContextFactory> socketContextFactory = nullptr;

    protected:
        std::function<void(int err)> onError;
        std::map<std::string, std::any> options;

    private:
        std::function<void(const SocketAddress&, const SocketAddress&)> onConnect;
        std::function<void(SocketConnection*)> onDestruct;
        std::function<void(SocketConnection*)> onConnected;
        std::function<void(SocketConnection*)> onDisconnect;
    };

} // namespace net::socket::stream

#endif // MARIADBSOCKETCONNECTOR_H
