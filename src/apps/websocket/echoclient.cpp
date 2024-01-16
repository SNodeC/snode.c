/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/SNodeC.h"
#include "log/Logger.h"
#include "web/http/client/Request.h"
#include "web/http/client/Response.h"
#include "web/http/legacy/in/Client.h"
#include "web/http/tls/in/Client.h"

#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <vector>

// IWYU pragma: no_include <openssl/ssl3.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    {
        using LegacySocketAddress = web::http::legacy::in::Client<web::http::client::Request, web::http::client::Response>::SocketAddress;

        const web::http::legacy::in::Client<web::http::client::Request, web::http::client::Response> legacyClient(
            "legacy",
            [](web::http::client::Request& request) -> void {
                VLOG(1) << "OnRequestBegin";

                request.set("Sec-WebSocket-Protocol", "test, echo");

                request.upgrade("/ws/", "hui, websocket");
            },
            [](web::http::client::Request& request, web::http::client::Response& response) -> void {
                VLOG(1) << "OnResponse";
                VLOG(2) << "     Status:";
                VLOG(2) << "       " << response.httpVersion << " " << response.statusCode << " " << response.reason;

                VLOG(2) << "     Headers:";
                for (const auto& [field, value] : response.headers) {
                    VLOG(2) << "       " << field + " = " + value;
                }

                VLOG(2) << "     Cookies:";
                for (const auto& [name, cookie] : response.cookies) {
                    VLOG(2) << "       " + name + " = " + cookie.getValue();
                    for (const auto& [option, value] : cookie.getOptions()) {
                        VLOG(2) << "         " + option + " = " + value;
                    }
                }

                response.body.push_back(0); // make it a c-string
                std::cout << "Body:\n----------- start body -----------\n"
                          << response.body.data() << "\n------------ end body ------------" << std::endl;

                response.upgrade(request);
            },
            [](int status, const std::string& reason) -> void {
                VLOG(1) << "OnResponseError";
                VLOG(1) << "     Status: " << status;
                VLOG(1) << "     Reason: " << reason;
            });

        legacyClient.connect([instanceName = legacyClient.getConfig().getInstanceName()](const LegacySocketAddress& socketAddress,
                                                                                         const core::socket::State& state) -> void {
            switch (state) {
                case core::socket::State::OK:
                    VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
                    break;
                case core::socket::State::DISABLED:
                    VLOG(1) << instanceName << ": disabled";
                    break;
                case core::socket::State::ERROR:
                    LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                    break;
                case core::socket::State::FATAL:
                    LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                    break;
            }
        }); // Connection:keep-alive\r\n\r\n"

        using TLSSocketAddress = web::http::tls::in::Client<web::http::client::Request, web::http::client::Response>::SocketAddress;

        const web::http::tls::in::Client<web::http::client::Request, web::http::client::Response> tlsClient(
            "tls",
            [](web::http::client::Request& request) -> void {
                VLOG(1) << "OnRequestBegin";

                request.set("Sec-WebSocket-Protocol", "test, echo");

                request.upgrade("/ws/", "hui, websocket");
            },
            [](web::http::client::Request& request, web::http::client::Response& response) -> void {
                VLOG(1) << "OnResponse";
                VLOG(2) << "     Status:";
                VLOG(2) << "       " << response.httpVersion << " " << response.statusCode << " " << response.reason;

                VLOG(2) << "     Headers:";
                for (auto& [field, value] : response.headers) {
                    VLOG(2) << "       " << field + " = " + value;
                }

                VLOG(2) << "     Cookies:";
                for (const auto& [name, cookie] : response.cookies) {
                    VLOG(2) << "       " + name + " = " + cookie.getValue();
                    for (const auto& [option, value] : cookie.getOptions()) {
                        VLOG(2) << "         " + option + " = " + value;
                    }
                }

                response.body.push_back(0); // make it a c-string
                std::cout << "Body:\n----------- start body -----------\n"
                          << response.body.data() << "\n------------ end body ------------" << std::endl;

                response.upgrade(request);
            },
            [](int status, const std::string& reason) -> void {
                VLOG(1) << "OnResponseError";
                VLOG(1) << "     Status: " << status;
                VLOG(1) << "     Reason: " << reason;
            });

        tlsClient.connect([instanceName = tlsClient.getConfig().getInstanceName()](const TLSSocketAddress& socketAddress,
                                                                                   const core::socket::State& state) -> void {
            switch (state) {
                case core::socket::State::OK:
                    VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
                    break;
                case core::socket::State::DISABLED:
                    VLOG(1) << instanceName << ": disabled";
                    break;
                case core::socket::State::ERROR:
                    LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                    break;
                case core::socket::State::FATAL:
                    LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                    break;
            }
        }); // Connection:keep-alive\r\n\r\n"
    }

    return core::SNodeC::start();
}
