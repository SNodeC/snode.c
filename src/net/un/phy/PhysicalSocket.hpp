/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "net/un/phy/PhysicalSocket.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cerrno>
#include <cstdio>
#include <fcntl.h>
#include <string>
#include <sys/file.h>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::un::phy {

    template <template <typename SocketAddress> typename PhysicalPeerSocket>
    PhysicalSocket<PhysicalPeerSocket>::PhysicalSocket(int type, int protocol)
        : Super(PF_UNIX, type, protocol) {
    }

    template <template <typename SocketAddress> typename PhysicalPeerSocket>
    PhysicalSocket<PhysicalPeerSocket>::PhysicalSocket(PhysicalSocket&& physicalSocket) noexcept
        : Super(std::move(physicalSocket))
        , lockFd(std::exchange(physicalSocket.lockFd, -1)) {
    }

    template <template <typename SocketAddress> typename PhysicalPeerSocket>
    PhysicalSocket<PhysicalPeerSocket>&
    PhysicalSocket<PhysicalPeerSocket>::operator=(PhysicalSocket&& physicalSocket) noexcept { // cppcheck-suppress operatorEqVarError
        Super::operator=(std::move(physicalSocket));
        lockFd = std::exchange(physicalSocket.lockFd, -1);

        return *this;
    }

    template <template <typename SocketAddress> typename PhysicalPeerSocket>
    PhysicalSocket<PhysicalPeerSocket>::~PhysicalSocket() {
        if (lockFd >= 0 && flock(lockFd, LOCK_UN) == 0) {
            close(lockFd);
            lockFd = -1;

            if (std::remove(Super::bindAddress.getSunPath().append(".lock").data()) == 0) {
                LOG(TRACE) << "Remove lock file: " << Super::getBindAddress().getSunPath().append(".lock");
            } else {
                PLOG(TRACE) << "Remove lock file: " << Super::getBindAddress().getSunPath().append(".lock");
            }
            if (std::remove(Super::getBindAddress().getSunPath().data()) == 0) {
                LOG(TRACE) << "Remove sunPath: " << Super::getBindAddress().getSunPath();
            } else {
                PLOG(TRACE) << "Remove sunPath: " << Super::getBindAddress().getSunPath();
            }
        }
    }

    template <template <typename SocketAddress> typename PhysicalPeerSocket>
    int PhysicalSocket<PhysicalPeerSocket>::bind(SocketAddress& bindAddress) {
        if (!bindAddress.getSunPath().empty()) {
            lockFd = open(bindAddress.getSunPath().append(".lock").data(), O_RDONLY | O_CREAT, 0600);

            if (lockFd >= 0) {
                LOG(TRACE) << "Opening lock file " << bindAddress.getSunPath().append(".lock").data();
                if (flock(lockFd, LOCK_EX | LOCK_NB) == 0) {
                    LOG(TRACE) << "Locking lock file " << bindAddress.getSunPath().append(".lock").data();
                    if (std::remove(bindAddress.getSunPath().data()) == 0) {
                        LOG(TRACE) << "Removed possible stalled sun_path: " << bindAddress.getSunPath().append(".lock").data();
                    } else {
                        PLOG(TRACE) << "Removed possible stalled sun path: " << bindAddress.getSunPath().append(".lock").data();
                    }
                } else {
                    PLOG(TRACE) << "Locking lock file " << bindAddress.getSunPath().append(".lock").data();

                    close(lockFd);
                    lockFd = -1;
                }
            } else {
                PLOG(TRACE) << "Opening lock file " << bindAddress.getSunPath().append(".lock").data();
            }
        }

        return Super::bind(bindAddress);
    }

} // namespace net::un::phy
