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

#include "net/in6/SocketAddress.h"
#include "support/TestResult.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <netinet/in.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main() {
    tests::support::TestResult testResult;

    sockaddr_in6 loopbackSockAddr{};
    loopbackSockAddr.sin6_family = AF_INET6;
    loopbackSockAddr.sin6_port = htons(8080);
    loopbackSockAddr.sin6_addr = in6addr_loopback;

    const net::in6::SocketAddress loopbackSocketAddress(loopbackSockAddr, sizeof(loopbackSockAddr), true);
    testResult.expectTrue(loopbackSocketAddress.getHost() == "::1", "native IPv6 loopback host is ::1");
    testResult.expectEqual(8080, loopbackSocketAddress.getPort(), "native IPv6 loopback port is 8080");
    testResult.expectTrue(loopbackSocketAddress.toString() == "::1:8080", "native IPv6 loopback address string includes host and port");
    testResult.expectTrue(loopbackSocketAddress.toString(false) == "::1:8080",
                          "compact native IPv6 loopback address string includes host and port");
    testResult.expectEqual(AF_INET6, loopbackSocketAddress.getAddressFamily(), "native IPv6 loopback address family is AF_INET6");
    testResult.expectEqual(
        sizeof(sockaddr_in6), loopbackSocketAddress.getSockAddrLen(), "native IPv6 loopback sockaddr length is sockaddr_in6 size");

    sockaddr_in6 anySockAddr{};
    anySockAddr.sin6_family = AF_INET6;
    anySockAddr.sin6_port = htons(0);
    anySockAddr.sin6_addr = in6addr_any;

    const net::in6::SocketAddress anySocketAddress(anySockAddr, sizeof(anySockAddr), true);
    testResult.expectTrue(anySocketAddress.getHost() == "::", "native IPv6 any host is ::");
    testResult.expectEqual(0, anySocketAddress.getPort(), "native IPv6 any port is 0");
    testResult.expectTrue(anySocketAddress.toString() == ":::0", "native IPv6 any address string includes host and port");
    testResult.expectTrue(anySocketAddress.toString(false) == ":::0", "compact native IPv6 any address string includes host and port");
    testResult.expectEqual(AF_INET6, anySocketAddress.getAddressFamily(), "native IPv6 any address family is AF_INET6");
    testResult.expectEqual(sizeof(sockaddr_in6), anySocketAddress.getSockAddrLen(), "native IPv6 any sockaddr length is sockaddr_in6 size");

    const int result = testResult.processResult();

    return result;
}
