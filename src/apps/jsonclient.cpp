/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#include "core/SNodeC.h"
#include "web/http/legacy/in/Client.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <string>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    using Client = web::http::legacy::in::Client;
    using Request = Client::Request;
    using Response = Client::Response;
    using SocketAddress = Client::SocketAddress;

    const Client jsonClient(
        "legacy",
        [](const std::shared_ptr<Request>& req) -> void {
            VLOG(0) << "-- OnRequest";
            req->method = "POST";
            req->url = "/index.html";
            req->type("application/json");
            req->set("Connection", "close");
            req->send("{\"userId\":1,\"schnitzel\":\"good\",\"hungry\":false}");
        },
        []([[maybe_unused]] const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) -> void {
            VLOG(0) << "-- OnResponse";
            VLOG(0) << "     Status:";
            VLOG(0) << "       " << res->httpVersion;
            VLOG(0) << "       " << res->statusCode;
            VLOG(0) << "       " << res->reason;

            VLOG(0) << "     Headers:";
            for (auto& [field, value] : res->headers) {
                VLOG(0) << "       " << field + " = " + value;
            }

            VLOG(0) << "     Cookies:";
            for (const auto& [name, cookie] : res->cookies) {
                VLOG(0) << "       " + name + " = " + cookie.getValue();
                for (auto& [option, value] : cookie.getOptions()) {
                    VLOG(0) << "         " + option + " = " + value;
                }
            }

            res->body.push_back(0);
            VLOG(0) << "     Body:\n----------- start body -----------" << res->body.data() << "\n------------ end body ------------";
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
