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

#include "net/in6/SocketAddress.h"
#include "support/TestResult.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <exception>
#include <netdb.h>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {

    bool throwsBadSocketAddress(const std::string& host) {
        net::in6::SocketAddress socketAddress(host, 8080);

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

    testResult.expectTrue(throwsBadSocketAddress("not-an-ipv6-address"), "syntactically invalid IPv6 host is rejected by init");
    testResult.expectTrue(throwsBadSocketAddress("2001:db8:::1"), "malformed compressed IPv6 host is rejected by init");
    testResult.expectTrue(throwsBadSocketAddress("2001:db8::g"), "IPv6 host with invalid characters is rejected by init");
    testResult.expectTrue(throwsBadSocketAddress(""), "empty IPv6 host is rejected by numeric init");

    net::in6::SocketAddress socketAddress("::1", 8080);
    socketAddress.setHost("2001:db8:::2");
    testResult.expectTrue(socketAddress.getHost() == "2001:db8:::2", "IPv6 setHost stores invalid input until init validates it");
    testResult.expectTrue(throwsBadSocketAddress(socketAddress.getHost()), "IPv6 setHost invalid input is rejected by init");

    const int result = testResult.processResult();

    return result;
}
