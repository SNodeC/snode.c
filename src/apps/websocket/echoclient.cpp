/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include <type_traits>

// IWYU pragma: no_include <bits/utility.h>
// IWYU pragma: no_include <openssl/ssl3.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    {
        using LegacySocketAddress = web::http::legacy::in::Client<web::http::client::Request, web::http::client::Response>::SocketAddress;

        web::http::legacy::in::Client<web::http::client::Request, web::http::client::Response> legacyClient(
            "legacy",
            [](web::http::client::Request& request) -> void {
                VLOG(0) << "OnRequestBegin";

                request.set("Sec-WebSocket-Protocol", "test, echo");

                request.upgrade("/ws/", "websocket");
            },
            [](web::http::client::Request& request, web::http::client::Response& response) -> void {
                VLOG(0) << "OnResponse";
                VLOG(0) << "     Status:";
                VLOG(0) << "       " << response.httpVersion << " " << response.statusCode << " " << response.reason;

                VLOG(0) << "     Headers:";
                for (const auto& [field, value] : response.headers) {
                    VLOG(0) << "       " << field + " = " + value;
                }

                VLOG(0) << "     Cookies:";
                for (const auto& [name, cookie] : response.cookies) {
                    VLOG(0) << "       " + name + " = " + cookie.getValue();
                    for (const auto& [option, value] : cookie.getOptions()) {
                        VLOG(0) << "         " + option + " = " + value;
                    }
                }

                response.body.push_back(0); // make it a c-string
                VLOG(0) << "Body:\n----------- start body -----------\n" << response.body.data() << "\n------------ end body ------------";

                response.upgrade(request);
            },
            [](int status, const std::string& reason) -> void {
                VLOG(0) << "OnResponseError";
                VLOG(0) << "     Status: " << status;
                VLOG(0) << "     Reason: " << reason;
            });

        legacyClient.connect([](const LegacySocketAddress& socketAddress, int err) -> void {
            if (err != 0) {
                PLOG(ERROR) << "OnError: " << err;
            } else {
                VLOG(0) << "wsechoclient connecting to " << socketAddress.toString();
            }
        }); // Connection:keep-alive\r\n\r\n"

        using TLSSocketAddress = web::http::tls::in::Client<web::http::client::Request, web::http::client::Response>::SocketAddress;

        web::http::tls::in::Client<web::http::client::Request, web::http::client::Response> tlsClient(
            "tls",
            [](web::http::client::Request& request) -> void {
                VLOG(0) << "OnRequestBegin";

                request.set("Sec-WebSocket-Protocol", "test, echo");

                request.upgrade("/ws/", "websocket");
            },
            [](web::http::client::Request& request, web::http::client::Response& response) -> void {
                VLOG(0) << "OnResponse";
                VLOG(0) << "     Status:";
                VLOG(0) << "       " << response.httpVersion << " " << response.statusCode << " " << response.reason;

                VLOG(0) << "     Headers:";
                for (const auto& [field, value] : response.headers) {
                    VLOG(0) << "       " << field + " = " + value;
                }

                VLOG(0) << "     Cookies:";
                for (const auto& [name, cookie] : response.cookies) {
                    VLOG(0) << "       " + name + " = " + cookie.getValue();
                    for (const auto& [option, value] : cookie.getOptions()) {
                        VLOG(0) << "         " + option + " = " + value;
                    }
                }

                response.body.push_back(0); // make it a c-string
                VLOG(0) << "Body:\n----------- start body -----------\n" << response.body.data() << "\n------------ end body ------------";

                response.upgrade(request);
            },
            [](int status, const std::string& reason) -> void {
                VLOG(0) << "OnResponseError";
                VLOG(0) << "     Status: " << status;
                VLOG(0) << "     Reason: " << reason;
            });

        tlsClient.connect([](const TLSSocketAddress& socketAddress, int err) -> void {
            if (err != 0) {
                PLOG(ERROR) << "OnError: " << err;
            } else {
                VLOG(0) << "wsechoclient connecting to " << socketAddress.toString();
            }
        }); // Connection:keep-alive\r\n\r\n"
    }

    return core::SNodeC::start();
}
