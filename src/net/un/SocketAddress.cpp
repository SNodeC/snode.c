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

#include <cstring>
#include <exception>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un {

    class bad_sunpath : public std::exception {
    public:
        explicit bad_sunpath(const std::string& sunPath) {
            message = "Bad sun-path \"" + sunPath + "\"";
        }

        const char* what() const noexcept override {
            return message.c_str();
        }

    protected:
        static std::string message;
    };

    std::string bad_sunpath::message;

    SocketAddress::SocketAddress() {
        std::memset(&sockAddr, 0, sizeof(sockAddr));

        sockAddr.sun_family = AF_UNIX;
    }

    SocketAddress::SocketAddress(const std::string& sunPath)
        : SocketAddress() {
        setSunPath(sunPath);
    }

    void SocketAddress::setSunPath(const std::string& sunPath) {
        if (sunPath.length() < sizeof(sockAddr.sun_path)) {
            std::size_t len = sizeof(sockAddr.sun_path) - 1 < sunPath.size() + 1 ? sizeof(sockAddr.sun_path) - 1 : sunPath.size() + 1;
            std::memcpy(sockAddr.sun_path, sunPath.data(), len);
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

} // namespace net::un

namespace net {
    template class SocketAddress<struct sockaddr_un>;
}
