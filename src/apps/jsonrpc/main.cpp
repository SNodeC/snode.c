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

#include "Calculator.h"
#include "core/SNodeC.h"
// IWYU pragma: no_include "core/socket/Socket.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketConnection.hpp"
#include "core/socket/stream/SocketContextFactory.h"
#include "express/legacy/in/WebApp.h"
#include "jsonrpc/SocketContext.h"
#include "jsonrpc/client/SubProtocol.h"
#include "jsonrpc/server/SubProtocol.h"
#include "net/in/stream/legacy/SocketClient.h"
#include "net/in/stream/legacy/SocketServer.h"
#include "net/stdio/SocketAddress.h"
#include "net/stdio/stream/SocketEndpoint.h"
#include "net/stdio/stream/config/ConfigSocketEndpoint.h"
#include "net/un/stream/legacy/SocketServer.h"
#include "utils/Config.h"
#include "web/http/legacy/in/Client.h"
#include "web/websocket/SubProtocolFactory.h"
#include "web/websocket/client/SocketContextUpgradeFactory.h"
#include "web/websocket/client/SubProtocol.h"
#include "web/websocket/client/SubProtocolFactorySelector.h"
#include "web/websocket/server/SocketContextUpgradeFactory.h"
#include "web/websocket/server/SubProtocol.h"
#include "web/websocket/server/SubProtocolFactorySelector.h"

namespace jsonrpc {
    class JsonRpc;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <fcntl.h>
#include <unistd.h>
#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace {

    class Factory final : public core::socket::stream::SocketContextFactory {
    public:
        explicit Factory(bool client = false)
            : client(client) {
        }

        ~Factory() final;

    public:
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* connection) final {
            ::jsonrpc::JsonRpc* protocol =
                client ? static_cast<::jsonrpc::JsonRpc*>(new apps::jsonrpc::CalculatorClient) : new apps::jsonrpc::Calculator;
            const auto framing = utils::Config::configRoot.getOption("--framing")->as<std::string>() == "content-length"
                                     ? ::jsonrpc::SocketContext::Framing::ContentLength
                                     : ::jsonrpc::SocketContext::Framing::Newline;
            return new ::jsonrpc::SocketContext(connection, protocol, framing);
        }

    private:
        bool client;
    };

    Factory::~Factory() = default;

    class WebSocketFactory final : public web::websocket::SubProtocolFactory<web::websocket::server::SubProtocol> {
    public:
        WebSocketFactory()
            : SubProtocolFactory("jsonrpc") {
        }

        ~WebSocketFactory() final;

    private:
        web::websocket::server::SubProtocol* create(web::websocket::SubProtocolContext* context) final {
            return new ::jsonrpc::server::SubProtocol(context, "jsonrpc", new apps::jsonrpc::Calculator);
        }
    };

    WebSocketFactory::~WebSocketFactory() = default;

    class ClientWebSocketFactory final : public web::websocket::SubProtocolFactory<web::websocket::client::SubProtocol> {
    public:
        ClientWebSocketFactory()
            : SubProtocolFactory("jsonrpc") {
        }

        ~ClientWebSocketFactory() final;

    private:
        web::websocket::client::SubProtocol* create(web::websocket::SubProtocolContext* context) final {
            return new ::jsonrpc::client::SubProtocol(context, "jsonrpc", new apps::jsonrpc::CalculatorClient);
        }
    };

    ClientWebSocketFactory::~ClientWebSocketFactory() = default;

    web::websocket::SubProtocolFactory<web::websocket::client::SubProtocol>* clientWebSocketFactory() {
        return new ClientWebSocketFactory;
    }

    web::websocket::SubProtocolFactory<web::websocket::server::SubProtocol>* webSocketFactory() {
        return new WebSocketFactory;
    }

} // namespace

int main(int argc, char* argv[]) {
    const int output = ::fcntl(STDOUT_FILENO, F_DUPFD_CLOEXEC, 3);
    int result = 1;
    if (output >= 0 && ::dup2(STDERR_FILENO, STDOUT_FILENO) >= 0) {
        utils::Config::configRoot.addOption("--transport",
                                            "Calculator transport",
                                            "transport",
                                            std::string("stdio"),
                                            CLI::IsMember({"stdio", "tcp", "unix", "websocket", "client", "websocket-client"}));
        utils::Config::configRoot.addOption(
            "--framing", "Stream framing", "framing", std::string("newline"), CLI::IsMember({"newline", "content-length"}));
        utils::Config::configRoot.addOption("--port", "TCP/WebSocket port", "port", std::uint16_t(7777), CLI::Validator());
        utils::Config::configRoot.addOption("--path", "Unix socket path", "path", std::string("calculator.sock"), CLI::Validator());
        const net::in::stream::legacy::SocketServer<Factory> tcp("calculator-tcp");
        const net::un::stream::legacy::SocketServer<Factory> local("calculator-unix");
        const net::stdio::stream::SocketEndpoint<Factory> stdio("calculator-stdio");
        const net::in::stream::legacy::SocketClient<Factory, bool> client("calculator-client", true);
        const express::legacy::in::WebApp app("calculator-websocket");
        const web::http::legacy::in::Client wsClient(
            "calculator-websocket-client",
            [](const auto& request) {
                request->set("Sec-WebSocket-Protocol", "jsonrpc");
                request->upgrade(
                    "/ws",
                    "websocket",
                    [](bool success) {
                        if (!success) {
                            core::SNodeC::stop();
                        }
                    },
                    [](const auto&, const auto&, bool success) {
                        if (!success) {
                            core::SNodeC::stop();
                        }
                    },
                    [](const auto&, const std::string&) {
                        core::SNodeC::stop();
                    });
            },
            [](const auto&) {
            });
        wsClient.getConfig()->Instance::forceUnrequired();
        tcp.getConfig()->Instance::forceUnrequired();
        local.getConfig()->Instance::forceUnrequired();
        stdio.getConfig()->Instance::forceUnrequired();
        client.getConfig()->Instance::forceUnrequired();
        app.getConfig()->Instance::forceUnrequired();
        core::SNodeC::init(argc, argv);
        const auto transport = utils::Config::configRoot.getOption("--transport")->as<std::string>();
        const auto port = utils::Config::configRoot.getOption("--port")->as<std::uint16_t>();
        const auto status = [](const auto&, auto state) {
            if (state != core::socket::State::OK) {
                core::SNodeC::stop();
            }
        };
        if (transport == "stdio") {
            stdio.attach(net::stdio::SocketAddress(STDIN_FILENO, output), status);
        } else if (transport == "tcp") {
            tcp.listen(net::in::SocketAddress("127.0.0.1", port), status);
        } else if (transport == "unix") {
            local.listen(net::un::SocketAddress(utils::Config::configRoot.getOption("--path")->as<std::string>()), status);
        } else if (transport == "client") {
            client.connect(net::in::SocketAddress("127.0.0.1", port), status);
        } else if (transport == "websocket-client") {
            web::websocket::client::SocketContextUpgradeFactory::link();
            web::websocket::client::SubProtocolFactorySelector::link("jsonrpc", clientWebSocketFactory);
            wsClient.connect(net::in::SocketAddress("127.0.0.1", port), status);
        } else {
            web::websocket::server::SocketContextUpgradeFactory::link();
            web::websocket::server::SubProtocolFactorySelector::link("jsonrpc", webSocketFactory);
            app.get("/ws", [](const auto& request, const auto& response) {
                response->upgrade(request, [response](const std::string& name) {
                    if (name == "websocket") {
                        response->end();
                    } else {
                        response->sendStatus(400);
                    }
                });
            });
            app.listen(net::in::SocketAddress("127.0.0.1", port), status);
        }
        if (transport != "stdio") {
            ::close(output);
        }
        result = core::SNodeC::start();
    }
    return result;
}
