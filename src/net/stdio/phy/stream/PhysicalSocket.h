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

#ifndef NET_STDIO_PHY_STREAM_PHYSICALSOCKET_H
#define NET_STDIO_PHY_STREAM_PHYSICALSOCKET_H

#include "core/socket/stream/stdio/DescriptorOwnership.h"
#include "net/stdio/SocketAddress.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <array>
#include <cstddef>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::stdio::phy::stream {

    /// Event-loop-owned descriptor pair; never derives from a single-descriptor owner.
    class PhysicalSocket {
    public:
        using SocketAddress = net::stdio::SocketAddress;
        using DescriptorOwnership = core::socket::stream::stdio::DescriptorOwnership;

        enum SHUT { RD = SHUT_RD, WR = SHUT_WR, RDWR = SHUT_RDWR };

        /// Bind caller-owned handles; prepare and activate before committing TAKE ownership.
        explicit PhysicalSocket(const SocketAddress& address, DescriptorOwnership ownership = DescriptorOwnership::TAKE);

        PhysicalSocket(const PhysicalSocket&) = delete;
        PhysicalSocket& operator=(const PhysicalSocket&) = delete;

        /// Transfer provisional or committed bindings; leaves the source empty.
        PhysicalSocket(PhysicalSocket&& other) noexcept;

        PhysicalSocket& operator=(PhysicalSocket&&) = delete;

        /// Roll back provisional flags or release committed handles; report cleanup errors.
        ~PhysicalSocket();

        /// Validate both handles and prepare nonblocking I/O transactionally; false preserves errno.
        bool prepare();
        /// Transfer TAKE ownership only after successful receiver activation; call once.
        void commit();
        /// Return the current input handle for identity only.
        int getFd() const;
        /// Return the current protocol input handle.
        int getReadFd() const;
        /// Return the current protocol output handle.
        int getWriteFd() const;
        /// Return the non-owning original binding snapshot.
        const SocketAddress& getBindAddress() const;
        /// Copy the original local binding; reject insufficient destination storage.
        int getSockName(SocketAddress::SockAddr& value, SocketAddress::SockLen& length);
        /// Copy the unspecified remote sentinel; performs no OS address lookup.
        int getPeerName(SocketAddress::SockAddr& value, SocketAddress::SockLen& length);
        /// Perform socket shutdown or a no-op; never close or replace a handle here.
        int shutdown(SHUT how);
        /// After publisher removal, detach a direction and release TAKE handles under I1.
        void release(SHUT how);

    private:
        struct Binding {
            int fd = -1;
            int flags = -1;
            bool socket = false;
            bool released = false;
        };

        void rollback();
        void releaseDirection(std::size_t index);
        int copyAddress(SocketAddress::SockAddr& value, SocketAddress::SockLen& length, const SocketAddress& address);

        SocketAddress address;
        DescriptorOwnership ownership;
        std::array<Binding, 2> bindings{};
        bool committed = false;
        bool sharedFlags = false;
    };

} // namespace net::stdio::phy::stream

#endif // NET_STDIO_PHY_STREAM_PHYSICALSOCKET_H
