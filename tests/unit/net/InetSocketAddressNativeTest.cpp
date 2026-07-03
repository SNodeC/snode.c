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

#include "net/in/SocketAddress.h"
#include "support/TestResult.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <arpa/inet.h>
#include <cstdint>
#include <netinet/in.h>
#include <sys/socket.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main() {
    tests::support::TestResult testResult;

    sockaddr_in loopbackSockAddr{};
    loopbackSockAddr.sin_family = AF_INET;
    loopbackSockAddr.sin_port = htons(8080);
    loopbackSockAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    const net::in::SocketAddress loopbackSocketAddress(loopbackSockAddr, sizeof(loopbackSockAddr), true);
    testResult.expectTrue(loopbackSocketAddress.getHost() == "127.0.0.1", "native IPv4 loopback host is 127.0.0.1");
    testResult.expectEqual(8080, loopbackSocketAddress.getPort(), "native IPv4 loopback port is 8080");
    testResult.expectTrue(loopbackSocketAddress.toString() == "127.0.0.1:8080",
                          "native IPv4 loopback address string includes host and port");
    testResult.expectTrue(loopbackSocketAddress.toString(false) == "127.0.0.1:8080",
                          "compact native IPv4 loopback address string includes host and port");
    testResult.expectEqual(AF_INET, loopbackSocketAddress.getAddressFamily(), "native IPv4 loopback address family is AF_INET");
    testResult.expectEqual(
        sizeof(sockaddr_in), loopbackSocketAddress.getSockAddrLen(), "native IPv4 loopback sockaddr length is sockaddr_in size");

    sockaddr_in anySockAddr{};
    anySockAddr.sin_family = AF_INET;
    anySockAddr.sin_port = htons(0);
    anySockAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    const net::in::SocketAddress anySocketAddress(anySockAddr, sizeof(anySockAddr), true);
    testResult.expectTrue(anySocketAddress.getHost() == "0.0.0.0", "native IPv4 any host is 0.0.0.0");
    testResult.expectEqual(0, anySocketAddress.getPort(), "native IPv4 any port is 0");
    testResult.expectTrue(anySocketAddress.toString() == "0.0.0.0:0", "native IPv4 any address string includes host and port");
    testResult.expectTrue(anySocketAddress.toString(false) == "0.0.0.0:0", "compact native IPv4 any address string includes host and port");
    testResult.expectEqual(AF_INET, anySocketAddress.getAddressFamily(), "native IPv4 any address family is AF_INET");
    testResult.expectEqual(sizeof(sockaddr_in), anySocketAddress.getSockAddrLen(), "native IPv4 any sockaddr length is sockaddr_in size");

    const int result = testResult.processResult();

    return result;
}
