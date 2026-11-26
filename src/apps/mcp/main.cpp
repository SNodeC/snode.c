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

#include "Features.h"
#include "FindTool.h"
#include "HouseStyleResource.h"
#include "Log.h"
#include "ReadTool.h"
#include "ReviewLabelResource.h"
#include "ReviewLabelTool.h"
#include "ReviewPrompt.h"
#include "SkeletonPreviewTool.h"
#include "StyleTemplate.h"
#include "Tools.h"
#include "mcp-config.h"
#include "mcp/server/Server.h"

#ifdef MCP_STDIO
#include "core/SNodeC.h"
#include "core/socket/State.h"
#include "mcp/server/Config.h"
#include "mcp/server/stream/SocketContextFactory.h"
#include "net/config/ConfigConnection.h" // IWYU pragma: keep - directly used configuration base
#include "net/stdio/SocketAddress.h"
#include "net/stdio/stream/SocketEndpoint.h"
#include "net/stdio/stream/config/ConfigSocketEndpoint.h"
#include "utils/CLI11.hpp"
#include "utils/Config.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <exception>
#include <fcntl.h>
#include <functional>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#else
#include "HttpConfig.h"
#include "core/SNodeC.h"
#include "express/legacy/in/WebApp.h"
#include "express/tls/in/WebApp.h"
#include "mcp/Types.h"
#include "mcp/server/http/Router.h"
#include "net/config/ConfigConnection.h" // IWYU pragma: keep - directly used configuration base
#include "net/config/ConfigInstance.h"
#include "web/http/ConfigHttpParser.h"
#include "web/http/server/ConfigHttpServer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <exception>
#include <nlohmann/detail/json_ref.hpp>
#include <nlohmann/json.hpp>
#include <optional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#endif

namespace {

    void registrations(mcp::server::Server& server, const apps::mcp::DevelopmentConfig& config) {
        server.tool(std::make_shared<apps::mcp::FindTool>(config));
        server.tool(std::make_shared<apps::mcp::ReadTool>(config));
        server.tool(std::make_shared<apps::mcp::SkeletonPreviewTool>());
        server.resource(std::make_shared<apps::mcp::HouseStyleResource>());
        server.prompt(std::make_shared<apps::mcp::ReviewPrompt>());
        server.resourceTemplate(std::make_shared<apps::mcp::StyleTemplate>());
        const auto label = std::make_shared<apps::mcp::ReviewLabelResource>(server);
        server.resource(label);
        server.tool(std::make_shared<apps::mcp::ReviewLabelTool>(label));
        server.setTasksPerRequestor(16)->setMaximumTaskTtlMs(60000);
        apps::mcp::features::registerContent(server);
        apps::mcp::features::registerInteractions(server);
        apps::mcp::features::registerResources(server);
    }

#ifndef MCP_STDIO
    apps::mcp::HttpConfig* configureHttp(net::config::ConfigInstance* instance, net::config::ConfigConnection* connection, bool secure) {
        if (secure) {
            instance->setDisabled();
        }
        auto* config = instance->newSubCommand<apps::mcp::HttpConfig>(secure);
        auto* http = instance->getSubCommand<web::http::server::ConfigHttpServer>();
        http->setAllowPipelining(false)->setMaximumPendingRequests(1);
        http->getParserConfig()->setMaximumBodyBytes(1048576)->setMaximumHeaderBytes(65536)->setMaximumHeaderFields(100);
        connection->setMaximumWriteQueueBytes(1048576);
        return config;
    }

    template <typename App>
    void mount(const App& app, mcp::server::Server& server, apps::mcp::HttpConfig& config) {
        using namespace mcp::server::http;
        const std::string token = config.getDemoToken();
        const std::string issuer = config.getAuthorizationServer();
        RouterOptions options;
        options.replay.maximumEvents = config.getReplayEvents();
        if (!issuer.empty()) {
            options.protectedResource =
                ProtectedResource{config.getResourceUri(), {issuer}, {"showcase:read", "showcase:write"}, "MCP showcase"};
        }
        const Router router(
            server,
            config,
            [token](const std::shared_ptr<express::Request>& request) -> Authorization {
                Authorization result = Unauthorized{"Bearer realm=\"showcase\"", "showcase:read"};
                const std::string credential = request->get("Authorization");
                if (token.empty()) {
                    result = Authorized{};
                } else if (credential == "Bearer " + token) {
                    result = Authorized{mcp::Principal{"showcase-user", {{"scope", "showcase:read showcase:write"}}}};
                } else if (credential == "Bearer " + token + ":read") {
                    result = InsufficientScope{"showcase:write"};
                } else if (credential == "Bearer " + token + ":forbidden") {
                    result = Forbidden{};
                }
                return result;
            },
            options);
        router.mountMetadata(app);
        app.use("/mcp", router);
        app.get("/showcase/approve", [](const std::shared_ptr<express::Request>&, const std::shared_ptr<express::Response>& response) {
            response->send("Demonstration only. No identity, credentials or authorization state are changed.");
        });
    }
#endif

} // namespace

int main(int argc, char* argv[]) {
    int result = 1;
#ifdef MCP_STDIO
    mcp::server::Server server(apps::mcp::features::implementation(SNODEC_DEV_VERSION), apps::mcp::instructions());
    int protocolFd = ::fcntl(STDOUT_FILENO, F_DUPFD_CLOEXEC, 3);
    struct stat protocol{};
    struct stat diagnostics{};
    const bool shared = protocolFd >= 0 && ::fstat(protocolFd, &protocol) == 0 && ::fstat(STDERR_FILENO, &diagnostics) == 0 &&
                        protocol.st_dev == diagnostics.st_dev && protocol.st_ino == diagnostics.st_ino;
    if (protocolFd >= 0 && ::dup2(STDERR_FILENO, STDOUT_FILENO) >= 0) {
        auto* config = utils::Config::configRoot.newSubCommand<apps::mcp::DevelopmentConfig>();
        using Endpoint =
            net::stdio::stream::SocketEndpoint<mcp::server::stream::SocketContextFactory, mcp::server::Server&, mcp::server::Config&>;
        const Endpoint endpoint("mcp-stdio", server, *config);
        endpoint.getConfig()->Instance::forceUnrequired();
        endpoint.getConfig()->Connection::setReadTimeout(0)->setWriteTimeout(0);
        try {
            core::SNodeC::init(argc, argv);
            if (!utils::Config::configRoot.getOption("--daemonize")->as<bool>() &&
                (!shared || utils::Config::configRoot.getOption("--quiet")->as<bool>())) {
                utils::Config::configRoot.getOption("--daemonize")->check(CLI::IsMember({"false"}));
                registrations(server, *config);
                snode::log::application("mcp.development").info() << "SNode.C development stdio ready";
                endpoint.setOnConnect([&protocolFd](auto*) {
                    protocolFd = -1;
                });
                int failure = 0;
                endpoint.attach(net::stdio::SocketAddress(STDIN_FILENO, protocolFd), [&failure](const auto&, auto state) {
                    if (state != core::socket::State::OK) {
                        failure = state == core::socket::State::DISABLED ? 0 : 1;
                        core::SNodeC::stop();
                    }
                });
                const int loop = core::SNodeC::start();
                result = loop != 0 ? loop : failure;
            } else {
                core::SNodeC::free();
            }
        } catch (const std::exception& error) {
            snode::log::application("mcp.development").error() << error.what();
            core::SNodeC::free();
        } catch (...) {
            snode::log::application("mcp.development").error() << "MCP stdio startup failed";
            core::SNodeC::free();
        }
    }

    if (protocolFd >= 0) {
        ::close(protocolFd);
    }

#else
    using WebApp = express::legacy::in::WebApp;
    mcp::server::Server server(apps::mcp::features::implementation(SNODEC_DEV_VERSION), apps::mcp::instructions());
    mcp::server::Server tlsServer(apps::mcp::features::implementation(SNODEC_DEV_VERSION), apps::mcp::instructions());
    const WebApp app("mcp-http");
    const express::tls::in::WebApp tls("mcp-https");
    app.getConfig()->Local::setHost("127.0.0.1")->setPort(3337);
    tls.getConfig()->Local::setHost("127.0.0.1")->setPort(3338);
    auto* config = configureHttp(app.getConfig(), app.getConfig(), false);
    auto* tlsConfig = configureHttp(tls.getConfig(), tls.getConfig(), true);
    int listenFailure = 0;
    try {
        WebApp::init(argc, argv);
        const bool certificates = !tls.getConfig()->getCert().empty() && !tls.getConfig()->getCertKey().empty();
        if (certificates) {
            tls.getConfig()->Instance::setDisabled(false);
        }
        if (app.getConfig()->Instance::getDisabled() && (tls.getConfig()->Instance::getDisabled() || !certificates)) {
            core::SNodeC::free();
            result = 0;
        } else {
            if (!app.getConfig()->Instance::getDisabled()) {
                registrations(server, *config);
            }
            if (certificates && !tls.getConfig()->Instance::getDisabled()) {
                registrations(tlsServer, *tlsConfig);
            }
            mount(app, server, *config);
            mount(tls, tlsServer, *tlsConfig);
            if (certificates) {
                tls.listen([&listenFailure](const auto& address, const auto& state) {
                    if (state == core::socket::State::OK) {
                        snode::log::application("mcp.development").info()
                            << "SNode.C development HTTPS listening on " << address.toString();
                    } else if (state != core::socket::State::DISABLED) {
                        listenFailure = 1;
                        core::SNodeC::stop();
                    }
                });
            }
            app.listen([&listenFailure](const auto& address, const auto& state) {
                if (state == core::socket::State::OK) {
                    snode::log::application("mcp.development").info() << "SNode.C development HTTP listening on " << address.toString();
                } else if (state != core::socket::State::DISABLED) {
                    listenFailure = 1;
                    snode::log::application("mcp.development").error() << "HTTP listener unavailable: " << state.what();
                    core::SNodeC::stop();
                }
            });
            if (core::SNodeC::state() == core::State::STOPPING) {
                core::SNodeC::free();
                result = listenFailure;
            } else {
                const int loopResult = WebApp::start();
                result = listenFailure != 0 ? listenFailure : loopResult;
            }
        }
    } catch (const std::exception& error) {
        snode::log::application("mcp.development").error() << error.what();
        core::SNodeC::free();
    }
#endif

    return result;
}
