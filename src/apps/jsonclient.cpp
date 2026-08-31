#include <SemanticLog.h>
/*
 * snode.c - a slim toolkit for network communication
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

#include <utility>

// IWYU pragma: no_include <bits/utility.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    using Request = web::http::client::Request;
    using Response = web::http::client::Response;
    using Client = web::http::legacy::in::Client<Request, Response>;
    using SocketAddress = Client::SocketAddress;

    Client jsonClient(
        "legacy",
        [](Request& request) -> void {
            snode::semantic::appLog().trace() << "-- OnRequest";
            request.method = "POST";
            request.url = "/index.html";
            request.type("application/json");
            request.set("Connection", "close");
            request.send("{\"userId\":1,\"schnitzel\":\"good\",\"hungry\":false}");
        },
        []([[maybe_unused]] Request& request, Response& response) -> void {
            snode::semantic::appLog().trace() << "-- OnResponse";
            snode::semantic::appLog().trace() << "     Status:";
            snode::semantic::appLog().trace() << "       " << response.httpVersion;
            snode::semantic::appLog().trace() << "       " << response.statusCode;
            snode::semantic::appLog().trace() << "       " << response.reason;

            snode::semantic::appLog().trace() << "     Headers:";
            for (auto& [field, value] : response.headers) {
                snode::semantic::appLog().trace() << "       " << field + " = " + value;
            }

            snode::semantic::appLog().trace() << "     Cookies:";
            for (auto& [name, cookie] : response.cookies) {
                snode::semantic::appLog().trace() << "       " + name + " = " + cookie.getValue();
                for (auto& [option, value] : cookie.getOptions()) {
                    snode::semantic::appLog().trace() << "         " + option + " = " + value;
                }
            }

            response.body.push_back(0);
            snode::semantic::appLog().trace() << "     Body:\n----------- start body -----------" << response.body.data() << "\n------------ end body ------------";
        },
        [](int status, const std::string& reason) -> void {
            snode::semantic::appLog().trace() << "-- OnResponseError";
            snode::semantic::appLog().trace() << "     Status: " << status;
            snode::semantic::appLog().trace() << "     Reason: " << reason;
        });

    jsonClient.connect("localhost", 8080, [](const SocketAddress& socketAddress, int errnum) -> void {
        if (errnum < 0) {
            snode::semantic::sysError(snode::semantic::appLog(), logger::LogLevel::Error, errno) << "OnError";
        } else if (errnum > 0) {
            snode::semantic::sysError(snode::semantic::appLog(), logger::LogLevel::Error, errno) << "OnError: " << socketAddress.toString();
        } else {
            snode::semantic::appLog().trace() << "snode.c connecting to " << socketAddress.toString();
        }
    });

    jsonClient.connect("localhost", 8080, [](const SocketAddress& socketAddress, int errnum) -> void {
        if (errnum < 0) {
            snode::semantic::sysError(snode::semantic::appLog(), logger::LogLevel::Error, errno) << "OnError";
        } else if (errnum > 0) {
            snode::semantic::sysError(snode::semantic::appLog(), logger::LogLevel::Error, errno) << "OnError: " << socketAddress.toString();
        } else {
            snode::semantic::appLog().trace() << "snode.c connecting to " << socketAddress.toString();
        }
    });

    /*
        jsonClient.post("localhost", 8080, "/index.html", "{\"userId\":1,\"schnitzel\":\"good\",\"hungry\":false}", [](int err) -> void {
            if (err != 0) {
                snode::semantic::sysError(snode::semantic::appLog(), logger::LogLevel::Error, errno) << "OnError: " << err;
            }
        });
    */

    return core::SNodeC::start();
}
