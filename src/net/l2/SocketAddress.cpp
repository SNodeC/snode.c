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

#include "net/l2/SocketAddress.h"

#include "net/SocketAddress.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::l2 {

    SocketAddress::SocketAddress()
        : Super(AF_BLUETOOTH) {
    }

    SocketAddress::SocketAddress(const std::string& btAddress)
        : SocketAddress() {
        setBtAddress(btAddress);
    }

    SocketAddress::SocketAddress(uint16_t psm)
        : SocketAddress() {
        setPsm(psm);
    }

    SocketAddress::SocketAddress(const std::string& btAddress, uint16_t psm)
        : SocketAddress() {
        setBtAddress(btAddress);
        setPsm(psm);
    }

    SocketAddress::SocketAddress(const SockAddr& sockAddr, SockLen sockAddrLen)
        : net::SocketAddress<SockAddr>(sockAddr, sockAddrLen) {
        psm = btohs(sockAddr.l2_psm);

        char btAddressC[15];
        ba2str(&sockAddr.l2_bdaddr, btAddressC);
        btAddress = btAddressC;
    }

    SocketAddress& SocketAddress::init() {
        sockAddr.l2_psm = htobs(psm);
        str2ba(btAddress.c_str(), &sockAddr.l2_bdaddr);

        return *this;
    }

    SocketAddress& SocketAddress::setBtAddress(const std::string& btAddress) {
        this->btAddress = btAddress;

        return *this;
    }

    std::string SocketAddress::getBtAddress() const {
        return btAddress;
    }

    SocketAddress& SocketAddress::setPsm(uint16_t psm) {
        this->psm = psm;

        return *this;
    }

    uint16_t SocketAddress::getPsm() const {
        return psm;
    }

    std::string SocketAddress::toString([[maybe_unused]] bool expanded) const {
        return std::string(btAddress).append(":").append(std::to_string(psm));
    }

} // namespace net::l2

template class net::SocketAddress<sockaddr_l2>;
