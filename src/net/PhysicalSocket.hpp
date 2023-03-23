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

#include "core/socket/PhysicalSocket.h"
#include "core/socket/PhysicalSocketOption.h"
#include "net/PhysicalSocket.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    template <typename SocketAddress>
    PhysicalSocket<SocketAddress>::PhysicalSocket() {
        core::Descriptor::attach(-1);
    }

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
    int PhysicalSocket<SocketAddress>::open(const std::map<int, const core::socket::PhysicalSocketOption>& socketOptions,
                                            typename core::socket::PhysicalSocket<SocketAddress>::Flags flags) {
        int ret = Super::attach(core::system::socket(domain, type | flags, protocol));

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
    void PhysicalSocket<SocketAddress>::shutdown(typename core::socket::PhysicalSocket<SocketAddress>::SHUT how) {
        core::system::shutdown(core::Descriptor::getFd(), how);
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
    ssize_t PhysicalSocket<SocketAddress>::sendFd(SocketAddress&& destAddress, int sendfd) {
        return sendFd(destAddress, sendfd);
    }

    template <typename SocketAddressT>
    ssize_t PhysicalSocket<SocketAddressT>::sendFd(SocketAddress& destAddress, int sendfd) {
        union {
            struct cmsghdr cm;
            char control[CMSG_SPACE(sizeof(int))] = {};
        } control_un;

        msghdr msg{};
        msg.msg_name = &destAddress.getSockAddr();
        msg.msg_namelen = destAddress.getSockAddrLen();

        msg.msg_control = control_un.control;
        msg.msg_controllen = sizeof(control_un.control);

        iovec iov[1];
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;

        char ptr = 0;
        msg.msg_iov[0].iov_base = &ptr;
        msg.msg_iov[0].iov_len = 1;

        cmsghdr* cmptr = CMSG_FIRSTHDR(&msg);
        cmptr->cmsg_level = SOL_SOCKET;
        cmptr->cmsg_type = SCM_RIGHTS;
        *reinterpret_cast<int*>(CMSG_DATA(cmptr)) = sendfd;
        cmptr->cmsg_len = CMSG_LEN(sizeof(int));

        return sendmsg(core::Descriptor::getFd(), &msg, 0);
    }

    template <typename SocketAddress>
    ssize_t PhysicalSocket<SocketAddress>::recvFd(int* recvfd) {
        union {
            struct cmsghdr cm;
            char control[CMSG_SPACE(sizeof(int))] = {};
        } control_un;

        msghdr msg{};
        msg.msg_control = control_un.control;
        msg.msg_controllen = sizeof(control_un.control);

        msg.msg_name = nullptr;
        msg.msg_namelen = 0;

        iovec iov[1];
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;

        char ptr = 0;
        msg.msg_iov[0].iov_base = &ptr;
        msg.msg_iov[0].iov_len = 1;

        ssize_t n = 0;

        if ((n = recvmsg(core::Descriptor::getFd(), &msg, 0)) > 0) {
            cmsghdr* cmptr = nullptr;

            if ((cmptr = CMSG_FIRSTHDR(&msg)) != nullptr && cmptr->cmsg_len == CMSG_LEN(sizeof(int))) {
                if (cmptr->cmsg_level != SOL_SOCKET || cmptr->cmsg_type != SCM_RIGHTS) {
                    errno = EBADE;
                    *recvfd = -1;
                    n = -1;
                } else {
                    *recvfd = *reinterpret_cast<int*>(CMSG_DATA(cmptr));
                }
            } else {
                errno = ENOMSG;
                *recvfd = -1;
                n = -1;
            }
        }

        return n;
    }

    template <typename SocketAddress>
    const SocketAddress& PhysicalSocket<SocketAddress>::getBindAddress() const {
        return bindAddress;
    }

} // namespace net
