// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "mcp/server/http/Router.h"

#include "Log.h"
#include "core/pipe/Pipe.h"
#include "core/pipe/PipeSink.h"
#include "core/pipe/PipeSource.h"
#include "core/pipe/Source.h"
#include "core/timer/Timer.h"
#include "express/middleware/JsonMiddleware.h"
#include "mcp/server/Config.h"
#include "mcp/server/Server.h"
#include "utils/base64.h"
#include "web/http/http_utils.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <nlohmann/json.hpp>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace core {
    struct ShutdownContext;
}

namespace mcp::server::http {
    namespace {
        std::string lower(std::string value) {
            std::transform(value.begin(), value.end(), value.begin(), [](char c) {
                return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            });
            return value;
        }
        bool accepts(const std::string& header, const std::string& type) {
            const auto tokens = httputils::splitCommaSeparatedTokens(header);
            return std::any_of(tokens.begin(), tokens.end(), [&type](std::string token) {
                token = httputils::str_split(token, ';').first;
                return httputils::tokenEquals(httputils::str_trimm(token), type);
            });
        }
        bool headerMatches(const std::string& header, const Json& value, bool encoded) {
            std::string decoded = header;
            bool valid = !header.empty() && std::all_of(header.begin(), header.end(), [](char c) {
                const auto byte = static_cast<unsigned char>(c);
                return byte == 9 || (byte >= 32 && byte <= 126);
            });
            if (valid && encoded && header.starts_with("=?base64?") && header.ends_with("?=")) {
                const std::string payload = header.substr(9, header.size() - 11);
                decoded = base64::base64_decode(payload);
                valid = base64::base64_encode(reinterpret_cast<const unsigned char*>(decoded.data()), decoded.size()) == payload;
            }
            return valid && value.is_string() && decoded == value.get_ref<const std::string&>();
        }
        void fail(const std::shared_ptr<express::Response>& response, const Json& request, bool legacy, int code, const std::string& text) {
            const Json id = request.is_object() ? request.value("id", Json()) : Json();
            Reply(
                id.is_string() || id.is_number_integer() ? id : Json(),
                !legacy,
                false,
                [response](const Json& value, bool) {
                    response->status(Server::httpStatus(value)).json(value);
                },
                std::chrono::seconds(30))
                .error(code, text);
        }
        struct Operation
            : core::pipe::Source
            , std::enable_shared_from_this<Operation> {
            Server& server;
            Config& config;
            Json request;
            std::shared_ptr<express::Response> response;
            std::shared_ptr<Operation> owner;
            std::shared_ptr<Reply> reply;
            core::timer::Timer timer;
            std::string queue;
            bool legacy;
            bool streaming;
            bool paused = false;
            bool ended = false;
            bool closed = false;
            Operation(
                Server& server, Config& config, Json request, std::shared_ptr<express::Response> response, bool legacy, bool streaming)
                : server(server)
                , config(config)
                , request(std::move(request))
                , response(std::move(response))
                , legacy(legacy)
                , streaming(streaming) {
            }
            ~Operation() override;
            bool isOpen() override {
                return !closed;
            }
            void suspend() override {
                paused = true;
            }
            void resume() override {
                paused = false;
                pump();
            }
            void stop() override {
                auto keep = shared_from_this();
                if (!closed) {
                    closed = true;
                    timer.cancel();
                    if (reply) {
                        reply->cancel();
                    }
                    owner.reset();
                }
            }
            void feed(const Json& value, bool terminal) {
                if (!closed) {
                    ended = terminal;
                    if (streaming) {
                        if (!value.is_null()) {
                            queue += "event: message\ndata: " + value.dump() + "\n\n";
                        }
                        if (queue.size() > config.outputBytes()) {
                            snode::log::framework("mcp.server", snode::log::Boundary::Context).error() << "HTTP output limit exceeded";
                            error(EMSGSIZE);
                        }
                    } else if (terminal) {
                        if (!value.is_null() && response->isConnected()) {
                            const std::string bytes = value.dump();
                            if (bytes.size() > config.outputBytes()) {
                                fail(response, request, legacy, -32603, "Output limit exceeded");
                            } else {
                                response->status(Server::httpStatus(value)).type("application/json").send(bytes);
                            }
                        }
                        stop();
                    }
                }
            }
            void pump() {
                auto keep = shared_from_this();
                std::size_t chunks = 0;
                while (!closed && !paused && !queue.empty() && chunks < 4) {
                    const std::string chunk = queue.substr(0, 16384);
                    queue.erase(0, chunk.size());
                    static_cast<void>(send(chunk.data(), chunk.size()));
                    ++chunks;
                }
                if (!closed && ended && queue.empty()) {
                    eof();
                }
            }
            void shutdown() {
                if (!closed && reply) {
                    reply->cancel(true);
                    if (streaming) {
                        pump();
                    } else {
                        stop();
                    }
                }
            }
            void start() override {
                const std::weak_ptr<Operation> weak = shared_from_this();
                reply = server.dispatch(
                    request,
                    legacy,
                    [weak](const Json& value, bool terminal) {
                        if (const auto current = weak.lock()) {
                            current->feed(value, terminal);
                        }
                    },
                    config.deadline());
                if (streaming) {
                    pump();
                }
                if (!closed) {
                    timer = core::timer::Timer::intervalTimer(
                        [weak] {
                            if (const auto current = weak.lock(); current && !current->closed) {
                                if (!current->legacy && !current->response->isConnected()) {
                                    current->stop();
                                } else {
                                    current->reply->expire();
                                }
                                if (current->streaming) {
                                    current->pump();
                                }
                            }
                        },
                        utils::Timeval(0.005));
                }
            }
        };
        Operation::~Operation() = default;
        struct State : std::enable_shared_from_this<State> {
            Server& server;
            Config& config;
            core::pipe::Pipe lifecycle{"mcp.http"};
            core::pipe::PipeSink* reader = nullptr;
            core::pipe::PipeSource* writer = nullptr;
            std::vector<std::weak_ptr<Operation>> operations;
            bool stopping = false;
            State(Server& server, Config& config)
                : server(server)
                , config(config) {
            }
            ~State() {
                if (reader) {
                    reader->close();
                }
                if (writer) {
                    writer->close();
                }
            }
            void shutdown() {
                if (!stopping) {
                    stopping = true;
                    for (const auto& weak : operations) {
                        if (const auto operation = weak.lock()) {
                            operation->shutdown();
                        }
                    }
                    if (reader) {
                        reader->close();
                    }
                    if (writer) {
                        writer->close();
                    }
                }
            }
            void observe() {
                const std::weak_ptr<State> weak = shared_from_this();
                reader = lifecycle.releaseReadAsSink(1, utils::Timeval(0));
                writer = lifecycle.releaseWriteAsSource(1, utils::Timeval(0));
                if (!reader || !writer) {
                    throw std::runtime_error("MCP HTTP lifecycle registration failed");
                }
                reader->setOnClosed([weak] {
                    if (const auto state = weak.lock()) {
                        state->reader = nullptr;
                    }
                });
                writer->setOnClosed([weak] {
                    if (const auto state = weak.lock()) {
                        state->writer = nullptr;
                    }
                });
                reader->setOnShutdown([weak](const core::ShutdownContext&) {
                    if (const auto state = weak.lock()) {
                        state->shutdown();
                    }
                });
                writer->setOnShutdown([weak](const core::ShutdownContext&) {
                    if (const auto state = weak.lock()) {
                        state->shutdown();
                    }
                });
            }
        };
    } // namespace
    Router::Router(Server& server, Config& config, Authorize authorize) {
        if (!server.valid()) {
            throw std::invalid_argument("Invalid MCP registration");
        }
        auto state = std::make_shared<State>(server, config);
        state->observe();
        use("/", [state, authorize](const auto& req, const auto& res, auto& next) {
            const std::string origin = req->get("Origin");
            const auto allowed = state->config.origins();
            std::string contentType = httputils::str_split(lower(req->get("Content-Type")), ';').first;
            httputils::str_trimm(contentType);
            static const std::regex originShape(R"(https?://([A-Za-z0-9.-]+|\[[0-9A-Fa-f:]+\])(:[0-9]{1,5})?)");
            if (req->headers.contains("Origin") && (origin.size() > 256 || !std::regex_match(origin, originShape) ||
                                                    std::find(allowed.begin(), allowed.end(), origin) == allowed.end())) {
                res->status(403).end();
            } else if (authorize && !authorize(req)) {
                res->status(401).set("WWW-Authenticate", "Bearer").end();
            } else if (req->method != "POST") {
                res->status(405).set("Allow", "POST").end();
            } else if (contentType != "application/json") {
                res->status(415).end();
            } else if (req->body.size() > state->config.messageBytes()) {
                res->status(413).end();
            } else {
                next();
            }
        });
        use("/", express::middleware::JsonMiddleware());
        post("/", [state](const auto& req, const auto& res) {
            const bool headerLegacy = req->get("MCP-Protocol-Version") == legacyVersion;
            if (!req->template hasAttribute<Json>()) {
                fail(res, nullptr, headerLegacy, -32700, "Parse error");
            } else {
                Json request;
                req->template getAttribute<Json>([&request](const Json& body) {
                    request = body;
                });
                if (!Server::validRequest(request)) {
                    state->server.dispatch(request, headerLegacy, [res](const Json& value, bool) {
                        res->status(Server::httpStatus(value)).json(value);
                    });
                } else {
                    const Json params =
                        request.is_object() && request.value("params", Json()).is_object() ? request["params"] : Json::object();
                    const Json meta = params.value("_meta", Json::object());
                    const bool modernMeta = meta.is_object() && (meta.contains(versionKey) || meta.contains(capabilitiesKey));
                    const Json method = request.is_object() ? request.value("method", Json()) : Json();
                    const bool legacy =
                        !modernMeta && (headerLegacy || (req->get("MCP-Protocol-Version").empty() && method == "initialize"));
                    const std::string accept = lower(req->get("Accept"));
                    const Json version = meta.is_object() ? meta.value(versionKey, Json()) : Json();
                    const bool needsName = method == "tools/call" || method == "prompts/get" || method == "resources/read";
                    const Json name = params.value(method == "resources/read" ? "uri" : "name", Json());
                    const bool notification = request.is_object() && !request.contains("id");
                    if (!accepts(accept, "application/json") || !accepts(accept, "text/event-stream")) {
                        res->status(406).end();
                    } else if (!legacy && !notification &&
                               (req->get("MCP-Protocol-Version").empty() ||
                                (version.is_string() && !headerMatches(req->get("MCP-Protocol-Version"), version, false)) ||
                                !headerMatches(req->get("Mcp-Method"), method, false) ||
                                (needsName && !headerMatches(req->get("Mcp-Name"), name, true)))) {
                        fail(res, request, false, -32020, "Header mismatch");
                    } else if (notification) {
                        res->status(202).end();
                    } else {
                        std::erase_if(state->operations, [](const auto& weak) {
                            return weak.expired();
                        });
                        if (state->stopping || state->operations.size() >= 64) {
                            res->status(429).end();
                        } else {
                            const bool streaming =
                                !legacy && (method == "subscriptions/listen" || (method == "tools/call" && req->httpMinor >= 1));
                            auto operation = std::make_shared<Operation>(state->server, state->config, request, res, legacy, streaming);
                            operation->owner = operation;
                            state->operations.push_back(operation);
                            if (streaming) {
                                res->status(200)
                                    .set("Content-Type", "text/event-stream")
                                    .set("Cache-Control", "no-cache")
                                    .set("X-Accel-Buffering", "no");
                                if (!res->pipe(operation.get())) {
                                    operation->stop();
                                }
                            } else {
                                operation->start();
                            }
                        }
                    }
                }
            }
        });
    }
} // namespace mcp::server::http
