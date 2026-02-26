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
#include "web/http/legacy/in/Client.h"
#include "web/http/tls/in/Client.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    {
        using LegacyClient = web::http::legacy::in::Client;
        using MasterRequest = LegacyClient::MasterRequest;
        using Request = LegacyClient::Request;
        using Response = LegacyClient::Response;
        using LegacySocketAddress = LegacyClient::SocketAddress;

        const LegacyClient legacyClient(
            "legacy",
            [](const std::shared_ptr<MasterRequest>& req) {
                const std::string connectionName = req->getSocketContext()->getSocketConnection()->getConnectionName();

                VLOG(1) << connectionName << ": OnRequestBegin";

                req->set("Sec-WebSocket-Protocol", "subprotocol, echo");

                req->upgrade(
                    "/ws",
                    "websocket",
                    [connectionName](bool success) {
                        VLOG(1) << connectionName << ": HTTP Upgrade (http -> websocket) start " << (success ? "success" : "failed");
                    },
                    [connectionName]([[maybe_unused]] const std::shared_ptr<Request>& req,
                                     const std::shared_ptr<Response>& res,
                                     [[maybe_unused]] bool success) {
                        VLOG(1) << connectionName << ": Upgrade success:";

                        VLOG(1) << connectionName << ":   Requested: " << req->header("upgrade");
                        VLOG(1) << connectionName << ":    Selected: " << res->get("upgrade");
                    },
                    [connectionName](const std::shared_ptr<Request>&, const std::string& message) {
                        VLOG(1) << connectionName << ": Request parse error: " << message;
                    });
            },
            []([[maybe_unused]] const std::shared_ptr<MasterRequest>& req) {
                const std::string connectionName = req->getSocketContext()->getSocketConnection()->getConnectionName();

                VLOG(1) << connectionName << ": OnRequestEnd";
            });

        legacyClient
            .setOnInitState([]([[maybe_unused]] core::eventreceiver::ConnectEventReceiver* connectEventReceiver) {
                VLOG(0) << "------------------- Legacy Client Init: " << connectEventReceiver;
            })
            .connect([instanceName = legacyClient.getConfig().getInstanceName()](const LegacySocketAddress& socketAddress,
                                                                                 const core::socket::State& state) {
                switch (state) {
                    case core::socket::State::OK:
                        VLOG(1) << instanceName << " connected to '" << socketAddress.toString() << "'";
                        break;
                    case core::socket::State::DISABLED:
                        VLOG(1) << instanceName << " disabled";
                        break;
                    case core::socket::State::ERROR:
                        VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
                        break;
                    case core::socket::State::FATAL:
                        VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
                        break;
                }
            }); // Connection:keep-alive\r\n\r\n"

        using TlsClient = web::http::tls::in::Client;
        using MasterRequest = TlsClient::MasterRequest;
        using Request = TlsClient::Request;
        using Response = TlsClient::Response;
        using TLSSocketAddress = TlsClient::SocketAddress;

        const TlsClient tlsClient(
            "tls",
            [](const std::shared_ptr<MasterRequest>& req) {
                const std::string connectionName = req->getSocketContext()->getSocketConnection()->getConnectionName();

                VLOG(1) << connectionName << ": OnRequestBegin";

                req->set("Sec-WebSocket-Protocol", "subprotocol, echo");

                req->upgrade(
                    "/ws",
                    "websocket",
                    [connectionName](bool success) {
                        VLOG(1) << connectionName << ": HTTP Upgrade (http -> websocket) start " << (success ? "success" : "failed");
                    },
                    [connectionName]([[maybe_unused]] const std::shared_ptr<Request>& req,
                                     [[maybe_unused]] const std::shared_ptr<Response>& res,
                                     [[maybe_unused]] bool success) {
                    },
                    [connectionName](const std::shared_ptr<Request>&, const std::string& message) {
                        VLOG(1) << connectionName << ": Request parse error: " << message;
                    });
            },
            []([[maybe_unused]] const std::shared_ptr<MasterRequest>& req) {
                const std::string connectionName = req->getSocketContext()->getSocketConnection()->getConnectionName();

                VLOG(1) << connectionName << ": OnRequestEnd";
            });

        tlsClient
            .setOnInitState([]([[maybe_unused]] core::eventreceiver::ConnectEventReceiver* connectEventReceiver) {
                VLOG(0) << "------------------- TLS Client Init: " << connectEventReceiver;
            })
            .connect([instanceName = tlsClient.getConfig().getInstanceName()](const TLSSocketAddress& socketAddress,
                                                                              const core::socket::State& state) {
                switch (state) {
                    case core::socket::State::OK:
                        VLOG(1) << instanceName << " connected to '" << socketAddress.toString() << "'";
                        break;
                    case core::socket::State::DISABLED:
                        VLOG(1) << instanceName << " disabled";
                        break;
                    case core::socket::State::ERROR:
                        VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
                        break;
                    case core::socket::State::FATAL:
                        VLOG(1) << instanceName << " " << socketAddress.toString() << ": " << state.what();
                        break;
                }
            }); // Connection:keep-alive\r\n\r\n"
    }

    return core::SNodeC::start();
}
