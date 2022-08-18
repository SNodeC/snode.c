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

#include "net/Socket.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    template <typename SocketAddress>
    int Socket<SocketAddress>::bind(const SocketAddress& bindAddress) {
        this->bindAddress = bindAddress;

        return core::system::bind(getFd(), bindAddress, bindAddress.getAddrLen());
    }

    template <typename SocketAddress>
    int Socket<SocketAddress>::getSockname(SocketAddress& socketAddress) {
        socketAddress.getAddrLen() = sizeof(typename SocketAddress::SockAddr);
        return core::system::getsockname(getFd(), socketAddress, &socketAddress.getAddrLen());
    }

    template <typename SocketAddress>
    int Socket<SocketAddress>::getPeername(SocketAddress& socketAddress) {
        socketAddress.getAddrLen() = sizeof(typename SocketAddress::SockAddr);
        return core::system::getpeername(getFd(), socketAddress, &socketAddress.getAddrLen());
    }

    template <typename SocketAddress>
    ssize_t Socket<SocketAddress>::sendFd(SocketAddress&& destAddress, int sendfd) {
        return sendFd(destAddress, sendfd);
    }

    template <typename SocketAddressT>
    ssize_t Socket<SocketAddressT>::sendFd(SocketAddress& destAddress, int sendfd) {
        union {
            struct cmsghdr cm;
            char control[CMSG_SPACE(sizeof(int))] = {};
        } control_un;

        msghdr msg;
        msg.msg_name = &destAddress.getSockAddr();
        msg.msg_namelen = destAddress.getAddrLen();

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

        return sendmsg(getFd(), &msg, 0);
    }

    template <typename SocketAddress>
    ssize_t Socket<SocketAddress>::recvFd(int* recvfd) {
        union {
            struct cmsghdr cm;
            char control[CMSG_SPACE(sizeof(int))] = {};
        } control_un;

        msghdr msg;
        msg.msg_control = control_un.control;
        msg.msg_controllen = sizeof(control_un.control);

        msg.msg_name = nullptr;
        msg.msg_namelen = 0;

        iovec iov[1];
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;

        char ptr;
        msg.msg_iov[0].iov_base = &ptr;
        msg.msg_iov[0].iov_len = 1;

        ssize_t n = 0;

        if ((n = recvmsg(getFd(), &msg, 0)) > 0) {
            cmsghdr* cmptr;

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
    const SocketAddress& Socket<SocketAddress>::getBindAddress() const {
        return bindAddress;
    }

} // namespace net
