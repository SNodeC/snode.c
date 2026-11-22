// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "core/SNodeC.h"
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/stdio/SocketConnection.hpp"
#include "net/config/ConfigConnection.h"
#include "net/config/ConfigInstance.h"
#include "net/stdio/config/ConfigAddress.h"
#include "net/stdio/phy/stream/PhysicalSocket.h"
#include "support/TestResult.h"

#include <cerrno>
#include <chrono>
#include <fcntl.h>
#include <string>
#include <unistd.h>

namespace {
    tests::support::TestResult result;
    std::string mode, reply;
    int disconnected = 0, writeError = 0, reads = 0, output = -1, input = -1, reused = -1;
    class Config
        : public net::config::ConfigInstance
        , public net::config::ConfigConnection
        , public net::stdio::config::ConfigAddress
        , public net::stdio::config::ConfigAddressRemote {
    public:
        using Local = net::stdio::config::ConfigAddress;
        using Remote = net::stdio::config::ConfigAddressRemote;
        Config()
            : ConfigInstance("stdio", Role::CLIENT)
            , ConfigConnection(this)
            , Local(this)
            , Remote(this) {
            Instance::forceUnrequired();
            setReadTimeout(0)->setWriteTimeout(0)->setReadBlockSize(2)->setWriteBlockSize(3)->setTerminateTimeout(0.3);
        }
    };
    using Physical = net::stdio::phy::stream::PhysicalSocket;
    using Connection = core::socket::stream::stdio::SocketConnection<Physical, Config>;
    class Context : public core::socket::stream::SocketContext {
    public:
        Context(core::socket::stream::SocketConnection* connection, bool initiator)
            : SocketContext(connection)
            , initiator(initiator) {
        }

    private:
        void onConnected() override {
            if (initiator) {
                sendToPeer("alpha\n");
                sendToPeer("beta\n");
                shutdownWrite();
            }
        }
        void onDisconnected() override {
        }
        bool onSignal([[maybe_unused]] int signal) override {
            return true;
        }
        std::size_t onReceivedFromPeer() override {
            char chunk[2];
            const auto length = readFromPeer(chunk, sizeof(chunk));
            received.append(chunk, length);
            ++reads;
            if (initiator) {
                reply = received;
            }
            return length;
        }
        void onReadError(int error) override {
            if (!initiator && error == 0) {
                if (mode == "pipes") {
                    result.expectTrue(::fcntl(output, F_GETFD) < 0 && ::fcntl(input, F_GETFD) >= 0, "output EOF with input still active");
                    reused = ::fcntl(getSocketConnection()->getFd(), F_DUPFD_CLOEXEC, output);
                    result.expectEqual(output, reused, "released output number can be reused immediately");
                }
                sendToPeer(received);
            }
            SocketContext::onReadError(error);
        }
        void onWriteError(int error) override {
            writeError = error;
            close();
            result.expectTrue(::fcntl(output, F_GETFD) >= 0, "disable retains output until publisher release");
        }
        bool initiator;
        std::string received;
    };
} // namespace

int main(int argc, char* argv[]) {
    mode = argc > 1 ? argv[1] : "pipes";
    auto config = std::make_shared<Config>();
    char file[] = "--config-file=/dev/null", quiet[] = "--quiet";
    char* arguments[] = {argv[0], file, quiet};
    core::SNodeC::init(3, arguments);
    int handles[4]{}, alias = -1;
    if (mode == "duplex") {
        result.expectEqual(0, ::socketpair(AF_UNIX, SOCK_STREAM, 0, handles), "create duplex pair");
        handles[2] = handles[1];
        handles[3] = handles[0];
        alias = ::dup(handles[0]);
    } else {
        result.expectEqual(0, ::pipe(handles), "reply pipe");
        result.expectEqual(0, ::pipe(handles + 2), "request pipe");
    }
    input = handles[0];
    output = handles[3];
    const auto onDisconnect = [](Connection*) {
        if (++disconnected == (mode == "epipe" ? 1 : 2)) {
            core::SNodeC::stop();
        }
    };
    if (mode == "epipe") {
        ::close(handles[2]);
        ::close(handles[1]);
    }
    auto* parent = new Connection(Physical(net::stdio::SocketAddress(input, output)), onDisconnect, 1, config);
    const bool parentActive = parent->activate();
    result.expectTrue(parentActive, "activate initiator");
    if (parentActive) {
        parent->setSocketContext(new Context(parent, true));
    }
    if (mode != "epipe") {
        auto* child = new Connection(Physical(net::stdio::SocketAddress(handles[2], handles[1])), onDisconnect, 2, config);
        const bool childActive = child->activate();
        result.expectTrue(childActive, "activate EOF-driven filter");
        if (childActive) {
            child->setSocketContext(new Context(child, false));
            char byte;
            result.expectTrue(::read(handles[2], &byte, 1) < 0 && errno == EAGAIN, "no synchronous write before start");
        }
    }
    result.expectEqual(0, writeError, "write error is delivered only through loop dispatch");
    const auto started = std::chrono::steady_clock::now();
    result.expectEqual(0, core::SNodeC::start(), "normal loop result, including EPIPE");
    const auto elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    result.expectEqual(mode == "epipe" ? 1 : 2, disconnected, "all connections released");
    result.expectTrue(mode == "epipe" ? writeError == EPIPE : reply == "alpha\nbeta\n" && reads > 2,
                      "exact fragmented/coalesced duplex bytes or EPIPE");
    result.expectTrue(elapsed < 0.25, "completion precedes the 0.3 second terminate timeout");
    if (alias >= 0) {
        result.expectTrue((::fcntl(alias, F_GETFL) & O_NONBLOCK) == 0, "shared description flags restored");
        ::close(alias);
    }
    if (reused >= 0) {
        result.expectTrue(::fcntl(reused, F_GETFD) >= 0, "teardown did not close reused number");
        ::close(reused);
    }
    return result.processResult();
}
