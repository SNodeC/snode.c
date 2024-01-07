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

#include "net/un/SocketAddress.h"

#include "core/socket/State.h"
#include "net/SocketAddress.hpp"

// IWYU pragma: no_include "core/socket/SocketAddress.h"

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
        : net::SocketAddress<SockAddr>(sockAddr, sockAddrLen) {
        sunPath = sockAddr.sun_path;
    }

    SocketAddress& SocketAddress::init() {
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

        return *this;
    }

    SocketAddress& SocketAddress::setSunPath(const std::string& sunPath) {
        this->sunPath = sunPath;

        return *this;
    }

    std::string SocketAddress::getSunPath() const {
        return sunPath;
    }

#if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wrestrict"
// Workaround https://gcc.gnu.org/bugzilla/show_bug.cgi?id=105329
#endif
    std::string SocketAddress::toString([[maybe_unused]] bool expanded) const {
        return sockAddr.sun_path[0] != '\0' ? std::string(sockAddr.sun_path) : std::string("@").append(std::string(sockAddr.sun_path + 1));
    }
#if defined(__GNUC__) && !defined(__llvm__) && !defined(__INTEL_COMPILER)
#pragma GCC diagnostic pop
#endif

} // namespace net::un

template class net::SocketAddress<sockaddr_un>;
