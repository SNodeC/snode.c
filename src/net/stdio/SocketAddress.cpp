// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "net/stdio/SocketAddress.h"

#include "core/socket/State.h"
#include "net/SocketAddress.hpp"

#include <cerrno>

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
