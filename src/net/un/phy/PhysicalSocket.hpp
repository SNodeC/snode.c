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

#include "core/system/unistd.h"
#include "log/Logger.h"

#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <string>
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
    PhysicalSocket<PhysicalPeerSocket>& PhysicalSocket<PhysicalPeerSocket>::operator=(PhysicalSocket&& physicalSocket) noexcept {
        Super::operator=(std::move(physicalSocket));
        lockFd = std::exchange(physicalSocket.lockFd, -1);

        return *this;
    }

    template <template <typename SocketAddress> typename PhysicalPeerSocket>
    PhysicalSocket<PhysicalPeerSocket>::~PhysicalSocket() {
        if (lockFd >= 0) {
            if (std::remove(Super::getBindAddress().getSunPath().data()) == 0) {
                LOG(DEBUG) << "Remove sun path: " << Super::getBindAddress().getSunPath();
            } else {
                PLOG(ERROR) << "Remove sun path: " << Super::getBindAddress().getSunPath();
            }

            if (core::system::flock(lockFd, LOCK_UN) == 0) {
                LOG(DEBUG) << "Remove lock from file: " << Super::getBindAddress().getSunPath().append(".lock");
            } else {
                PLOG(ERROR) << "Remove lock from file: " << Super::getBindAddress().getSunPath().append(".lock");
            }

            if (std::remove(Super::bindAddress.getSunPath().append(".lock").data()) == 0) {
                LOG(DEBUG) << "Remove lock file: " << Super::getBindAddress().getSunPath().append(".lock");
            } else {
                PLOG(ERROR) << "Remove lock file: " << Super::getBindAddress().getSunPath().append(".lock");
            }

            core::system::close(lockFd);
            lockFd = -1;
        }
    }

    template <template <typename SocketAddress> typename PhysicalPeerSocket>
    int PhysicalSocket<PhysicalPeerSocket>::bind(SocketAddress& bindAddress) {
        if (!bindAddress.getSunPath().empty() && !bindAddress.getSunPath().starts_with('\0')) {
            if ((lockFd = open(bindAddress.getSunPath().append(".lock").data(), O_RDONLY | O_CREAT, 0600)) >= 0) {
                LOG(DEBUG) << "Opening lock file: " << bindAddress.getSunPath().append(".lock").data();
                if (core::system::flock(lockFd, LOCK_EX | LOCK_NB) == 0) {
                    LOG(DEBUG) << "Locking lock file: " << bindAddress.getSunPath().append(".lock").data();
                    if (std::filesystem::exists(bindAddress.getSunPath().data())) {
                        if (std::remove(bindAddress.getSunPath().data()) == 0) {
                            LOG(WARNING) << "Removed stalled sun_path: " << bindAddress.getSunPath().data();
                        } else {
                            PLOG(ERROR) << "Removed stalled sun path: " << bindAddress.getSunPath().data();
                        }
                    }
                } else {
                    PLOG(ERROR) << "Locking lock file " << bindAddress.getSunPath().append(".lock").data();

                    core::system::close(lockFd);
                    lockFd = -1;
                }
            } else {
                PLOG(ERROR) << "Opening lock file: " << bindAddress.getSunPath().append(".lock").data();
            }
        }

        return Super::bind(bindAddress);
    }

} // namespace net::un::phy
