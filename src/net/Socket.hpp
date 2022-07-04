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
    Socket<SocketAddress>::Socket() {
        Descriptor::open(-1);
    }

    template <typename SocketAddress>
    Socket<SocketAddress>::Socket(int fd) {
        Descriptor::open(fd);

        socklen_t optLen = sizeof(domain);
        getSockopt(SOL_SOCKET, SO_DOMAIN, &domain, &optLen);
        getSockopt(SOL_SOCKET, SO_TYPE, &type, &optLen);
        getSockopt(SOL_SOCKET, SO_PROTOCOL, &protocol, &optLen);
    }

    template <typename SocketAddress>
    Socket<SocketAddress>::Socket(int domain, int type, int protocol)
        : domain(domain)
        , type(type)
        , protocol(protocol) {
    }

    template <typename SocketAddress>
    Socket<SocketAddress>& Socket<SocketAddress>::operator=(int fd) {
        Descriptor::open(fd);

        socklen_t optLen = sizeof(domain);
        getSockopt(SOL_SOCKET, SO_DOMAIN, &domain, &optLen);
        getSockopt(SOL_SOCKET, SO_TYPE, &type, &optLen);
        getSockopt(SOL_SOCKET, SO_PROTOCOL, &protocol, &optLen);

        return *this;
    }

    template <typename SocketAddressT>
    int Socket<SocketAddressT>::create(SOCK flags) {
        return core::system::socket(domain, type | flags, protocol);
    }

    template <typename SocketAddress>
    int Socket<SocketAddress>::open(SOCK flags) {
        return Descriptor::open(create(flags));
    }

    template <typename SocketAddress>
    int Socket<SocketAddress>::bind(const SocketAddress& bindAddress) {
        this->bindAddress = bindAddress;

        return core::system::bind(getFd(), bindAddress, bindAddress.getAddrLen());
    }

    template <typename SocketAddress>
    bool Socket<SocketAddress>::isValid() const {
        return getFd() >= 0;
    }

    template <typename SocketAddress>
    int Socket<SocketAddress>::reuseAddress() {
        int sockopt = 1;
        return setSockopt(SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
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
    int Socket<SocketAddress>::getSockError() {
        int cErrno = 0;
        socklen_t cErrnoLen = sizeof(cErrno);
        int err = getSockopt(SOL_SOCKET, SO_ERROR, &cErrno, &cErrnoLen);

        return err == 0 ? cErrno : err;
    }

    template <typename SocketAddress>
    int Socket<SocketAddress>::setSockopt(int level, int optname, const void* optval, socklen_t optlen) {
        return core::system::setsockopt(Socket::getFd(), level, optname, optval, optlen);
    }

    template <typename SocketAddress>
    int Socket<SocketAddress>::getSockopt(int level, int optname, void* optval, socklen_t* optlen) {
        return core::system::getsockopt(Socket::getFd(), level, optname, optval, optlen);
    }

    template <typename SocketAddress>
    ssize_t Socket<SocketAddress>::write_fd(const SocketAddress& destAddress, void* ptr, size_t nbytes, int sendfd) {
        union {
            struct cmsghdr cm;
            char control[CMSG_SPACE(sizeof(int))] = {};
        } control_un;

        msghdr msg;
        msg.msg_name = const_cast<sockaddr*>(&destAddress.getSockAddr());
        msg.msg_namelen = destAddress.getAddrLen();

        msg.msg_control = control_un.control;
        msg.msg_controllen = sizeof(control_un.control);

        iovec iov[1];
        msg.msg_iov = iov;
        msg.msg_iovlen = 1;
        msg.msg_iov[0].iov_base = ptr;
        msg.msg_iov[0].iov_len = nbytes;

        cmsghdr* cmptr = CMSG_FIRSTHDR(&msg);
        cmptr->cmsg_level = SOL_SOCKET;
        cmptr->cmsg_type = SCM_RIGHTS;
        *reinterpret_cast<int*>(CMSG_DATA(cmptr)) = sendfd;
        cmptr->cmsg_len = CMSG_LEN(sizeof(int));

        return sendmsg(getFd(), &msg, 0);
    }

    template <typename SocketAddress>
    ssize_t Socket<SocketAddress>::read_fd(void* ptr, size_t nbytes, int* recvfd) {
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
        msg.msg_iov[0].iov_base = ptr;
        msg.msg_iov[0].iov_len = nbytes;

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
    void Socket<SocketAddress>::shutdown(SHUT how) {
        core::system::shutdown(getFd(), how);
    }

    template <typename SocketAddress>
    const SocketAddress& Socket<SocketAddress>::getBindAddress() const {
        return bindAddress;
    }

} // namespace net
