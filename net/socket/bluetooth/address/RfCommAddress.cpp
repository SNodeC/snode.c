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

#include "RfCommAddress.h"

namespace net::socket::bluetooth::address {

    RfCommAddress::RfCommAddress() {
        sockAddr.rc_family = AF_BLUETOOTH;
        sockAddr.rc_bdaddr = {{0, 0, 0, 0, 0, 0}};
        sockAddr.rc_channel = (uint8_t) 0;
    }

    RfCommAddress::RfCommAddress(const RfCommAddress& bta) {
        memcpy(&sockAddr, &bta.sockAddr, sizeof(sockAddr));
    }

    RfCommAddress::RfCommAddress(const struct sockaddr_rc& addr) {
        memcpy(&sockAddr, &addr, sizeof(sockAddr));
    }

    RfCommAddress::RfCommAddress(const std::string& btAddress) {
        sockAddr.rc_family = AF_BLUETOOTH;
        str2ba(btAddress.c_str(), &sockAddr.rc_bdaddr);
        sockAddr.rc_channel = (uint8_t) 0;
    }

    RfCommAddress::RfCommAddress(uint8_t channel) {
        sockAddr.rc_family = AF_BLUETOOTH;
        sockAddr.rc_bdaddr = {{0, 0, 0, 0, 0, 0}};
        sockAddr.rc_channel = channel;
    }

    RfCommAddress::RfCommAddress(const std::string& btAddress, uint8_t channel) {
        sockAddr.rc_family = AF_BLUETOOTH;
        str2ba(btAddress.c_str(), &sockAddr.rc_bdaddr);
        sockAddr.rc_channel = channel;
    }

    uint8_t RfCommAddress::channel() const {
        return sockAddr.rc_channel;
    }

    std::string RfCommAddress::address() const {
        char ip[256];
        ba2str(&sockAddr.rc_bdaddr, ip);

        return std::string(ip);
    }

    RfCommAddress& RfCommAddress::operator=(const RfCommAddress& bta) {
        if (this != &bta) {
            memcpy(&sockAddr, &bta.sockAddr, sizeof(sockAddr));
        }

        return *this;
    }

    std::string RfCommAddress::toString() const {
        return address() + "(" + address() + "):" + std::to_string(channel());
    }

    const struct sockaddr_rc& RfCommAddress::getSockAddrRc() const {
        return sockAddr;
    }

} // namespace net::socket::bluetooth::address
