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

    const std::string path = "/tmp/snodec-init-test.sock";
    const std::string alternatePath = "/tmp/snodec-init-test-alt.sock";
    const net::un::SocketAddress::SockLen emptyPathSockAddrLen =
        static_cast<net::un::SocketAddress::SockLen>(offsetof(sockaddr_un, sun_path));
    const net::un::SocketAddress::SockLen pathSockAddrLen =
        static_cast<net::un::SocketAddress::SockLen>(offsetof(sockaddr_un, sun_path) + path.length() + 1);
    const net::un::SocketAddress::SockLen alternatePathSockAddrLen =
        static_cast<net::un::SocketAddress::SockLen>(offsetof(sockaddr_un, sun_path) + alternatePath.length() + 1);

    net::un::SocketAddress socketAddress(path);
    socketAddress.init();

    testResult.expectTrue(socketAddress.getSunPath() == path, "initialized Unix-domain path remains configured");
    testResult.expectTrue(socketAddress.toString() == path, "initialized Unix-domain string returns path");
    testResult.expectTrue(socketAddress.toString(false) == path, "compact initialized Unix-domain string returns path");
    testResult.expectTrue(socketAddress.getAddressFamily() == AF_UNIX, "initialized Unix-domain address family is AF_UNIX");
    testResult.expectTrue(socketAddress.getSockAddrLen() > emptyPathSockAddrLen,
                          "initialized Unix-domain socket length is greater than empty path base length");
    testResult.expectTrue(socketAddress.getSockAddr().sa_family == AF_UNIX, "initialized Unix-domain sockaddr is available");
    testResult.expectTrue(socketAddress.getSockAddrLen() == pathSockAddrLen,
                          "initialized Unix-domain socket length includes path terminator");

    socketAddress.setSunPath(alternatePath);
    socketAddress.init();

    testResult.expectTrue(socketAddress.getSunPath() == alternatePath, "reinitialized Unix-domain path is updated");
    testResult.expectTrue(socketAddress.toString() == alternatePath, "reinitialized Unix-domain string returns updated path");
    testResult.expectTrue(socketAddress.toString(false) == alternatePath, "compact reinitialized Unix-domain string returns updated path");
    testResult.expectTrue(socketAddress.getSockAddrLen() == alternatePathSockAddrLen,
                          "reinitialized Unix-domain socket length reflects updated path length");

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
