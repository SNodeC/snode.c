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

#ifndef NET_STDIO_SOCKETADDRESS_H
#define NET_STDIO_SOCKETADDRESS_H

#include "net/SocketAddress.h" // IWYU pragma: export

// IWYU pragma: no_include "net/SocketAddress.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <string>
#include <type_traits>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::stdio {

    namespace detail {

        struct SockAddr {
            sockaddr header;
            int inputFd;
            int outputFd;
        };

        static_assert(std::is_standard_layout_v<SockAddr>);
        static_assert(std::is_trivially_copyable_v<SockAddr>);
        static_assert(offsetof(SockAddr, header) == 0);

    } // namespace detail

    /// Process-local descriptor binding; address copies never own or access descriptors.
    class SocketAddress final : public net::SocketAddress<detail::SockAddr> {
    private:
        using Super = net::SocketAddress<detail::SockAddr>;

    public:
        /// Construct the unspecified (-1, -1) sentinel.
        SocketAddress();
        /// Describe already-open input and output handles without accessing them.
        SocketAddress(int inputFd, int outputFd);
        /// Reconstruct a complete AF_UNSPEC binding or the unspecified sentinel; throw on invalid input.
        SocketAddress(const SockAddr& sockAddr, SockLen sockAddrLen);

        /// Reject unspecified or negative descriptor bindings; performs no descriptor I/O.
        void init();
        /// Update the non-owning input binding.
        SocketAddress& setInputFd(int inputFd);
        /// Return the input binding, including -1 for the unspecified address.
        int getInputFd() const;
        /// Update the non-owning output binding.
        SocketAddress& setOutputFd(int outputFd);
        /// Return the output binding, including -1 for the unspecified address.
        int getOutputFd() const;
        /// Format the binding identically for either presentation mode.
        std::string toString(bool expanded = true) const override;
    };

} // namespace net::stdio

extern template class net::SocketAddress<net::stdio::detail::SockAddr>;

#endif // NET_STDIO_SOCKETADDRESS_H
