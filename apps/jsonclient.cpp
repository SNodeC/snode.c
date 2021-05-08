/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 * Json Middleware 2020 Marlene Mayr, Anna Moser, Matteo Prock, Eric Thalhammer
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

#include "config.h" // just for this example app
#include "http/client/Response.h"
#include "http/client/legacy/Client.h"
#include "log/Logger.h"
#include "net/SNodeC.h"

#include <cstring>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    net::SNodeC::init(argc, argv);

    http::client::legacy::Client<> jsonClient(
        [](const http::client::legacy::Client<>::SocketAddress& localAddress,
           const http::client::legacy::Client<>::SocketAddress& remoteAddress) -> void {
            VLOG(0) << "-- OnConnect";

            VLOG(0) << "\tServer: " + remoteAddress.toString();
            VLOG(0) << "\tClient: " + localAddress.toString();
        },
        []([[maybe_unused]] http::client::legacy::Client<>::SocketConnection* socketConnection) -> void {
            VLOG(0) << "-- OnConnected";
        },
        [](http::client::Request& request) -> void {
            request.method = "POST";
            request.url = "/index.html";
            request.type("application/json");
            request.send("{\"userId\":1,\"schnitzel\":\"good\",\"hungry\":false}");
        },
        [](const http::client::Response& response) -> void {
            VLOG(0) << "-- OnResponse";
            VLOG(0) << "     Status:";
            VLOG(0) << "       " << response.httpVersion;
            VLOG(0) << "       " << response.statusCode;
            VLOG(0) << "       " << response.reason;

            VLOG(0) << "     Headers:";
            for (auto [field, value] : *response.headers) {
                VLOG(0) << "       " << field + " = " + value;
            }

            VLOG(0) << "     Cookies:";
            for (auto [name, cookie] : *response.cookies) {
                VLOG(0) << "       " + name + " = " + cookie.getValue();
                for (auto [option, value] : cookie.getOptions()) {
                    VLOG(0) << "         " + option + " = " + value;
                }
            }

            char* body = new char[response.contentLength + 1];
            memcpy(body, response.body, response.contentLength);
            body[response.contentLength] = 0;

            VLOG(1) << "     Body:\n----------- start body -----------\n" << body << "\n------------ end body ------------";

            delete[] body;
        },
        [](int status, const std::string& reason) -> void {
            VLOG(0) << "-- OnResponseError";
            VLOG(0) << "     Status: " << status;
            VLOG(0) << "     Reason: " << reason;
        },
        [](http::client::legacy::Client<>::SocketConnection* socketConnection) -> void {
            VLOG(0) << "-- OnDisconnect";

            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();
        });

    jsonClient.connect("localhost", 8080, [](int err) -> void {
        if (err != 0) {
            PLOG(ERROR) << "OnError: " << err;
        }
    });

    jsonClient.connect("localhost", 8080, [](int err) -> void {
        if (err != 0) {
            PLOG(ERROR) << "OnError: " << err;
        }
    });
    /*
        jsonClient.post("localhost", 8080, "/index.html", "{\"userId\":1,\"schnitzel\":\"good\",\"hungry\":false}", [](int err) -> void {
            if (err != 0) {
                PLOG(ERROR) << "OnError: " << err;
            }
        });
    */
    return net::SNodeC::start();
}
