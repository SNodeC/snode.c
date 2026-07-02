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

#include <cstdint>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main() {
    tests::support::TestResult testResult;

    const std::string defaultHost = "::";
    const std::string host = "::1";
    const std::string alternateHost = "::";
    const uint16_t port = 8080;
    const uint16_t alternatePort = 12345;

    const net::in6::SocketAddress defaultSocketAddress;
    testResult.expectTrue(defaultSocketAddress.getHost() == defaultHost, "default IPv6 host is ::");
    testResult.expectEqual(0, defaultSocketAddress.getPort(), "default IPv6 port is 0");
    testResult.expectTrue(defaultSocketAddress.toString() == ":::0", "default IPv6 address string includes host and port");
    testResult.expectTrue(defaultSocketAddress.toString(false) == ":::0", "compact default IPv6 address string includes host and port");

    const net::in6::SocketAddress hostSocketAddress(host);
    testResult.expectTrue(hostSocketAddress.getHost() == host, "IPv6 host constructor stores host");
    testResult.expectEqual(0, hostSocketAddress.getPort(), "IPv6 host constructor leaves port at 0");
    testResult.expectTrue(hostSocketAddress.toString() == "::1:0", "IPv6 host constructor string includes host and port");

    const net::in6::SocketAddress portSocketAddress(port);
    testResult.expectTrue(portSocketAddress.getHost() == defaultHost, "IPv6 port constructor leaves default host");
    testResult.expectEqual(port, portSocketAddress.getPort(), "IPv6 port constructor stores port");
    testResult.expectTrue(portSocketAddress.toString() == ":::8080", "IPv6 port constructor string includes host and port");

    net::in6::SocketAddress hostPortSocketAddress(host, port);
    testResult.expectTrue(hostPortSocketAddress.getHost() == host, "IPv6 host and port constructor stores host");
    testResult.expectEqual(port, hostPortSocketAddress.getPort(), "IPv6 host and port constructor stores port");
    testResult.expectTrue(hostPortSocketAddress.toString() == "::1:8080", "IPv6 host and port constructor string includes host and port");

    hostPortSocketAddress.setHost(alternateHost);
    testResult.expectTrue(hostPortSocketAddress.getHost() == alternateHost, "IPv6 setHost updates host");
    testResult.expectTrue(hostPortSocketAddress.toString() == ":::8080", "IPv6 string reflects updated host");

    hostPortSocketAddress.setPort(alternatePort);
    testResult.expectEqual(alternatePort, hostPortSocketAddress.getPort(), "IPv6 setPort updates port");
    testResult.expectTrue(hostPortSocketAddress.toString() == ":::12345", "IPv6 string reflects updated port");

    const int result = testResult.processResult();

    return result;
}
