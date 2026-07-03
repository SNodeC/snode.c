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

#ifndef CORE_SOCKET_STREAM_SOCKETADDRESSUTILS_HPP
#define CORE_SOCKET_STREAM_SOCKETADDRESSUTILS_HPP

#include "core/socket/stream/SocketAddressUtils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <iomanip>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::socket::stream {

    template <typename SocketAddress, typename PhysicalSocket, typename Config>
    SocketAddress getLocalSocketAddress(PhysicalSocket& physicalSocket, Config& config) {
        typename SocketAddress::SockAddr localSockAddr;
        typename SocketAddress::SockLen localSockAddrLen = sizeof(typename SocketAddress::SockAddr);

        SocketAddress localPeerAddress;
        if (physicalSocket.getSockName(localSockAddr, localSockAddrLen) == 0) {
            try {
                localPeerAddress = config->Local::getSocketAddress(localSockAddr, localSockAddrLen);
                LOG(TRACE) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
                           << "  PeerAddress (local): " << localPeerAddress.toString();
            } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                LOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
                             << "  PeerAddress (local): " << badSocketAddress.what();
            }
        } else {
            PLOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
                          << " PeerAddress (local) not retrievable";
        }

        return localPeerAddress;
    }

    template <typename SocketAddress, typename PhysicalSocket, typename Config>
    SocketAddress getRemoteSocketAddress(PhysicalSocket& physicalSocket, Config& config) {
        typename SocketAddress::SockAddr remoteSockAddr;
        typename SocketAddress::SockLen remoteSockAddrLen = sizeof(typename SocketAddress::SockAddr);

        SocketAddress remotePeerAddress;
        if (physicalSocket.getPeerName(remoteSockAddr, remoteSockAddrLen) == 0) {
            try {
                remotePeerAddress = config->Remote::getSocketAddress(remoteSockAddr, remoteSockAddrLen);
                LOG(TRACE) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
                           << "  PeerAddress (remote): " << remotePeerAddress.toString();
            } catch (const typename SocketAddress::BadSocketAddress& badSocketAddress) {
                LOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
                             << "  PeerAddress (remote): " << badSocketAddress.what();
            }
        } else {
            PLOG(WARNING) << config->getInstanceName() << " [" << physicalSocket.getFd() << "]" << std::setw(25)
                          << " PeerAddress (remote) not retrievble";
        }

        return remotePeerAddress;
    }

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_SOCKETADDRESSUTILS_HPP
