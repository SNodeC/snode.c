/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022  Volker Christian <me@vchrist.at>
 * Json Middleware 2020, 2021 Marlene Mayr, Anna Moser, Matteo Prock, Eric Thalhammer
 * Github <MarleneMayr><moseranna><MatteoMatteoMatteo><peregrin-tuk>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/SNodeC.h"               // for SNodeC
#include "log/Logger.h"                // for Writer, Storage
#include "web/http/client/Request.h"   // for Request
#include "web/http/client/Response.h"  // for Response
#include "web/http/legacy/in/Client.h" // for Client, Client<>...

#include <type_traits> // for add_const<>::type
#include <utility>     // for tuple_element<>:...

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    using Request = web::http::client::Request;
    using Response = web::http::client::Response;
    using Client = web::http::legacy::in::Client<Request, Response>;
    using SocketConnection = Client::SocketConnection;
    using SocketAddress = Client::SocketAddress;

    Client jsonClient(
        "legacy",
        [](SocketConnection* socketConnection) -> void {
            VLOG(0) << "-- OnConnect";

            VLOG(0) << "\tServer: (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();
            VLOG(0) << "\tClient: (" + socketConnection->getLocalAddress().address() + ") " +
                           socketConnection->getLocalAddress().toString();
        },
        []([[maybe_unused]] SocketConnection* socketConnection) -> void {
            VLOG(0) << "-- OnConnected";
        },
        [](Request& request) -> void {
            request.method = "POST";
            request.url = "/index.html";
            request.type("application/json");
            request.set("Connection", "close");
            request.send("{\"userId\":1,\"schnitzel\":\"good\",\"hungry\":false}");
        },
        []([[maybe_unused]] Request& request, Response& response) -> void {
            VLOG(0) << "-- OnResponse";
            VLOG(0) << "     Status:";
            VLOG(0) << "       " << response.httpVersion;
            VLOG(0) << "       " << response.statusCode;
            VLOG(0) << "       " << response.reason;

            VLOG(0) << "     Headers:";
            for (const auto& [field, value] : *response.headers) {
                VLOG(0) << "       " << field + " = " + value;
            }

            VLOG(0) << "     Cookies:";
            for (const auto& [name, cookie] : *response.cookies) {
                VLOG(0) << "       " + name + " = " + cookie.getValue();
                for (const auto& [option, value] : cookie.getOptions()) {
                    VLOG(0) << "         " + option + " = " + value;
                }
            }

            response.body.push_back(0);
            VLOG(0) << "     Body:\n----------- start body -----------" << response.body.data() << "\n------------ end body ------------";
        },
        [](int status, const std::string& reason) -> void {
            VLOG(0) << "-- OnResponseError";
            VLOG(0) << "     Status: " << status;
            VLOG(0) << "     Reason: " << reason;
        },
        [](SocketConnection* socketConnection) -> void {
            VLOG(0) << "-- OnDisconnect";

            VLOG(0) << "\tServer: (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();
            VLOG(0) << "\tClient: (" + socketConnection->getLocalAddress().address() + ") " +
                           socketConnection->getLocalAddress().toString();
        });

    jsonClient.connect("localhost", 8080, [](const SocketAddress& socketAddress, int err) -> void {
        if (err != 0) {
            PLOG(ERROR) << "OnError: " << err;
        } else {
            VLOG(0) << "jsonclient.c connected to " << socketAddress.toString();
        }
    });

    jsonClient.connect("localhost", 8080, [](const SocketAddress& socketAddress, int err) -> void {
        if (err != 0) {
            PLOG(ERROR) << "OnError: " << err;
        } else {
            VLOG(0) << "jsonclient.c connecting to " << socketAddress.toString();
        }
    });

    /*
        jsonClient.post("localhost", 8080, "/index.html", "{\"userId\":1,\"schnitzel\":\"good\",\"hungry\":false}", [](int err) -> void {
            if (err != 0) {
                PLOG(ERROR) << "OnError: " << err;
            }
        });
    */

    return core::SNodeC::start();
}
