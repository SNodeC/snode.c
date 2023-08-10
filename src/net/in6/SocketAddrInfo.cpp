/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "net/in6/SocketAddrInfo.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in6 {

    SocketAddrInfo::SocketAddrInfo() {
        VLOG(0) << "SocketAddrInfo::SocketAddrInfo()";
    }

    SocketAddrInfo::~SocketAddrInfo() {
        if (res != nullptr) {
            freeaddrinfo(res);
        }
    }

    int SocketAddrInfo::init(const std::string& node, const std::string& service, const addrinfo& hints) {
        int err = 0;

        VLOG(0) << "SocketAddrInfo::init(" << node << ", " << service << ")";

        if (!node.empty() && !service.empty() && (this->node != node || this->service != service)) {
            this->node = node;
            this->service = service;

            if (res != nullptr) {
                freeaddrinfo(res);
                res = nullptr;
            }

            err = core::system::getaddrinfo(node.c_str(), service.c_str(), &hints, &res);

            currentAddrInfo = res;
        }

        return err;
    }

    bool SocketAddrInfo::hasNext() {
        if (currentAddrInfo->ai_next != nullptr) {
            currentAddrInfo = currentAddrInfo->ai_next;
        } else {
            currentAddrInfo = nullptr;
        }

        return currentAddrInfo != nullptr;
    }

    addrinfo* SocketAddrInfo::getAddrInfo() {
        VLOG(0) << "GetAddrInfo: " << currentAddrInfo;
        return currentAddrInfo;
    }

} // namespace net::in6
