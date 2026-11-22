// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "net/stdio/phy/stream/PhysicalSocket.h"

#include "Log.h"
#include "core/system/socket.h" // IWYU pragma: keep
#include "core/system/unistd.h"
#include "utils/PreserveErrno.h"

#include <cerrno>
#include <cstddef>
#include <string>
#include <utility>

namespace net::stdio::phy::stream {

    PhysicalSocket::PhysicalSocket(const SocketAddress& address, DescriptorOwnership ownership)
        : address(address)
        , ownership(ownership) {
        bindings[0].fd = address.getInputFd();
        bindings[1].fd = address.getOutputFd();
    }

    PhysicalSocket::PhysicalSocket(PhysicalSocket&& other) noexcept
        : address(other.address)
        , ownership(other.ownership)
        , bindings(std::exchange(other.bindings, {}))
        , committed(other.committed)
        , sharedFlags(other.sharedFlags) {
    }

    PhysicalSocket::~PhysicalSocket() {
        const utils::PreserveErrno preserveErrno;
        if (committed) {
            release(RDWR);
        } else {
            rollback();
        }
    }

    bool PhysicalSocket::prepare() {
        bool ready = true;
        const bool identical = bindings[0].fd == bindings[1].fd;
        const std::size_t count = identical ? 1 : bindings.size();
        for (std::size_t index = 0; index < count && ready; ++index) {
            auto& binding = bindings[index];
            struct stat metadata{};
            binding.flags = ::fcntl(binding.fd, F_GETFL);
            ready = binding.flags >= 0 && ::fstat(binding.fd, &metadata) == 0;
            if (ready) {
                binding.socket = S_ISSOCK(metadata.st_mode);
                const int access = binding.flags & O_ACCMODE;
                if (S_ISREG(metadata.st_mode) || (index == 0 ? access == O_WRONLY : access == O_RDONLY) ||
                    (identical && access != O_RDWR) || (ownership == DescriptorOwnership::BORROW && (binding.flags & O_NONBLOCK) == 0)) {
                    errno = EINVAL;
                    ready = false;
                }
            }
        }
        if (identical) {
            bindings[1].socket = bindings[0].socket;
        }
        if (ready && ownership == DescriptorOwnership::TAKE) {
            sharedFlags = identical;
            ready = ::fcntl(bindings[0].fd, F_SETFL, bindings[0].flags | O_NONBLOCK) == 0;
            if (ready && !identical) {
                const int outputFlags = ::fcntl(bindings[1].fd, F_GETFL);
                ready = outputFlags >= 0;
                sharedFlags =
                    (bindings[0].flags & O_NONBLOCK) == 0 && (bindings[1].flags & O_NONBLOCK) == 0 && (outputFlags & O_NONBLOCK) != 0;
            }
            if (ready && !identical) {
                ready = ::fcntl(bindings[1].fd, F_SETFL, bindings[1].flags | O_NONBLOCK) == 0;
            }
        }
        const int error = errno;
        if (!ready) {
            rollback();
        }
        errno = error;
        return ready;
    }

    void PhysicalSocket::commit() {
        committed = true;
    }

    int PhysicalSocket::getFd() const {
        return getReadFd();
    }

    int PhysicalSocket::getReadFd() const {
        return bindings[0].fd;
    }

    int PhysicalSocket::getWriteFd() const {
        return bindings[1].fd;
    }

    const PhysicalSocket::SocketAddress& PhysicalSocket::getBindAddress() const {
        return address;
    }

    int PhysicalSocket::copyAddress(SocketAddress::SockAddr& value, SocketAddress::SockLen& length, const SocketAddress& source) {
        int result = -1;
        if (length >= sizeof(value)) {
            value = {};
            value.header.sa_family = AF_UNSPEC;
            value.inputFd = source.getInputFd();
            value.outputFd = source.getOutputFd();
            length = sizeof(value);
            result = 0;
        } else {
            errno = EINVAL;
        }
        return result;
    }

    int PhysicalSocket::getSockName(SocketAddress::SockAddr& value, SocketAddress::SockLen& length) {
        return copyAddress(value, length, address);
    }

    int PhysicalSocket::getPeerName(SocketAddress::SockAddr& value, SocketAddress::SockLen& length) {
        return copyAddress(value, length, SocketAddress());
    }

    int PhysicalSocket::shutdown(SHUT how) {
        int result = 0;
        for (std::size_t index = 0; index < bindings.size() && result == 0; ++index) {
            if ((how == RDWR || static_cast<std::size_t>(how) == index) && bindings[index].socket && bindings[index].fd >= 0) {
                result = core::system::shutdown(bindings[index].fd, index == 0 ? SHUT_RD : SHUT_WR);
            }
        }
        return result;
    }

    void PhysicalSocket::rollback() {
        for (auto& binding : bindings) {
            if (ownership == DescriptorOwnership::TAKE && binding.fd >= 0 && binding.flags >= 0) {
                if (::fcntl(binding.fd, F_SETFL, binding.flags) < 0) {
                    snode::log::framework("net.stdio", snode::log::Boundary::Connection).systemError(snode::log::Level::Error, errno)
                        << "restore provisional descriptor flags";
                }
                binding.flags = -1;
            }
        }
    }

    void PhysicalSocket::release(SHUT how) {
        for (std::size_t index = 0; index < bindings.size(); ++index) {
            if (how == RDWR || static_cast<std::size_t>(how) == index) {
                bindings[index].released = true;
            }
        }
        if (committed) {
            releaseDirection(0);
            releaseDirection(1);
        }
    }

    void PhysicalSocket::releaseDirection(std::size_t index) {
        auto& binding = bindings[index];
        auto& other = bindings[1 - index];
        const bool identical = binding.fd == other.fd;
        if (binding.fd >= 0 && binding.released && (!identical || other.released)) {
            int result = 0;
            if (ownership == DescriptorOwnership::TAKE) {
                if ((!sharedFlags || other.released || other.fd < 0) && ::fcntl(binding.fd, F_SETFL, binding.flags) < 0) {
                    snode::log::framework("net.stdio", snode::log::Boundary::Connection).systemError(snode::log::Level::Error, errno)
                        << "restore released descriptor flags";
                }
                if (binding.fd < 3) {
                    const int nullFd = core::system::open("/dev/null", O_CLOEXEC | (identical ? O_RDWR : index == 0 ? O_RDONLY : O_WRONLY));
                    result = nullFd;
                    if (nullFd >= 0) {
                        result = ::dup2(nullFd, binding.fd);
                        const utils::PreserveErrno preserveErrno;
                        core::system::close(nullFd);
                    }
                } else {
                    result = core::system::close(binding.fd);
                    binding.fd = -1;
                    if (identical) {
                        other.fd = -1;
                    }
                }
            }
            if (result >= 0) {
                binding.fd = -1;
                if (identical) {
                    other.fd = -1;
                }
            } else {
                snode::log::framework("net.stdio", snode::log::Boundary::Connection).systemError(snode::log::Level::Error, errno)
                    << "release descriptor";
            }
        }
    }

} // namespace net::stdio::phy::stream
