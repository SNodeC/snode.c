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

#include "net/rf/SocketAddress.h"

#include "net/SocketAddress.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstring> // for memset
#include <exception>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::rf {

    class bad_bdaddress : public std::exception {
    public:
        explicit bad_bdaddress(const std::string& bdAddress) {
            message = "Bad bad bdaddress \"" + bdAddress + "\"";
        }

        const char* what() const noexcept override {
            return message.c_str();
        }

    protected:
        static std::string message;
    };

    std::string bad_bdaddress::message;

    SocketAddress::SocketAddress() {
        std::memset(&sockAddr, 0, sizeof(sockAddr));

        sockAddr.rc_family = AF_BLUETOOTH;
        sockAddr.rc_bdaddr = {{0, 0, 0, 0, 0, 0}};
        sockAddr.rc_channel = static_cast<unsigned char>(0);
    }

    SocketAddress::SocketAddress(const std::string& btAddress)
        : SocketAddress() {
        setAddress(btAddress);
    }

    SocketAddress::SocketAddress(uint8_t channel)
        : SocketAddress() {
        setChannel(channel);
    }

    SocketAddress::SocketAddress(const std::string& btAddress, uint8_t channel)
        : SocketAddress() {
        setAddress(btAddress);
        setChannel(channel);
    }

    void SocketAddress::setAddress(const std::string& btAddress) {
        str2ba(btAddress.c_str(), &sockAddr.rc_bdaddr);
    }

    void SocketAddress::setChannel(uint8_t channel) {
        sockAddr.rc_channel = channel;
    }

    uint8_t SocketAddress::channel() const {
        return sockAddr.rc_channel;
    }

    std::string SocketAddress::address() const {
        char address[256];
        ba2str(&sockAddr.rc_bdaddr, address);

        return address;
    }

    std::string SocketAddress::toString() const {
        return address() + ":" + std::to_string(channel());
    }

} // namespace net::rf

namespace net {
    template class SocketAddress<struct sockaddr_rc>;
}
