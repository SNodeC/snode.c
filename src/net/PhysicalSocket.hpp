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

#include "net/PhysicalSocket.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    template <typename SocketAddress>
    PhysicalSocket<SocketAddress>::PhysicalSocket(int fd) {
        core::Descriptor::attach(fd);

        socklen_t optLen = sizeof(domain);
        getSockopt(SOL_SOCKET, SO_DOMAIN, &domain, &optLen);
        getSockopt(SOL_SOCKET, SO_TYPE, &type, &optLen);
        getSockopt(SOL_SOCKET, SO_PROTOCOL, &protocol, &optLen);
    }

    template <typename SocketAddress>
    PhysicalSocket<SocketAddress>::PhysicalSocket(int domain, int type, int protocol)
        : domain(domain)
        , type(type)
        , protocol(protocol) {
    }

    template <typename SocketAddress>
    PhysicalSocket<SocketAddress>& PhysicalSocket<SocketAddress>::operator=(int fd) {
        core::Descriptor::attach(fd);

        socklen_t optLen = sizeof(domain);
        getSockopt(SOL_SOCKET, SO_DOMAIN, &domain, &optLen);
        getSockopt(SOL_SOCKET, SO_TYPE, &type, &optLen);
        getSockopt(SOL_SOCKET, SO_PROTOCOL, &protocol, &optLen);

        return *this;
    }

    template <typename SocketAddress>
    PhysicalSocket<SocketAddress>::~PhysicalSocket() {
    }

    template <typename SocketAddress>
    int PhysicalSocket<SocketAddress>::open(Flags flags) {
        return Super::attach(core::system::socket(domain, type | flags, protocol));
    }

    template <typename SocketAddress>
    int PhysicalSocket<SocketAddress>::open(const std::map<int, const net::PhysicalSocketOption>& socketOptions, Flags flags) {
        int ret = open(flags);

        if (ret >= 0) {
            for (const auto& [optName, socketOption] : socketOptions) {
                int setSockoptRet =
                    setSockopt(socketOption.getOptLevel(), socketOption.getOptName(), socketOption.getOptValue(), socketOption.getOptLen());

                ret = (ret >= 0 && setSockoptRet < 0) ? setSockoptRet : ret;

                if (ret < 0) {
                    break;
                }
            }
        }

        return ret;
    }

    template <typename SocketAddress>
    int PhysicalSocket<SocketAddress>::bind(const SocketAddress& bindAddress) {
        int ret = core::system::bind(core::Descriptor::getFd(), &bindAddress.getSockAddr(), bindAddress.getSockAddrLen());

        if (ret == 0) {
            this->bindAddress = bindAddress;
        }

        return ret;
    }

    template <typename SocketAddress>
    bool PhysicalSocket<SocketAddress>::isValid() const {
        return core::Descriptor::getFd() >= 0;
    }

    template <typename SocketAddress>
    int PhysicalSocket<SocketAddress>::getSockError() const {
        int cErrno = 0;
        socklen_t cErrnoLen = sizeof(cErrno);
        int err = getSockopt(SOL_SOCKET, SO_ERROR, &cErrno, &cErrnoLen);

        return err < 0 ? err : cErrno;
    }

    template <typename SocketAddress>
    int PhysicalSocket<SocketAddress>::setSockopt(int level, int optname, const void* optval, socklen_t optlen) const {
        return core::system::setsockopt(PhysicalSocket::getFd(), level, optname, optval, optlen);
    }

    template <typename SocketAddress>
    int PhysicalSocket<SocketAddress>::getSockopt(int level, int optname, void* optval, socklen_t* optlen) const {
        return core::system::getsockopt(PhysicalSocket::getFd(), level, optname, optval, optlen);
    }

    template <typename SocketAddress>
    int PhysicalSocket<SocketAddress>::getSockname(SocketAddress& socketAddress) {
        socketAddress.getSockAddrLen() = sizeof(typename SocketAddress::SockAddr);
        return core::system::getsockname(core::Descriptor::getFd(), &socketAddress.getSockAddr(), &socketAddress.getSockAddrLen());
    }

    template <typename SocketAddress>
    int PhysicalSocket<SocketAddress>::getPeername(SocketAddress& socketAddress) {
        socketAddress.getSockAddrLen() = sizeof(typename SocketAddress::SockAddr);
        return core::system::getpeername(core::Descriptor::getFd(), &socketAddress.getSockAddr(), &socketAddress.getSockAddrLen());
    }

    template <typename SocketAddress>
    const SocketAddress& PhysicalSocket<SocketAddress>::getBindAddress() const {
        return bindAddress;
    }

} // namespace net
