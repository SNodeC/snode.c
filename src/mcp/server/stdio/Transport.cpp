// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "mcp/server/stdio/Transport.h"

#include "Log.h"
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
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
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
        bool legacy = false;
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
                if (keep->incoming.size() > keep->config.messageBytes() + sizeof(bytes)) {
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
            if (bytes.size() > config.outputBytes() - std::min(outgoing.size(), config.outputBytes())) {
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
            return !item.second->active();
        });
        Json request;
        bool parsed = false;
        try {
            request = Json::parse(line);
            parsed = true;
        } catch (const Json::exception&) {
            WireReply(
                nullptr,
                !legacy,
                false,
                [this](const Json& value, bool) {
                    send(value);
                },
                config.deadline())
                .error(-32700, "Parse error");
        }
        if (parsed) {
            const bool hasMeta = request.is_object() && request.contains("params") && request["params"].is_object() &&
                                 request["params"].contains("_meta") && request["params"]["_meta"].is_object() &&
                                 (request["params"]["_meta"].contains(versionKey) || request["params"]["_meta"].contains(capabilitiesKey));
            const std::string method =
                request.is_object() && request.value("method", Json()).is_string() ? request["method"].get<std::string>() : "";
            const bool initializing = method == "initialize" && !hasMeta;
            const bool legacyRequest = (legacy || initializing) && !hasMeta;
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
                        !legacyRequest,
                        false,
                        [this](const Json& value, bool) {
                            send(value);
                        },
                        config.deadline())
                        .error(-32600, "Duplicate id or in-flight limit");
                } else {
                    const std::weak_ptr<State> weak = shared_from_this();
                    auto reply = Access::registry(server).dispatch(
                        request,
                        legacyRequest,
                        [weak, initializing](const Json& value, bool) {
                            if (const auto owner = weak.lock()) {
                                if (initializing && value.contains("result")) {
                                    owner->legacy = true;
                                }
                                owner->send(value);
                            }
                        },
                        config.deadline());
                    if (reply->active()) {
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
            if (newline > config.messageBytes()) {
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
        if (!closing && incoming.size() > config.messageBytes() && newline == std::string::npos) {
            failed = true;
            finish();
        }
    }
    int Transport::retainProtocol() {
        const int fd = fcntl(STDOUT_FILENO, F_DUPFD_CLOEXEC, 3);
        const int flags = fd >= 0 ? fcntl(fd, F_GETFL) : -1;
        int result = fd;
        if (fd < 0 || flags < 0 || dup2(STDERR_FILENO, STDOUT_FILENO) < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            if (fd >= 0) {
                close(fd);
            }
            result = -1;
        }
        return result;
    }
    Transport::Transport(Server& server, Config& config, int protocolFd)
        : state(std::make_shared<State>(server, config)) {
        if (!Access::registry(server).valid()) {
            close(protocolFd);
            throw std::invalid_argument("Invalid MCP registration");
        }
        try {
            state->output = new State::Output(state, protocolFd);
            state->input = new State::Input(state);
        } catch (...) {
            state->failed = true;
            state->finish();
            throw;
        }
        const std::weak_ptr<State> weak = state;
        state->timer = core::timer::Timer::intervalTimer(
            [weak] {
                if (const auto owner = weak.lock(); owner && !owner->closing) {
                    owner->process();
                    std::erase_if(owner->pending, [](const auto& item) {
                        return !item.second->active();
                    });
                }
            },
            utils::Timeval(0.005));
    }
    Transport::~Transport() {
        state->finish();
    }
} // namespace mcp::server::stdio
