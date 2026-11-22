// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef CORE_SOCKET_STREAM_STDIO_DESCRIPTOROWNERSHIP_H
#define CORE_SOCKET_STREAM_STDIO_DESCRIPTOROWNERSHIP_H

namespace core::socket::stream::stdio {

    /// TAKE transfers prepared handles after activation; BORROW never changes or closes them.
    enum class DescriptorOwnership { TAKE, BORROW };

} // namespace core::socket::stream::stdio

#endif // CORE_SOCKET_STREAM_STDIO_DESCRIPTOROWNERSHIP_H
