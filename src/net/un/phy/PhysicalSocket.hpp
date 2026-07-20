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

#include "net/un/phy/PhysicalSocket.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/socket.h"
#include "core/system/unistd.h"
#include "log/Logger.h"

#include <array>
#include <cerrno>
#include <cstring>
#include <string>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un::phy {

    namespace detail {

        inline constexpr char FILESYSTEM_LOCK_MARKER[] = "snodec.unix-socket-lock:v1\n";
        inline constexpr std::size_t FILESYSTEM_LOCK_MARKER_SIZE = sizeof(FILESYSTEM_LOCK_MARKER) - 1;

        enum class ExistingSocketState { Stale, ActiveOrUnknown };

        inline bool writeFilesystemLockMarker(int fd) noexcept {
            std::size_t written = 0;

            while (written < FILESYSTEM_LOCK_MARKER_SIZE) {
                const ssize_t writeResult =
                    ::pwrite(fd, FILESYSTEM_LOCK_MARKER + written, FILESYSTEM_LOCK_MARKER_SIZE - written, static_cast<off_t>(written));
                if (writeResult > 0) {
                    written += static_cast<std::size_t>(writeResult);
                } else if (writeResult < 0 && errno == EINTR) {
                    continue;
                } else {
                    if (writeResult == 0) {
                        errno = EIO;
                    }
                    return false;
                }
            }

            return ::ftruncate(fd, static_cast<off_t>(FILESYSTEM_LOCK_MARKER_SIZE)) == 0;
        }

        inline bool validateFilesystemLockMarker(int fd, const struct stat& lockStatus) noexcept {
            if (lockStatus.st_size != static_cast<off_t>(FILESYSTEM_LOCK_MARKER_SIZE)) {
                return false;
            }

            std::array<char, FILESYSTEM_LOCK_MARKER_SIZE> marker = {};
            std::size_t read = 0;

            while (read < marker.size()) {
                const ssize_t readResult = ::pread(fd, marker.data() + read, marker.size() - read, static_cast<off_t>(read));
                if (readResult > 0) {
                    read += static_cast<std::size_t>(readResult);
                } else if (readResult < 0 && errno == EINTR) {
                    continue;
                } else {
                    return false;
                }
            }

            return std::memcmp(marker.data(), FILESYSTEM_LOCK_MARKER, marker.size()) == 0;
        }

        inline ExistingSocketState probeExistingSocket(net::un::SocketAddress& address, int type, int protocol) noexcept {
            const int probeFd = core::system::socket(PF_UNIX, type | SOCK_NONBLOCK | SOCK_CLOEXEC, protocol);
            if (probeFd < 0) {
                return ExistingSocketState::ActiveOrUnknown;
            }

            const int connectResult = core::system::connect(probeFd, &address.getSockAddr(), address.getSockAddrLen());
            const int connectError = connectResult == 0 ? 0 : errno;
            core::system::close(probeFd);

            // A closed Unix endpoint remains as a socket node and rejects a
            // new connection with ECONNREFUSED. ENOENT is a benign race with
            // another stale-path cleaner. Every other result is treated
            // conservatively so an active or unrelated endpoint is preserved.
            return connectResult != 0 && (connectError == ECONNREFUSED || connectError == ENOENT) ? ExistingSocketState::Stale
                                                                                                  : ExistingSocketState::ActiveOrUnknown;
        }

    } // namespace detail

    template <template <typename SocketAddress> typename PhysicalPeerSocket>
    PhysicalSocket<PhysicalPeerSocket>::PhysicalSocket(int type, int protocol)
        : Super(PF_UNIX, type, protocol)
        , logScope(logger::LogOrigin::Framework, logger::LogBoundary::System, "net.un") {
    }

    template <template <typename SocketAddress> typename PhysicalPeerSocket>
    PhysicalSocket<PhysicalPeerSocket>::PhysicalSocket(int fd, const SocketAddress& bindAddress)
        : Super(fd, bindAddress)
        , logScope(logger::LogOrigin::Framework, logger::LogBoundary::System, "net.un") {
    }

    template <template <typename SocketAddress> typename PhysicalPeerSocket>
    PhysicalSocket<PhysicalPeerSocket>::PhysicalSocket(PhysicalSocket&& physicalSocket) noexcept
        : Super(std::move(physicalSocket))
        , logScope(std::move(physicalSocket.logScope))
        , lockFd(std::exchange(physicalSocket.lockFd, -1))
        , lockHeld(std::exchange(physicalSocket.lockHeld, false))
        , lockCleanupOwned(std::exchange(physicalSocket.lockCleanupOwned, false))
        , lockIdentityValid(std::exchange(physicalSocket.lockIdentityValid, false))
        , lockDevice(std::exchange(physicalSocket.lockDevice, 0))
        , lockInode(std::exchange(physicalSocket.lockInode, 0))
        , lockPath(std::move(physicalSocket.lockPath))
        , socketIdentityValid(std::exchange(physicalSocket.socketIdentityValid, false))
        , socketDevice(std::exchange(physicalSocket.socketDevice, 0))
        , socketInode(std::exchange(physicalSocket.socketInode, 0))
        , socketPath(std::move(physicalSocket.socketPath)) {
    }

    template <template <typename SocketAddress> typename PhysicalPeerSocket>
    PhysicalSocket<PhysicalPeerSocket>& PhysicalSocket<PhysicalPeerSocket>::operator=(PhysicalSocket&& physicalSocket) noexcept {
        if (this == &physicalSocket) {
            return *this;
        }

        releaseFilesystemOwnership();
        Super::operator=(std::move(physicalSocket));
        logScope = std::move(physicalSocket.logScope);
        lockFd = std::exchange(physicalSocket.lockFd, -1);
        lockHeld = std::exchange(physicalSocket.lockHeld, false);
        lockCleanupOwned = std::exchange(physicalSocket.lockCleanupOwned, false);
        lockIdentityValid = std::exchange(physicalSocket.lockIdentityValid, false);
        lockDevice = std::exchange(physicalSocket.lockDevice, 0);
        lockInode = std::exchange(physicalSocket.lockInode, 0);
        lockPath = std::move(physicalSocket.lockPath);
        socketIdentityValid = std::exchange(physicalSocket.socketIdentityValid, false);
        socketDevice = std::exchange(physicalSocket.socketDevice, 0);
        socketInode = std::exchange(physicalSocket.socketInode, 0);
        socketPath = std::move(physicalSocket.socketPath);

        return *this;
    }

    template <template <typename SocketAddress> typename PhysicalPeerSocket>
    PhysicalSocket<PhysicalPeerSocket>::~PhysicalSocket() {
        releaseFilesystemOwnership();
    }

    template <template <typename SocketAddress> typename PhysicalPeerSocket>
    void PhysicalSocket<PhysicalPeerSocket>::releaseFilesystemOwnership() noexcept {
        if (socketIdentityValid && !socketPath.empty()) {
            struct stat socketStatus = {};

            if (::lstat(socketPath.c_str(), &socketStatus) == 0) {
                if (S_ISSOCK(socketStatus.st_mode) && static_cast<std::uintmax_t>(socketStatus.st_dev) == socketDevice &&
                    static_cast<std::uintmax_t>(socketStatus.st_ino) == socketInode) {
                    if (::unlink(socketPath.c_str()) == 0) {
                        log().debug("Remove sun path: {}", socketPath);
                    } else {
                        const int errnum = errno;
                        log().sysError(logger::LogLevel::Error, errnum, "Remove sun path: {}", socketPath);
                    }
                } else {
                    log().warn("Preserve replaced sun path: {}", socketPath);
                }
            } else if (errno != ENOENT) {
                const int errnum = errno;
                log().sysError(logger::LogLevel::Error, errnum, "Inspect sun path before removal: {}", socketPath);
            }
        }

        socketIdentityValid = false;
        socketDevice = 0;
        socketInode = 0;
        socketPath.clear();

        if (lockFd >= 0) {
            if (lockHeld && lockCleanupOwned && lockIdentityValid && !lockPath.empty()) {
                struct stat lockStatus = {};

                if (::lstat(lockPath.c_str(), &lockStatus) == 0) {
                    if (S_ISREG(lockStatus.st_mode) && static_cast<std::uintmax_t>(lockStatus.st_dev) == lockDevice &&
                        static_cast<std::uintmax_t>(lockStatus.st_ino) == lockInode) {
                        if (::unlink(lockPath.c_str()) == 0) {
                            log().debug("Remove lock file: {}", lockPath);
                        } else {
                            const int errnum = errno;
                            log().sysError(logger::LogLevel::Error, errnum, "Remove lock file: {}", lockPath);
                        }
                    } else {
                        log().warn("Preserve replaced lock file: {}", lockPath);
                    }
                } else if (errno != ENOENT) {
                    const int errnum = errno;
                    log().sysError(logger::LogLevel::Error, errnum, "Inspect lock file before removal: {}", lockPath);
                }
            }

            if (lockHeld) {
                if (core::system::flock(lockFd, LOCK_UN) == 0) {
                    log().debug("Remove lock from file: {}", lockPath);
                } else {
                    const int errnum = errno;
                    log().sysError(logger::LogLevel::Error, errnum, "Remove lock from file: {}", lockPath);
                }
            }

            core::system::close(lockFd);
            lockFd = -1;
        }

        lockHeld = false;
        lockCleanupOwned = false;
        lockIdentityValid = false;
        lockDevice = 0;
        lockInode = 0;
        lockPath.clear();
    }

    template <template <typename SocketAddress> typename PhysicalPeerSocket>
    logger::BoundaryLogger PhysicalSocket<PhysicalPeerSocket>::log() const {
        return logScope.logger(logger::Logger::semanticSink());
    }

    template <template <typename SocketAddress> typename PhysicalPeerSocket>
    logger::BoundaryLogger PhysicalSocket<PhysicalPeerSocket>::log(logger::BoundaryLogger::Sink sink,
                                                                   logger::LogLevel threshold,
                                                                   logger::BoundaryLogger::Clock clock) const {
        return logScope.logger(std::move(sink), threshold, std::move(clock));
    }

    template <template <typename SocketAddress> typename PhysicalPeerSocket>
    int PhysicalSocket<PhysicalPeerSocket>::bind(SocketAddress& bindAddress) {
        const std::string requestedSocketPath = bindAddress.getSunPath();
        const bool filesystemAddress = !requestedSocketPath.empty() && !requestedSocketPath.starts_with('\0');
        const auto lockPathIdentityError = [this]() noexcept {
            struct stat status = {};
            if (::lstat(lockPath.c_str(), &status) != 0) {
                return errno;
            }
            return S_ISREG(status.st_mode) && static_cast<std::uintmax_t>(status.st_dev) == lockDevice &&
                           static_cast<std::uintmax_t>(status.st_ino) == lockInode
                       ? 0
                       : ESTALE;
        };

        if (filesystemAddress) {
            if (lockFd >= 0) {
                errno = EINVAL;
                return -1;
            }

            socketPath = requestedSocketPath;
            lockPath = requestedSocketPath + ".lock";

            bool markerCreated = false;
            lockFd = ::open(lockPath.c_str(), O_RDWR | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK, 0600);

            if (lockFd >= 0) {
                markerCreated = true;
                lockCleanupOwned = true;
            } else if (errno == EEXIST) {
                lockFd = ::open(lockPath.c_str(), O_RDWR | O_CLOEXEC | O_NOFOLLOW | O_NONBLOCK);
            }

            if (lockFd < 0) {
                const int errnum = errno;
                log().sysError(logger::LogLevel::Error, errnum, "Opening lock file: {}", lockPath);
                socketPath.clear();
                lockPath.clear();
                errno = errnum;
                return -1;
            }

            log().debug("Opening lock file: {}", lockPath);

            struct stat lockStatus = {};
            const int lockStatResult = ::fstat(lockFd, &lockStatus);
            if (lockStatResult != 0 || !S_ISREG(lockStatus.st_mode)) {
                const int errnum = lockStatResult != 0 ? errno : EINVAL;
                log().sysError(logger::LogLevel::Error, errnum, "Inspecting lock file: {}", lockPath);
                core::system::close(lockFd);
                lockFd = -1;
                lockCleanupOwned = false;
                socketPath.clear();
                lockPath.clear();
                errno = errnum;
                return -1;
            }

            lockDevice = static_cast<std::uintmax_t>(lockStatus.st_dev);
            lockInode = static_cast<std::uintmax_t>(lockStatus.st_ino);
            lockIdentityValid = true;

            if (core::system::flock(lockFd, LOCK_EX | LOCK_NB) != 0) {
                const int errnum = errno;
                log().sysError(logger::LogLevel::Error, errnum, "Locking lock file {}", lockPath);
                core::system::close(lockFd);
                lockFd = -1;
                lockCleanupOwned = false;
                lockIdentityValid = false;
                lockDevice = 0;
                lockInode = 0;
                socketPath.clear();
                lockPath.clear();
                errno = errnum;
                return -1;
            }

            lockHeld = true;
            log().debug("Locking lock file: {}", lockPath);

            if (const int errnum = lockPathIdentityError(); errnum != 0) {
                log().sysError(logger::LogLevel::Error, errnum, "Lock file identity changed: {}", lockPath);
                releaseFilesystemOwnership();
                errno = errnum;
                return -1;
            }

            if (markerCreated) {
                if (!detail::writeFilesystemLockMarker(lockFd)) {
                    const int errnum = errno != 0 ? errno : EIO;
                    log().sysError(logger::LogLevel::Error, errnum, "Writing lock marker: {}", lockPath);
                    releaseFilesystemOwnership();
                    errno = errnum;
                    return -1;
                }
            } else {
                struct stat markerStatus = {};
                if (::fstat(lockFd, &markerStatus) != 0 || !detail::validateFilesystemLockMarker(lockFd, markerStatus)) {
                    log().error("Refusing unrecognized lock file: {}", lockPath);
                    releaseFilesystemOwnership();
                    errno = EADDRINUSE;
                    return -1;
                }
            }

            struct stat existingStatus = {};
            if (::lstat(socketPath.c_str(), &existingStatus) == 0) {
                // A marker created next to a pre-existing path cannot establish
                // ownership retroactively. Preserve that path regardless of
                // whether it appears stale, active, or is not a socket.
                if (markerCreated) {
                    log().error("Refusing to claim a pre-existing sun path: {}", socketPath);
                    releaseFilesystemOwnership();
                    errno = EADDRINUSE;
                    return -1;
                }

                if (!S_ISSOCK(existingStatus.st_mode)) {
                    log().error("Refusing to replace non-socket sun path: {}", socketPath);
                    releaseFilesystemOwnership();
                    errno = EADDRINUSE;
                    return -1;
                }

                if (detail::probeExistingSocket(bindAddress, this->type, this->protocol) != detail::ExistingSocketState::Stale) {
                    log().error("Refusing to replace an active or unverifiable sun path: {}", socketPath);
                    releaseFilesystemOwnership();
                    errno = EADDRINUSE;
                    return -1;
                }

                if (const int errnum = lockPathIdentityError(); errnum != 0) {
                    log().sysError(logger::LogLevel::Error, errnum, "Lock file identity changed before stale cleanup: {}", lockPath);
                    releaseFilesystemOwnership();
                    errno = errnum;
                    return -1;
                }

                struct stat unlinkStatus = {};
                if (::lstat(socketPath.c_str(), &unlinkStatus) != 0) {
                    if (errno == ENOENT) {
                        log().debug("Stale sun path disappeared before removal: {}", socketPath);
                    } else {
                        const int errnum = errno;
                        log().sysError(logger::LogLevel::Error, errnum, "Reinspect stale sun path: {}", socketPath);
                        releaseFilesystemOwnership();
                        errno = errnum;
                        return -1;
                    }
                } else if (!S_ISSOCK(unlinkStatus.st_mode) || unlinkStatus.st_dev != existingStatus.st_dev ||
                           unlinkStatus.st_ino != existingStatus.st_ino) {
                    log().error("Refusing to remove a replaced stale sun path: {}", socketPath);
                    releaseFilesystemOwnership();
                    errno = ESTALE;
                    return -1;
                } else if (::unlink(socketPath.c_str()) != 0) {
                    const int errnum = errno;
                    log().sysError(logger::LogLevel::Error, errnum, "Remove stale sun path: {}", socketPath);
                    releaseFilesystemOwnership();
                    errno = errnum;
                    return -1;
                } else {
                    log().warn("Removed stale sun path: {}", socketPath);
                }

            } else if (errno != ENOENT) {
                const int errnum = errno;
                log().sysError(logger::LogLevel::Error, errnum, "Inspect sun path before bind: {}", socketPath);
                releaseFilesystemOwnership();
                errno = errnum;
                return -1;
            }
        }

        if (filesystemAddress) {
            if (const int errnum = lockPathIdentityError(); errnum != 0) {
                log().sysError(logger::LogLevel::Error, errnum, "Lock file identity changed before bind: {}", lockPath);
                releaseFilesystemOwnership();
                errno = errnum;
                return -1;
            }
        }

        const int bindResult = Super::bind(bindAddress);

        if (bindResult != 0 || !filesystemAddress) {
            if (bindResult != 0 && filesystemAddress) {
                const int errnum = errno;
                releaseFilesystemOwnership();
                errno = errnum;
            }

            return bindResult;
        }

        struct stat boundStatus = {};
        const int boundStatResult = ::lstat(socketPath.c_str(), &boundStatus);
        if (boundStatResult != 0 || !S_ISSOCK(boundStatus.st_mode)) {
            const int errnum = boundStatResult != 0 ? errno : ESTALE;
            log().sysError(logger::LogLevel::Error, errnum, "Inspect bound sun path: {}", socketPath);
            releaseFilesystemOwnership();
            errno = errnum;
            return -1;
        }

        socketDevice = static_cast<std::uintmax_t>(boundStatus.st_dev);
        socketInode = static_cast<std::uintmax_t>(boundStatus.st_ino);
        socketIdentityValid = true;
        lockCleanupOwned = true;

        return 0;
    }

} // namespace net::un::phy
