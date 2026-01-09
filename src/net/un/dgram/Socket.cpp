/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "net/un/dgram/Socket.h"

#include "net/phy/dgram/PeerSocket.hpp" // IWYU pragma: keep
#include "net/un/phy/PhysicalSocket.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <sys/uio.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un::dgram {

    Socket::Socket()
        : Super(SOCK_DGRAM, 0) {
    }

    Socket::~Socket() {
    }

    ssize_t Socket::sendFd(SocketAddress&& destAddress, int sendfd) {
        return sendFd(destAddress, sendfd);
    }

    ssize_t Socket::sendFd(SocketAddress& destAddress, int sendfd) {
        union {
            struct cmsghdr cm;
            char control[CMSG_SPACE(sizeof(int))] = {};
        } control_un;

        msghdr msg{};
        msg.msg_name = const_cast<sockaddr*>(&destAddress.getSockAddr());
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

    ssize_t Socket::recvFd(int* recvfd) {
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

} // namespace net::un::dgram

template class net::phy::dgram::PeerSocket<net::un::SocketAddress>;
template class net::un::phy::PhysicalSocket<net::phy::dgram::PeerSocket>;
