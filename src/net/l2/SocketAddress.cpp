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

#include "net/l2/SocketAddress.h"

#include "net/SocketAddress.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstring> // for memset

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::l2 {

    std::string bad_bdaddress::message;

    SocketAddress::SocketAddress() {
        std::memset(&sockAddr, 0, sizeof(sockAddr));

        sockAddr.l2_family = AF_BLUETOOTH;
        sockAddr.l2_bdaddr = {{0, 0, 0, 0, 0, 0}};
        sockAddr.l2_psm = htobs(0);
    }

    SocketAddress::SocketAddress(const std::string& btAddress)
        : SocketAddress() {
        setAddress(btAddress);
    }

    SocketAddress::SocketAddress(uint16_t psm)
        : SocketAddress() {
        setPsm(psm);
    }

    SocketAddress::SocketAddress(const std::string& btAddress, uint16_t psm)
        : SocketAddress() {
        setAddress(btAddress);
        setPsm(psm);
    }

    void SocketAddress::setAddress(const std::string& btAddress) {
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

    bad_bdaddress::bad_bdaddress(const std::string& bdAddress) {
        message = "Bad bad bdaddress \"" + bdAddress + "\"";
    }

    const char* bad_bdaddress::what() const noexcept {
        return message.c_str();
    }

} // namespace net::l2

namespace net {
    template class SocketAddress<struct sockaddr_l2>;
} // namespace net
