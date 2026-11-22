// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "core/DescriptorEventPublisher.h"
#include "core/EventLoop.h"
#include "core/EventMultiplexer.h"
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "core/timer/Timer.h"
#include "net/in/stream/legacy/SocketClient.h"
#include "net/in/stream/legacy/SocketServer.h"
#include "net/in/stream/tls/SocketClient.h"
#include "net/in/stream/tls/SocketServer.h"
#include "support/TestResult.h"

#include <arpa/inet.h>
#include <cerrno>
#include <dlfcn.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <unistd.h>
#include <vector>

namespace {
    std::string mode;
    int targetFd = -1, closed = 0, injections = 0, contexts = 0;
    bool readerAdded = false;
    bool serverCase() {
        return mode.find("accept") != std::string::npos;
    }
    class Factory : public core::socket::stream::SocketContextFactory {
    public:
        core::socket::stream::SocketContext* create([[maybe_unused]] core::socket::stream::SocketConnection* connection) override {
            ++contexts;
            return nullptr;
        }
    };
} // namespace

// The existing AddressResolutionPolicyTest uses this libc interposition seam.
extern "C" int epoll_ctl(int epfd, int op, int fd, epoll_event* event) noexcept {
    static const auto real = reinterpret_cast<decltype(&epoll_ctl)>(dlsym(RTLD_NEXT, "epoll_ctl"));
    int result = 0;
    const bool second = mode.ends_with("second");
    if (fd == targetFd && op == EPOLL_CTL_ADD && event != nullptr && injections == 0 &&
        ((!second && (event->events & EPOLLIN) != 0) || (second && readerAdded && (event->events & EPOLLOUT) != 0))) {
        ++injections;
        errno = ENOSPC;
        result = -1;
    } else {
        result = real(epfd, op, fd, event);
        if (fd == targetFd && result == 0 && event != nullptr && op == EPOLL_CTL_ADD && (event->events & EPOLLIN) != 0) {
            readerAdded = true;
        }
    }
    return result;
}
extern "C" int accept4(int fd, sockaddr* address, socklen_t* length, int flags) {
    static const auto real = reinterpret_cast<decltype(&accept4)>(dlsym(RTLD_NEXT, "accept4"));
    const int result = real(fd, address, length, flags);
    if (serverCase() && result >= 0) {
        targetFd = result;
    }
    return result;
}
extern "C" int connect(int fd, const sockaddr* address, socklen_t length) {
    static const auto real = reinterpret_cast<decltype(&connect)>(dlsym(RTLD_NEXT, "connect"));
    int result = real(fd, address, length);
    if (!serverCase() && address->sa_family == AF_INET) {
        targetFd = fd;
        if (result == 0 || errno == EINPROGRESS) {
            result = mode.find("immediate") != std::string::npos ? 0 : -1;
            errno = result == 0 ? 0 : EINPROGRESS;
        }
    }
    return result;
}
extern "C" int close(int fd) {
    static const auto real = reinterpret_cast<decltype(&close)>(dlsym(RTLD_NEXT, "close"));
    if (fd == targetFd) {
        ++closed;
    }
    return real(fd);
}

namespace {
    template <class Endpoint>
    void run(tests::support::TestResult& result, char* program) {
        Endpoint endpoint("activation");
        endpoint.getConfig()->Instance::forceUnrequired();
        int callbacks = 0, failures = 0;
        std::string failure;
        endpoint.setOnConnect([&](auto*) {
            ++callbacks;
        });
        endpoint.setOnConnected([&](auto*) {
            ++callbacks;
        });
        endpoint.setOnDisconnect([&](auto*) {
            ++callbacks;
        });
        const int peer = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (!serverCase()) {
            result.expectEqual(0, ::bind(peer, reinterpret_cast<sockaddr*>(&address), sizeof(address)), "raw listener binds");
            result.expectEqual(0, ::listen(peer, 1), "raw listener listens");
            socklen_t length = sizeof(address);
            result.expectEqual(0, ::getsockname(peer, reinterpret_cast<sockaddr*>(&address), &length), "listener port");
        }
        char config[] = "--config-file=/dev/null", quiet[] = "--quiet";
        char* arguments[] = {program, config, quiet};
        core::SNodeC::init(3, arguments);
        std::vector<int> held;
        typename Endpoint::FlowHandle flow;
        const auto onStatus = [&](const auto& bound, const core::socket::State& state) {
            if (serverCase() && state == core::socket::State::OK) {
                address.sin_port = htons(bound.getPort());
                if (mode == "accept-select") {
                    int fd = 0;
                    while (fd >= 0 && fd < FD_SETSIZE) {
                        fd = ::fcntl(peer, F_DUPFD_CLOEXEC, 0);
                        held.push_back(fd);
                    }
                    result.expectTrue(fd >= FD_SETSIZE, "low descriptor slots filled before real accept");
                }
                const int connected = ::connect(peer, reinterpret_cast<sockaddr*>(&address), sizeof(address));
                result.expectTrue(connected == 0 || errno == EINPROGRESS, "raw peer connects");
            } else if (state != core::socket::State::OK) {
                ++failures;
                failure = state.what();
            }
        };
        if constexpr (requires { endpoint.listen(onStatus); }) {
            flow = endpoint.listen(net::in::SocketAddress("127.0.0.1", 0), onStatus);
        } else {
            flow = endpoint.connect(net::in::SocketAddress("127.0.0.1", ntohs(address.sin_port)), onStatus);
        }
        core::EventReceiver::atNextTick([&] {
            core::timer::Timer::singleshotTimer(
                [&] {
                    flow->terminateFlow();
                },
                0.03);
            core::timer::Timer::singleshotTimer(
                [] {
                    core::SNodeC::stop();
                },
                0.06);
        });
        result.expectEqual(0, core::SNodeC::start(), "event loop exits normally");
        result.expectEqual(0, callbacks, "no connect, connected or disconnect callback for failed activation");
        result.expectEqual(0, contexts, "no context is created");
        result.expectEqual(1, closed, "connection descriptor closed exactly once");
        result.expectEqual(serverCase() ? 0 : 1, failures, "failed client attempt reports exactly once");
        if (mode == "accept-select") {
            std::cout << "accepted fd=" << targetFd << " FD_SETSIZE=" << FD_SETSIZE << std::endl;
            result.expectTrue(targetFd >= FD_SETSIZE, "real accepted descriptor exceeds select capacity");
        } else {
            result.expectEqual(1, injections, "requested first/second registration failure was exercised");
        }
        if (!serverCase()) {
            result.expectTrue(failure.find("[28]") != std::string::npos, "activation errno ENOSPC survives deletion and status reporting");
        }
        auto& mux = core::EventLoop::instance().getEventMultiplexer();
        result.expectEqual(
            0, mux.getDescriptorEventPublisher(core::EventMultiplexer::RD).getObservedEventReceiverCount(), "no stale reader");
        result.expectEqual(
            0, mux.getDescriptorEventPublisher(core::EventMultiplexer::WR).getObservedEventReceiverCount(), "no stale writer");
        for (int fd : held) {
            if (fd >= 0) {
                ::close(fd);
            }
        }
        ::close(peer);
    }
} // namespace
int main(int argc, char* argv[]) {
    tests::support::TestResult result;
    mode = argc > 1 ? argv[1] : "accept-second";
    bool ready = true;
    if (mode == "accept-select") {
        rlimit limit{};
        ready = ::getrlimit(RLIMIT_NOFILE, &limit) == 0 && limit.rlim_max > FD_SETSIZE + 64;
        if (ready) {
            limit.rlim_cur = limit.rlim_max;
            ready = ::setrlimit(RLIMIT_NOFILE, &limit) == 0;
        }
        std::cout << "RLIMIT_NOFILE=" << limit.rlim_cur << std::endl;
        result.expectTrue(ready, "prerequisite: RLIMIT_NOFILE raised above FD_SETSIZE");
    }
    if (ready) {
        if (mode.starts_with("tls-accept")) {
            run<net::in::stream::tls::SocketServer<Factory>>(result, argv[0]);
        } else if (mode.starts_with("tls-client")) {
            run<net::in::stream::tls::SocketClient<Factory>>(result, argv[0]);
        } else if (serverCase()) {
            run<net::in::stream::legacy::SocketServer<Factory>>(result, argv[0]);
        } else {
            run<net::in::stream::legacy::SocketClient<Factory>>(result, argv[0]);
        }
    }
    return result.processResult();
}
