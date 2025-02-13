/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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
        using Request = LegacyClient::Request;
        using Response = LegacyClient::Response;
        using LegacySocketAddress = LegacyClient::SocketAddress;

        const LegacyClient legacyClient(
            "legacy",
            [](const std::shared_ptr<Request>& req) {
                VLOG(1) << "OnRequestBegin";

                VLOG(1) << "Requesting upgrade to 'websocket' and any of the subprotocols 'subprotocol' and 'echo'";

                req->set("Sec-WebSocket-Protocol", "subprotocol, echo");

                if (!req->upgrade("/ws/",
                                  "upgradeprotocol, websocket",
                                  [](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                                      VLOG(1) << "OnResponse";
                                      VLOG(2) << "  Status:";
                                      VLOG(2) << "    " << res->httpVersion << " " << res->statusCode << " " << res->reason;
                                      VLOG(2) << "  Headers:";
                                      for (const auto& [field, value] : res->headers) {
                                          VLOG(2) << "    " << field + " = " + value;
                                      }

                                      VLOG(2) << "  Cookies:";
                                      for (const auto& [name, cookie] : res->cookies) {
                                          VLOG(2) << "    " + name + " = " + cookie.getValue();
                                          for (const auto& [option, value] : cookie.getOptions()) {
                                              VLOG(2) << "      " + option + " = " + value;
                                          }
                                      }

                                      req->upgrade(res, [req](const std::string& name) {
                                          if (!name.empty()) {
                                              VLOG(1) << "Successful upgrade to '" << name << "' from options: " << req->header("Upgrade");
                                          } else {
                                              VLOG(1) << "Can not upgrade to any of '" << req->header("Upgrade") << "'";
                                          }
                                      });
                                  })) {
                    VLOG(1) << "Initiating upgrade to any of 'upgradeprotocol, websocket' failed";
                }
            },
            []([[maybe_unused]] const std::shared_ptr<Request>& req) {
                VLOG(1) << "OnRequestEnd";
            });

        legacyClient.connect([instanceName = legacyClient.getConfig().getInstanceName()](const LegacySocketAddress& socketAddress,
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
        using Request = TlsClient::Request;
        using Response = TlsClient::Response;
        using TLSSocketAddress = TlsClient::SocketAddress;

        const TlsClient tlsClient(
            "tls",
            [](const std::shared_ptr<Request>& req) {
                VLOG(1) << "OnRequestBegin";

                VLOG(1) << "Requesting upgrade to 'websocket' and any of the subprotocols 'subprotocol' and 'echo'";

                req->set("Sec-WebSocket-Protocol", "subprotocol, echo");

                if (!req->upgrade("/ws/",
                                  "upgradeprotocol, websocket",
                                  [](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                                      VLOG(1) << "OnResponse";
                                      VLOG(2) << "  Status:";
                                      VLOG(2) << "    " << res->httpVersion << " " << res->statusCode << " " << res->reason;
                                      VLOG(2) << "  Headers:";
                                      for (const auto& [field, value] : res->headers) {
                                          VLOG(2) << "    " << field + " = " + value;
                                      }

                                      VLOG(2) << "  Cookies:";
                                      for (const auto& [name, cookie] : res->cookies) {
                                          VLOG(2) << "    " + name + " = " + cookie.getValue();
                                          for (const auto& [option, value] : cookie.getOptions()) {
                                              VLOG(2) << "      " + option + " = " + value;
                                          }
                                      }

                                      req->upgrade(res, [req](const std::string& name) {
                                          if (!name.empty()) {
                                              VLOG(1) << "Successful upgrade to '" << name << "' from options: " << req->header("Upgrade");
                                          } else {
                                              VLOG(1) << "Can not upgrade to any of '" << req->header("Upgrade") << "'";
                                          }
                                      });
                                  })) {
                    VLOG(1) << "Initiating upgrade to any of 'upgradeprotocol, websocket' failed";
                }
            },
            []([[maybe_unused]] const std::shared_ptr<Request>& req) {
                VLOG(1) << "OnRequestEnd";
            });

        tlsClient.connect([instanceName = tlsClient.getConfig().getInstanceName()](const TLSSocketAddress& socketAddress,
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
