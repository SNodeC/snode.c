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

#include <cstdint>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main() {
    tests::support::TestResult testResult;

    const std::string defaultHost = "0.0.0.0";
    const std::string host = "127.0.0.1";
    const std::string alternateHost = "0.0.0.0";
    const uint16_t port = 8080;
    const uint16_t alternatePort = 12345;

    const net::in::SocketAddress defaultSocketAddress;
    testResult.expectTrue(defaultSocketAddress.getHost() == defaultHost, "default IPv4 host is 0.0.0.0");
    testResult.expectEqual(0, defaultSocketAddress.getPort(), "default IPv4 port is 0");
    testResult.expectTrue(defaultSocketAddress.toString() == "0.0.0.0:0", "default IPv4 address string includes host and port");
    testResult.expectTrue(defaultSocketAddress.toString(false) == "0.0.0.0:0",
                          "compact default IPv4 address string includes host and port");

    const net::in::SocketAddress hostSocketAddress(host);
    testResult.expectTrue(hostSocketAddress.getHost() == host, "IPv4 host constructor stores host");
    testResult.expectEqual(0, hostSocketAddress.getPort(), "IPv4 host constructor leaves port at 0");
    testResult.expectTrue(hostSocketAddress.toString() == "127.0.0.1:0", "IPv4 host constructor string includes host and port");

    const net::in::SocketAddress portSocketAddress(port);
    testResult.expectTrue(portSocketAddress.getHost() == defaultHost, "IPv4 port constructor leaves default host");
    testResult.expectEqual(port, portSocketAddress.getPort(), "IPv4 port constructor stores port");
    testResult.expectTrue(portSocketAddress.toString() == "0.0.0.0:8080", "IPv4 port constructor string includes host and port");

    net::in::SocketAddress hostPortSocketAddress(host, port);
    testResult.expectTrue(hostPortSocketAddress.getHost() == host, "IPv4 host and port constructor stores host");
    testResult.expectEqual(port, hostPortSocketAddress.getPort(), "IPv4 host and port constructor stores port");
    testResult.expectTrue(hostPortSocketAddress.toString() == "127.0.0.1:8080",
                          "IPv4 host and port constructor string includes host and port");

    hostPortSocketAddress.setHost(alternateHost);
    testResult.expectTrue(hostPortSocketAddress.getHost() == alternateHost, "IPv4 setHost updates host");
    testResult.expectTrue(hostPortSocketAddress.toString() == "0.0.0.0:8080", "IPv4 string reflects updated host");

    hostPortSocketAddress.setPort(alternatePort);
    testResult.expectEqual(alternatePort, hostPortSocketAddress.getPort(), "IPv4 setPort updates port");
    testResult.expectTrue(hostPortSocketAddress.toString() == "0.0.0.0:12345", "IPv4 string reflects updated port");

    const int result = testResult.processResult();

    return result;
}
