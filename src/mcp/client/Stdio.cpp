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

#include "mcp/client/Stdio.h"

#include "core/eventreceiver/ReadEventReceiver.h"
#include "core/socket/State.h"
#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "core/socket/stream/stdio/DescriptorOwnership.h"
#include "core/timer/Timer.h"
#include "jsonrpc/SocketContext.h"
#include "mcp/client/Mcp.h"
#include "net/config/ConfigInstance.h"
#include "net/stdio/SocketAddress.h"
#include "net/stdio/config/ConfigAddress.h"
#include "net/stdio/stream/SocketEndpoint.h"
#include "net/stdio/stream/config/ConfigSocketEndpoint.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <array>
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <initializer_list>
#include <optional>
#include <spawn.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <system_error>
#include <unistd.h>
#include <utility>
// These template definitions are provided once by SocketEndpoint.hpp.
// IWYU pragma: no_include "core/socket/Socket.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketConnection.hpp"
#include "log/SemanticLogger.h"

#endif
extern char** environ;

namespace mcp::client {
    namespace {
        class Factory final : public core::socket::stream::SocketContextFactory {
        public:
            explicit Factory(Stdio::Creator creator)
                : creator(std::move(creator)) {
            }

            core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* connection) final {
                return new jsonrpc::SocketContext(connection, creator());
            }

        private:
            Stdio::Creator creator;
        };

        void closeFd(int& fd) {
            if (fd >= 0) {
                ::close(std::exchange(fd, -1));
            }
        }
    } // namespace

    struct Stdio::State : std::enable_shared_from_this<State> {
        using Endpoint = net::stdio::stream::SocketEndpoint<Factory, Creator>;

        class Reaper final : public core::eventreceiver::ReadEventReceiver {
        public:
            Reaper(int fd, pid_t pid, std::shared_ptr<State> state)
                : ReadEventReceiver(
                      "mcp-child",
                      logger::LogScope{logger::LogOrigin::Framework, logger::LogBoundary::System, "mcp", "", logger::LogRole::Client, ""},
                      TIMEOUT::DISABLE)
                , state(std::move(state))
                , fd(fd)
                , pid(pid) {
                if (!enable(fd)) {
                    const int error = errno;
                    ::close(fd);
                    throw std::system_error(error, std::generic_category(), "Cannot observe MCP child");
                }
                this->state->reaper = this;
            }

            void terminate(int signal) const {
                if (pid > 0) {
                    ::kill(pid, signal);
                }
            }

        private:
            ~Reaper() final {
                ::close(fd);
                if (pid > 0) {
                    ::kill(pid, SIGKILL);
                    int status = 0;
                    pid_t result = 0;
                    do {
                        result = ::waitpid(pid, &status, 0);
                    } while (result < 0 && errno == EINTR);
                }
                if (state->reaper == this) {
                    state->reaper = nullptr;
                    state->killTimer.cancel();
                }
            }

            void destruct() final {
                delete this;
            }

            void unobservedEvent() final {
                delete this;
            }

            void shutdownEvent(const core::ShutdownContext&) final {
                state->stop();
            }

            void readEvent() final {
                int status = 0;
                pid_t result = 0;
                do {
                    result = ::waitpid(pid, &status, WNOHANG);
                } while (result < 0 && errno == EINTR);
                if (result > 0 || (result < 0 && errno == ECHILD)) {
                    pid = -1;
                    state->reaper = nullptr;
                    state->pendingExit = result > 0 ? status : -ECHILD;
                    state->killTimer.cancel();
                    if (state->connection != nullptr) {
                        state->connection->close();
                    }
                    state->notifyExit();
                    disable();
                }
            }

            std::shared_ptr<State> state;
            int fd;
            pid_t pid;
        };

        State(const std::string& name, Creator creator)
            : endpoint(name, std::move(creator)) {
            endpoint.getConfig()->Local::setDescriptorOwnership(net::stdio::config::DescriptorOwnership::TAKE);
            endpoint.getConfig()->net::config::ConfigInstance::forceUnrequired();
        }

        ~State() {
            closeFd(input);
            closeFd(output);
            killTimer.cancel();
        }

        void stop() {
            if (connection != nullptr) {
                connection->close();
            }
            if (reaper != nullptr && !stopping) {
                stopping = true;
                reaper->terminate(SIGTERM);
                killTimer = core::timer::Timer::singleshotTimer(
                    [weak = weak_from_this()]() {
                        if (const auto self = weak.lock(); self && self->reaper != nullptr) {
                            self->reaper->terminate(SIGKILL);
                        }
                    },
                    utils::Timeval{1});
            }
            stopping = true;
        }

        void notifyExit() {
            if (pendingExit && connection == nullptr && input < 0 && output < 0) {
                const int status = *pendingExit;
                pendingExit.reset();
                auto callback = std::move(onExit);
                if (callback) {
                    callback(status);
                }
            }
        }

        void spawn(std::vector<std::string> arguments, ExitHandler handler) {
            pid_t pid = -1;
            onExit = std::move(handler);
            std::array<int, 4> fds{-1, -1, -1, -1};
            int error = arguments.empty() || reaper != nullptr ? EINVAL : 0;
            if (error == 0 && (::pipe2(fds.data(), O_CLOEXEC) < 0 || ::pipe2(fds.data() + 2, O_CLOEXEC) < 0)) {
                error = errno;
            }
            for (int& fd : fds) {
                if (error == 0 && fd >= 0 && fd < 3) {
                    const int replacement = ::fcntl(fd, F_DUPFD_CLOEXEC, 3);
                    if (replacement < 0) {
                        error = errno;
                    } else {
                        closeFd(fd);
                        fd = replacement;
                    }
                }
            }
            if (error == 0) {
                posix_spawn_file_actions_t actions;
                posix_spawnattr_t attributes;
                error = ::posix_spawn_file_actions_init(&actions);
                if (error == 0) {
                    error = ::posix_spawnattr_init(&attributes);
                    if (error == 0) {
                        error = ::posix_spawn_file_actions_adddup2(&actions, fds[0], STDIN_FILENO);
                        if (error == 0) {
                            error = ::posix_spawn_file_actions_adddup2(&actions, fds[3], STDOUT_FILENO);
                        }
                        for (const int fd : fds) {
                            if (error == 0) {
                                error = ::posix_spawn_file_actions_addclose(&actions, fd);
                            }
                        }
                        sigset_t mask;
                        sigset_t defaults;
                        ::sigemptyset(&mask);
                        ::sigemptyset(&defaults);
                        for (const int signal : {SIGPIPE, SIGINT, SIGTERM, SIGHUP, SIGCHLD}) {
                            ::sigaddset(&defaults, signal);
                        }
                        if (error == 0) {
                            error = ::posix_spawnattr_setsigmask(&attributes, &mask);
                        }
                        if (error == 0) {
                            error = ::posix_spawnattr_setsigdefault(&attributes, &defaults);
                        }
                        if (error == 0) {
                            error = ::posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETSIGMASK | POSIX_SPAWN_SETSIGDEF);
                        }
                        std::vector<char*> argv;
                        for (auto& argument : arguments) {
                            argv.push_back(argument.data());
                        }
                        argv.push_back(nullptr);
                        if (error == 0) {
                            error = ::posix_spawnp(&pid, argv.front(), &actions, &attributes, argv.data(), environ);
                        }
                        ::posix_spawnattr_destroy(&attributes);
                    }
                    ::posix_spawn_file_actions_destroy(&actions);
                }
            }
            if (error == 0) {
                const int pidfd = static_cast<int>(::syscall(SYS_pidfd_open, pid, 0));
                if (pidfd < 0) {
                    error = errno;
                    ::kill(pid, SIGKILL);
                    int status = 0;
                    pid_t result = 0;
                    do {
                        result = ::waitpid(pid, &status, 0);
                    } while (result < 0 && errno == EINTR);
                    pid = -1;
                } else {
                    try {
                        new Reaper(pidfd, pid, shared_from_this());
                    } catch (const std::system_error& failure) {
                        error = failure.code().value();
                        ::kill(pid, SIGKILL);
                        int status = 0;
                        pid_t result = 0;
                        do {
                            result = ::waitpid(pid, &status, 0);
                        } while (result < 0 && errno == EINTR);
                        pid = -1;
                    }
                    if (error == 0) {
                        input = std::exchange(fds[2], -1);
                        output = std::exchange(fds[1], -1);
                        stopping = false;
                        endpoint.setOnConnect(
                            [weak = weak_from_this()](Endpoint::SocketConnection* active) {
                                if (const auto self = weak.lock()) {
                                    self->connection = active;
                                    self->input = -1;
                                    self->output = -1;
                                    if (self->stopping) {
                                        active->close();
                                    }
                                }
                            },
                            true);
                        endpoint.setOnDisconnect(
                            [weak = weak_from_this()](Endpoint::SocketConnection*) {
                                if (const auto self = weak.lock()) {
                                    self->connection = nullptr;
                                    self->stop();
                                    self->notifyExit();
                                }
                            },
                            true);
                        endpoint.attach(net::stdio::SocketAddress(input, output),
                                        [self = shared_from_this()](const auto&, const auto& status) {
                                            if (status != core::socket::State::OK) {
                                                closeFd(self->input);
                                                closeFd(self->output);
                                                self->stop();
                                            }
                                            self->notifyExit();
                                        });
                    }
                }
            }
            for (int& fd : fds) {
                closeFd(fd);
            }
            if (error != 0 && onExit) {
                onExit(-error);
            }
        }

        Endpoint endpoint;
        ExitHandler onExit;
        Reaper* reaper = nullptr;
        std::optional<int> pendingExit;
        int input = -1;
        int output = -1;
        core::socket::stream::SocketConnection* connection = nullptr;
        core::timer::Timer killTimer;
        bool stopping = false;
    };

    Stdio::Stdio(const std::string& name, Creator creator)
        : state(std::make_shared<State>(name, std::move(creator))) {
    }

    Stdio::~Stdio() {
        state->onExit = {};
        stop();
    }

    void Stdio::start(std::vector<std::string> argv, ExitHandler onExit) {
        state->stopping = false;
        core::EventReceiver::atNextTick([state = state, argv = std::move(argv), onExit = std::move(onExit)]() mutable {
            if (!state->stopping) {
                state->spawn(std::move(argv), std::move(onExit));
            }
        });
    }

    void Stdio::stop() {
        state->stop();
    }
} // namespace mcp::client
