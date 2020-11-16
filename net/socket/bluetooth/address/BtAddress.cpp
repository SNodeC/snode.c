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

#include <netdb.h>
#include <sys/socket.h>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#include "BtAddress.h"

namespace net::socket::bluetooth::address {

    BtAddress::BtAddress() {
        sockAddr.rc_family = AF_BLUETOOTH;
        sockAddr.rc_bdaddr = {{0, 0, 0, 0, 0, 0}};
        sockAddr.rc_channel = (uint8_t) 0;
    }

    BtAddress::BtAddress(const BtAddress& bta) {
        memcpy(&sockAddr, &bta.sockAddr, sizeof(struct sockaddr_rc));
    }

    BtAddress::BtAddress(const struct sockaddr_rc& addr) {
        memcpy(&sockAddr, &addr, sizeof(struct sockaddr_rc));
    }

    BtAddress::BtAddress(const std::string& btAddress) {
        sockAddr.rc_family = AF_BLUETOOTH;
        str2ba(btAddress.c_str(), &sockAddr.rc_bdaddr);
        sockAddr.rc_channel = (uint8_t) 0;
    }

    BtAddress::BtAddress(uint8_t channel) {
        sockAddr.rc_family = AF_BLUETOOTH;
        sockAddr.rc_bdaddr = {{0, 0, 0, 0, 0, 0}};
        sockAddr.rc_channel = channel;
    }

    BtAddress::BtAddress(const std::string& btAddress, uint8_t channel) {
        sockAddr.rc_family = AF_BLUETOOTH;
        str2ba(btAddress.c_str(), &sockAddr.rc_bdaddr);
        sockAddr.rc_channel = channel;
    }

    uint8_t BtAddress::channel() const {
        return sockAddr.rc_channel;
    }

    std::string BtAddress::address() const {
        char ip[256];
        ba2str(&sockAddr.rc_bdaddr, ip);

        return std::string(ip);
    }

    BtAddress& BtAddress::operator=(const BtAddress& bta) {
        if (this != &bta) {
            memcpy(&sockAddr, &bta.sockAddr, sizeof(struct sockaddr_rc));
        }

        return *this;
    }

    const struct sockaddr_rc& BtAddress::getSockAddrRc() const {
        return sockAddr;
    }

} // namespace net::socket::bluetooth::address
