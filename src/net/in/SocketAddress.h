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

#ifndef NET_IN_SOCKETADDRESS_H
#define NET_IN_SOCKETADDRESS_H

#include "net/SocketAddress.h" // IWYU pragma: export

namespace net::in {
    class SocketAddrInfo;
} // namespace net::in

// IWYU pragma: no_include "net/SocketAddress.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <memory>
#include <netinet/in.h>
#include <string>
#include <string_view>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::in {

    class SocketAddress final : public net::SocketAddress<sockaddr_in> {
    private:
        using Super = net::SocketAddress<sockaddr_in>;

    public:
        struct Hints {
            int aiFlags = 0;
            int aiSockType = 0;
            int aiProtocol = 0;
        };

        SocketAddress();
        explicit SocketAddress(const std::string& ipOrHostname);
        explicit SocketAddress(uint16_t port);
        SocketAddress(const std::string& ipOrHostname, uint16_t port);
        SocketAddress(const SockAddr& sockAddr, SockLen sockAddrLen, bool numeric = true);

        void init(const Hints& hints = {.aiFlags = 0, .aiSockType = 0, .aiProtocol = 0});

        bool useNext() override;

        SocketAddress& setHost(const std::string& ipOrHostname);
        std::string getHost() const;

        SocketAddress& setPort(uint16_t port);
        uint16_t getPort() const;

        std::string getCanonName() const;

        std::string toString(bool expanded = true) const override;
        std::string getEndpoint(const std::string_view& format = {}) const override;

    private:
        std::string host = "0.0.0.0";
        uint16_t port = 0;

        std::string canonName;

        std::shared_ptr<SocketAddrInfo> socketAddrInfo;
    };

} // namespace net::in

extern template class net::SocketAddress<sockaddr_in>;

#endif // NET_IN_SOCKETADDRESS_H
