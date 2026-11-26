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
#include "Tools.h"
#include "mcp-config.h"
#include "mcp/server/Server.h"

#ifdef MCP_STDIO
#include "Stdio.h"
#include "utils/Config.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#else
#include "HttpConfig.h"
#include "Log.h"
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
        apps::mcp::registerDevelopment(server, config);
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
    auto* config = utils::Config::configRoot.newSubCommand<apps::mcp::DevelopmentConfig>();
    config->setPageSize(3);
    result = apps::mcp::runStdio(argc, argv, server, *config, [&server, config] {
        registrations(server, *config);
    });

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
                        snode::log::application("mcp.showcase").info() << "SNode.C showcase HTTPS listening on " << address.toString();
                    } else if (state != core::socket::State::DISABLED) {
                        listenFailure = 1;
                        core::SNodeC::stop();
                    }
                });
            }
            app.listen([&listenFailure](const auto& address, const auto& state) {
                if (state == core::socket::State::OK) {
                    snode::log::application("mcp.showcase").info() << "SNode.C showcase HTTP listening on " << address.toString();
                } else if (state != core::socket::State::DISABLED) {
                    listenFailure = 1;
                    snode::log::application("mcp.showcase").error() << "HTTP listener unavailable: " << state.what();
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
        snode::log::application("mcp.showcase").error() << error.what();
        core::SNodeC::free();
    }
#endif

    return result;
}
