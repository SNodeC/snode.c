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

#include "net/l2/SocketAddress.h"

#include "net/SocketAddress.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/socket.h"

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::l2 {

    SocketAddress::SocketAddress() {
        sockAddr.l2_family = AF_BLUETOOTH;
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

    void SocketAddress::setBtAddress(const std::string& btAddress) {
        str2ba(btAddress.c_str(), &sockAddr.l2_bdaddr);
    }

    void SocketAddress::setPsm(uint16_t psm) {
        sockAddr.l2_psm = htobs(psm);
    }

    uint16_t SocketAddress::psm() const {
        return btohs(sockAddr.l2_psm);
    }

    std::string SocketAddress::address() const {
        char address[256];
        ba2str(&sockAddr.l2_bdaddr, address);

        return address;
    }

    std::string SocketAddress::toString() const {
        return address() + ":" + std::to_string(psm());
    }

} // namespace net::l2

template class net::SocketAddress<sockaddr_l2>;
