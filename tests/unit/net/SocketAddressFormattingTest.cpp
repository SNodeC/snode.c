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
 */

#include "net/in/SocketAddress.h"
#include "net/in6/SocketAddress.h"
#include "net/un/SocketAddress.h"
#include "support/TestResult.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main() {
    tests::support::TestResult testResult;

    const net::in::SocketAddress ipv4SocketAddress("127.0.0.1", 8080);
    testResult.expectTrue(ipv4SocketAddress.toString() == "127.0.0.1:8080", "IPv4 address formats as host:port");
    testResult.expectTrue(ipv4SocketAddress.toString(false) == "127.0.0.1:8080", "compact IPv4 address formats as host:port");

    const net::in6::SocketAddress ipv6SocketAddress("::1", 8080);
    testResult.expectTrue(ipv6SocketAddress.toString() == "::1:8080", "IPv6 address formats as host:port");
    testResult.expectTrue(ipv6SocketAddress.toString(false) == "::1:8080", "compact IPv6 address formats as host:port");

    net::un::SocketAddress unixSocketAddress("/tmp/snodec-format-test.sock");
    unixSocketAddress.init();
    testResult.expectTrue(unixSocketAddress.toString() == "/tmp/snodec-format-test.sock", "Unix-domain address formats as path");
    testResult.expectTrue(unixSocketAddress.toString(false) == "/tmp/snodec-format-test.sock", "compact Unix-domain address formats as path");

    const int result = testResult.processResult();

    return result;
}
