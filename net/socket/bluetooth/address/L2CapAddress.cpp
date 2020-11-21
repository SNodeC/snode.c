/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstring> // for memcpy
#include <sys/socket.h>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#include "L2CapAddress.h"

namespace net::socket::bluetooth::address {

    L2CapAddress::L2CapAddress() {
        sockAddr.l2_family = AF_BLUETOOTH;
        sockAddr.l2_bdaddr = {{0, 0, 0, 0, 0, 0}};
        sockAddr.l2_psm = htobs(0);
    }

    L2CapAddress::L2CapAddress(const L2CapAddress& bta) {
        memcpy(&sockAddr, &bta.sockAddr, sizeof(sockAddr));
    }

    L2CapAddress::L2CapAddress(const struct sockaddr_l2& addr) {
        memcpy(&sockAddr, &addr, sizeof(sockAddr));
    }

    L2CapAddress::L2CapAddress(const std::string& btAddress) {
        sockAddr.l2_family = AF_BLUETOOTH;
        str2ba(btAddress.c_str(), &sockAddr.l2_bdaddr);
        sockAddr.l2_psm = htobs(0);
    }

    L2CapAddress::L2CapAddress(uint16_t psm) {
        sockAddr.l2_family = AF_BLUETOOTH;
        sockAddr.l2_bdaddr = {{0, 0, 0, 0, 0, 0}};
        sockAddr.l2_psm = htobs(psm);
    }

    L2CapAddress::L2CapAddress(const std::string& btAddress, uint16_t psm) {
        sockAddr.l2_family = AF_BLUETOOTH;
        str2ba(btAddress.c_str(), &sockAddr.l2_bdaddr);
        sockAddr.l2_psm = htobs(psm);
    }

    uint16_t L2CapAddress::psm() const {
        return btohs(sockAddr.l2_psm);
    }

    std::string L2CapAddress::address() const {
        char ip[256];
        ba2str(&sockAddr.l2_bdaddr, ip);

        return std::string(ip);
    }

    L2CapAddress& L2CapAddress::operator=(const L2CapAddress& bta) {
        if (this != &bta) {
            memcpy(&sockAddr, &bta.sockAddr, sizeof(sockAddr));
        }

        return *this;
    }

    std::string L2CapAddress::toString() const {
        return address() + "(" + address() + "):" + std::to_string(psm());
    }

    const struct sockaddr_l2& L2CapAddress::getSockAddrRc() const {
        return sockAddr;
    }

} // namespace net::socket::bluetooth::address
