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

#include "net/un/PeerCredentials.h"
#include "support/TestResult.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <sys/socket.h>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main() {
    tests::support::TestResult result;

    int sockets[2] = {-1, -1};
    const bool socketPairCreated = ::socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0;
    result.expectTrue(socketPairCreated, "Unix socket pair is created");

    if (socketPairCreated) {
        const net::un::PeerCredentials credentials = net::un::peerCredentials(sockets[0]);

#if defined(__linux__)
        result.expectTrue(credentials.status == net::un::PeerCredentialsStatus::Success, "connected Unix socket reports peer credentials");
        result.expectTrue(credentials.uid == ::geteuid(), "reported peer uid matches effective uid");
        result.expectTrue(credentials.gid == ::getegid(), "reported peer gid matches effective gid");
        result.expectTrue(credentials.pid.has_value(), "Linux peer credentials include pid");
        if (credentials.pid.has_value()) {
            result.expectTrue(*credentials.pid == ::getpid(), "reported Linux peer pid matches process pid");
        }
#else
        if (credentials.status == net::un::PeerCredentialsStatus::Success) {
            result.expectTrue(credentials.uid == ::geteuid(), "getpeereid reports the effective peer uid");
            result.expectTrue(credentials.gid == ::getegid(), "getpeereid reports the effective peer gid");
            result.expectTrue(!credentials.pid.has_value(), "getpeereid platforms report no peer pid");
        } else {
            result.expectTrue(credentials.status == net::un::PeerCredentialsStatus::Unsupported,
                              "platform without a detected credential API reports unsupported");
            result.expectEqual(ENOTSUP, credentials.error, "unsupported platform reports ENOTSUP");
        }
#endif

        ::close(sockets[0]);
        ::close(sockets[1]);
    }

    const net::un::PeerCredentials invalid = net::un::peerCredentials(-1);
#if defined(__linux__)
    result.expectTrue(invalid.status == net::un::PeerCredentialsStatus::Error, "invalid descriptor reports credential error");
    result.expectEqual(EBADF, invalid.error, "invalid descriptor preserves system error");
#else
    result.expectTrue(invalid.status == net::un::PeerCredentialsStatus::Error ||
                          invalid.status == net::un::PeerCredentialsStatus::Unsupported,
                      "invalid descriptor reports either a detected-API error or unsupported status");
    result.expectEqual(invalid.status == net::un::PeerCredentialsStatus::Error ? EBADF : ENOTSUP,
                       invalid.error,
                       "invalid descriptor preserves the status-specific platform error");
#endif

    return result.processResult();
}
