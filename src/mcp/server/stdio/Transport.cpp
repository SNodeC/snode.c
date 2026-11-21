// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "mcp/server/stdio/Transport.h"

#include "Log.h"
#include "core/SNodeC.h"
#include "core/eventreceiver/ReadEventReceiver.h"
#include "core/eventreceiver/WriteEventReceiver.h"
#include "core/timer/Timer.h"
#include "mcp/server/Config.h"
#include "mcp/server/Types.h"
#include "mcp/server/detail/Server.h"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <fcntl.h>
#include <functional>
#include <exception>
#include <vector>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>
#include <utility>

namespace mcp::server::stdio {
    using namespace mcp::server::detail;
    using WireReply = Access::Response;
    using WireServer = Access::Registry;
    struct Transport::State : std::enable_shared_from_this<State> {
        struct Input;
        struct Output;
        Server& server;
        Config& config;
        Input* input = nullptr;
        Output* output = nullptr;
        std::string incoming;
        std::string outgoing;
        std::map<std::string, std::shared_ptr<WireReply>> pending;
        core::timer::Timer timer;
        bool inputEof = false;
        ProtocolEra era = ProtocolEra::PerRequest20260728;
        bool closing = false;
        bool failed = false;
        State(Server& server, Config& config)
            : server(server)
            , config(config) {
        }
        void send(const Json& value);
        void process();
        void finish();
        void receive(const std::string& line);
    };
    struct Transport::State::Input : core::eventreceiver::ReadEventReceiver {
        ~Input() override;
        std::shared_ptr<State> state;
        explicit Input(const std::shared_ptr<State>& state)
            : ReadEventReceiver("MCP", snode::log::Scope{.component = "mcp.server", .identity = {}}, 0)
            , state(state) {
            const int flags = fcntl(STDIN_FILENO, F_GETFL);
            if (flags < 0 || fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) < 0 || !enable(STDIN_FILENO)) {
                throw std::system_error(errno, std::generic_category(), "MCP input registration");
            }
        }
        void halt() {
            disable();
        }
        void unobservedEvent() override {
            state->input = nullptr;
            delete this;
        }
        void shutdownEvent(const core::ShutdownContext&) override {
            state->finish();
        }
        void readEvent() override {
            auto keep = state;
            char bytes[16384];
            const ssize_t count = read(STDIN_FILENO, bytes, sizeof(bytes));
            if (count > 0) {
                keep->incoming.append(bytes, static_cast<std::size_t>(count));
                if (keep->incoming.size() > keep->config.getMaximumMessageBytes() + sizeof(bytes)) {
                    keep->failed = true;
                    keep->finish();
                } else {
                    keep->process();
                }
            } else if (count == 0) {
                keep->inputEof = true;
                disable();
                keep->process();
            } else if (errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) {
                keep->finish();
            }
        }
    };
    struct Transport::State::Output : core::eventreceiver::WriteEventReceiver {
        std::shared_ptr<State> state;
        int fd;
        Output(const std::shared_ptr<State>& state, int fd)
            : WriteEventReceiver("MCP", snode::log::Scope{.component = "mcp.server", .identity = {}}, 0)
            , state(state)
            , fd(fd) {
            if (!enable(fd)) {
                close(fd);
                throw std::system_error(errno, std::generic_category(), "MCP output registration");
            }
            suspend();
        }
        ~Output() override;
        void wake() {
            if (isSuspended()) {
                resume();
            }
        }
        void halt() {
            disable();
        }
        void unobservedEvent() override {
            state->output = nullptr;
            delete this;
        }
        void shutdownEvent(const core::ShutdownContext&) override {
            state->finish();
        }
        void writeEvent() override {
            auto keep = state;
            const ssize_t count = write(fd, keep->outgoing.data(), std::min<std::size_t>(16384, keep->outgoing.size()));
            if (count > 0) {
                keep->outgoing.erase(0, static_cast<std::size_t>(count));
            } else if (count < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
                keep->failed = true;
                keep->finish();
            }
            if (isEnabled() && (keep->outgoing.empty() || keep->failed)) {
                if (keep->closing || keep->failed) {
                    disable();
                } else {
                    suspend();
                }
            }
        }
    };
    Transport::State::Input::~Input() = default;
    Transport::State::Output::~Output() {
        close(fd);
    }
    void Transport::State::send(const Json& value) {
        if (!value.is_null() && !failed && output != nullptr) {
            const std::string bytes = value.dump() + "\n";
            if (bytes.size() > config.getMaximumOutputBytes() - std::min(outgoing.size(), config.getMaximumOutputBytes())) {
                snode::log::framework("mcp.server", snode::log::Boundary::Context).error() << "stdio output limit exceeded";
                failed = true;
                finish();
            } else {
                outgoing += bytes;
                output->wake();
            }
        }
    }
    void Transport::State::finish() {
        if (!closing) {
            closing = true;
            timer.cancel();
            if (input != nullptr && !inputEof) {
                input->halt();
            }
            for (const auto& [id, reply] : pending) {
                static_cast<void>(id);
                reply->cancel(CancelReason::Shutdown);
            }
            pending.clear();
            if (output != nullptr && (outgoing.empty() || failed)) {
                output->halt();
            }
        }
    }
    void Transport::State::receive(const std::string& line) {
        std::erase_if(pending, [](const auto& item) {
            return item.second->done;
        });
        Json request;
        bool parsed = false;
        try {
            request = Json::parse(line);
            parsed = true;
        } catch (const Json::exception&) {
            WireReply(
                nullptr,
                era,
                CachePolicy::None,
                [this](const Json& value, bool) {
                    send(value);
                },
                config.getDeadline())
                .error(-32700, "Parse error");
        }
        if (parsed) {
            const bool hasMeta = request.is_object() && request.contains("params") && request["params"].is_object() &&
                                 request["params"].contains("_meta") && request["params"]["_meta"].is_object() &&
                                 (request["params"]["_meta"].contains(versionKey) || request["params"]["_meta"].contains(capabilitiesKey));
            const std::string method =
                request.is_object() && request.value("method", Json()).is_string() ? request["method"].get<std::string>() : "";
            const bool initializing = method == "initialize" && !hasMeta;
            const ProtocolEra requestEra = (era == ProtocolEra::Initialization20251125 || initializing) && !hasMeta
                                               ? ProtocolEra::Initialization20251125
                                               : ProtocolEra::PerRequest20260728;
            if (method == "notifications/cancelled" && WireServer::validRequest(request) && !request.contains("id") &&
                request.value("params", Json()).is_object()) {
                const std::string key = request["params"].value("requestId", Json()).dump();
                if (pending.contains(key)) {
                    pending[key]->cancel();
                }
            } else {
                const Json id = request.is_object() ? request.value("id", Json()) : Json();
                const std::string key = id.dump();
                if ((!id.is_null() && pending.contains(key)) || pending.size() >= 64) {
                    WireReply(
                        id,
                        requestEra,
                        CachePolicy::None,
                        [this](const Json& value, bool) {
                            send(value);
                        },
                        config.getDeadline())
                        .error(-32600, "Duplicate id or in-flight limit");
                } else {
                    const std::weak_ptr<State> weak = shared_from_this();
                    auto reply = Access::registry(server).dispatch(
                        request,
                        requestEra,
                        [weak, initializing](const Json& value, bool) {
                            if (const auto owner = weak.lock()) {
                                if (initializing && value.contains("result")) {
                                    owner->era = ProtocolEra::Initialization20251125;
                                }
                                owner->send(value);
                            }
                        },
                        config.getDeadline());
                    if (!reply->done) {
                        pending[key] = std::move(reply);
                    }
                }
            }
        }
    }
    void Transport::State::process() {
        std::size_t count = 0;
        std::size_t newline = incoming.find('\n');
        while (!closing && count < 8 && newline != std::string::npos) {
            if (newline > config.getMaximumMessageBytes()) {
                failed = true;
                finish();
            } else {
                const std::string line = incoming.substr(0, newline);
                incoming.erase(0, newline + 1);
                receive(line);
                ++count;
                newline = incoming.find('\n');
            }
        }
        if (!closing && inputEof && newline == std::string::npos) {
            if (!incoming.empty()) {
                receive("{");
                incoming.clear();
            }
            finish();
        }
        if (!closing && incoming.size() > config.getMaximumMessageBytes() && newline == std::string::npos) {
            failed = true;
            finish();
        }
    }
    int Transport::retainProtocol() {
        int result = fcntl(STDOUT_FILENO, F_DUPFD_CLOEXEC, 3);
        const int flags = result >= 0 ? fcntl(result, F_GETFL) : -1;
        if (result < 0 || flags < 0 || dup2(STDERR_FILENO, STDOUT_FILENO) < 0 || fcntl(result, F_SETFL, flags | O_NONBLOCK) < 0) {
            if (result >= 0) {
                close(result);
            }
            result = -1;
        }
        return result;
    }
    int Transport::run(int argc, char* argv[], Server& server, Config& config, Ready ready) {
        int result = 1;
        if (core::SNodeC::state() == core::State::LOADED) {
            int fd = retainProtocol();
            if (fd >= 0) {
                std::shared_ptr<State> state;
                std::vector<char*> arguments(argv, argv + argc);
                char foreground[] = "--daemonize=false";
                arguments.push_back(foreground);
                arguments.push_back(nullptr);
                try {
                    core::SNodeC::init(argc + 1, arguments.data());
                    if (ready) {
                        ready();
                    }
                    state = std::make_shared<State>(server, config);
                    state->output = new State::Output(state, std::exchange(fd, -1));
                    state->input = new State::Input(state);
                    Access::registry(server).seal();
                    const std::weak_ptr<State> weak = state;
                    state->timer = core::timer::Timer::intervalTimer(
                        [weak] {
                            if (const auto owner = weak.lock(); owner && !owner->closing) {
                                owner->process();
                                std::erase_if(owner->pending, [](const auto& item) {
                                    return item.second->done;
                                });
                            }
                        },
                        utils::Timeval(0.005));
                    result = core::SNodeC::start();
                } catch (...) {
                    try {
                        throw;
                    } catch (const std::exception& error) {
                        snode::log::application("mcp.server").error() << error.what();
                    } catch (...) {
                        snode::log::application("mcp.server").error() << "MCP stdio processing failed";
                    }
                    if (state) {
                        state->failed = true;
                        state->finish();
                    }
                    core::SNodeC::free();
                }
                if (fd >= 0) {
                    close(fd);
                }
            }
        }
        return result;
    }
} // namespace mcp::server::stdio
