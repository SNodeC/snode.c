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

#include "net/un/SocketAddress.h"
#include "net/un/phy/stream/PhysicalSocketServer.h"
#include "support/TestResult.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {

    constexpr const char* FILESYSTEM_LOCK_MARKER = "snodec.unix-socket-lock:v1\n";

    bool createRegularFile(const std::string& path, const std::string& contents) {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << contents;

        return output.good();
    }

    std::string readFile(const std::string& path) {
        std::ifstream input(path, std::ios::binary);

        return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    }

    bool inspectPath(const std::string& path, struct stat& status) {
        status = {};

        return ::lstat(path.c_str(), &status) == 0;
    }

    bool pathAbsent(const std::string& path) {
        struct stat status = {};

        return ::lstat(path.c_str(), &status) != 0 && errno == ENOENT;
    }

    bool createStaleSocket(const std::string& path) {
        const int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (fd < 0) {
            return false;
        }

        net::un::SocketAddress address(path);
        address.init();
        const bool bound = ::bind(fd, &address.getSockAddr(), address.getSockAddrLen()) == 0;
        ::close(fd);

        return bound;
    }

    int createBoundSocket(const std::string& path, bool listen) {
        const int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (fd < 0) {
            return -1;
        }

        net::un::SocketAddress address(path);
        address.init();
        if (::bind(fd, &address.getSockAddr(), address.getSockAddrLen()) != 0 || (listen && ::listen(fd, 4) != 0)) {
            ::close(fd);
            return -1;
        }
        return fd;
    }

    int createActiveSocket(const std::string& path) {
        return createBoundSocket(path, true);
    }

    bool openPhysicalSocket(net::un::phy::stream::PhysicalSocketServer& socket) {
        return socket.open({}, net::un::phy::stream::PhysicalSocketServer::Flags::NONBLOCK) >= 0;
    }

    int bindPhysicalSocket(net::un::phy::stream::PhysicalSocketServer& socket, const std::string& path) {
        net::un::SocketAddress address(path);
        address.init();

        return socket.bind(address);
    }

} // namespace

int main() {
    tests::support::TestResult testResult;
    std::array<char, 48> directoryTemplate = {};
    const std::string templateValue = "/tmp/snodec-unix-path-safety-XXXXXX";
    std::copy(templateValue.begin(), templateValue.end(), directoryTemplate.begin());
    char* const directoryResult = ::mkdtemp(directoryTemplate.data());

    testResult.expectTrue(directoryResult != nullptr, "temporary Unix socket test directory is created");
    if (directoryResult == nullptr) {
        return testResult.processResult();
    }

    const std::string directory(directoryResult);
    const std::string regularPath = directory + "/regular.sock";
    const std::string activePath = directory + "/active.sock";
    const std::string unmarkedStalePath = directory + "/unmarked-stale.sock";
    const std::string ownedStalePath = directory + "/owned-stale.sock";
    const std::string boundBeforeListenPath = directory + "/bound-before-listen.sock";
    const std::string unrecognizedLockPath = directory + "/unrecognized.sock";
    const std::string replacementPath = directory + "/replacement.sock";

    testResult.expectTrue(createRegularFile(regularPath, "do-not-remove"), "regular-file collision fixture is created");
    struct stat regularBefore = {};
    testResult.expectTrue(inspectPath(regularPath, regularBefore) && S_ISREG(regularBefore.st_mode),
                          "regular-file collision fixture is a regular file");

    int regularBindResult = 0;
    int regularBindError = 0;
    {
        net::un::phy::stream::PhysicalSocketServer socket;
        testResult.expectTrue(openPhysicalSocket(socket), "physical socket opens for regular-file collision test");
        errno = 0;
        regularBindResult = bindPhysicalSocket(socket, regularPath);
        regularBindError = errno;
    }

    struct stat regularAfter = {};
    testResult.expectTrue(regularBindResult < 0 && regularBindError == EADDRINUSE, "bind rejects a regular-file collision with EADDRINUSE");
    testResult.expectTrue(inspectPath(regularPath, regularAfter) && S_ISREG(regularAfter.st_mode),
                          "regular-file collision remains after the socket is destroyed");
    testResult.expectTrue(regularBefore.st_dev == regularAfter.st_dev && regularBefore.st_ino == regularAfter.st_ino,
                          "regular-file collision retains its filesystem identity");
    testResult.expectTrue(readFile(regularPath) == "do-not-remove", "regular-file collision retains its contents");
    testResult.expectTrue(pathAbsent(regularPath + ".lock"), "failed bind cleans up its owned sidecar lock");

    const int activeFd = createActiveSocket(activePath);
    struct stat activeBefore = {};
    testResult.expectTrue(activeFd >= 0, "unrelated active Unix socket fixture is created");
    testResult.expectTrue(inspectPath(activePath, activeBefore) && S_ISSOCK(activeBefore.st_mode),
                          "unrelated active fixture is a socket node");
    int activeBindResult = 0;
    int activeBindError = 0;
    {
        net::un::phy::stream::PhysicalSocketServer socket;
        testResult.expectTrue(openPhysicalSocket(socket), "physical socket opens for active-socket collision test");
        errno = 0;
        activeBindResult = bindPhysicalSocket(socket, activePath);
        activeBindError = errno;
    }
    struct stat activeAfter = {};
    testResult.expectTrue(activeBindResult < 0 && activeBindError == EADDRINUSE, "bind rejects an active unrelated socket with EADDRINUSE");
    testResult.expectTrue(inspectPath(activePath, activeAfter) && S_ISSOCK(activeAfter.st_mode) &&
                              activeBefore.st_dev == activeAfter.st_dev && activeBefore.st_ino == activeAfter.st_ino,
                          "active unrelated socket retains its filesystem identity");
    testResult.expectTrue(pathAbsent(activePath + ".lock"), "active-socket collision cleans up its owned sidecar lock");
    if (activeFd >= 0) {
        ::close(activeFd);
    }
    ::unlink(activePath.c_str());

    testResult.expectTrue(createStaleSocket(unmarkedStalePath), "unmarked stale Unix socket fixture is created");
    struct stat unmarkedStaleBefore = {};
    testResult.expectTrue(inspectPath(unmarkedStalePath, unmarkedStaleBefore) && S_ISSOCK(unmarkedStaleBefore.st_mode),
                          "unmarked stale Unix socket fixture is a socket node");

    int unmarkedStaleBindResult = 0;
    int unmarkedStaleBindError = 0;
    {
        net::un::phy::stream::PhysicalSocketServer socket;
        testResult.expectTrue(openPhysicalSocket(socket), "physical socket opens for unmarked stale-socket test");
        errno = 0;
        unmarkedStaleBindResult = bindPhysicalSocket(socket, unmarkedStalePath);
        unmarkedStaleBindError = errno;
    }
    struct stat unmarkedStaleAfter = {};
    testResult.expectTrue(unmarkedStaleBindResult < 0 && unmarkedStaleBindError == EADDRINUSE,
                          "bind refuses to establish ownership retroactively for an unmarked stale socket");
    testResult.expectTrue(inspectPath(unmarkedStalePath, unmarkedStaleAfter) && S_ISSOCK(unmarkedStaleAfter.st_mode) &&
                              unmarkedStaleBefore.st_dev == unmarkedStaleAfter.st_dev &&
                              unmarkedStaleBefore.st_ino == unmarkedStaleAfter.st_ino,
                          "unmarked stale socket retains its filesystem identity");
    testResult.expectTrue(pathAbsent(unmarkedStalePath + ".lock"),
                          "refusing an unmarked stale socket removes only the newly created marker");

    testResult.expectTrue(createStaleSocket(ownedStalePath), "owned stale Unix socket fixture is created");
    testResult.expectTrue(createRegularFile(ownedStalePath + ".lock", FILESYSTEM_LOCK_MARKER),
                          "recognized ownership marker is created beside the stale socket");
    struct stat ownedStaleBefore = {};
    testResult.expectTrue(inspectPath(ownedStalePath, ownedStaleBefore) && S_ISSOCK(ownedStaleBefore.st_mode),
                          "owned stale Unix socket fixture is a socket node");

    int ownedStaleBindResult = -1;
    struct stat recoveredStatus = {};
    {
        net::un::phy::stream::PhysicalSocketServer socket;
        testResult.expectTrue(openPhysicalSocket(socket), "physical socket opens for recognized stale-socket test");
        ownedStaleBindResult = bindPhysicalSocket(socket, ownedStalePath);
        testResult.expectTrue(ownedStaleBindResult == 0 && inspectPath(ownedStalePath, recoveredStatus) &&
                                  S_ISSOCK(recoveredStatus.st_mode) &&
                                  (ownedStaleBefore.st_dev != recoveredStatus.st_dev || ownedStaleBefore.st_ino != recoveredStatus.st_ino),
                              "bind replaces a stale socket only when a recognized ownership marker exists");
    }

    testResult.expectTrue(ownedStaleBindResult == 0, "bind succeeds after recognized stale socket recovery");
    testResult.expectTrue(pathAbsent(ownedStalePath), "recovered owned socket node is removed during normal cleanup");
    testResult.expectTrue(pathAbsent(ownedStalePath + ".lock"), "normal cleanup removes the recognized ownership marker");

    const int boundBeforeListenFd = createBoundSocket(boundBeforeListenPath, false);
    struct stat boundBeforeListenBefore = {};
    testResult.expectTrue(boundBeforeListenFd >= 0, "bound-before-listen Unix socket fixture is created");
    testResult.expectTrue(inspectPath(boundBeforeListenPath, boundBeforeListenBefore) && S_ISSOCK(boundBeforeListenBefore.st_mode),
                          "bound-before-listen fixture is a socket node");
    int boundBeforeListenResult = 0;
    int boundBeforeListenError = 0;
    {
        net::un::phy::stream::PhysicalSocketServer socket;
        testResult.expectTrue(openPhysicalSocket(socket), "physical socket opens for bound-before-listen collision test");
        errno = 0;
        boundBeforeListenResult = bindPhysicalSocket(socket, boundBeforeListenPath);
        boundBeforeListenError = errno;
    }
    struct stat boundBeforeListenAfter = {};
    testResult.expectTrue(boundBeforeListenResult < 0 && boundBeforeListenError == EADDRINUSE,
                          "bind conservatively rejects an unmarked bound-before-listen socket");
    testResult.expectTrue(inspectPath(boundBeforeListenPath, boundBeforeListenAfter) && S_ISSOCK(boundBeforeListenAfter.st_mode) &&
                              boundBeforeListenBefore.st_dev == boundBeforeListenAfter.st_dev &&
                              boundBeforeListenBefore.st_ino == boundBeforeListenAfter.st_ino,
                          "bound-before-listen socket retains its filesystem identity");
    testResult.expectTrue(pathAbsent(boundBeforeListenPath + ".lock"),
                          "bound-before-listen rejection removes only the newly created marker");
    if (boundBeforeListenFd >= 0) {
        ::close(boundBeforeListenFd);
    }
    ::unlink(boundBeforeListenPath.c_str());

    testResult.expectTrue(createRegularFile(unrecognizedLockPath + ".lock", "unrelated-marker"), "unrecognized sidecar fixture is created");
    struct stat unrecognizedLockBefore = {};
    testResult.expectTrue(inspectPath(unrecognizedLockPath + ".lock", unrecognizedLockBefore) && S_ISREG(unrecognizedLockBefore.st_mode),
                          "unrecognized sidecar fixture is a regular file");
    int unrecognizedLockBindResult = 0;
    int unrecognizedLockBindError = 0;
    {
        net::un::phy::stream::PhysicalSocketServer socket;
        testResult.expectTrue(openPhysicalSocket(socket), "physical socket opens for unrecognized-sidecar test");
        errno = 0;
        unrecognizedLockBindResult = bindPhysicalSocket(socket, unrecognizedLockPath);
        unrecognizedLockBindError = errno;
    }
    struct stat unrecognizedLockAfter = {};
    testResult.expectTrue(unrecognizedLockBindResult < 0 && unrecognizedLockBindError == EADDRINUSE,
                          "bind rejects an unrecognized sidecar with EADDRINUSE");
    testResult.expectTrue(inspectPath(unrecognizedLockPath + ".lock", unrecognizedLockAfter) && S_ISREG(unrecognizedLockAfter.st_mode) &&
                              unrecognizedLockBefore.st_dev == unrecognizedLockAfter.st_dev &&
                              unrecognizedLockBefore.st_ino == unrecognizedLockAfter.st_ino,
                          "unrecognized sidecar retains its filesystem identity");
    testResult.expectTrue(readFile(unrecognizedLockPath + ".lock") == "unrelated-marker", "unrecognized sidecar retains its contents");
    testResult.expectTrue(pathAbsent(unrecognizedLockPath), "an unrecognized sidecar never causes a socket path to be created");

    struct stat ownedSocketStatus = {};
    struct stat replacementStatus = {};
    struct stat replacementLockStatus = {};
    bool replacementCreated = false;
    bool replacementLockCreated = false;
    {
        net::un::phy::stream::PhysicalSocketServer socket;
        testResult.expectTrue(openPhysicalSocket(socket), "physical socket opens for replacement-identity test");
        testResult.expectEqual(0, bindPhysicalSocket(socket, replacementPath), "physical socket binds for replacement-identity test");
        testResult.expectTrue(inspectPath(replacementPath, ownedSocketStatus) && S_ISSOCK(ownedSocketStatus.st_mode),
                              "bound path starts as the owned socket node");
        testResult.expectTrue(inspectPath(replacementPath + ".lock", replacementLockStatus) && S_ISREG(replacementLockStatus.st_mode),
                              "bound path starts with its owned marker");
        testResult.expectEqual(0, ::unlink(replacementPath.c_str()), "test unlinks the owned socket pathname");
        replacementCreated = createRegularFile(replacementPath, "replacement");
        testResult.expectTrue(replacementCreated && inspectPath(replacementPath, replacementStatus) && S_ISREG(replacementStatus.st_mode),
                              "test installs a regular-file replacement at the bound pathname");
        testResult.expectEqual(0, ::unlink((replacementPath + ".lock").c_str()), "test unlinks the owned marker pathname");
        replacementLockCreated = createRegularFile(replacementPath + ".lock", "replacement-lock");
        testResult.expectTrue(replacementLockCreated && inspectPath(replacementPath + ".lock", replacementLockStatus) &&
                                  S_ISREG(replacementLockStatus.st_mode),
                              "test installs an unrecognized replacement at the marker pathname");
    }

    struct stat replacementAfter = {};
    testResult.expectTrue(replacementCreated && inspectPath(replacementPath, replacementAfter) && S_ISREG(replacementAfter.st_mode),
                          "socket cleanup preserves a replacement filesystem object");
    testResult.expectTrue(replacementStatus.st_dev == replacementAfter.st_dev && replacementStatus.st_ino == replacementAfter.st_ino,
                          "socket cleanup preserves the replacement identity");
    testResult.expectTrue(readFile(replacementPath) == "replacement", "socket cleanup preserves replacement contents");
    struct stat replacementLockAfter = {};
    testResult.expectTrue(replacementLockCreated && inspectPath(replacementPath + ".lock", replacementLockAfter) &&
                              S_ISREG(replacementLockAfter.st_mode),
                          "marker cleanup preserves a replacement filesystem object");
    testResult.expectTrue(replacementLockStatus.st_dev == replacementLockAfter.st_dev &&
                              replacementLockStatus.st_ino == replacementLockAfter.st_ino,
                          "marker cleanup preserves the replacement identity");
    testResult.expectTrue(readFile(replacementPath + ".lock") == "replacement-lock", "marker cleanup preserves replacement contents");

    ::unlink(regularPath.c_str());
    ::unlink(activePath.c_str());
    ::unlink(unmarkedStalePath.c_str());
    ::unlink(ownedStalePath.c_str());
    ::unlink(boundBeforeListenPath.c_str());
    ::unlink(unrecognizedLockPath.c_str());
    ::unlink((unrecognizedLockPath + ".lock").c_str());
    ::unlink(replacementPath.c_str());
    ::unlink((replacementPath + ".lock").c_str());
    testResult.expectEqual(0, ::rmdir(directory.c_str()), "temporary Unix socket test directory is removed");

    return testResult.processResult();
}
