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

#include "net/Socket.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net {

    template <typename SocketAddress>
    Socket<SocketAddress>::Socket(int domain, int type, int protocol)
        : domain(domain)
        , type(type)
        , protocol(protocol) {
    }

    template <typename SocketAddressT>
    int Socket<SocketAddressT>::create(int flags) {
        return core::system::socket(domain, type | flags, protocol);
    }

    template <typename SocketAddress>
    int Socket<SocketAddress>::open(int flags) {
        return attachFd(create(flags));
    }

    template <typename SocketAddress>
    int Socket<SocketAddress>::bind(const SocketAddress& bindAddress) {
        this->bindAddress = bindAddress;

        return core::system::bind(getFd(), &bindAddress.getSockAddr(), sizeof(typename SocketAddress::SockAddr));
    }

    template <typename SocketAddress>
    bool Socket<SocketAddress>::connectInProgress() {
        return errno == EINPROGRESS;
    }

    template <typename SocketAddress>
    ssize_t Socket<SocketAddress>::write_fd([[maybe_unused]] const SocketAddress& destAddress, void* ptr, size_t nbytes, int sendfd) {
        struct msghdr msg;

        struct iovec iov[1];

        union {
            struct cmsghdr cm;
            char control[CMSG_SPACE(sizeof(int))] = {};
        } control_un;
        struct cmsghdr* cmptr;

        msg.msg_control = control_un.control;
        msg.msg_controllen = sizeof(control_un.control);

        cmptr = CMSG_FIRSTHDR(&msg);
        cmptr->cmsg_len = CMSG_LEN(sizeof(int));
        cmptr->cmsg_level = SOL_SOCKET;
        cmptr->cmsg_type = SCM_RIGHTS;

        *reinterpret_cast<int*>(CMSG_DATA(cmptr)) = sendfd;
        msg.msg_name = const_cast<sockaddr*>(&destAddress.getSockAddr());
        msg.msg_namelen = sizeof(destAddress.getSockAddr());
        iov[0].iov_base = ptr;
        iov[0].iov_len = nbytes;
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;

        return sendmsg(getFd(), &msg, 0);
    }

    template <typename SocketAddress>
    ssize_t Socket<SocketAddress>::read_fd(void* ptr, size_t nbytes, int* recvfd) {
        struct msghdr msg;
        struct iovec iov[1];

        union {
            struct cmsghdr cm;
            char control[CMSG_SPACE(sizeof(int))] = {};

        } control_un;

        msg.msg_control = control_un.control;
        msg.msg_controllen = sizeof(control_un.control);

        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        iov[0].iov_base = ptr;
        iov[0].iov_len = nbytes;
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;

        ssize_t n;

        if ((n = recvmsg(getFd(), &msg, 0)) <= 0) {
            return n;
        }

        struct cmsghdr* cmptr;

        if ((cmptr = CMSG_FIRSTHDR(&msg)) != NULL && cmptr->cmsg_len == CMSG_LEN(sizeof(int))) {
            if (cmptr->cmsg_level != SOL_SOCKET) {
                errno = EBADE;
                *recvfd = -1;
                n = -1;
            } else if (cmptr->cmsg_type != SCM_RIGHTS) {
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

        return n;
    }

    template <typename SocketAddress>
    void Socket<SocketAddress>::shutdown(enum shutdown how) {
        core::system::shutdown(getFd(), how);
    }

    template <typename SocketAddress>
    const SocketAddress& Socket<SocketAddress>::getBindAddress() const {
        return bindAddress;
    }

} // namespace net
