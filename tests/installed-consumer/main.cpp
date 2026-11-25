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

#include <core/SNodeC.h>
#include <core/socket/stream/SocketClient.h>
#include <core/socket/stream/SocketContext.h>
#include <core/socket/stream/SocketContextFactory.h>
#include <core/socket/stream/SocketServer.h>
#include <express/legacy/in/Server.h>
#include <jsonrpc/JsonRpc.h>
#include <jsonrpc/JsonRpcContext.h>
#include <jsonrpc/SocketContext.h>
#include <jsonrpc/client/SubProtocol.h>
#include <jsonrpc/server/SubProtocol.h>
#include <mcp/Protocol.h>
#include <mcp/Results.h>
#include <mcp/Schema.h>
#include <mcp/client/Mcp.h>
#include <mcp/client/Stdio.h>
#include <mcp/client/http/Client.h>
#include <mcp/server/Config.h>
#include <mcp/server/Request.h>
#include <mcp/server/ResourceTemplate.h>
#include <mcp/server/Server.h>
#include <mcp/server/Tool.h>
#include <mcp/server/http/Router.h>
#include <mcp/server/stream/SocketContext.h>
#include <mcp/server/stream/SocketContextFactory.h>
#include <net/in/stream/legacy/SocketClient.h>
#include <net/in/stream/legacy/SocketServer.h>
#include <net/stdio/stream/SocketEndpoint.h>
#include <net/un/stream/legacy/SocketClient.h>
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <sys/socket.h>
#include <type_traits>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include <utils/Config.h>

namespace {

    class InstalledRpcContext final : public jsonrpc::JsonRpcContext {
    public:
        InstalledRpcContext()
            : JsonRpcContext(new jsonrpc::JsonRpc("installed")) {
        }

        ~InstalledRpcContext() final;

        void sendMessage(const std::string& message) final {
            output = message;
        }

        void end() final {
        }

        void close() final {
        }

        core::socket::stream::SocketConnection* getSocketConnection() const final {
            return nullptr;
        }

        std::string output;
    };

    InstalledRpcContext::~InstalledRpcContext() = default;

    class InstalledContext final : public core::socket::stream::SocketContext {
    public:
        explicit InstalledContext(core::socket::stream::SocketConnection* connection)
            : SocketContext(connection) {
        }

    private:
        void onConnected() override {
            close();
        }

        void onDisconnected() override {
        }

        std::size_t onReceivedFromPeer() override {
            char value;

            return readFromPeer(&value, 1);
        }

        bool onSignal([[maybe_unused]] int signal) override {
            return true;
        }
    };

    class InstalledFactory final : public core::socket::stream::SocketContextFactory {
    public:
        ~InstalledFactory() override;

        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* connection) override {
            return new InstalledContext(connection);
        }
    };

    InstalledFactory::~InstalledFactory() = default;

    class InstalledTemplate final : public mcp::server::ResourceTemplate {
    public:
        InstalledTemplate()
            : ResourceTemplate(
                  {"installed://{name}", "installed-template", {}, "text/plain", {}, {{"name", [](const auto&, const auto& reply) {
                                                                                           reply->result(mcp::server::CompletionResult{
                                                                                               {"example"}, 1, false});
                                                                                       }}}}) {
        }

        void read(const mcp::server::ResourceRead& request, const std::shared_ptr<mcp::server::ResourceReply>& reply) override {
            reply->result(
                mcp::server::ResourceResult{{mcp::server::TextResource{request.uri, "text/plain", request.variables.at("name")}}});
        }
    };

    class InstalledTool final : public mcp::server::Tool {
    public:
        InstalledTool();

        void call(const mcp::server::ToolCall&, const std::shared_ptr<mcp::server::ToolReply>& reply) override;
    };

    InstalledTool::InstalledTool()
        : Tool({"installed", {}, nlohmann::ordered_json::parse(R"json(
{
    "type": "object"
}
)json")}) {
    }

    void InstalledTool::call(const mcp::server::ToolCall&, const std::shared_ptr<mcp::server::ToolReply>& reply) {
        reply->result(mcp::server::ToolResult::text("installed consumer"));
    }

} // namespace

static_assert(std::is_base_of_v<core::socket::stream::SocketContext, jsonrpc::SocketContext>);
static_assert(std::is_base_of_v<web::websocket::server::SubProtocol, jsonrpc::server::SubProtocol>);
static_assert(std::is_base_of_v<web::websocket::client::SubProtocol, jsonrpc::client::SubProtocol>);

int main([[maybe_unused]] int argc, char* argv[]) {
    mcp::client::Stdio child("installed-child", []() {
        return new mcp::client::Mcp();
    });
    mcp::client::http::Http http("installed-http-client", new mcp::client::Mcp());
    mcp::client::Mcp client;
    const bool mcpClientReady = client.getProtocolEra() == mcp::ProtocolEra::PerRequest20260728;
    InstalledRpcContext rpc;
    rpc.onConnected();
    rpc.onMessage(R"({"jsonrpc":"2.0","id":"consumer","method":"missing"})");
    const bool jsonrpcReady =
        nlohmann::json::parse(rpc.output) ==
        nlohmann::json::parse(R"({"jsonrpc":"2.0","id":"consumer","error":{"code":-32601,"message":"Method not found"}})");
    rpc.onDisconnected();
    int result = 1;
    int pair[2]{};
    static_assert(std::is_base_of_v<core::socket::stream::SocketContextFactory, mcp::server::stream::SocketContextFactory>);
    static_assert(std::is_same_v<decltype(std::declval<mcp::server::stream::SocketContext&>().getSocketConnection()),
                                 core::socket::stream::SocketConnection*>);
    static_assert(std::is_same_v<decltype(std::declval<mcp::server::stream::SocketContext&>().close()), void>);
    static_assert(sizeof(mcp::server::http::Router) >= sizeof(express::Router));
    mcp::server::Server server("installed-consumer", "test-version");
    server.tool(std::make_shared<InstalledTool>());
    server.resourceTemplate(std::make_shared<InstalledTemplate>());
    auto* config = utils::Config::configRoot.newSubCommand<mcp::server::Config>();
    const mcp::server::stream::SocketContextFactory protocolFactory(server, *config);
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0) {
        const net::stdio::stream::SocketEndpoint<InstalledFactory> endpoint("installed-endpoint");
        endpoint.getConfig()->Instance::forceUnrequired();
        char file[] = "--config-file=/dev/null";
        char quiet[] = "--quiet";
        char* arguments[] = {argv[0], file, quiet};
        core::SNodeC::init(3, arguments);
        int attached = 0;
        endpoint.attach(net::stdio::SocketAddress(pair[0], pair[0]), [&attached](const auto&, auto state) {
            if (state == core::socket::State::OK) {
                ++attached;
            }
        });
        const int loop = core::SNodeC::start();
        ::close(pair[1]);
        result = loop == 0 && attached == 1 && jsonrpcReady && mcpClientReady ? 0 : 1;
    }

    return result;
}
