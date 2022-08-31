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

#include "net/un/SocketAddress.h"

#include "net/SocketAddress.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/socket.h"

#include <cstring>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un {

    SocketAddress::SocketAddress()
        : Super(offsetof(sockaddr_un, sun_path)) {
        sockAddr.sun_family = AF_UNIX;
        *sockAddr.sun_path = 0;
    }

    SocketAddress::SocketAddress(const std::string& sunPath)
        : SocketAddress() {
        setSunPath(sunPath);
        addrLen = static_cast<socklen_t>(offsetof(sockaddr_un, sun_path) + sunPath.length() + 1);
    }

    void SocketAddress::setSunPath(const std::string& sunPath) {
        if (sunPath.length() < sizeof(sockAddr.sun_path)) {
            std::size_t len = sizeof(sockAddr.sun_path) - 1 < sunPath.size() + 1 ? sizeof(sockAddr.sun_path) - 1 : sunPath.size() + 1;
            std::memcpy(sockAddr.sun_path, sunPath.data(), len);
            addrLen = static_cast<socklen_t>(offsetof(sockaddr_un, sun_path) + sunPath.length() + 1);
        } else {
            throw bad_sunpath(sunPath);
        }
    }

    std::string SocketAddress::address() const {
        return std::string(sockAddr.sun_path);
    }

    std::string SocketAddress::toString() const {
        return address();
    }

    bad_sunpath::bad_sunpath(const std::string& sunPath) {
        message = "Bad sun-path \"" + sunPath + "\"";
    }

    const char* bad_sunpath::what() const noexcept {
        return message.c_str();
    }

} // namespace net::un

template class net::SocketAddress<sockaddr_un>;
