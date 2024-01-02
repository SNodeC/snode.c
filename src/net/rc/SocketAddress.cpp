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

#include "net/rc/SocketAddress.h"

#include "net/SocketAddress.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::rc {

    SocketAddress::SocketAddress()
        : Super(AF_BLUETOOTH) {
    }

    SocketAddress::SocketAddress(const std::string& btAddress)
        : SocketAddress() {
        setBtAddress(btAddress);
    }

    SocketAddress::SocketAddress(uint8_t channel)
        : SocketAddress() {
        setChannel(channel);
    }

    SocketAddress::SocketAddress(const std::string& btAddress, uint8_t channel)
        : SocketAddress() {
        setBtAddress(btAddress);
        setChannel(channel);
    }

    SocketAddress::SocketAddress(const SockAddr& sockAddr, SockLen sockAddrLen)
        : net::SocketAddress<SockAddr>(sockAddr, sockAddrLen) {
        channel = sockAddr.rc_channel;

        char btAddressC[15];
        ba2str(&sockAddr.rc_bdaddr, btAddressC);
        btAddress = btAddressC;
    }

    SocketAddress& SocketAddress::init() {
        sockAddr.rc_channel = channel;
        str2ba(btAddress.c_str(), &sockAddr.rc_bdaddr);

        return *this;
    }

    SocketAddress& SocketAddress::setBtAddress(const std::string& btAddress) {
        this->btAddress = btAddress;

        return *this;
    }

    std::string SocketAddress::getBtAddress() const {
        return btAddress;
    }

    SocketAddress& SocketAddress::setChannel(uint8_t channel) {
        this->channel = channel;

        return *this;
    }

    uint8_t SocketAddress::getChannel() const {
        return channel;
    }

    std::string SocketAddress::toString() const {
        return std::string(btAddress).append(":").append(std::to_string(channel));
    }

} // namespace net::rc

template class net::SocketAddress<sockaddr_rc>;
