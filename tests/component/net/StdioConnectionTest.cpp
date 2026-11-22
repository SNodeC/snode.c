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

#include "core/SNodeC.h"
#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/stdio/SocketConnection.hpp"
#include "net/config/ConfigConnection.h"
#include "net/config/ConfigInstance.h"
#include "net/stdio/config/ConfigAddress.h"
#include "net/stdio/phy/stream/PhysicalSocket.h"
#include "support/TestResult.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <chrono>
#include <fcntl.h>
#include <string>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {

    tests::support::TestResult result;
    std::string mode;
    std::string reply;
    int disconnected = 0;
    int writeError = 0;
    int reads = 0;
    int output = -1;
    int input = -1;
    int reused = -1;

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
    char file[] = "--config-file=/dev/null";
    char quiet[] = "--quiet";
    char* arguments[] = {argv[0], file, quiet};
    core::SNodeC::init(3, arguments);
    int handles[4]{};
    int alias = -1;
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
