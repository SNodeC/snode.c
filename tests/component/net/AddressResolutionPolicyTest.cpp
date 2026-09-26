// SPDX-License-Identifier: MIT OR LGPL-3.0-or-later
// Controlled resolver and syscall fixtures exercise the real public socket APIs.
#include "core/SNodeC.h"
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "net/in/stream/legacy/SocketClient.h"
#include "net/in/stream/legacy/SocketServer.h"
#include "net/in/stream/tls/SocketClient.h"
#include "net/in6/stream/legacy/SocketClient.h"
#include "support/TestResult.h"

#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <netdb.h>
#include <string>
#include <unistd.h>
#include <vector>

namespace {
    std::string mode;
    std::vector<std::string> attempts;
    int resolverFlags = 0;
    int pendingFd = -1;
    int pendingPeer = -1;
    int testPort = 0;
    int injectedError = 0;
    int forcedFailures = 0;

    std::string numericHost(const sockaddr* address) {
        char host[INET6_ADDRSTRLEN]{};
        if (address->sa_family == AF_INET)
            inet_ntop(AF_INET, &reinterpret_cast<const sockaddr_in*>(address)->sin_addr, host, sizeof(host));
        else if (address->sa_family == AF_INET6)
            inet_ntop(AF_INET6, &reinterpret_cast<const sockaddr_in6*>(address)->sin6_addr, host, sizeof(host));
        return host;
    }
} // namespace

// Interpose only test names. All numeric address translation remains in libc.
extern "C" int getaddrinfo(const char* node, const char* service, const addrinfo* hints, addrinfo** output) {
    static const auto real = reinterpret_cast<decltype(&getaddrinfo)>(dlsym(RTLD_NEXT, "getaddrinfo"));
    if (node && std::string(node) == "mapped.test") {
        resolverFlags = hints->ai_flags;
        return real("127.0.0.1", service, hints, output);
    }
    if (!node || std::string(node) != "multi.test")
        return real(node, service, hints, output);
    const bool ipv6 = hints->ai_family == AF_INET6;
    const char* first = ipv6 ? "::1" : "127.0.0.2";
    const char* second = ipv6 ? "::ffff:127.0.0.1" : "127.0.0.1";
    const int error = real(first, service, hints, output);
    if (error)
        return error;
    addrinfo* next = nullptr;
    const int nextError = real(second, service, hints, &next);
    if (nextError) {
        freeaddrinfo(*output);
        *output = nullptr;
        return nextError;
    }
    addrinfo* tail = *output;
    while (tail->ai_next)
        tail = tail->ai_next;
    tail->ai_next = next;
    return 0;
}

extern "C" int connect(int fd, const sockaddr* address, socklen_t length) {
    static const auto real = reinterpret_cast<decltype(&connect)>(dlsym(RTLD_NEXT, "connect"));
    const bool inet = address->sa_family == AF_INET || address->sa_family == AF_INET6;
    const int port = address->sa_family == AF_INET6 ? ntohs(reinterpret_cast<const sockaddr_in6*>(address)->sin6_port)
                                                    : (inet ? ntohs(reinterpret_cast<const sockaddr_in*>(address)->sin_port) : 0);
    if (inet && port == testPort) {
        attempts.push_back(numericHost(address));
        if (forcedFailures > 0) {
            --forcedFailures;
            if (mode.starts_with("async") || mode == "timer") {
                pendingFd = fd;
                if (mode == "timer") {
                    // A full socket-pair send buffer stays non-writable until
                    // the framework timeout, without contacting any network.
                    int pair[2];
                    if (socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, pair) != 0)
                        std::abort();
                    char data[4096]{};
                    while (send(pair[0], data, sizeof(data), MSG_NOSIGNAL) > 0) {
                    }
                    if (dup2(pair[0], fd) < 0)
                        std::abort();
                    close(pair[0]);
                    pendingPeer = pair[1];
                } else {
                    // A completed connection makes the descriptor writable;
                    // the SO_ERROR fixture controls the reported failure.
                    real(fd, address, length);
                }
                errno = EINPROGRESS;
            } else {
                errno = injectedError;
            }
            return -1;
        }
    }
    return real(fd, address, length);
}

extern "C" int getsockopt(int fd, int level, int option, void* value, socklen_t* length) noexcept {
    static const auto real = reinterpret_cast<decltype(&getsockopt)>(dlsym(RTLD_NEXT, "getsockopt"));
    if (fd == pendingFd && level == SOL_SOCKET && option == SO_ERROR) {
        *static_cast<int*>(value) = mode == "timer" ? EINPROGRESS : injectedError;
        *length = sizeof(int);
        if (mode != "timer")
            pendingFd = -1;
        return 0;
    }
    return real(fd, level, option, value, length);
}

extern "C" int close(int fd) {
    static const auto real = reinterpret_cast<decltype(&close)>(dlsym(RTLD_NEXT, "close"));
    if (fd == pendingFd) {
        pendingFd = -1;
        if (pendingPeer >= 0) {
            real(pendingPeer);
            pendingPeer = -1;
        }
    }
    return real(fd);
}

namespace {
    class Context : public core::socket::stream::SocketContext {
    public:
        using SocketContext::SocketContext;

    private:
        void onConnected() override {
        }
        void onDisconnected() override {
        }
        std::size_t onReceivedFromPeer() override {
            return 0;
        }
        bool onSignal(int) override {
            return true;
        }
    };
    class Factory : public core::socket::stream::SocketContextFactory {
    public:
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* connection) override {
            return new Context(connection);
        }
    };

    template <typename Address>
    void resolution(tests::support::TestResult& result, const std::vector<std::string>& literals) {
        for (const auto& literal : literals) {
            Address address(literal, 8080);
            address.init({.aiFlags = AI_NUMERICHOST, .aiSockType = SOCK_STREAM});
            result.expectTrue(numericHost(&address.getSockAddr()) == (literal == "0:0:0:0:0:0:0:1" ? "::1" : literal),
                              "numeric literal resolves to expected address: " + literal);
        }
        Address original("multi.test", 8080);
        original.init({.aiSockType = SOCK_STREAM});
        const auto first = numericHost(&original.getSockAddr());
        Address a = original;
        Address b = original;
        result.expectTrue(a.useNext(), "first independent copy has next candidate");
        result.expectTrue(b.useNext(), "second independent copy has next candidate");
        result.expectTrue(numericHost(&a.getSockAddr()) == numericHost(&b.getSockAddr()), "copies select the same second candidate");
        result.expectTrue(numericHost(&original.getSockAddr()) == first, "cached original retains first candidate");
        result.expectTrue(!a.useNext(), "first copy reaches end");
        b.setHost(literals.front());
        b.init({.aiSockType = SOCK_STREAM});
        result.expectTrue(original.useNext(), "reinitializing a copy preserves original resolution and cursor");
        result.expectTrue(numericHost(&original.getSockAddr()) != first, "original advances to distinct second candidate");
    }

    template <typename Client, typename Address>
    void clientTest(tests::support::TestResult& result, int count) {
        const int listener = socket(AF_INET, SOCK_STREAM, 0);
        sockaddr_in address{};
        address.sin_family = AF_INET;
        inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
        result.expectEqual(0, bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)), "raw fixture binds");
        result.expectEqual(0, listen(listener, 16), "raw fixture listens");
        socklen_t length = sizeof(address);
        getsockname(listener, reinterpret_cast<sockaddr*>(&address), &length);
        testPort = ntohs(address.sin_port);
        Client client("policy-client");
        client.getConfig()->Instance::forceUnrequired();
        client.getConfig()->setRetry(true)->setRetryTimeout(0.01)->setRetryTries(1);
        int connected = 0;
        const bool multiple = mode == "client" || mode == "client6" || mode == "client-tls" || mode == "retry";
        for (int i = 0; i < count; ++i)
            client.connect(Address(multiple ? "multi.test" : "127.0.0.1", testPort), [&](const Address&, core::socket::State state) {
                if (state == core::socket::State::OK && ++connected == count)
                    core::SNodeC::stop();
                else if (state == core::socket::State::FATAL)
                    core::SNodeC::stop();
            });
        core::SNodeC::start();
        result.expectEqual(mode == "permanent" ? 0 : count, connected, "connection outcome matches error policy");
        if (mode == "retry")
            result.expectTrue(attempts == std::vector<std::string>{"127.0.0.2", "127.0.0.1", "127.0.0.2", "127.0.0.1"},
                              "retry starts at first candidate and reaches second");
        else if (!multiple)
            result.expectEqual(mode == "permanent" ? 1 : 2,
                               static_cast<int>(attempts.size()),
                               "transient errors retry once; permission errors do not retry");
        else {
            result.expectEqual(4, static_cast<int>(attempts.size()), "two flows each try both candidates");
            const std::string second = mode == "client6" ? "::ffff:127.0.0.1" : "127.0.0.1";
            result.expectEqual(
                2, static_cast<int>(std::count(attempts.begin(), attempts.end(), second)), "both flows reach second candidate");
        }
        close(listener);
    }
} // namespace

int main(int argc, char* argv[]) {
    mode = argc > 1 ? argv[1] : "resolution";
    tests::support::TestResult result;
    if (mode == "resolution") {
        resolution<net::in::SocketAddress>(result, {"127.0.0.1", "0.0.0.0"});
        resolution<net::in6::SocketAddress>(result, {"::1", "0:0:0:0:0:0:0:1", "::", "::ffff:127.0.0.1"});
        return result.processResult();
    }
    char clientName[] = "policy-client";
    char socketSection[] = "socket";
    char timeoutOption[] = "--connect-timeout=0.03";
    char* arguments[] = {argv[0], clientName, socketSection, timeoutOption};
    core::SNodeC::init(mode == "server" || mode == "mapped" ? 1 : 4, arguments);
    if (mode == "server") {
        net::in::stream::legacy::SocketServer<Factory> server("policy-server");
        server.getConfig()->Instance::forceUnrequired();
        std::vector<std::string> bound;
        server.listen(net::in::SocketAddress("multi.test", 0), [&](const auto& address, core::socket::State state) {
            if (state == core::socket::State::OK) {
                auto copy = address;
                bound.push_back(numericHost(&copy.getSockAddr()));
            }
            if (bound.size() == 2 || state == core::socket::State::FATAL)
                core::SNodeC::stop();
        });
        core::SNodeC::start();
        result.expectTrue(bound == std::vector<std::string>{"127.0.0.2", "127.0.0.1"}, "server binds both distinct candidates");
    } else if (mode == "mapped") {
        for (bool enabled : {false, true}) {
            net::in6::stream::legacy::SocketClient<Factory> client(enabled ? "mapped-on" : "mapped-off");
            client.getConfig()->Remote::setHost("mapped.test")->setPort(8080);
            client.getConfig()->Remote::getOption("--ipv4-mapped")->add_result(enabled ? "true" : "false");
            bool resolved = false;
            try {
                auto address = client.getConfig()->Remote::getSocketAddress();
                resolved = numericHost(&address.getSockAddr()) == "::ffff:127.0.0.1";
            } catch (const core::socket::SocketAddress::BadSocketAddress&) {
            }
            result.expectTrue(bool(resolverFlags & AI_V4MAPPED) == enabled, "mapped option reaches resolver");
            result.expectTrue(resolved == enabled, "IPv4-only host resolves only with mapping enabled");
        }
    } else {
        if (mode == "retry") {
            injectedError = ECONNREFUSED;
            forcedFailures = 2;
        } else if (mode != "client" && mode != "client6" && mode != "client-tls") {
            injectedError = mode == "permanent" ? EACCES : (mode.find("host") != std::string::npos ? EHOSTUNREACH : ETIMEDOUT);
            forcedFailures = 1;
        }
        if (mode == "client-tls")
            clientTest<net::in::stream::tls::SocketClient<Factory>, net::in::SocketAddress>(result, 2);
        else if (mode == "client6")
            clientTest<net::in6::stream::legacy::SocketClient<Factory>, net::in6::SocketAddress>(result, 2);
        else
            clientTest<net::in::stream::legacy::SocketClient<Factory>, net::in::SocketAddress>(result, mode == "client" ? 2 : 1);
    }
    core::SNodeC::free();
    return result.processResult();
}
