// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef NET_STDIO_PHY_STREAM_PHYSICALSOCKET_H
#define NET_STDIO_PHY_STREAM_PHYSICALSOCKET_H

#include "core/socket/stream/stdio/DescriptorOwnership.h"
#include "net/stdio/SocketAddress.h"

#include <array>
#include <cstddef>
#include <string>

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
