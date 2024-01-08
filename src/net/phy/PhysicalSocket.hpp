/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#include "net/phy/PhysicalSocket.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::phy {

    template <typename SocketAddress>
    PhysicalSocket<SocketAddress>::PhysicalSocket(int domain, int type, int protocol)
        : Descriptor(-1)
        , domain(domain)
        , type(type)
        , protocol(protocol) {
    }

    template <typename SocketAddress>
    PhysicalSocket<SocketAddress>::PhysicalSocket(int fd, const SocketAddress& bindAddress)
        : Descriptor(fd)
        , bindAddress(bindAddress) {
        typename SocketAddress::SockLen optLen = sizeof(domain);
        getSockopt(SOL_SOCKET, SO_DOMAIN, &domain, &optLen);

        optLen = sizeof(type);
        getSockopt(SOL_SOCKET, SO_TYPE, &type, &optLen);

        optLen = sizeof(protocol);
        getSockopt(SOL_SOCKET, SO_PROTOCOL, &protocol, &optLen);
    }

    template <typename SocketAddress>
    PhysicalSocket<SocketAddress>::~PhysicalSocket() {
    }

    template <typename SocketAddress>
    int PhysicalSocket<SocketAddress>::open(const std::map<int, const net::phy::PhysicalSocketOption>& socketOptions, Flags flags) {
        int ret = Super::operator=(core::system::socket(domain, type | flags, protocol)).getFd();

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
    int PhysicalSocket<SocketAddress>::bind(SocketAddress& bindAddress) {
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
    int PhysicalSocket<SocketAddress>::getSockError(int& cErrno) const {
        typename SocketAddress::SockLen cErrnoLen = sizeof(cErrno);
        return getSockopt(SOL_SOCKET, SO_ERROR, &cErrno, &cErrnoLen);
    }

    template <typename SocketAddress>
    int
    PhysicalSocket<SocketAddress>::setSockopt(int level, int optname, const void* optval, typename SocketAddress::SockLen optlen) const {
        return core::system::setsockopt(PhysicalSocket::getFd(), level, optname, optval, optlen);
    }

    template <typename SocketAddress>
    int PhysicalSocket<SocketAddress>::getSockopt(int level, int optname, void* optval, typename SocketAddress::SockLen* optlen) const {
        return core::system::getsockopt(PhysicalSocket::getFd(), level, optname, optval, optlen);
    }

    template <typename SocketAddress>
    int PhysicalSocket<SocketAddress>::getSockName(typename SocketAddress::SockAddr& localSockAddr,
                                                   typename SocketAddress::SockLen& localSockAddrLen) {
        return core::system::getsockname(core::Descriptor::getFd(), reinterpret_cast<sockaddr*>(&localSockAddr), &localSockAddrLen);
    }

    template <typename SocketAddress>
    int PhysicalSocket<SocketAddress>::getPeerName(typename SocketAddress::SockAddr& remoteSockAddr,
                                                   typename SocketAddress::SockLen& remoteSockAddrLen) {
        return core::system::getpeername(core::Descriptor::getFd(), reinterpret_cast<sockaddr*>(&remoteSockAddr), &remoteSockAddrLen);
    }
    /*
        template <typename SocketAddress>
        template <typename Config>
        SocketAddress PhysicalSocket<SocketAddress>::getLocalSocketAddress(Config& config) const {
            typename SocketAddress::SockAddr localSockAddr;
            typename SocketAddress::SockLen localSockAddrLen = sizeof(typename SocketAddress::SockAddr);

            SocketAddress localPeerAddress;
            if (getSockName(localSockAddr, localSockAddrLen) == 0) {
                try {
                    localPeerAddress = config->Local::getSocketAddress(localSockAddr, localSockAddrLen);
                    LOG(TRACE) << config->getInstanceName() << ": Local PeerAddress " << localPeerAddress.toString();
                } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                    LOG(TRACE) << config->getInstanceName() << ": Local PeerAddress: " << badSocketAddress.what();

                    try {
                        localPeerAddress = config->Local::ConfigAddressReverse::getSocketAddress(localSockAddr, localSockAddrLen);
                        LOG(TRACE) << config->getInstanceName() << ": Local PeerAddress " << localPeerAddress.toString();
                    } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) { // cppcheck-suppress shadowVariable
                        LOG(TRACE) << config->getInstanceName() << ": Local PeerAddress: " << badSocketAddress.what();
                    }
                }
            }

            return localPeerAddress;
        }

        template <typename SocketAddress>
        template <typename Config>
        SocketAddress PhysicalSocket<SocketAddress>::getRemoteSocketAddress(Config& config) const {
            typename SocketAddress::SockAddr remoteSockAddr;
            typename SocketAddress::SockLen remoteSockAddrLen = sizeof(typename SocketAddress::SockAddr);

            SocketAddress remotePeerAddress;
            if (getPeerName(remoteSockAddr, remoteSockAddrLen) == 0) {
                try {
                    remotePeerAddress = config->Remote::getSocketAddress(remoteSockAddr, remoteSockAddrLen);
                    LOG(TRACE) << config->getInstanceName() << ": Remote PeerAddress " << remotePeerAddress.toString();
                } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                    LOG(TRACE) << config->getInstanceName() << ": Remote PeerAddress: " << badSocketAddress.what();

                    try {
                        remotePeerAddress = config->Remote::ConfigAddressReverse::getSocketAddress(remoteSockAddr, remoteSockAddrLen);
                        LOG(TRACE) << config->getInstanceName() << ": Remote PeerAddress " << remotePeerAddress.toString();
                    } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) { // cppcheck-suppress shadowVariable
                        LOG(TRACE) << config->getInstanceName() << ": Remote PeerAddress: " << badSocketAddress.what();
                    }
                }
            }

            return remotePeerAddress;
        }
    */
    template <typename SocketAddress>
    SocketAddress PhysicalSocket<SocketAddress>::getBindAddress() const {
        return bindAddress;
    }

} // namespace net::phy
