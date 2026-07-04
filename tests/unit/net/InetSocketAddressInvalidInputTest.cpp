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
#include "support/TestResult.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <exception>
#include <netdb.h>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {

    bool throwsBadSocketAddress(const std::string& host) {
        net::in::SocketAddress socketAddress(host, 8080);

        try {
            socketAddress.init({.aiFlags = AI_NUMERICHOST, .aiSockType = 0, .aiProtocol = 0});
        } catch (const core::socket::SocketAddress::BadSocketAddress&) {
            return true;
        } catch (const std::exception&) {
            return false;
        }

        return false;
    }

} // namespace

int main() {
    tests::support::TestResult testResult;

    testResult.expectTrue(throwsBadSocketAddress("not-an-ipv4-address"), "syntactically invalid IPv4 host is rejected by init");
    testResult.expectTrue(throwsBadSocketAddress("256.0.0.1"), "out-of-range IPv4 octet is rejected by init");
    testResult.expectTrue(throwsBadSocketAddress(""), "empty IPv4 host is rejected by numeric init");

    net::in::SocketAddress socketAddress("127.0.0.1", 8080);
    socketAddress.setHost("300.1.2.3");
    testResult.expectTrue(socketAddress.getHost() == "300.1.2.3", "IPv4 setHost stores invalid input until init validates it");
    testResult.expectTrue(throwsBadSocketAddress(socketAddress.getHost()), "IPv4 setHost invalid input is rejected by init");

    const int result = testResult.processResult();

    return result;
}
