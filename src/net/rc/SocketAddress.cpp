/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

    void SocketAddress::init() {
        sockAddr.rc_channel = channel;
        str2ba(btAddress.c_str(), &sockAddr.rc_bdaddr);
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

    std::string SocketAddress::toString(bool expanded) const {
        return std::string(btAddress).append(expanded ? std::string(":").append(std::to_string(channel)) : "");
    }

} // namespace net::rc

template class net::SocketAddress<sockaddr_rc>;
