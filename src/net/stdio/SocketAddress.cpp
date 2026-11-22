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

#include "net/stdio/SocketAddress.h"

#include "core/socket/State.h"
#include "net/SocketAddress.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::stdio {

    SocketAddress::SocketAddress()
        : Super(AF_UNSPEC) {
        sockAddr.inputFd = -1;
        sockAddr.outputFd = -1;
    }

    SocketAddress::SocketAddress(int inputFd, int outputFd)
        : SocketAddress() {
        setInputFd(inputFd);
        setOutputFd(outputFd);
    }

    SocketAddress::SocketAddress(const SockAddr& value, SockLen length)
        : SocketAddress() {
        if (length != sizeof(SockAddr) || value.header.sa_family != AF_UNSPEC ||
            !((value.inputFd == -1 && value.outputFd == -1) || (value.inputFd >= 0 && value.outputFd >= 0))) {
            throw BadSocketAddress(core::socket::STATE_FATAL, "Invalid stdio address", EINVAL);
        }

        sockAddr = value;
    }

    void SocketAddress::init() {
        if (sockAddr.inputFd < 0 || sockAddr.outputFd < 0) {
            throw BadSocketAddress(core::socket::STATE_FATAL, "Unspecified stdio descriptors", EINVAL);
        }
    }

    SocketAddress& SocketAddress::setInputFd(int inputFd) {
        sockAddr.inputFd = inputFd;

        return *this;
    }

    int SocketAddress::getInputFd() const {
        return sockAddr.inputFd;
    }

    SocketAddress& SocketAddress::setOutputFd(int outputFd) {
        sockAddr.outputFd = outputFd;

        return *this;
    }

    int SocketAddress::getOutputFd() const {
        return sockAddr.outputFd;
    }

    std::string SocketAddress::toString([[maybe_unused]] bool expanded) const {
        return sockAddr.inputFd == -1 && sockAddr.outputFd == -1
                   ? "stdio:unspecified"
                   : "stdio:input=" + std::to_string(sockAddr.inputFd) + ",output=" + std::to_string(sockAddr.outputFd);
    }

} // namespace net::stdio

template class net::SocketAddress<net::stdio::detail::SockAddr>;
