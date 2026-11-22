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

#include "net/config/ConfigInstance.h"
#include "net/stdio/config/ConfigAddress.h"
#include "net/stdio/phy/stream/PhysicalSocket.h"
#include "support/TestResult.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {

    using Address = net::stdio::SocketAddress;
    using Physical = net::stdio::phy::stream::PhysicalSocket;
    using Ownership = Physical::DescriptorOwnership;

    class Config
        : public net::config::ConfigInstance
        , public net::stdio::config::ConfigAddress
        , public net::stdio::config::ConfigAddressRemote {
    public:
        using Local = net::stdio::config::ConfigAddress;
        using Remote = net::stdio::config::ConfigAddressRemote;

        Config()
            : ConfigInstance("stdio", Role::SERVER)
            , Local(this)
            , Remote(this) {
        }
    };

    template <typename Function>
    void rejected(tests::support::TestResult& result, Function function) {
        bool invalid = false;
        try {
            function();
        } catch (const Address::BadSocketAddress&) {
            invalid = true;
        }
        result.expectTrue(invalid, "invalid address throws BadSocketAddress");
    }

    void addressAndConfig(tests::support::TestResult& result) {
        Address address;
        result.expectTrue(address.getInputFd() == -1 && address.getOutputFd() == -1 && address.toString() == "stdio:unspecified",
                          "explicit unspecified initialization");
        rejected(result, [&] {
            address.init();
        });
        Address::SockAddr raw{};
        raw.header.sa_family = AF_UNSPEC;
        raw.inputFd = -1;
        raw.outputFd = -1;
        result.expectTrue(Address(raw, sizeof(raw)).toString() == "stdio:unspecified", "raw sentinel accepted");
        raw.inputFd = 7;
        raw.outputFd = 8;
        address = Address(raw, sizeof(raw));
        address.init();
        result.expectTrue(address.toString(false) == "stdio:input=7,output=8", "raw binding round trip");
        rejected(result, [&] {
            Address invalid(raw, 0);
        });
        raw.header.sa_family = AF_INET;
        rejected(result, [&] {
            Address invalid(raw, sizeof(raw));
        });
        raw.header.sa_family = AF_UNSPEC;
        raw.outputFd = -1;
        rejected(result, [&] {
            Address invalid(raw, sizeof(raw));
        });
        Config config;
        result.expectTrue(config.getInputFd() == 0 && config.getOutputFd() == 1 && config.getDescriptorOwnership() == Ownership::TAKE,
                          "local defaults are 0/1 TAKE");
        result.expectEqual(1, static_cast<int>(config.Instance::getAppWithPtr()->get_subcommands({}).size()), "only local CLI section");
        config.setSocketAddress(Address(3, 4))->setDescriptorOwnership(Ownership::BORROW);
        result.expectTrue(config.getInputFd() == 3 && config.getOutputFd() == 4 && config.getDescriptorOwnership() == Ownership::BORROW,
                          "chainable overrides");
        config.Local::getAppWithPtr()->parse("--input-fd 9 --output-fd 10 --ownership take");
        result.expectTrue(config.getInputFd() == 9 && config.getOutputFd() == 10 && config.getDescriptorOwnership() == Ownership::TAKE,
                          "CLI overrides API defaults");
        result.expectTrue(config.Local::getSocketAddress().toString() == "stdio:input=9,output=10", "CLI-backed address resolution");
        raw.inputFd = -1;
        result.expectTrue(config.Remote::getSocketAddress(raw, sizeof(raw)).toString() == "stdio:unspecified", "non-CLI remote sentinel");
    }

    void ownership(tests::support::TestResult& result) {
        int pair[2]{};
        result.expectEqual(0, ::socketpair(AF_UNIX, SOCK_STREAM, 0, pair), "create duplex descriptors");
        const int original = ::fcntl(pair[0], F_GETFL);
        const int duplicate = ::dup(pair[0]);
        const int observer = ::dup(pair[0]);
        {
            Physical provisional(Address(pair[0], duplicate));
            Address::SockAddr raw{};
            Address::SockLen length = 0;
            result.expectTrue(provisional.getSockName(raw, length) < 0, "short local destination rejected");
            length = sizeof(raw);
            result.expectEqual(0, provisional.getSockName(raw, length), "local address copied without socket lookup");
            result.expectTrue(raw.inputFd == pair[0] && raw.outputFd == duplicate, "local copy preserves descriptor pair");
            result.expectEqual(0, provisional.getPeerName(raw, length), "remote address copied without socket lookup");
            result.expectTrue(raw.inputFd == -1 && raw.outputFd == -1, "remote copy is unspecified");
            result.expectTrue(provisional.prepare(), "prepare shared description");
            result.expectTrue((::fcntl(pair[0], F_GETFL) & O_NONBLOCK) != 0, "TAKE enables nonblocking");
            Physical moved(std::move(provisional));
            result.expectTrue(provisional.getFd() == -1 && moved.getFd() == pair[0], "move transfers provisional ownership record");
        }
        result.expectEqual(original, ::fcntl(pair[0], F_GETFL), "provisional destruction restores shared flags");
        result.expectTrue(::fcntl(duplicate, F_GETFD) >= 0, "failed activation retains caller ownership");
        {
            Physical physical(Address(pair[0], duplicate));
            result.expectTrue(physical.prepare(), "prepare TAKE");
            physical.commit();
            physical.release(Physical::WR);
            result.expectTrue(::fcntl(duplicate, F_GETFD) < 0 && errno == EBADF, "output direction closed");
            result.expectTrue((::fcntl(pair[0], F_GETFL) & O_NONBLOCK) != 0, "remaining shared input stays nonblocking");
            physical.release(Physical::RD);
        }
        result.expectTrue(::fcntl(pair[0], F_GETFD) < 0 && errno == EBADF, "remaining input closed once");
        result.expectEqual(original, ::fcntl(observer, F_GETFL), "last shared direction restores original flags");
        ::close(observer);
        ::close(pair[1]);
        result.expectEqual(0, ::socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, pair), "create borrowed duplex");
        {
            Physical borrowed(Address(pair[0], pair[0]), Ownership::BORROW);
            result.expectTrue(borrowed.prepare(), "borrow prepared nonblocking handle");
            borrowed.commit();
            borrowed.release(Physical::RDWR);
        }
        result.expectTrue(::fcntl(pair[0], F_GETFD) >= 0, "BORROW never closes");
        {
            Physical same(Address(pair[0], pair[0]));
            result.expectTrue(same.prepare(), "identical duplex binding");
            same.commit();
            same.release(Physical::WR);
            result.expectTrue(::fcntl(pair[0], F_GETFD) >= 0, "same number retained while input active");
        }
        result.expectTrue(::fcntl(pair[0], F_GETFD) < 0, "same number released once at final teardown");
        ::close(pair[1]);
    }

    void validation(tests::support::TestResult& result) {
        int pair[2]{};
        result.expectEqual(0, ::pipe(pair), "create pipe");
        const int original = ::fcntl(pair[0], F_GETFL);
        {
            Physical invalid(Address(pair[0], -1));
            result.expectTrue(!invalid.prepare() && errno == EBADF, "invalid second handle rejected");
        }
        result.expectEqual(original, ::fcntl(pair[0], F_GETFL), "failed pair validation leaves first flags unchanged");
        Physical wrongMode(Address(pair[1], pair[0]));
        result.expectTrue(!wrongMode.prepare() && errno == EINVAL, "incorrect access modes rejected");
        Physical blockingBorrow(Address(pair[0], pair[1]), Ownership::BORROW);
        result.expectTrue(!blockingBorrow.prepare(), "BORROW rejects blocking handles without changing flags");
        const int regular = ::open(__FILE__, O_RDONLY);
        Physical file(Address(regular, pair[1]));
        result.expectTrue(!file.prepare() && errno == EINVAL, "regular file rejected independently of multiplexer");
        ::close(regular);
        ::close(pair[0]);
        ::close(pair[1]);
    }

    void standardNumbers(tests::support::TestResult& result) {
        for (int fd = 0; fd < 3; ++fd) {
            const int saved = ::fcntl(fd, F_DUPFD_CLOEXEC, 3);
            int pair[2]{};
            result.expectEqual(0, ::pipe(pair), "pipe for standard-number replacement");
            const int index = fd == 0 ? 0 : 1;
            const int assigned = ::dup2(pair[index], fd);
            ::close(pair[index]);
            bool prepared = false;
            {
                Physical physical(fd == 0 ? Address(fd, pair[1]) : Address(pair[0], fd));
                prepared = physical.prepare();
                physical.commit();
            }
            struct stat metadata{};
            const bool nullHandle = ::fstat(fd, &metadata) == 0 && S_ISCHR(metadata.st_mode);
            const int access = ::fcntl(fd, F_GETFL) & O_ACCMODE;
            ::dup2(saved, fd);
            ::close(saved);
            result.expectTrue(assigned == fd && prepared && nullHandle, "standard number replaced instead of freed");
            result.expectEqual(fd == 0 ? O_RDONLY : O_WRONLY, access, "null handle has correct directional access mode");
        }

        const int saved = ::fcntl(0, F_DUPFD_CLOEXEC, 3);
        int pair[2]{};
        result.expectEqual(0, ::socketpair(AF_UNIX, SOCK_STREAM, 0, pair), "duplex standard handle");
        ::dup2(pair[0], 0);
        ::close(pair[0]);
        {
            Physical physical(Address(0, 0));
            result.expectTrue(physical.prepare(), "prepare identical standard binding");
            physical.commit();
            physical.release(Physical::WR);
            result.expectTrue(::fcntl(0, F_GETFD) >= 0, "standard duplex handle retained until both directions release");
        }
        const int access = ::fcntl(0, F_GETFL) & O_ACCMODE;
        struct stat metadata{};
        const bool replaced = ::fstat(0, &metadata) == 0 && S_ISCHR(metadata.st_mode);
        ::dup2(saved, 0);
        ::close(saved);
        ::close(pair[1]);
        result.expectTrue(replaced, "identical standard descriptor replaced rather than retained");
        result.expectEqual(O_RDWR, access, "identical standard handle replaced with duplex null handle");
    }

} // namespace

int main() {
    tests::support::TestResult result;
    addressAndConfig(result);
    ownership(result);
    validation(result);
    standardNumbers(result);

    return result.processResult();
}
