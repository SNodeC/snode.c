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

#include "mcp/server/http/Router.h"

#include "Log.h"
#include "core/pipe/Pipe.h"
#include "core/pipe/PipeSink.h"
#include "core/pipe/PipeSource.h"
#include "core/pipe/Source.h"
#include "core/timer/Timer.h"
#include "express/middleware/JsonMiddleware.h"
#include "mcp/server/Config.h"
#include "mcp/server/Types.h"
#include "mcp/server/detail/Server.h"
#include "utils/base64.h"
#include "web/http/http_utils.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstddef>
#include <nlohmann/detail/json_ref.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <sys/random.h>
#include <sys/types.h>
#include <utility>
#include <variant>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {
    struct ShutdownContext;
}

namespace mcp::server::http {

    using namespace mcp::server::detail;
    using WireReply = Access::Response;
    using WireServer = Access::Registry;

    namespace {

        enum class StreamMode { Json, Sse };

        std::string quoted(const std::string& value) {
            std::string result = "\"";
            for (const char byte : value) {
                const auto c = static_cast<unsigned char>(byte);
                if (c < 32 || c > 126) {
                    throw std::invalid_argument("Authentication parameter is not header-safe");
                }

                if (c == '"' || c == '\\') {
                    result += '\\';
                }

                result += static_cast<char>(c);
            }

            result += '"';

            return result;
        }

        std::string metadataUrl(const ProtectedResource& resource) {
            static const std::regex shape(R"((https?://[^/?#@\s]+)(/[^?#\s]*)?)");
            std::smatch parts;
            if (!std::regex_match(resource.resource, parts, shape) || resource.authorizationServers.empty()) {
                throw std::invalid_argument("Protected resource requires an absolute canonical URI and authorization server");
            }

            for (const auto& issuer : resource.authorizationServers) {
                if (!issuer.starts_with("https://") || issuer.find_first_of("\r\n\"#") != std::string::npos) {
                    throw std::invalid_argument("Authorization server must be an HTTPS issuer URI");
                }
            }

            return parts[1].str() + "/.well-known/oauth-protected-resource" + parts[2].str();
        }

        std::string
        challenge(const ProtectedResource& resource, const std::optional<std::string>& scope, const std::string& base = "Bearer") {
            std::string result = base + (base == "Bearer" ? " " : ", ") + "resource_metadata=" + quoted(metadataUrl(resource));
            if (scope) {
                result += ", scope=" + quoted(*scope);
            }

            return result;
        }

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

        void
        fail(const std::shared_ptr<express::Response>& response, const Json& request, ProtocolEra era, int code, const std::string& text) {
            const Json id = request.is_object() ? request.value("id", Json()) : Json();
            WireReply(
                id.is_string() || id.is_number_integer() ? id : Json(),
                era,
                CachePolicy::None,
                [response](const Json& value, bool) {
                    response->status(WireServer::httpStatus(value)).json(value);
                },
                utils::Timeval(30))
                .error(code, text);
        }

        struct Operation
            : core::pipe::Source
            , std::enable_shared_from_this<Operation> {
            Server& server;
            Config& config;
            Json request;
            std::optional<Principal> principal;
            std::shared_ptr<express::Response> response;
            std::shared_ptr<Operation> owner;
            std::shared_ptr<WireReply> reply;
            std::shared_ptr<Channel> channel;
            core::timer::Timer timer;
            std::string queue;
            ProtocolEra era;
            StreamMode mode;
            bool paused = false;
            bool ended = false;
            bool closed = false;

            Operation(Server& server,
                      Config& config,
                      Json request,
                      std::shared_ptr<express::Response> response,
                      ProtocolEra era,
                      StreamMode mode,
                      std::optional<Principal> principal)
                : server(server)
                , config(config)
                , request(std::move(request))
                , principal(std::move(principal))
                , response(std::move(response))
                , era(era)
                , mode(mode) {
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
                        reply->cancel(CancelReason::Disconnected);
                    }

                    if (request.is_null() && channel) {
                        channel->active = false;
                        channel->resources.clear();
                        channel->output = {};
                    }

                    owner.reset();
                }
            }

            void feed(const Json& value, bool terminal) {
                if (!closed) {
                    ended = terminal;
                    if (mode == StreamMode::Sse) {
                        if (!value.is_null()) {
                            queue += "event: message\ndata: " + value.dump() + "\n\n";
                        }

                        if (queue.size() > config.getMaximumOutputBytes()) {
                            snode::log::framework("mcp.server", snode::log::Boundary::Context).error() << "HTTP output limit exceeded";
                            error(EMSGSIZE);
                        }
                    } else if (terminal) {
                        if (!value.is_null() && response->isConnected()) {
                            const std::string bytes = value.dump();
                            if (bytes.size() > config.getMaximumOutputBytes()) {
                                fail(response, request, era, -32603, "Output limit exceeded");
                            } else {
                                response->status(WireServer::httpStatus(value)).type("application/json").send(bytes);
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
                if (!closed && (reply || request.is_null())) {
                    if (reply) {
                        reply->cancel(CancelReason::Shutdown);
                    } else {
                        ended = true;
                    }
                    if (mode == StreamMode::Sse) {
                        pump();
                    } else {
                        stop();
                    }
                }
            }

            void start() override {
                const std::weak_ptr<Operation> weak = shared_from_this();
                if (request.is_null()) {
                    channel->output = [weak](const Json& value) {
                        if (const auto current = weak.lock()) {
                            if (value.is_null()) {
                                current->ended = true;
                            } else {
                                current->feed(value, false);
                            }
                        }
                    };
                    queue = ": connected\n\n";
                } else {
                    reply = Access::registry(server).dispatch(
                        request,
                        era,
                        [weak](const Json& value, bool terminal) {
                            if (const auto current = weak.lock()) {
                                current->feed(value, terminal);
                            }
                        },
                        config.getDeadline(),
                        std::move(principal),
                        channel);
                }
                if (mode == StreamMode::Sse) {
                    pump();
                }

                if (!closed) {
                    timer = core::timer::Timer::intervalTimer(
                        [weak] {
                            if (const auto current = weak.lock(); current && !current->closed) {
                                if (!current->response->isConnected()) {
                                    current->stop();
                                }

                                if (current->mode == StreamMode::Sse) {
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
            RouterOptions options;

            struct Session {
                std::shared_ptr<Channel> channel;
                utils::Timeval touched;
            };

            std::map<std::string, Session> sessions;

            State(Server& server, Config& config, RouterOptions options)
                : server(server)
                , config(config)
                , options(std::move(options)) {
            }

            std::shared_ptr<Channel> findSession(const std::shared_ptr<express::Request>& req) {
                const auto now = utils::Timeval::currentTime();
                std::erase_if(sessions, [&now](const auto& item) {
                    return !item.second.channel->active || item.second.touched + utils::Timeval(1800) < now;
                });
                std::shared_ptr<Channel> result;
                const auto found = sessions.find(req->get("Mcp-Session-Id"));
                if (found != sessions.end() && found->second.channel->credential == req->get("Authorization")) {
                    found->second.touched = now;
                    result = found->second.channel;
                }

                return result;
            }

            std::shared_ptr<Channel> newSession(const std::shared_ptr<express::Request>& req,
                                                const std::shared_ptr<express::Response>& res) {
                static_cast<void>(findSession(req));
                std::shared_ptr<Channel> result;
                std::array<unsigned char, 24> bytes{};
                if (sessions.size() < 64 && ::getrandom(bytes.data(), bytes.size(), 0) == static_cast<ssize_t>(bytes.size())) {
                    std::string token;
                    for (const auto byte : bytes) {
                        token += "0123456789abcdef"[byte >> 4U];
                        token += "0123456789abcdef"[byte & 15U];
                    }

                    result = std::make_shared<Channel>();
                    result->credential = req->get("Authorization");
                    sessions.emplace(token, Session{result, utils::Timeval::currentTime()});
                    res->set("Mcp-Session-Id", token);
                }

                return result;
            }

            void legacy(const std::shared_ptr<express::Request>& req, const std::shared_ptr<express::Response>& res) {
                const auto channel = findSession(req);
                if (!channel) {
                    res->status(req->get("Mcp-Session-Id").empty() ? 400 : 404).end();
                } else if (req->method == "DELETE") {
                    if (channel->output) {
                        channel->output(nullptr);
                    }

                    channel->active = false;
                    channel->resources.clear();
                    res->status(204).end();
                } else if (!accepts(lower(req->get("Accept")), "text/event-stream")) {
                    res->status(406).end();
                } else if (channel->output || stopping) {
                    res->status(409).end();
                } else {
                    auto operation = std::make_shared<Operation>(
                        server, config, nullptr, res, ProtocolEra::Initialization20251125, StreamMode::Sse, std::nullopt);
                    operation->owner = operation;
                    operation->channel = channel;
                    operations.push_back(operation);
                    res->status(200).set("Content-Type", "text/event-stream").set("Cache-Control", "no-cache");
                    if (!res->pipe(operation.get())) {
                        operation->stop();
                    }
                }
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

    Router::Router(Server& server, Config& config, Authorize authorize, RouterOptions options)
        : protectedResource(options.protectedResource) {
        if (protectedResource) {
            static_cast<void>(metadataUrl(*protectedResource));
        }

        auto state = std::make_shared<State>(server, config, std::move(options));
        state->observe();
        Access::registry(server).seal();
        use("/", [state, authorize](const auto& req, const auto& res, auto& next) {
            const std::string origin = req->get("Origin");
            const auto& allowed = state->config.getAllowedOrigins();
            std::string contentType = httputils::str_split(lower(req->get("Content-Type")), ';').first;
            httputils::str_trimm(contentType);
            static const std::regex originShape(R"(https?://([A-Za-z0-9.-]+|\[[0-9A-Fa-f:]+\])(:[0-9]{1,5})?)");
            if (req->headers.contains("Origin") && (origin.size() > 256 || !std::regex_match(origin, originShape) ||
                                                    std::find(allowed.begin(), allowed.end(), origin) == allowed.end())) {
                res->status(403).end();
            } else {
                std::optional<Authorization> authorization;
                try {
                    authorization = authorize ? authorize(req) : Authorized{};
                    if (const auto* denied = std::get_if<Unauthorized>(&*authorization);
                        denied && ((!state->options.protectedResource && denied->wwwAuthenticate.empty()) ||
                                   !std::all_of(denied->wwwAuthenticate.begin(), denied->wwwAuthenticate.end(), [](unsigned char c) {
                                       return c >= 32 && c <= 126;
                                   }))) {
                        throw std::invalid_argument("Invalid authentication challenge");
                    }

                    if (const auto* denied = std::get_if<Unauthorized>(&*authorization); denied && denied->scope) {
                        static_cast<void>(quoted(*denied->scope));
                    }

                    if (const auto* denied = std::get_if<InsufficientScope>(&*authorization)) {
                        static_cast<void>(quoted(denied->scope));
                    }
                } catch (...) {
                    authorization.reset();
                    snode::log::framework("mcp.server", snode::log::Boundary::Application).error() << "HTTP authorization callback failed";
                    res->status(500).end();
                }
                if (authorization) {
                    if (const auto* denied = std::get_if<Unauthorized>(&*authorization)) {
                        std::string header = denied->wwwAuthenticate;
                        if (state->options.protectedResource) {
                            header = challenge(
                                *state->options.protectedResource, denied->scope, header.starts_with("Bearer") ? header : "Bearer");
                        }

                        res->status(401).set("WWW-Authenticate", header).end();
                    } else if (const auto* denied = std::get_if<InsufficientScope>(&*authorization)) {
                        std::string header = "Bearer error=\"insufficient_scope\", scope=" + quoted(denied->scope);
                        if (state->options.protectedResource) {
                            header += ", resource_metadata=" + quoted(metadataUrl(*state->options.protectedResource));
                        }

                        res->status(403).set("WWW-Authenticate", header).end();
                    } else if (std::holds_alternative<Forbidden>(*authorization)) {
                        res->status(403).end();
                    } else if (req->method != "POST") {
                        if (state->options.legacySubscriptions == LegacySubscriptions::Enabled &&
                            req->get("MCP-Protocol-Version") == protocolVersion2025 && (req->method == "GET" || req->method == "DELETE")) {
                            state->legacy(req, res);
                        } else {
                            res->status(405).set("Allow", "POST").end();
                        }
                    } else if (contentType != "application/json") {
                        res->status(415).end();
                    } else if (req->body.size() > state->config.getMaximumMessageBytes()) {
                        res->status(413).end();
                    } else {
                        req->template setAttribute<Authorized>(std::get<Authorized>(*authorization));
                        next();
                    }
                }
            }
        });
        use("/", express::middleware::JsonMiddleware());
        post("/", [state](const auto& req, const auto& res) {
            const ProtocolEra headerEra = req->get("MCP-Protocol-Version") == protocolVersion2025 ? ProtocolEra::Initialization20251125
                                                                                                  : ProtocolEra::PerRequest20260728;
            if (!req->template hasAttribute<Json>()) {
                fail(res, nullptr, headerEra, -32700, "Parse error");
            } else {
                Json request;
                req->template getAttribute<Json>([&request](const Json& body) {
                    request = body;
                });
                if (!WireServer::validRequest(request)) {
                    Access::registry(state->server).dispatch(request, headerEra, [res](const Json& value, bool) {
                        res->status(WireServer::httpStatus(value)).json(value);
                    });
                } else {
                    const Json params =
                        request.is_object() && request.value("params", Json()).is_object() ? request["params"] : Json::object();
                    const Json meta = params.value("_meta", Json::object());
                    const bool modernMeta = meta.is_object() && (meta.contains(versionKey) || meta.contains(capabilitiesKey));
                    const Json method = request.is_object() ? request.value("method", Json()) : Json();
                    const ProtocolEra era = !modernMeta && (headerEra == ProtocolEra::Initialization20251125 ||
                                                            (req->get("MCP-Protocol-Version").empty() && method == "initialize"))
                                                ? ProtocolEra::Initialization20251125
                                                : ProtocolEra::PerRequest20260728;
                    const std::string accept = lower(req->get("Accept"));
                    const Json version = meta.is_object() ? meta.value(versionKey, Json()) : Json();
                    const bool needsName = method == "tools/call" || method == "prompts/get" || method == "resources/read";
                    const Json name = params.value(method == "resources/read" ? "uri" : "name", Json());
                    const bool notification = request.is_object() && !request.contains("id");
                    if (!accepts(accept, "application/json") || !accepts(accept, "text/event-stream")) {
                        res->status(406).end();
                    } else if (era == ProtocolEra::PerRequest20260728 && !notification &&
                               (req->get("MCP-Protocol-Version").empty() ||
                                (version.is_string() && !headerMatches(req->get("MCP-Protocol-Version"), version, false)) ||
                                !headerMatches(req->get("Mcp-Method"), method, false) ||
                                (needsName && !headerMatches(req->get("Mcp-Name"), name, true)))) {
                        fail(res, request, ProtocolEra::PerRequest20260728, -32020, "Header mismatch");
                    } else if (notification) {
                        res->status(202).end();
                    } else {
                        std::erase_if(state->operations, [](const auto& weak) {
                            return weak.expired();
                        });
                        if (state->stopping || state->operations.size() >= 64) {
                            res->status(429).end();
                        } else {
                            const StreamMode mode =
                                era == ProtocolEra::PerRequest20260728 &&
                                        (method == "subscriptions/listen" || (method == "tools/call" && req->httpMinor >= 1))
                                    ? StreamMode::Sse
                                    : StreamMode::Json;
                            std::optional<Principal> principal;
                            req->template getAttribute<Authorized>([&principal](const Authorized& authorization) {
                                principal = authorization.principal;
                            });
                            auto operation =
                                std::make_shared<Operation>(state->server, state->config, request, res, era, mode, std::move(principal));
                            if (era == ProtocolEra::Initialization20251125 &&
                                state->options.legacySubscriptions == LegacySubscriptions::Enabled) {
                                operation->channel = method == "initialize" && params.contains("capabilities") &&
                                                             params.contains("clientInfo") && params.contains("protocolVersion")
                                                         ? state->newSession(req, res)
                                                         : state->findSession(req);
                            }

                            operation->owner = operation;
                            state->operations.push_back(operation);
                            if (mode == StreamMode::Sse) {
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

    const Router& Router::mountMetadata(const express::Router& root) const {
        if (protectedResource) {
            const auto& resource = *protectedResource;
            const std::string url = metadataUrl(resource);
            const auto start = url.find('/', url.find("://") + 3);
            const std::string path = url.substr(start);
            Json metadata{{"resource", resource.resource},
                          {"authorization_servers", resource.authorizationServers},
                          {"bearer_methods_supported", {"header"}}};
            if (!resource.scopesSupported.empty()) {
                metadata["scopes_supported"] = resource.scopesSupported;
            }

            if (resource.resourceName) {
                metadata["resource_name"] = *resource.resourceName;
            }

            if (resource.documentation) {
                metadata["resource_documentation"] = *resource.documentation;
            }

            const auto respond = [metadata](const auto&, const auto& response) {
                response->json(metadata);
            };
            root.get(path, respond);
            if (path != "/.well-known/oauth-protected-resource") {
                root.get("/.well-known/oauth-protected-resource", respond);
            }
        }

        return *this;
    }

} // namespace mcp::server::http
