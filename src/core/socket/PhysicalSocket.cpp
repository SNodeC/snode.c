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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket {

    PhysicalSocket::PhysicalSocket() {
        Descriptor::open(-1);
    }

    PhysicalSocket::PhysicalSocket(int fd) {
        Descriptor::open(fd);

        socklen_t optLen = sizeof(domain);
        getSockopt(SOL_SOCKET, SO_DOMAIN, &domain, &optLen);
        getSockopt(SOL_SOCKET, SO_TYPE, &type, &optLen);
        getSockopt(SOL_SOCKET, SO_PROTOCOL, &protocol, &optLen);
    }

    PhysicalSocket::PhysicalSocket(int domain, int type, int protocol)
        : domain(domain)
        , type(type)
        , protocol(protocol) {
    }

    PhysicalSocket& PhysicalSocket::operator=(int fd) {
        Descriptor::open(fd);

        socklen_t optLen = sizeof(domain);
        getSockopt(SOL_SOCKET, SO_DOMAIN, &domain, &optLen);
        getSockopt(SOL_SOCKET, SO_TYPE, &type, &optLen);
        getSockopt(SOL_SOCKET, SO_PROTOCOL, &protocol, &optLen);

        return *this;
    }

    int PhysicalSocket::open(const std::map<int, PhysicalSocketOption>& socketOptions, Flags flags) {
        int ret = Super::open(core::system::socket(domain, type | flags, protocol));

        if (ret >= 0) {
            for (auto [optName, socketOption] : socketOptions) {
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

    bool PhysicalSocket::isValid() const {
        return getFd() >= 0;
    }

    int PhysicalSocket::getSockError() const {
        int cErrno = 0;
        socklen_t cErrnoLen = sizeof(cErrno);
        int err = getSockopt(SOL_SOCKET, SO_ERROR, &cErrno, &cErrnoLen);

        return err < 0 ? err : cErrno;
    }

    void PhysicalSocket::shutdown(SHUT how) {
        core::system::shutdown(getFd(), how);
    }

    int PhysicalSocket::setSockopt(int level, int optname, const void* optval, socklen_t optlen) const {
        return core::system::setsockopt(PhysicalSocket::getFd(), level, optname, optval, optlen);
    }

    int PhysicalSocket::getSockopt(int level, int optname, void* optval, socklen_t* optlen) const {
        return core::system::getsockopt(PhysicalSocket::getFd(), level, optname, optval, optlen);
    }

} // namespace core::socket
