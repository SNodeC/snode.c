// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef NET_STDIO_SOCKETADDRESS_H
#define NET_STDIO_SOCKETADDRESS_H

#include "net/SocketAddress.h" // IWYU pragma: export

#include <cstddef>
#include <string>
#include <type_traits>

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
