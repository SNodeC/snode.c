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

#include "net/un/SocketAddress.h"
#include "support/TestResult.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <exception>
#include <string>
#include <sys/un.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main() {
    tests::support::TestResult testResult;

    const std::string longestValidPath(sizeof(sockaddr_un::sun_path) - 1, 'x');
    net::un::SocketAddress longestValidSocketAddress(longestValidPath);
    longestValidSocketAddress.init();

    const auto longestValidSockAddrLen = static_cast<net::un::SocketAddress::SockLen>(
        offsetof(sockaddr_un, sun_path) + longestValidPath.length() + 1);
    testResult.expectTrue(longestValidSocketAddress.getSunPath() == longestValidPath, "longest valid Unix-domain path remains configured");
    testResult.expectTrue(longestValidSocketAddress.toString() == longestValidPath, "longest valid Unix-domain path formats exactly");
    testResult.expectTrue(longestValidSocketAddress.getSockAddrLen() == longestValidSockAddrLen,
                          "longest valid Unix-domain path length includes terminator");

    const std::string tooLongPath(sizeof(sockaddr_un::sun_path), 'x');
    net::un::SocketAddress tooLongSocketAddress(tooLongPath);
    bool expectedExceptionThrown = false;
    bool unexpectedExceptionThrown = false;

    try {
        tooLongSocketAddress.init();
    } catch (const core::socket::SocketAddress::BadSocketAddress&) {
        expectedExceptionThrown = true;
    } catch (const std::exception&) {
        unexpectedExceptionThrown = true;
    }

    testResult.expectTrue(expectedExceptionThrown, "too-long Unix-domain path throws BadSocketAddress");
    testResult.expectTrue(!unexpectedExceptionThrown, "too-long Unix-domain path does not throw an unexpected exception type");

    const int result = testResult.processResult();

    return result;
}
