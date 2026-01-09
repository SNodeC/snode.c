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

#include "net/un/SocketAddress.h"

#include "core/socket/State.h"
#include "net/SocketAddress.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <cstddef>
#include <cstring>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un {

    SocketAddress::SocketAddress()
        : Super(AF_UNIX, offsetof(sockaddr_un, sun_path)) {
    }

    SocketAddress::SocketAddress(const std::string& sunPath)
        : SocketAddress() {
        setSunPath(sunPath);
    }

    SocketAddress::SocketAddress(const SockAddr& sockAddr, SockLen sockAddrLen)
        : net::SocketAddress<SockAddr>(sockAddr, sockAddrLen)
        , sunPath(sockAddr.sun_path) {
    }

    void SocketAddress::init() {
        if (sunPath.length() < sizeof(sockAddr.sun_path)) {
            const std::size_t len = sunPath.length();
            std::memcpy(sockAddr.sun_path, sunPath.data(), len);
            sockAddr.sun_path[len] = 0;
            sockAddrLen = static_cast<SockLen>(offsetof(sockaddr_un, sun_path) + len + 1);
        } else {
            throw core::socket::SocketAddress::BadSocketAddress(
                core::socket::STATE_FATAL,
                "Unix-Domain error sun-path to long: Lenght is = " + std::to_string(sunPath.length()) +
                    ", should be: " + std::to_string(sizeof(sockAddr.sun_path) - 1),
                EINVAL);
        }
    }

    SocketAddress& SocketAddress::setSunPath(const std::string& sunPath) {
        this->sunPath = sunPath;

        return *this;
    }

    std::string SocketAddress::getSunPath() const {
        return sunPath;
    }

    std::string SocketAddress::toString([[maybe_unused]] bool expanded) const {
        return sockAddr.sun_path[0] != '\0' ? sockAddr.sun_path : std::string("@").append(sockAddr.sun_path + 1);
    }

} // namespace net::un

template class net::SocketAddress<sockaddr_un>;
