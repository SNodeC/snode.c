/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023  Volker Christian <me@vchrist.at>
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

#include "core/SNodeC.h"
#include "log/Logger.h"
#include "web/http/client/Request.h"
#include "web/http/client/Response.h"
#include "web/http/legacy/in/Client.h"

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    using Request = web::http::client::Request;
    using Response = web::http::client::Response;
    using Client = web::http::legacy::in::Client<Request, Response>;
    using SocketAddress = Client::SocketAddress;

    const Client jsonClient(
        "legacy",
        [](Request& request) -> void {
            VLOG(0) << "-- OnRequest";
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
            for (auto& [field, value] : response.headers) {
                VLOG(0) << "       " << field + " = " + value;
            }

            VLOG(0) << "     Cookies:";
            for (const auto& [name, cookie] : response.cookies) {
                VLOG(0) << "       " + name + " = " + cookie.getValue();
                for (auto& [option, value] : cookie.getOptions()) {
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
        });

    jsonClient.connect("localhost",
                       8080,
                       [instanceName = jsonClient.getConfig().getInstanceName()](
                           const SocketAddress& socketAddress,
                           const core::socket::State& state) -> void { // example.com:81 simulate connnect timeout
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
                       });

    jsonClient.connect("localhost",
                       8080,
                       [instanceName = jsonClient.getConfig().getInstanceName()](
                           const SocketAddress& socketAddress,
                           const core::socket::State& state) -> void { // example.com:81 simulate connnect timeout
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
